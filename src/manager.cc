#include <memory>

#include <iostream> // debug only!

#include "boost/algorithm/string.hpp"
#include "boost/url.hpp"
#include "boost/date_time.hpp"
#include "boost/current_function.hpp"
#include "boost/format.hpp"
#include "boost/uuid/uuid.hpp"
#include "boost/uuid/uuid_generators.hpp"
#include "boost/uuid/uuid_io.hpp"

#include "soci/soci.h"
#include "soci/sqlite3/soci-sqlite3.h"
#include "manager.hh"
#include "model.hh"

#ifndef TUPAL_MESSAGE
# define TUPAL_MESSAGE(stream) stream << boost::posix_time::to_iso_extended_string(boost::posix_time::microsec_clock::local_time()) << ":" << __FILE__ << ":" << __LINE__ << ": " << BOOST_CURRENT_FUNCTION << ": "
#endif

namespace {

    static const char * const SCHEMA_VERSION = "1"; // change, if any of the above changes in non-comapatible way
    static const char * const SCHEMA_CONFIGURATION =
        "create table configuration ("
        "   name text primary key,"
        "   value text"
        ")";
    static const char * const SCHEMA_COMPETITION = 
        "create table competition ("
        "   id  text primary key,"
        "   date text not null,"  // ISO-8601 date, e.g. '2025-07-26'
        "   title text not null"
        ")";
    static const char * const SCHEMA_START_GROUP = 
        "create table start_group ("
        "   id text not null,"
        "   comp_id text not null,"
        "   title text not null,"
        "   start_time text not null," // ISO-8601 datetime, e.g. '2025-07-26T18:30:00.000+03:00'
        "   first_bib integer not null,"
        "   start_interval text not null,"
        "   primary key (id, comp_id),"
        "   foreign key (comp_id) references competition(id)"
        ")";
    static const char * const SCHEMA_COMPETITION_CLASS =
        "create table competition_class ("
        "   id text not null, "
        "   comp_id text not null, "
        "   start_group_id text not null, "
        "   title text not null, "
        "   primary key (id, comp_id),"
        "   foreign key (start_group_id, comp_id) references start_group"
        ")";
    static const char * const SCHEMA_COMPETITOR =
        "create table competitor ("
        "   id text not null, "
        "   comp_id text not null, "
        "   comp_class_id text not null, "
        "   bib integer not null,"
        "   start_time_offset text not null," // in ISO-8601 duration, e.g. 'P5M30S'
        "   finish_time text," // ISO-8601 datetime, e.g. '2025-07-26T18:59:21.000+03:00'
        "   status integer," // 1-dns, 2-dnf, 3-dsq, any other-open
        "   name text not null,"
        "   primary key (id, comp_id),"
        "   foreign key (comp_class_id, comp_id) references competition_class"
        ")";
    static const char * const SCHEMA_FINISH_TIME_ENTRY =
        "create table finish_time_entry ("
        "   id text not null, "
        "   comp_id text not null, "
        "   finish_time text not null," // ISO-8601 datetime; also determines queue order
        "   status integer not null," // confirmed, uncertain, incorrect, or missing
        "   primary key (id, comp_id),"
        "   foreign key (comp_id) references competition(id)"
        ")";
    static const char * const SCHEMA_FINISH_BIB_ENTRY =
        "create table finish_bib_entry ("
        "   id text not null, "
        "   comp_id text not null, "
        "   sequence_no integer not null,"
        "   bib integer,"
        "   status integer not null," // confirmed, uncertain, incorrect, or missing
        "   confirmed_time_entry_id text,"
        "   primary key (id, comp_id),"
        "   unique (comp_id, sequence_no),"
        "   foreign key (comp_id) references competition(id),"
        "   foreign key (confirmed_time_entry_id, comp_id) references finish_time_entry(id, comp_id),"
        "   unique (comp_id, confirmed_time_entry_id)"
        ")";
    static const char * const INDEX_COMPETITOR_COMPETITION_BIB =
        "create unique index competitor_competition_bib "
        "on competitor(comp_id, bib) where bib > 0";

    const static std::error_code ok = std::error_code {};

    std::string finish_entry_id(const boost::json::object & data) {
        const auto id = data.find("id");
        if (id != data.end() && !id->value().is_null()) {
            if (!id->value().is_string() || id->value().as_string().empty())
                throw std::invalid_argument("'id' must be a non-empty string or null");
            return std::string { id->value().as_string() };
        }

        static boost::uuids::random_generator generator;
        return boost::uuids::to_string(generator());
    }

    int finish_entry_status(const boost::json::object & data) {
        const auto status = data.find("status");
        if (status == data.end())
            return 0;
        if (!status->value().is_int64())
            throw std::invalid_argument("'status' must be an integer from 0 to 3");
        return static_cast<int>(tupal::to_finish_entry_status(status->value().as_int64()));
    }

    std::string required_string(const boost::json::object & data, const char * name) {
        const auto value = data.find(name);
        if (value == data.end() || !value->value().is_string() || value->value().as_string().empty())
            throw std::invalid_argument(str(boost::format("'%1%' must be a non-empty string") % name));
        return std::string { value->value().as_string() };
    }

    bool competition_exists(soci::session & session, const std::string & competition_id) {
        int count = 0;
        session << "select count(*) from competition where id=:comp_id",
            soci::into(count), soci::use(competition_id);
        return count != 0;
    }

    boost::json::object finish_time_entry_json(const std::string & id, const std::string & competition_id,
            const std::string & finish_time, int status) {
        return tupal::to_json(tupal::finish_time_entry {
            { id, competition_id },
            tupal::from_date_string(finish_time),
            tupal::to_finish_entry_status(status)
        });
    }

    boost::json::object finish_bib_entry_json(const std::string & id, const std::string & competition_id,
            int sequence_no, const soci::row & row) {
        std::optional<int> bib;
        if (row.get_indicator("bib") != soci::i_null)
            bib = row.get<int>("bib");
        std::optional<std::string> confirmed_time_entry_id;
        if (row.get_indicator("confirmed_time_entry_id") != soci::i_null)
            confirmed_time_entry_id = row.get<std::string>("confirmed_time_entry_id");
        return tupal::to_json(tupal::finish_bib_entry {
            { id, competition_id },
            sequence_no,
            bib,
            tupal::to_finish_entry_status(row.get<int>("status")),
            confirmed_time_entry_id
        });
    }

    boost::json::value to_json(const std::string & message, const boost::json::value & data = {}) {
        return boost::json::object {
            { "message", message },
            { "item", data }
        };
    }

    std::error_code handle_soci_error(const std::string & /* backend_name */, const soci::soci_error & e) {

        switch (e.get_error_category()) {
            case soci::soci_error::unknown:
                break;
            case soci::soci_error::no_data:
                return tupal::make_error_code(tupal::error_code::unknown_key);
            case soci::soci_error::constraint_violation:
                if (boost::icontains(e.what(), "UNIQUE constraint failed") || boost::icontains(e.what(), "duplicate key"))
                    return tupal::make_error_code(tupal::error_code::duplicate_key);
                return tupal::make_error_code(tupal::error_code::constraint_violation);
            case soci::soci_error::connection_error:
            case soci::soci_error::invalid_statement:
            case soci::soci_error::no_privilege:
            case soci::soci_error::unknown_transaction_state:
            case soci::soci_error::system_error:
            default:
                return tupal::make_error_code(tupal::error_code::system_error);
        }

        return tupal::make_error_code(tupal::error_code::system_error);
    }

    class StartGroupManagerImpl : public tupal::StartGroupManager
    {
    public:
        StartGroupManagerImpl() = delete;
        StartGroupManagerImpl(const StartGroupManagerImpl &) = delete;
        StartGroupManagerImpl(StartGroupManagerImpl &&) = delete;
        ~StartGroupManagerImpl() noexcept = default;

        StartGroupManagerImpl(tupal::CompetitionManager & cm, std::shared_ptr<soci::session> & session) 
            : competition_manager(cm), soci_session(session) {
        }

        virtual tupal::result_type list(const std::string & competition_id) const {
            try {
                soci::rowset<> rows = (soci_session->prepare 
                    << "select s.id, c.id, s.title, s.start_time, first_bib, start_interval from competition c "
                            "left join start_group s on c.id = s.comp_id where c.id=:comp_id",
                        soci::use(competition_id));
    
                auto begin = rows.begin();
                if (begin == rows.end()) { // competition not found
                    return { tupal::make_error_code(tupal::error_code::unknown_key),
                        to_json(str(boost::format("Competition '%1%' not found!") % competition_id)) };
                }

                boost::json::array objects;
                if (begin->get_indicator(0) != soci::i_null) {
                    std::transform(begin, rows.end(), std::back_insert_iterator(objects), [](const soci::row & row) -> boost::json::value { 
                        return tupal::to_json(tupal::start_group {
                            { row.get<std::string>(0), row.get<std::string>(1) },
                            row.get<std::string>(2),
                            tupal::from_date_string(row.get<std::string>(3)),
                            static_cast<uint16_t>(row.get<int>(4)),
                            tupal::from_duration_string(row.get<std::string>(5))
                        });
                    });
                }

                return { ok, std::move(objects) };
            }

            catch (const soci::soci_error & e) {
                TUPAL_MESSAGE(std::cerr) << e.what() << std::endl;
                return { tupal::make_error_code(tupal::error_code::system_error),
                    to_json(str(boost::format("%1%: competition '%2%'") % e.what() % competition_id)) };
            }
        }

        virtual tupal::result_type get(const std::string & competition_id, const std::string & id) const {
            try {
                tupal::start_group sg;
                std::string first_start_time_str;
                std::string start_interval_str;
                *soci_session << "select id, comp_id, title, start_time, first_bib, start_interval "
                        "from start_group "
                        "where id = :id and comp_id=:comp_id",
                    soci::into(sg.id.id), soci::into(sg.id.comp_id), soci::into(sg.title), soci::into(first_start_time_str),
                    soci::into(sg.first_bib), soci::into(start_interval_str), soci::use(id), soci::use(competition_id);
                    
                if (soci_session->got_data()) {
                    sg.first_start_time = tupal::from_date_string(first_start_time_str);
                    sg.start_interval = tupal::from_duration_string(start_interval_str);
                    return { ok, tupal::to_json(sg) };
                }

                return { tupal::make_error_code(tupal::error_code::unknown_key),
                    to_json(str(boost::format("Competition '%1%' or start group '%2%' not found!") % competition_id % id)) };
            }

            catch (const soci::soci_error & e) {
                TUPAL_MESSAGE(std::cerr) << e.what() << std::endl;
                return { tupal::make_error_code(tupal::error_code::system_error),
                    to_json(str(boost::format("%1%: competition '%2%', start group '%3%'") % e.what() % competition_id % id)) };
            }
        }

        virtual tupal::result_type create(const std::string & competition_id, const boost::json::object & new_data) {
            try {
                auto new_sg = tupal::to_start_group(new_data);
                new_sg.id.comp_id = std::string { competition_id };
                const auto first_start_time_str = to_date_string(new_sg.first_start_time);
                const auto start_interval_str = to_duration_string(new_sg.start_interval);

                soci::transaction trx(*soci_session);
                *soci_session << "insert into start_group(id, comp_id, title, start_time, first_bib, start_interval) "
                        "values (:id, :comp_id, :title, :start_time, :first_bib, :start_interval)",
                    soci::use(new_sg.id.id), soci::use(new_sg.id.comp_id), soci::use(new_sg.title),
                    soci::use(first_start_time_str), soci::use(new_sg.first_bib),
                    soci::use(start_interval_str);
                trx.commit();

                return { ok, tupal::to_json(new_sg) };
            }

            catch (const soci::soci_error & e) {
                TUPAL_MESSAGE(std::cerr) << "SOCI system error: " << e.what() << std::endl;

                const auto error_msg { str(boost::format("%1%: competition '%2%'") % e.what() % competition_id) };
                const auto ec = handle_soci_error(soci_session->get_backend_name(), e);

                if (ec == tupal::make_error_condition(tupal::error_code::constraint_violation))
                    return { tupal::make_error_code(tupal::error_code::unknown_key), to_json(error_msg, new_data) };
                if (ec == tupal::make_error_condition(tupal::error_code::system_error))
                    return { tupal::make_error_code(tupal::error_code::system_error), to_json(error_msg, new_data) };

                return { ec, to_json(error_msg, new_data) };
            }
            catch (const std::invalid_argument & e) {
                return { tupal::make_error_code(tupal::error_code::invalid_arguments),
                    to_json(e.what(), new_data) };
            }
        }

        virtual tupal::result_type update(const std::string & competition_id, const boost::json::object & new_data) {
            try {
                auto new_sg = tupal::to_start_group(new_data);
                new_sg.id.comp_id = std::string { competition_id };
                soci::transaction trx(*soci_session);
                if (new_data.find("start_interval") == new_data.end()) {
                    std::string existing_interval;
                    *soci_session << "select start_interval from start_group "
                            "where id=:id and comp_id=:comp_id",
                        soci::into(existing_interval), soci::use(new_sg.id.id),
                        soci::use(new_sg.id.comp_id);
                    if (soci_session->got_data()) {
                        new_sg.start_interval = tupal::from_duration_string(existing_interval);
                    }
                }
                const auto first_start_time_str = to_date_string(new_sg.first_start_time);
                const auto start_interval_str = to_duration_string(new_sg.start_interval);

                soci::statement stmt = (soci_session->prepare 
                    << "update start_group set title=:title, start_time=:start_time, first_bib=:first_bib, "
                        "start_interval=:start_interval "
                        "where id=:id and comp_id=:comp_id",
                    soci::use(new_sg.title), soci::use(first_start_time_str), soci::use(new_sg.first_bib),
                    soci::use(start_interval_str),
                    soci::use(new_sg.id.id), soci::use(new_sg.id.comp_id));
                stmt.execute(true);
                switch (stmt.get_affected_rows()) {
                    case 0: return { tupal::make_error_code(tupal::error_code::unknown_key),
                        to_json(str(boost::format("Not found: competition '%1%'") % competition_id), new_data) };
                    case 1: break;
                    default: return { tupal::make_error_code(tupal::error_code::constraint_violation),
                        to_json(str(boost::format("Multiple match: competition '%1%'") % competition_id), new_data) };
                }
                trx.commit();

                return { ok, tupal::to_json(new_sg) };
            }

            catch (const soci::soci_error & e) {
                TUPAL_MESSAGE(std::cerr) << e.what() << std::endl;
                return { tupal::make_error_code(tupal::error_code::system_error),
                    to_json(str(boost::format("%1%: competition '%2%'") % e.what() % competition_id), new_data) };
            }
            catch (const std::invalid_argument & e) {
                return { tupal::make_error_code(tupal::error_code::invalid_arguments),
                    to_json(e.what(), new_data) };
            }
        }

        virtual tupal::result_type remove(const std::string & competition_id, const std::string & id) {
            try {
                soci::transaction trx(*soci_session);
                soci::statement stmt = (soci_session->prepare <<
                     "delete from start_group where id=:id and comp_id=:comp_id",
                     soci::use(id), soci::use(competition_id));
                stmt.execute(true);
                switch (stmt.get_affected_rows()) {
                    case 0: return { tupal::make_error_code(tupal::error_code::unknown_key),
                        to_json(str(boost::format("Not found: competition '%1%', start group '%2%'") % competition_id % id)) };
                    case 1: break;
                    default: return { tupal::make_error_code(tupal::error_code::constraint_violation),
                        to_json(str(boost::format("Multiple match: competition '%1%', start group '%2%'") % competition_id % id)) };
                }
                trx.commit();

                return { ok, tupal::to_json( tupal::key { id, competition_id } ) };
            }

            catch (const soci::soci_error & e) {
                TUPAL_MESSAGE(std::cerr) << e.what() << std::endl;
                return { tupal::make_error_code(tupal::error_code::system_error),
                    to_json(str(boost::format("%1%: competition '%2%', start group '%3%'") % e.what() % competition_id % id)) };
            }
        }

        static void init_database(soci::session & session) {
            session.once << SCHEMA_START_GROUP;
        }

    private:
        tupal::CompetitionManager & competition_manager;
        std::shared_ptr<soci::session> soci_session;
    };

    class CompetitionClassManagerImpl : public tupal::CompetitionClassManager
    {
    public:
        CompetitionClassManagerImpl() = delete;
        CompetitionClassManagerImpl(const CompetitionClassManagerImpl &) = delete;
        CompetitionClassManagerImpl(CompetitionClassManagerImpl &&) = delete;
        ~CompetitionClassManagerImpl() noexcept = default;

        CompetitionClassManagerImpl(tupal::CompetitionManager & cm, std::shared_ptr<soci::session> & session)
            : competition_manager(cm), soci_session(session) {
        }

        virtual tupal::result_type list(const std::string & competition_id) const {
            try {
                soci::rowset<> rows = (soci_session->prepare << 
                        "select     sg.comp_id, cc.id, cc.start_group_id, cc.title "
                        "from       start_group sg "
                        "left join  competition_class cc on cc.start_group_id = sg.id and cc.comp_id = sg.comp_id "
                        "where      sg.comp_id = :comp_id ",
                    soci::use(competition_id));

                auto begin = rows.begin();
                if (begin == rows.end()) { // no competition found
                    return { tupal::make_error_code(tupal::error_code::unknown_key),
                        to_json(str(boost::format("Competition '%1%' not found!") % competition_id)) };
                }

                boost::json::array objects;
                if (begin->get_indicator(1) != soci::i_null) {
                    std::transform(begin, rows.end(), std::back_insert_iterator(objects), [](const soci::row & row) -> boost::json::value { 
                        return tupal::to_json( tupal::competition_class {
                            { row.get<std::string>(1), row.get<std::string>(0) },
                            row.get<std::string>(3),
                            row.get<std::string>(2)
                        });
                    });
                }
                return { ok, std::move(objects) };                
            }

            catch (const soci::soci_error & e) {
                TUPAL_MESSAGE(std::cerr) << e.what() << std::endl;
                return { tupal::make_error_code(tupal::error_code::system_error),
                    to_json(str(boost::format("%1%: competition '%2%'") % e.what() % competition_id)) };
            }
        }

        virtual tupal::result_type get(const std::string & competition_id, const std::string & id) const {
            try {
                tupal::competition_class cc;
                *soci_session << 
                        "select comp_id, id, start_group_id, title from competition_class "
                        "where id = :id and comp_id=:comp_id",
                    soci::into(cc.id.comp_id), soci::into(cc.id.id), soci::into(cc.start_group_id), soci::into(cc.title),
                    soci::use(id), soci::use(competition_id);
                if (soci_session->got_data())
                    return { ok, tupal::to_json(cc) };

                return { tupal::make_error_code(tupal::error_code::unknown_key),
                    to_json(str(boost::format("Competition '%1%' or class '%2%' not found!") % competition_id % id)) };
            }

            catch (const soci::soci_error & e) {
                TUPAL_MESSAGE(std::cerr) << e.what() << std::endl;
                return { tupal::make_error_code(tupal::error_code::system_error),
                    to_json(str(boost::format("%1%: competition '%2%', competition class '%3%'") % e.what() % competition_id % id)) };
            }
        }

        virtual tupal::result_type create(const std::string & competition_id, const boost::json::object & new_data) {
            try {
                auto cc = tupal::to_competition_class(new_data);
                cc.id.comp_id = std::string { competition_id };

                soci::transaction trx(*soci_session);
                *soci_session <<
                        "insert into competition_class(id, comp_id, start_group_id, title) "
                        "values (:id, :comp_id, :start_group_id, :title)",
                    soci::use(cc.id.id), soci::use(cc.id.comp_id), soci::use(cc.start_group_id), soci::use(cc.title);
                trx.commit();

                return { ok, tupal::to_json(cc) };
            }

            catch (const soci::soci_error & e) {
                TUPAL_MESSAGE(std::cerr) << "SOCI system error: " << e.what() << std::endl;
                const auto error_msg { str(boost::format("%1%: competition '%2%'") % e.what() % competition_id) };

                const auto ec = handle_soci_error(soci_session->get_backend_name(), e);
                if (ec == tupal::make_error_condition(tupal::error_code::constraint_violation))
                    return { tupal::make_error_code(tupal::error_code::unknown_key), to_json(
                        str(boost::format("New competition class refers likely to non-existent competition '%1%") % competition_id),
                        new_data) };

                if (ec == tupal::make_error_condition(tupal::error_code::system_error))
                    return { tupal::make_error_code(tupal::error_code::system_error),
                       to_json(str(boost::format("System error: %1%") % e.what())) };

                return { ec, to_json(str(boost::format("%1%: competition '%2%'") % e.what() % competition_id), new_data) };
            }
        }

        virtual tupal::result_type update(const std::string & competition_id, const boost::json::object & new_data) {
            try {
                auto cc = tupal::to_competition_class(new_data);
                cc.id.comp_id = std::string { competition_id };

                soci::transaction trx(*soci_session);
                soci::statement stmt = (soci_session->prepare << 
                        "update competition_class "
                        "set title=:title, start_group_id=:start_group_id "
                        "where id=:id and comp_id=:comp_id",
                    soci::use(cc.title), soci::use(cc.start_group_id), soci::use(cc.id.id), soci::use(cc.id.comp_id));
                stmt.execute(true);
                switch (stmt.get_affected_rows()) {
                    case 0: return { tupal::make_error_code(tupal::error_code::unknown_key),
                        to_json(str(boost::format("Not found: competition '%1%'") % competition_id), new_data) };
                    case 1: break;
                    default: return { tupal::make_error_code(tupal::error_code::constraint_violation),
                        to_json(str(boost::format("Multiple match: competition '%1%'") % competition_id), new_data) };
                }
                trx.commit();

                return { ok, tupal::to_json(cc) };
            }

            catch (const soci::soci_error & e) {
                TUPAL_MESSAGE(std::cerr) << e.what() << std::endl;
                return { tupal::make_error_code(tupal::error_code::system_error),
                    to_json(str(boost::format("%1%: competition '%2%'") % e.what() % competition_id), new_data) };
            }
        }

        virtual tupal::result_type remove(const std::string & competition_id, const std::string & id) {
            try {
                soci::transaction trx(*soci_session);
                soci::statement stmt = (soci_session->prepare <<
                        "delete from competition_class where id=:id and comp_id=:comp_id",
                    soci::use(id), soci::use(competition_id));
                stmt.execute(true);
                switch (stmt.get_affected_rows()) {
                    case 0: return { tupal::make_error_code(tupal::error_code::unknown_key),
                        to_json(str(boost::format("Not found: competition '%1%', competition class '%2%'") % competition_id % id)) };
                    case 1: break;
                    default: return { tupal::make_error_code(tupal::error_code::constraint_violation),
                        to_json(str(boost::format("Multiple match: competition '%1%', competition class '%2%'") % competition_id % id)) };
                }

                trx.commit();
                return { ok, tupal::to_json( tupal::key { id, competition_id } ) };
            }

            catch (const soci::soci_error & e) {
                TUPAL_MESSAGE(std::cerr) << e.what() << std::endl;
                return { tupal::make_error_code(tupal::error_code::system_error),
                    to_json(str(boost::format("%1%: competition '%2%', competition class '%3%'") % e.what() % competition_id % id)) };
            }
        }

        static void init_database(soci::session & session) {
            session.once << SCHEMA_COMPETITION_CLASS;
        }

    private:
        tupal::CompetitionManager & competition_manager;
        std::shared_ptr<soci::session> soci_session;
    };

    class CompetitorManagerImpl : public tupal::CompetitorManager
    {
    public:
        CompetitorManagerImpl() = delete;
        CompetitorManagerImpl(const CompetitorManagerImpl &) = delete;
        CompetitorManagerImpl(CompetitorManagerImpl &&) = delete;
        ~CompetitorManagerImpl() noexcept = default;

        CompetitorManagerImpl(tupal::CompetitionManager & cm, std::shared_ptr<soci::session> & session)
            : competition_manager(cm), soci_session(session) {
        }

        virtual tupal::result_type list(const std::string & competition_id) const {
            try {
                soci::rowset<> rows = (soci_session->prepare << 
                        "select "
                        "   ct.id, cc.comp_id, cc.id, ct.bib, ct.start_time_offset, ct.finish_time, ct.status, ct.name "
                        "from competition_class cc "
                        "left join competitor ct on ct.comp_class_id = cc.id "
                        "where cc.comp_id = :comp_id",
                    soci::use(competition_id));

                auto begin = rows.begin();
                if (begin == rows.end()) { // no competition found
                    return { tupal::make_error_code(tupal::error_code::unknown_key),
                        to_json(str(boost::format("Competition '%1%' not found!") % competition_id)) };
                }

                boost::json::array objects;
                if (begin->get_indicator(0) != soci::i_null) {
                    std::transform(begin, rows.end(), std::back_insert_iterator(objects), [&](const soci::row & row) {
                        return tupal::to_json( tupal::competitor {
                            { row.get<std::string>(0), row.get<std::string>(1) },
                            static_cast<uint16_t>(row.get<int>(3)),
                            row.get<std::string>(7),
                            tupal::from_duration_string(row.get<std::string>(4)),
                            tupal::from_date_string(row.get<std::string>(5)),
                            tupal::to_competitor_status(static_cast<uint8_t>(row.get<int>(6))),
                            row.get<std::string>(2)
                        });
                    });
                }

                return { ok, std::move(objects) };
            }

            catch (const soci::soci_error & e) {
                TUPAL_MESSAGE(std::cerr) << e.what() << std::endl;
                return { tupal::make_error_code(tupal::error_code::system_error),
                    to_json(str(boost::format("%1%: competition '%2%'") % e.what() % competition_id)) };
            }
        }

        virtual tupal::result_type get(const std::string & competition_id, const std::string & id) const {
            try {
                tupal::competitor ctor;
                std::string start_time_offset_str, finish_time_str;
                uint8_t status;

                *soci_session << "select id, comp_id, comp_class_id, bib, start_time_offset, finish_time, status, name from competitor where comp_id=:comp_id and id=:id",
                    soci::into(ctor.id.id), soci::into(ctor.id.comp_id), soci::into(ctor.comp_class_id), soci::into(ctor.bib),
                    soci::into(start_time_offset_str), soci::into(finish_time_str), 
                    soci::into(status), soci::into(ctor.name),
                    soci::use(competition_id), soci::use(id);

                if (soci_session->got_data()) {
                    ctor.start_time_offset = tupal::from_duration_string(start_time_offset_str);
                    ctor.finish_time = tupal::from_date_string(finish_time_str);
                    ctor.status = tupal::to_competitor_status(status);
                    return { ok, tupal::to_json(ctor) };
                }

                return { tupal::make_error_code(tupal::error_code::unknown_key),
                    to_json(str(boost::format("Competition '%1%' or competitor '%2%' not found!") % competition_id % id)) };
            }

            catch (const soci::soci_error & e) {
                TUPAL_MESSAGE(std::cerr) << e.what() << std::endl;
                return { tupal::make_error_code(tupal::error_code::system_error),
                    to_json(str(boost::format("%1%: competition '%2%', competitor '%3%'") % e.what() % competition_id % id)) };
            }
        }

        virtual tupal::result_type create(const std::string & competition_id, const boost::json::object & new_data) {
            try {               
                auto ctor = tupal::to_competitor(new_data);
                ctor.id.comp_id = std::string { competition_id };
                const std::string start_time_offset_str = tupal::to_duration_string(ctor.start_time_offset);
                const std::string finish_time_str = tupal::to_date_string(ctor.finish_time);
                uint8_t status = static_cast<uint8_t>(ctor.status);

                soci::transaction trx(*soci_session);
                *soci_session <<
                    "insert into competitor(id, comp_id, comp_class_id, bib, start_time_offset, finish_time, status, name) "
                        "values (:id, :comp_id, :comp_class_id, :bib, :start_time, :finish_time, :status, :name)",
                    soci::use(ctor.id.id), soci::use(ctor.id.comp_id), soci::use(ctor.comp_class_id), soci::use(ctor.bib),
                    soci::use(start_time_offset_str), soci::use(finish_time_str), soci::use(status), soci::use(ctor.name);
                trx.commit();

                return { ok, tupal::to_json(ctor) };
            }

            catch (const soci::soci_error & e) {
                const auto backend_name = soci_session->get_backend_name();
                TUPAL_MESSAGE(std::cerr) << "SOCI exception (" << backend_name << "): " << e.what() << std::endl;
                const auto error_msg { str(boost::format("%1%: competition '%2%'") % e.what() % competition_id) };

                const auto ec = handle_soci_error(backend_name, e);
                if (ec == tupal::make_error_condition(tupal::error_code::duplicate_key))
                    return { ec, to_json("Bib number or competitor ID is already assigned in this competition", new_data) };

                if (ec == tupal::make_error_condition(tupal::error_code::constraint_violation))
                    return { tupal::make_error_code(tupal::error_code::unknown_key), to_json(
                        str(boost::format("New competitor refers likely to non-existent competition '%1%") % competition_id),
                        new_data) };

                if (ec == tupal::make_error_condition(tupal::error_code::system_error))
                    return { tupal::make_error_code(tupal::error_code::system_error),
                        to_json(str(boost::format("System error: %1%") % e.what())) };

                return { ec, to_json(str(boost::format("%1%: competition '%2%'") % e.what() % competition_id), new_data) };
            }
        }

        virtual tupal::result_type update(const std::string & competition_id, const boost::json::object & new_data) {
            try {
                auto ctor = tupal::to_competitor(new_data);
                ctor.id.comp_id = std::string { competition_id };
                const std::string start_time_offset_str = tupal::to_duration_string(ctor.start_time_offset);
                const std::string finish_time_str = tupal::to_date_string(ctor.finish_time);
                uint8_t status = static_cast<uint8_t>(ctor.status);

                soci::transaction trx(*soci_session);
                soci::statement stmt = (soci_session->prepare <<
                        "update competitor set "
                        "   bib=:bib, comp_class_id=:comp_class_id, start_time_offset=:start_time_offset, "
                        "   finish_time=:finish_time, status=:status, name=:name "
                        "where id=:id and comp_id=:comp_id",
                    soci::use(ctor.bib), soci::use(ctor.comp_class_id), soci::use(start_time_offset_str), 
                    soci::use(finish_time_str), soci::use(status), soci::use(ctor.name), 
                    soci::use(ctor.id.id), soci::use(ctor.id.comp_id));
                stmt.execute(true);

                switch (stmt.get_affected_rows()) {
                    case 0: return { tupal::make_error_code(tupal::error_code::unknown_key),
                        to_json(str(boost::format("Not found: competition '%1%'") % competition_id), new_data) };
                    case 1: break;
                    default: return { tupal::make_error_code(tupal::error_code::constraint_violation),
                        to_json(str(boost::format("Multiple match: competition '%1%'") % competition_id), new_data) };
                }

                trx.commit();

                return { ok, tupal::to_json(ctor) };
            }

            catch (const soci::soci_error & e) {
                const auto backend_name = soci_session->get_backend_name();
                TUPAL_MESSAGE(std::cerr) << "SOCI exception (" << backend_name << "): " << e.what() << std::endl;
                const auto ec = handle_soci_error(backend_name, e);
                if (ec == tupal::make_error_condition(tupal::error_code::duplicate_key))
                    return { ec, to_json("Bib number is already assigned in this competition", new_data) };
                return { ec, to_json(str(boost::format("%1%: competition '%2%'") % e.what() % competition_id), new_data) };
            }
        }

        virtual tupal::result_type remove(const std::string & competition_id, const std::string & id) {
            try {
                soci::transaction trx(*soci_session);
                soci::statement stmt = (soci_session->prepare << "delete from competitor where id=:id and comp_id=:comp_id",
                    soci::use(id), soci::use(competition_id));
                stmt.execute(true);
                switch (stmt.get_affected_rows()) {
                    case 0: return { tupal::make_error_code(tupal::error_code::unknown_key),
                        to_json(str(boost::format("Not found: competition '%1%', competitor '%2%'") % competition_id % id)) };
                    case 1: break;
                    default: return { tupal::make_error_code(tupal::error_code::constraint_violation),
                        to_json(str(boost::format("Multiple match: competition '%1%', competitor '%2%'") % competition_id % id)) };
                }

                trx.commit();
                return { ok, tupal::to_json( tupal::key { id, competition_id } ) };
            }

            catch (const soci::soci_error & e) {
                TUPAL_MESSAGE(std::cerr) << e.what() << std::endl;
                return { tupal::make_error_code(tupal::error_code::system_error),
                    to_json(str(boost::format("%1%: competition '%2%', competitor '%3%'") % e.what() % competition_id % id)) };
            }
        }

        static void init_database(soci::session & session) {
            session.once << SCHEMA_COMPETITOR;
            session.once << INDEX_COMPETITOR_COMPETITION_BIB;
        }

    private:
        tupal::CompetitionManager & competition_manager;
        std::shared_ptr<soci::session> soci_session;
    };

    class FinishTimeEntryManagerImpl : public tupal::FinishTimeEntryManager
    {
    public:
        FinishTimeEntryManagerImpl() = delete;
        FinishTimeEntryManagerImpl(const FinishTimeEntryManagerImpl &) = delete;
        FinishTimeEntryManagerImpl(FinishTimeEntryManagerImpl &&) = delete;
        ~FinishTimeEntryManagerImpl() noexcept = default;

        FinishTimeEntryManagerImpl(tupal::CompetitionManager & cm, std::shared_ptr<soci::session> & session)
            : competition_manager(cm), soci_session(session) {
        }

        virtual tupal::result_type list(const std::string & competition_id) const {
            try {
                if (!competition_exists(*soci_session, competition_id)) {
                    return { tupal::make_error_code(tupal::error_code::unknown_key),
                        to_json(str(boost::format("Competition '%1%' not found!") % competition_id)) };
                }

                soci::rowset<> rows = (soci_session->prepare <<
                        "select id, comp_id, finish_time, status from finish_time_entry where comp_id=:comp_id order by finish_time asc, id asc",
                    soci::use(competition_id));

                boost::json::array objects;
                for (auto it = rows.begin(); it != rows.end(); ++it) {
                    objects.push_back(finish_time_entry_json(
                        it->get<std::string>(0), it->get<std::string>(1), it->get<std::string>(2), it->get<int>(3)));
                }

                return { ok, std::move(objects) };
            }
            catch (const soci::soci_error & e) {
                TUPAL_MESSAGE(std::cerr) << e.what() << std::endl;
                return { tupal::make_error_code(tupal::error_code::system_error),
                    to_json(str(boost::format("%1%: competition '%2%'") % e.what() % competition_id)) };
            }
        }

        virtual tupal::result_type get(const std::string & competition_id, const std::string & id) const {
            try {
                std::string entry_id;
                std::string entry_comp_id;
                std::string finish_time_str;
                int status = 0;

                *soci_session << "select id, comp_id, finish_time, status from finish_time_entry where comp_id=:comp_id and id=:id",
                    soci::into(entry_id), soci::into(entry_comp_id), soci::into(finish_time_str), soci::into(status),
                    soci::use(competition_id), soci::use(id);

                if (soci_session->got_data()) {
                    return { ok, finish_time_entry_json(entry_id, entry_comp_id, finish_time_str, status) };
                }

                return { tupal::make_error_code(tupal::error_code::unknown_key),
                    to_json(str(boost::format("Competition '%1%' or finish time entry '%2%' not found!") % competition_id % id)) };
            }
            catch (const soci::soci_error & e) {
                TUPAL_MESSAGE(std::cerr) << e.what() << std::endl;
                return { tupal::make_error_code(tupal::error_code::system_error),
                    to_json(str(boost::format("%1%: competition '%2%', finish time entry '%3%'") % e.what() % competition_id % id)) };
            }
        }

        virtual tupal::result_type create(const std::string & competition_id, const boost::json::object & new_data) {
            try {
                if (!competition_exists(*soci_session, competition_id)) {
                    return { tupal::make_error_code(tupal::error_code::unknown_key),
                        to_json(str(boost::format("Competition '%1%' not found!") % competition_id), new_data) };
                }

                auto entry = tupal::to_finish_time_entry(new_data);
                entry.id.comp_id = competition_id;
                const auto finish_time = tupal::to_date_string(entry.finish_time);
                const auto status = static_cast<int>(entry.status);
                const auto id = entry.id.id;

                soci::transaction trx(*soci_session);
                *soci_session << "insert into finish_time_entry (id, comp_id, finish_time, status) values (:id, :comp_id, :finish_time, :status)",
                    soci::use(id), soci::use(competition_id), soci::use(finish_time), soci::use(status);
                trx.commit();

                return { ok, finish_time_entry_json(id, competition_id, finish_time, status) };
            }
            catch (const soci::soci_error & e) {
                const auto backend_name = soci_session->get_backend_name();
                TUPAL_MESSAGE(std::cerr) << "SOCI exception (" << backend_name << "): " << e.what() << std::endl;
                const auto ec = handle_soci_error(backend_name, e);
                if (ec == tupal::make_error_condition(tupal::error_code::duplicate_key))
                    return { ec, to_json("Finish time entry with the same id already exists in this competition", new_data) };
                return { ec, to_json(str(boost::format("%1%: competition '%2%'") % e.what() % competition_id), new_data) };
            }
            catch (const std::invalid_argument & e) {
                return { tupal::make_error_code(tupal::error_code::invalid_arguments),
                    to_json(e.what(), new_data) };
            }
        }

        virtual tupal::result_type update(const std::string & competition_id, const boost::json::object & new_data) {
            try {
                const auto id = finish_entry_id(new_data);
                std::string finish_time = "";
                int status = 0;

                if (new_data.find("finish_time") == new_data.end()) {
                    *soci_session << "select finish_time from finish_time_entry where id=:id and comp_id=:comp_id",
                        soci::into(finish_time), soci::use(id), soci::use(competition_id);
                    if (!soci_session->got_data())
                        return { tupal::make_error_code(tupal::error_code::unknown_key),
                            to_json(str(boost::format("Not found: competition '%1%', finish time entry '%2%'") % competition_id % id), new_data) };
                } else {
                    finish_time = required_string(new_data, "finish_time");
                }

                if (new_data.find("status") == new_data.end()) {
                    *soci_session << "select status from finish_time_entry where id=:id and comp_id=:comp_id",
                        soci::into(status), soci::use(id), soci::use(competition_id);
                    if (!soci_session->got_data())
                        return { tupal::make_error_code(tupal::error_code::unknown_key),
                            to_json(str(boost::format("Not found: competition '%1%', finish time entry '%2%'") % competition_id % id), new_data) };
                } else {
                    status = finish_entry_status(new_data);
                }

                soci::transaction trx(*soci_session);
                soci::statement stmt = (soci_session->prepare <<
                        "update finish_time_entry set finish_time=:finish_time, status=:status where id=:id and comp_id=:comp_id",
                    soci::use(finish_time), soci::use(status), soci::use(id), soci::use(competition_id));
                stmt.execute(true);
                switch (stmt.get_affected_rows()) {
                    case 0: return { tupal::make_error_code(tupal::error_code::unknown_key),
                        to_json(str(boost::format("Not found: competition '%1%'") % competition_id), new_data) };
                    case 1: break;
                    default: return { tupal::make_error_code(tupal::error_code::constraint_violation),
                        to_json(str(boost::format("Multiple match: competition '%1%'") % competition_id), new_data) };
                }
                trx.commit();

                return { ok, finish_time_entry_json(id, competition_id, finish_time, status) };
            }
            catch (const soci::soci_error & e) {
                TUPAL_MESSAGE(std::cerr) << e.what() << std::endl;
                return { tupal::make_error_code(tupal::error_code::system_error),
                    to_json(str(boost::format("%1%: competition '%2%'") % e.what() % competition_id), new_data) };
            }
            catch (const std::invalid_argument & e) {
                return { tupal::make_error_code(tupal::error_code::invalid_arguments),
                    to_json(e.what(), new_data) };
            }
        }

        virtual tupal::result_type remove(const std::string & competition_id, const std::string & id) {
            try {
                soci::transaction trx(*soci_session);
                soci::statement stmt = (soci_session->prepare <<
                        "delete from finish_time_entry where id=:id and comp_id=:comp_id",
                    soci::use(id), soci::use(competition_id));
                stmt.execute(true);
                switch (stmt.get_affected_rows()) {
                    case 0: return { tupal::make_error_code(tupal::error_code::unknown_key),
                        to_json(str(boost::format("Not found: competition '%1%', finish time entry '%2%'") % competition_id % id)) };
                    case 1: break;
                    default: return { tupal::make_error_code(tupal::error_code::constraint_violation),
                        to_json(str(boost::format("Multiple match: competition '%1%', finish time entry '%2%'") % competition_id % id)) };
                }

                trx.commit();
                return { ok, tupal::to_json(tupal::key { id, competition_id }) };
            }
            catch (const soci::soci_error & e) {
                TUPAL_MESSAGE(std::cerr) << e.what() << std::endl;
                return { tupal::make_error_code(tupal::error_code::system_error),
                    to_json(str(boost::format("%1%: competition '%2%', finish time entry '%3%'") % e.what() % competition_id % id)) };
            }
        }

        static void init_database(soci::session & session) {
            session.once << SCHEMA_FINISH_TIME_ENTRY;
        }

    private:
        tupal::CompetitionManager & competition_manager;
        std::shared_ptr<soci::session> soci_session;
    };

    class FinishBibEntryManagerImpl : public tupal::FinishBibEntryManager
    {
    public:
        FinishBibEntryManagerImpl() = delete;
        FinishBibEntryManagerImpl(const FinishBibEntryManagerImpl &) = delete;
        FinishBibEntryManagerImpl(FinishBibEntryManagerImpl &&) = delete;
        ~FinishBibEntryManagerImpl() noexcept = default;

        FinishBibEntryManagerImpl(tupal::CompetitionManager & cm, std::shared_ptr<soci::session> & session)
            : competition_manager(cm), soci_session(session) {
        }

        virtual tupal::result_type list(const std::string & competition_id) const {
            try {
                if (!competition_exists(*soci_session, competition_id)) {
                    return { tupal::make_error_code(tupal::error_code::unknown_key),
                        to_json(str(boost::format("Competition '%1%' not found!") % competition_id)) };
                }

                soci::rowset<> rows = (soci_session->prepare <<
                        "select id, comp_id, sequence_no, bib, status, confirmed_time_entry_id from finish_bib_entry "
                        "where comp_id=:comp_id order by sequence_no asc, id asc",
                    soci::use(competition_id));

                boost::json::array objects;
                for (auto it = rows.begin(); it != rows.end(); ++it) {
                    objects.push_back(finish_bib_entry_json(
                        it->get<std::string>(0), it->get<std::string>(1), it->get<int>(2), *it));
                }

                return { ok, std::move(objects) };
            }
            catch (const soci::soci_error & e) {
                TUPAL_MESSAGE(std::cerr) << e.what() << std::endl;
                return { tupal::make_error_code(tupal::error_code::system_error),
                    to_json(str(boost::format("%1%: competition '%2%'") % e.what() % competition_id)) };
            }
        }

        virtual tupal::result_type get(const std::string & competition_id, const std::string & id) const {
            try {
                std::string entry_id;
                std::string entry_comp_id;
                int sequence_no = 0;
                int bib = 0;
                soci::indicator bib_indicator = soci::i_ok;
                int status = 0;
                std::string confirmed_time_entry_id;
                soci::indicator confirmed_time_entry_indicator = soci::i_ok;

                *soci_session << "select id, comp_id, sequence_no, bib, status, confirmed_time_entry_id from finish_bib_entry where comp_id=:comp_id and id=:id",
                    soci::into(entry_id), soci::into(entry_comp_id), soci::into(sequence_no),
                    soci::into(bib, bib_indicator), soci::into(status), soci::into(confirmed_time_entry_id, confirmed_time_entry_indicator),
                    soci::use(competition_id), soci::use(id);

                if (soci_session->got_data()) {
                    boost::json::object result {
                        { "id", entry_id }, { "comp_id", entry_comp_id },
                        { "sequence_no", sequence_no }, { "status", status }
                    };
                    result["bib"] = bib_indicator == soci::i_null ? boost::json::value {} : boost::json::value(bib);
                    result["confirmed_time_entry_id"] = confirmed_time_entry_indicator == soci::i_null
                        ? boost::json::value {} : boost::json::value(confirmed_time_entry_id);
                    return { ok, std::move(result) };
                }

                return { tupal::make_error_code(tupal::error_code::unknown_key),
                    to_json(str(boost::format("Competition '%1%' or finish bib entry '%2%' not found!") % competition_id % id)) };
            }
            catch (const soci::soci_error & e) {
                TUPAL_MESSAGE(std::cerr) << e.what() << std::endl;
                return { tupal::make_error_code(tupal::error_code::system_error),
                    to_json(str(boost::format("%1%: competition '%2%', finish bib entry '%3%'") % e.what() % competition_id % id)) };
            }
        }

        virtual tupal::result_type create(const std::string & competition_id, const boost::json::object & new_data) {
            try {
                if (!competition_exists(*soci_session, competition_id)) {
                    return { tupal::make_error_code(tupal::error_code::unknown_key),
                        to_json(str(boost::format("Competition '%1%' not found!") % competition_id), new_data) };
                }

                auto entry = tupal::to_finish_bib_entry(new_data);
                entry.id.comp_id = competition_id;
                const auto id = entry.id.id;
                const auto sequence_no = entry.sequence_no;
                auto bib_value = entry.bib.value_or(0);
                auto bib_indicator = entry.bib ? soci::i_ok : soci::i_null;
                const auto status = static_cast<int>(entry.status);
                auto confirmed_time_entry_id = entry.confirmed_time_entry_id.value_or("");
                auto confirmed_time_entry_id_indicator =
                    entry.confirmed_time_entry_id ? soci::i_ok : soci::i_null;

                soci::transaction trx(*soci_session);
                *soci_session << "insert into finish_bib_entry (id, comp_id, sequence_no, bib, status, confirmed_time_entry_id) values (:id, :comp_id, :sequence_no, :bib, :status, :confirmed_time_entry_id)",
                    soci::use(id), soci::use(competition_id), soci::use(sequence_no),
                    soci::use(bib_value, bib_indicator), soci::use(status),
                    soci::use(confirmed_time_entry_id, confirmed_time_entry_id_indicator);
                trx.commit();

                boost::json::object result {
                    { "id", id }, { "comp_id", competition_id },
                    { "sequence_no", sequence_no }, { "status", status }
                };
                result["bib"] = bib_indicator == soci::i_null ? boost::json::value {} : boost::json::value(bib_value);
                result["confirmed_time_entry_id"] = confirmed_time_entry_id_indicator == soci::i_null
                    ? boost::json::value {} : boost::json::value(confirmed_time_entry_id);
                return { ok, std::move(result) };
            }
            catch (const soci::soci_error & e) {
                const auto backend_name = soci_session->get_backend_name();
                TUPAL_MESSAGE(std::cerr) << "SOCI exception (" << backend_name << "): " << e.what() << std::endl;
                const auto ec = handle_soci_error(backend_name, e);
                if (ec == tupal::make_error_condition(tupal::error_code::duplicate_key))
                    return { ec, to_json("Finish bib entry with the same sequence number already exists in this competition", new_data) };
                return { ec, to_json(str(boost::format("%1%: competition '%2%'") % e.what() % competition_id), new_data) };
            }
            catch (const std::invalid_argument & e) {
                return { tupal::make_error_code(tupal::error_code::invalid_arguments),
                    to_json(e.what(), new_data) };
            }
        }

        virtual tupal::result_type update(const std::string & competition_id, const boost::json::object & new_data) {
            try {
                const auto id = finish_entry_id(new_data);
                int sequence_no = 0;
                if (new_data.find("sequence_no") == new_data.end()) {
                    *soci_session << "select sequence_no from finish_bib_entry where id=:id and comp_id=:comp_id",
                        soci::into(sequence_no), soci::use(id), soci::use(competition_id);
                    if (!soci_session->got_data())
                        return { tupal::make_error_code(tupal::error_code::unknown_key),
                            to_json(str(boost::format("Not found: competition '%1%', finish bib entry '%2%'") % competition_id % id), new_data) };
                } else {
                    const auto value = new_data.find("sequence_no");
                    if (!value->value().is_int64())
                        throw std::invalid_argument("'sequence_no' must be an integer");
                    sequence_no = static_cast<int>(value->value().as_int64());
                    if (sequence_no < 0)
                        throw std::invalid_argument("'sequence_no' must be a non-negative integer");
                }

                int bib_value = 0;
                soci::indicator bib_indicator = soci::i_null;
                if (new_data.find("bib") == new_data.end() || new_data.find("bib")->value().is_null()) {
                    *soci_session << "select bib from finish_bib_entry where id=:id and comp_id=:comp_id",
                        soci::into(bib_value, bib_indicator), soci::use(id), soci::use(competition_id);
                    if (!soci_session->got_data())
                        return { tupal::make_error_code(tupal::error_code::unknown_key),
                            to_json(str(boost::format("Not found: competition '%1%', finish bib entry '%2%'") % competition_id % id), new_data) };
                } else {
                    if (!new_data.find("bib")->value().is_int64())
                        throw std::invalid_argument("'bib' must be an integer or null");
                    bib_value = static_cast<int>(new_data.find("bib")->value().as_int64());
                    bib_indicator = soci::i_ok;
                }

                int status = 0;
                if (new_data.find("status") == new_data.end()) {
                    *soci_session << "select status from finish_bib_entry where id=:id and comp_id=:comp_id",
                        soci::into(status), soci::use(id), soci::use(competition_id);
                    if (!soci_session->got_data())
                        return { tupal::make_error_code(tupal::error_code::unknown_key),
                            to_json(str(boost::format("Not found: competition '%1%', finish bib entry '%2%'") % competition_id % id), new_data) };
                } else {
                    status = finish_entry_status(new_data);
                }

                std::string confirmed_time_entry_id;
                soci::indicator confirmed_time_entry_id_indicator = soci::i_null;
                if (new_data.find("confirmed_time_entry_id") == new_data.end() || new_data.find("confirmed_time_entry_id")->value().is_null()) {
                    *soci_session << "select confirmed_time_entry_id from finish_bib_entry where id=:id and comp_id=:comp_id",
                        soci::into(confirmed_time_entry_id, confirmed_time_entry_id_indicator), soci::use(id), soci::use(competition_id);
                    if (!soci_session->got_data())
                        return { tupal::make_error_code(tupal::error_code::unknown_key),
                            to_json(str(boost::format("Not found: competition '%1%', finish bib entry '%2%'") % competition_id % id), new_data) };
                } else {
                    const auto value = new_data.find("confirmed_time_entry_id");
                    if (!value->value().is_string() || value->value().as_string().empty())
                        throw std::invalid_argument("'confirmed_time_entry_id' must be a non-empty string or null");
                    confirmed_time_entry_id = std::string { value->value().as_string() };
                    confirmed_time_entry_id_indicator = soci::i_ok;
                }

                soci::transaction trx(*soci_session);
                soci::statement stmt = (soci_session->prepare <<
                        "update finish_bib_entry set sequence_no=:sequence_no, bib=:bib, status=:status, confirmed_time_entry_id=:confirmed_time_entry_id where id=:id and comp_id=:comp_id",
                    soci::use(sequence_no), soci::use(bib_value, bib_indicator), soci::use(status),
                    soci::use(confirmed_time_entry_id, confirmed_time_entry_id_indicator),
                    soci::use(id), soci::use(competition_id));
                stmt.execute(true);
                switch (stmt.get_affected_rows()) {
                    case 0: return { tupal::make_error_code(tupal::error_code::unknown_key),
                        to_json(str(boost::format("Not found: competition '%1%'") % competition_id), new_data) };
                    case 1: break;
                    default: return { tupal::make_error_code(tupal::error_code::constraint_violation),
                        to_json(str(boost::format("Multiple match: competition '%1%'") % competition_id), new_data) };
                }
                trx.commit();

                boost::json::object result {
                    { "id", id }, { "comp_id", competition_id },
                    { "sequence_no", sequence_no }, { "status", status }
                };
                result["bib"] = bib_indicator == soci::i_null ? boost::json::value {} : boost::json::value(bib_value);
                result["confirmed_time_entry_id"] = confirmed_time_entry_id_indicator == soci::i_null
                    ? boost::json::value {} : boost::json::value(confirmed_time_entry_id);
                return { ok, std::move(result) };
            }
            catch (const soci::soci_error & e) {
                TUPAL_MESSAGE(std::cerr) << e.what() << std::endl;
                return { tupal::make_error_code(tupal::error_code::system_error),
                    to_json(str(boost::format("%1%: competition '%2%'") % e.what() % competition_id), new_data) };
            }
            catch (const std::invalid_argument & e) {
                return { tupal::make_error_code(tupal::error_code::invalid_arguments),
                    to_json(e.what(), new_data) };
            }
        }

        virtual tupal::result_type remove(const std::string & competition_id, const std::string & id) {
            try {
                soci::transaction trx(*soci_session);
                soci::statement stmt = (soci_session->prepare <<
                        "delete from finish_bib_entry where id=:id and comp_id=:comp_id",
                    soci::use(id), soci::use(competition_id));
                stmt.execute(true);
                switch (stmt.get_affected_rows()) {
                    case 0: return { tupal::make_error_code(tupal::error_code::unknown_key),
                        to_json(str(boost::format("Not found: competition '%1%', finish bib entry '%2%'") % competition_id % id)) };
                    case 1: break;
                    default: return { tupal::make_error_code(tupal::error_code::constraint_violation),
                        to_json(str(boost::format("Multiple match: competition '%1%', finish bib entry '%2%'") % competition_id % id)) };
                }

                trx.commit();
                return { ok, tupal::to_json(tupal::key { id, competition_id }) };
            }
            catch (const soci::soci_error & e) {
                TUPAL_MESSAGE(std::cerr) << e.what() << std::endl;
                return { tupal::make_error_code(tupal::error_code::system_error),
                    to_json(str(boost::format("%1%: competition '%2%', finish bib entry '%3%'") % e.what() % competition_id % id)) };
            }
        }

        static void init_database(soci::session & session) {
            session.once << SCHEMA_FINISH_BIB_ENTRY;
        }

    private:
        tupal::CompetitionManager & competition_manager;
        std::shared_ptr<soci::session> soci_session;
    };

    class CompetitionManagerImpl : public tupal::CompetitionManager
    {
    public:
        static bool is_database_empty(soci::session & session) {
            std::vector<std::string> tables(10);
            session.get_table_names(), soci::into(tables);
            return std::find_if(tables.begin(), tables.end(), 
                    [](const auto & val) -> bool { return boost::iequals(val, "configuration"); }) == tables.end();
        }

        static std::string database_schema_version(soci::session & session) {
            std::string version;
            session << "SELECT value FROM configuration WHERE name = 'schema_version'", soci::into(version);
            return version;
        }

        static void init_database(soci::session & session) {
            session.once << SCHEMA_CONFIGURATION;
            session.once << "insert into configuration values ('schema_version', '" << SCHEMA_VERSION << "')";
            session.once << SCHEMA_COMPETITION;            
        }

        CompetitionManagerImpl() = delete;
        CompetitionManagerImpl(const CompetitionManagerImpl &) = delete;
        CompetitionManagerImpl(CompetitionManagerImpl &&) = delete;
        ~CompetitionManagerImpl() noexcept = default;

        CompetitionManagerImpl(std::shared_ptr<soci::session> session) 
            : soci_session(session),
                start_group_manager(new StartGroupManagerImpl(*this, session)),
                competition_class_manager(new CompetitionClassManagerImpl(*this, session)),
                competitor_manager(new CompetitorManagerImpl(*this, session)),
                finish_time_entry_manager(new FinishTimeEntryManagerImpl(*this, session)),
                finish_bib_entry_manager(new FinishBibEntryManagerImpl(*this, session)) {

            if (is_database_empty(*session)) {
                soci::transaction trx(*session);
                CompetitionManagerImpl::init_database(*session);
                StartGroupManagerImpl::init_database(*session);
                CompetitionClassManagerImpl::init_database(*session);
                CompetitorManagerImpl::init_database(*session);
                FinishTimeEntryManagerImpl::init_database(*session);
                FinishBibEntryManagerImpl::init_database(*session);
                trx.commit();
            } else if (database_schema_version(*session) != SCHEMA_VERSION) {
                throw std::runtime_error("Unexpected database schema version!");
            }
        }

        virtual tupal::result_type list() const {
            try {
                soci::rowset<> rows = (soci_session->prepare << "select id, date, title from competition");

                boost::json::array objects;
                for (auto it = rows.begin(); it != rows.end(); it++) {
                    objects.push_back(tupal::to_json( tupal::competition {
                        it->get<std::string>(0),
                        tupal::from_date_string(it->get<std::string>(1)),
                        it->get<std::string>(2)
                    }) );
                }

                return { ok, std::move(objects) };
            }

            catch (const soci::soci_error & e) {
                TUPAL_MESSAGE(std::cerr) << e.what() << std::endl;
                return { tupal::make_error_code(tupal::error_code::system_error), to_json(e.what()) };
            }
        }

        virtual tupal::result_type get(const std::string & id) const {
            try {
                tupal::competition comp;
                comp.id = std::string { id };
                std::string date_str;

                *soci_session << "select id, date, title from competition where id = :id",
                    soci::into(comp.id), soci::into(date_str), soci::into(comp.title), soci::use(comp.id);
                if (soci_session->got_data()) {
                    comp.date = tupal::from_date_string(date_str);
                    return { ok, tupal::to_json(comp) };
                }

                return { tupal::make_error_code(tupal::error_code::unknown_key),
                    to_json(str(boost::format("Competition '%1%' not found!") % id)) };
            }

            catch (const soci::soci_error & e) {
                TUPAL_MESSAGE(std::cerr) << e.what() << std::endl;
                return { tupal::make_error_code(tupal::error_code::system_error),
                    to_json(str(boost::format("%1%: competition '%2%'") % e.what() % id)) };
            }
        }

        virtual tupal::result_type create(const boost::json::object & new_data) {
            try {
                auto comp = tupal::to_competition(new_data);
                std::string date_str = to_date_string(comp.date);

                soci::transaction trx(*soci_session);
                *soci_session << "insert into competition(id, date, title) values (:id, :date, :title)",
                    soci::use(comp.id), soci::use(date_str), soci::use(comp.title);
                trx.commit();

                return { ok, tupal::to_json(comp) };
            }

            catch (const soci::soci_error & e) {
                TUPAL_MESSAGE(std::cerr) << "SOCI system error: " << e.what() << std::endl;

                const auto ec = handle_soci_error(soci_session->get_backend_name(), e);
                if (ec == tupal::make_error_condition(tupal::error_code::system_error))
                    return { tupal::make_error_code(tupal::error_code::system_error), to_json(e.what(), new_data) };

                return { ec, to_json(e.what(), new_data) };
            }
        }

        virtual tupal::result_type update(const boost::json::object & new_data) {
            try {
                auto comp = tupal::to_competition(new_data);
                std::string date_str = to_date_string(comp.date);

                soci::transaction trx(*soci_session);
                soci::statement stmt = (soci_session->prepare << "update competition set date=:date, title=:title where id=:id",
                    soci::use(date_str), soci::use(comp.title), soci::use(comp.id));
                stmt.execute(true);
                switch (stmt.get_affected_rows()) {
                    case 0: return { tupal::make_error_code(tupal::error_code::unknown_key),
                        to_json("Not found", new_data) };
                    case 1: break;
                    default: return { tupal::make_error_code(tupal::error_code::constraint_violation),
                        to_json("Multiple match", new_data) };
                }
                trx.commit();

                return { ok, tupal::to_json(comp) };
            }

            catch (const soci::soci_error & e) {
                TUPAL_MESSAGE(std::cerr) << e.what() << std::endl;
                return { tupal::make_error_code(tupal::error_code::system_error), to_json(e.what(), new_data) };
            }
        }

        virtual tupal::result_type remove(const std::string & id) {
            try {
                soci::transaction trx(*soci_session);
                soci::statement stmt = (soci_session->prepare << "delete from competition where id=:id", soci::use(id));
                stmt.execute(true);
                switch (stmt.get_affected_rows()) {
                    case 0: return { tupal::make_error_code(tupal::error_code::unknown_key),
                        to_json(str(boost::format("Not found: competition '%1%'") % id)) };
                    case 1: break;
                    default: return { tupal::make_error_code(tupal::error_code::constraint_violation),
                        to_json(str(boost::format("Multiple match: competition '%1%'") % id)) };
                }
                trx.commit();
                return { ok, boost::json::object { {"id", id} } };
            }

            catch (const soci::soci_error & e) {
                TUPAL_MESSAGE(std::cerr) << e.what() << std::endl;
                return { tupal::make_error_code(tupal::error_code::system_error),
                    to_json(str(boost::format("%1%: competition '%2%'") % e.what() % id)) };
            }
        }

        std::shared_ptr<tupal::StartGroupManager> getStartGroupManager() {
            return start_group_manager;
        }

        std::shared_ptr<tupal::CompetitionClassManager> getCompetitionClassManager() {
            return competition_class_manager;
        }
    
        std::shared_ptr<tupal::CompetitorManager> getCompetitorManager() {
            return competitor_manager;
        }

        std::shared_ptr<tupal::FinishTimeEntryManager> getFinishTimeEntryManager() {
            return finish_time_entry_manager;
        }

        std::shared_ptr<tupal::FinishBibEntryManager> getFinishBibEntryManager() {
            return finish_bib_entry_manager;
        }

    private:
        std::shared_ptr<soci::session> soci_session;
        std::shared_ptr<tupal::StartGroupManager> start_group_manager;
        std::shared_ptr<tupal::CompetitionClassManager> competition_class_manager;
        std::shared_ptr<tupal::CompetitorManager> competitor_manager;
        std::shared_ptr<tupal::FinishTimeEntryManager> finish_time_entry_manager;
        std::shared_ptr<tupal::FinishBibEntryManager> finish_bib_entry_manager;
    };

    std::unique_ptr<soci::session> new_session(const std::string & connection_spec) {

        auto session = std::make_unique<soci::session>();
        session->open(connection_spec);

        const auto backend_name = session->get_backend_name();
        if (backend_name == "sqlite3") {
            *session << "PRAGMA foreign_keys=ON;";
        }

        return session;
    }

    class tupal_error_category_impl : public std::error_category {
    public:
        const char * name() const noexcept override { return "tupal"; }
        std::string message(int ev) const override {
            if (ev == static_cast<int>(tupal::error_code::ok)) return "Success";
            if (ev == static_cast<int>(tupal::error_code::duplicate_key)) return "Duplicate key";
            if (ev == static_cast<int>(tupal::error_code::unknown_key)) return "Unknown key";
            if (ev == static_cast<int>(tupal::error_code::constraint_violation)) return "Constraint violation";
            if (ev == static_cast<int>(tupal::error_code::invalid_arguments)) return "Invalid arguments";
            if (ev == static_cast<int>(tupal::error_code::system_error)) return "System error";
            return "Unknown error code";
        }
    };
}

namespace tupal {

    std::shared_ptr<CompetitionManager> 
    CompetitionManager::new_competition_manager(const std::string & connection_spec) {
        return std::shared_ptr<CompetitionManager>(new CompetitionManagerImpl(std::shared_ptr<soci::session>(new_session(connection_spec))));
    }

    std::error_category & error_category() {
        static tupal_error_category_impl instance;
        return instance;
    }

    std::error_code make_error_code(tupal::error_code e) {
        return std::error_code(static_cast<int>(e), tupal::error_category());
    }

    std::error_condition make_error_condition(tupal::error_code e) {
        return std::error_condition(static_cast<int>(e), tupal::error_category());
    }
}

#include <memory>

#include <iostream> // debug only!

#include "boost/algorithm/string.hpp"
#include "boost/url.hpp"
#include "boost/date_time.hpp"
#include "boost/current_function.hpp"

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

    const static std::error_code ok = std::error_code {};

#if 0
    const boost::json::value make_start_group(const std::string & id, const std::string & comp_id, const std::string & title,
            const std::string & start_time, int first_bib) {
        return boost::json::object {
            { "id", id },
            { "comp_id", comp_id },
            { "title", title },
            { "start_time", start_time },
            { "first_bib", first_bib }
        };
    }

    boost::json::value make_competitor(const std::string & id, const std::string & comp_id,
            const std::string & comp_class_id, int bib, const std::string & start_time_offset,
            const std::string & finish_time, int status, const std::string & name) {
        return boost::json::object {
            { "id", id },
            { "comp_id", comp_id },
            { "comp_class_id", comp_class_id },
            { "bib", bib },
            { "start_time_offset", start_time_offset },
            { "finish_time", finish_time },
            { "status", status },
            { "name", name }
        };
    }

    boost::json::value make_competition(const std::string & id, const std::string & date,
            const std::string & title) {
        return boost::json::object {
            { "id", id },
            { "date", date },
            { "title", title }
        };
    }

    boost::json::value make_competition_class(const std::string & start_group_id, const std::string & id,
            const std::string & name) {
        return boost::json::object {
            { "start_group_id", start_group_id },
            { "id", id },
            { "title", name }
        };
    }

    boost::json::value make_removed(const std::string & comp_id, const std::string & sub_elem_id) {
        return boost::json::object {
            { "comp_id", comp_id },
            { "id", sub_elem_id }
        };
    }
#endif

    std::error_code handle_soci_error(const std::string & backend_name, const soci::soci_error & e) {
        switch (e.get_error_category()) {
            case soci::soci_error::unknown:
                break;
            case soci::soci_error::no_data:
                return tupal::make_error_code(tupal::error_code::unknown_key);
            case soci::soci_error::constraint_violation:
                return tupal::make_error_code(tupal::error_code::constraint_violation);
            case soci::soci_error::connection_error:
            case soci::soci_error::invalid_statement:
            case soci::soci_error::no_privilege:
            case soci::soci_error::unknown_transaction_state:
            case soci::soci_error::system_error:
            default:
                return tupal::make_error_code(tupal::error_code::system_error);
        }

        if (backend_name == "sqlite3" && boost::icontains(e.what(), "unique constraint")) {
            return tupal::make_error_code(tupal::error_code::duplicate_key);
        }

        if (backend_name == "sqlite3" && boost::icontains(e.what(), "foreign key constraint")) {
            return tupal::make_error_code(tupal::error_code::constraint_violation);
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
                    << "select s.id, c.id, s.title, s.start_time, first_bib from competition c "
                            "left join start_group s on c.id = s.comp_id where c.id=:comp_id",
                        soci::use(competition_id));
    
                auto begin = rows.begin();
                if (begin == rows.end()) { // competition not found
                    return { tupal::make_error_code(tupal::error_code::unknown_key), nullptr };
                }

                boost::json::array objects;
                if (begin->get_indicator(0) != soci::i_null) {
                    std::transform(begin, rows.end(), std::back_insert_iterator(objects), [](const soci::row & row) -> boost::json::value { 
                        return tupal::to_json(tupal::start_group {
                            .id = tupal::key {
                                .id = row.get<std::string>(0),
                                .comp_id = row.get<std::string>(1),
                            },
                            .title = row.get<std::string>(2),
                            .first_start_time = tupal::from_date_string(row.get<std::string>(3)),
                            .first_bib = static_cast<uint16_t>(row.get<int>(4))
                        });
                    });
                }

                return { ok, std::move(objects) };
            }

            catch (const soci::soci_error & e) {
                TUPAL_MESSAGE(std::cerr) << e.what() << std::endl;
                return { tupal::make_error_code(tupal::error_code::system_error), nullptr };
            }
        }

        virtual tupal::result_type get(const std::string & competition_id, const std::string & id) const {
            try {
                tupal::start_group sg;
                std::string first_start_time_str;
                *soci_session << "select id, comp_id, title, start_time, first_bib "
                        "from start_group "
                        "where id = :id and comp_id=:comp_id",
                    soci::into(sg.id.id), soci::into(sg.id.comp_id), soci::into(sg.title), soci::into(first_start_time_str),
                    soci::into(sg.first_bib), soci::use(id), soci::use(competition_id);
                    
                if (soci_session->got_data()) {
                    sg.first_start_time = tupal::from_date_string(first_start_time_str);
                    return { ok, tupal::to_json(sg) };
                }

                return { tupal::make_error_code(tupal::error_code::unknown_key), nullptr };
            }

            catch (const soci::soci_error & e) {
                TUPAL_MESSAGE(std::cerr) << e.what() << std::endl;
                return { tupal::make_error_code(tupal::error_code::system_error), nullptr };
            }
        }

        virtual tupal::result_type create(const std::string & competition_id, const boost::json::object & new_data) {
            try {
                auto new_sg = tupal::to_start_group(new_data);
                new_sg.id.comp_id = std::string { competition_id };
                const auto first_start_time_str = to_date_string(new_sg.first_start_time);

                soci::transaction trx(*soci_session);
                *soci_session << "insert into start_group(id, comp_id, title, start_time, first_bib) "
                        "values (:id, :comp_id, :title, :start_time, :first_bib)",
                    soci::use(new_sg.id.id), soci::use(new_sg.id.comp_id), soci::use(new_sg.title),
                    soci::use(first_start_time_str), soci::use(new_sg.first_bib);
                trx.commit();

                return { ok, tupal::to_json(new_sg) };
            }

            catch (const soci::soci_error & e) {
                const auto ec = handle_soci_error(soci_session->get_backend_name(), e);
                if (ec == tupal::make_error_condition(tupal::error_code::constraint_violation)) {
                    return { tupal::make_error_code(tupal::error_code::unknown_key), nullptr };
                } else if (ec == tupal::make_error_condition(tupal::error_code::system_error)) {
                    TUPAL_MESSAGE(std::cerr) << "SOCI system error: " << e.what() << std::endl;
                }

                return { ec, nullptr };
            }
        }

        virtual tupal::result_type update(const std::string & competition_id, const boost::json::object & new_data) {
            try {
                auto new_sg = tupal::to_start_group(new_data);
                new_sg.id.comp_id = std::string { competition_id };
                const auto first_start_time_str = to_date_string(new_sg.first_start_time);

                soci::transaction trx(*soci_session);
                soci::statement stmt = (soci_session->prepare 
                    << "update start_group set title=:title, start_time=:start_time, first_bib=:first_bib "
                        "where id=:id and comp_id=:comp_id",
                    soci::use(new_sg.title), soci::use(first_start_time_str), soci::use(new_sg.first_bib),
                    soci::use(new_sg.id.id), soci::use(new_sg.id.comp_id));
                stmt.execute(true);
                switch (stmt.get_affected_rows()) {
                    case 0: return { tupal::make_error_code(tupal::error_code::unknown_key), nullptr };
                    case 1: break;
                    default: return { tupal::make_error_code(tupal::error_code::constraint_violation), nullptr };
                }
                trx.commit();

                return { ok, tupal::to_json(new_sg) };
            }

            catch (const soci::soci_error & e) {
                TUPAL_MESSAGE(std::cerr) << e.what() << std::endl;
                return { tupal::make_error_code(tupal::error_code::system_error), nullptr };
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
                    case 0: return { tupal::make_error_code(tupal::error_code::unknown_key), nullptr };
                    case 1: break;
                    default: return { tupal::make_error_code(tupal::error_code::constraint_violation), nullptr };
                }
                trx.commit();

                return { ok, tupal::to_json( tupal::key { .id = id, .comp_id = competition_id } ) };
            }

            catch (const soci::soci_error & e) {
                TUPAL_MESSAGE(std::cerr) << e.what() << std::endl;
                return { tupal::make_error_code(tupal::error_code::system_error), nullptr };
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
                    return { tupal::make_error_code(tupal::error_code::unknown_key), nullptr };
                }

                boost::json::array objects;
                if (begin->get_indicator(1) != soci::i_null) {
                    std::transform(begin, rows.end(), std::back_insert_iterator(objects), [](const soci::row & row) -> boost::json::value { 
                        return tupal::to_json( tupal::competition_class {
                            .id = tupal::key {
                                .id = row.get<std::string>(1),
                                .comp_id = row.get<std::string>(0)
                            },
                            .title = row.get<std::string>(3),
                            .start_group_id = row.get<std::string>(2)
                        });
                    });
                }
                return { ok, std::move(objects) };                
            }

            catch (const soci::soci_error & e) {
                TUPAL_MESSAGE(std::cerr) << e.what() << std::endl;
                return { tupal::make_error_code(tupal::error_code::system_error), nullptr };
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

                return { tupal::make_error_code(tupal::error_code::unknown_key), nullptr };
            }

            catch (const soci::soci_error & e) {
                TUPAL_MESSAGE(std::cerr) << e.what() << std::endl;
                return { tupal::make_error_code(tupal::error_code::system_error), nullptr };
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
                const auto ec = handle_soci_error(soci_session->get_backend_name(), e);
                if (ec == tupal::make_error_condition(tupal::error_code::constraint_violation))
                    return { tupal::make_error_code(tupal::error_code::unknown_key), nullptr };

                if (ec == tupal::make_error_condition(tupal::error_code::system_error))
                    TUPAL_MESSAGE(std::cerr) << "SOCI system error: " << e.what() << std::endl;

                return { ec, nullptr };
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
                    case 0: return { tupal::make_error_code(tupal::error_code::unknown_key), nullptr };
                    case 1: break;
                    default: return { tupal::make_error_code(tupal::error_code::constraint_violation), nullptr };
                }
                trx.commit();

                return { ok, tupal::to_json(cc) };
            }

            catch (const soci::soci_error & e) {
                TUPAL_MESSAGE(std::cerr) << e.what() << std::endl;
                return { tupal::make_error_code(tupal::error_code::system_error), nullptr };
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
                    case 0: return { tupal::make_error_code(tupal::error_code::unknown_key), nullptr };
                    case 1: break;
                    default: return { tupal::make_error_code(tupal::error_code::constraint_violation), nullptr };
                }

                trx.commit();
                return { ok, tupal::to_json( tupal::key { .id = id, .comp_id = competition_id } ) };
            }

            catch (const soci::soci_error & e) {
                TUPAL_MESSAGE(std::cerr) << e.what() << std::endl;
                return { tupal::make_error_code(tupal::error_code::system_error), nullptr };
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
                    return { tupal::make_error_code(tupal::error_code::unknown_key), nullptr };
                }

                boost::json::array objects;
                if (begin->get_indicator(0) != soci::i_null) {
                    std::transform(begin, rows.end(), std::back_insert_iterator(objects), [&](const soci::row & row) {
                        return tupal::to_json( tupal::competitor {
                            .id = tupal::key {
                                .id = row.get<std::string>(0),
                                .comp_id = row.get<std::string>(1)
                            },
                            .bib = static_cast<uint16_t>(row.get<int>(3)),
                            .name = row.get<std::string>(7),
                            .start_time_offset = tupal::from_duration_string(row.get<std::string>(4)),
                            .finish_time = tupal::from_date_string(row.get<std::string>(5)),
                            .status = tupal::to_competitor_status(static_cast<uint8_t>(row.get<int>(6))),
                            .comp_class_id = row.get<std::string>(2)
                        });
                    });
                }

                return { ok, std::move(objects) };
            }

            catch (const soci::soci_error & e) {
                TUPAL_MESSAGE(std::cerr) << e.what() << std::endl;
                return { tupal::make_error_code(tupal::error_code::system_error), nullptr };
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

                return { tupal::make_error_code(tupal::error_code::unknown_key), nullptr };
            }

            catch (const soci::soci_error & e) {
                TUPAL_MESSAGE(std::cerr) << e.what() << std::endl;
                return { tupal::make_error_code(tupal::error_code::system_error), nullptr };
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
                const auto ec = handle_soci_error(soci_session->get_backend_name(), e);
                if (ec == tupal::make_error_condition(tupal::error_code::constraint_violation))
                    return { tupal::make_error_code(tupal::error_code::unknown_key), nullptr };

                if (ec == tupal::make_error_condition(tupal::error_code::system_error))
                    TUPAL_MESSAGE(std::cerr) << "SOCI system error: " << e.what() << std::endl;

                return { ec, nullptr };
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
                    case 0: return { tupal::make_error_code(tupal::error_code::unknown_key), nullptr };
                    case 1: break;
                    default: return { tupal::make_error_code(tupal::error_code::constraint_violation), nullptr };
                }
                trx.commit();

                return { ok, tupal::to_json(ctor) };
            }

            catch (const soci::soci_error & e) {
                TUPAL_MESSAGE(std::cerr) << e.what() << std::endl;
                return { tupal::make_error_code(tupal::error_code::system_error), nullptr };
            }
        }

        virtual tupal::result_type remove(const std::string & competition_id, const std::string & id) {
            try {
                soci::transaction trx(*soci_session);
                soci::statement stmt = (soci_session->prepare << "delete from competitor where id=:id and comp_id=:comp_id",
                    soci::use(id), soci::use(competition_id));
                stmt.execute(true);
                switch (stmt.get_affected_rows()) {
                    case 0: return { tupal::make_error_code(tupal::error_code::unknown_key), nullptr };
                    case 1: break;
                    default: return { tupal::make_error_code(tupal::error_code::constraint_violation), nullptr };
                }

                trx.commit();
                return { ok, tupal::to_json( tupal::key { .id = id, .comp_id = competition_id } ) };
            }

            catch (const soci::soci_error & e) {
                TUPAL_MESSAGE(std::cerr) << e.what() << std::endl;
                return { tupal::make_error_code(tupal::error_code::system_error), nullptr };
            }
        }

        static void init_database(soci::session & session) {
            session.once << SCHEMA_COMPETITOR;
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
                competitor_manager(new CompetitorManagerImpl(*this, session)) {

            if (is_database_empty(*session)) {
                soci::transaction trx(*session);
                CompetitionManagerImpl::init_database(*session);
                StartGroupManagerImpl::init_database(*session);
                CompetitionClassManagerImpl::init_database(*session);
                CompetitorManagerImpl::init_database(*session);
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
                        .id = it->get<std::string>(0),
                        .date = tupal::from_date_string(it->get<std::string>(1)),
                        .title = it->get<std::string>(2)
                    }) );
                }

                return { ok, std::move(objects) };
            }

            catch (const soci::soci_error & e) {
                TUPAL_MESSAGE(std::cerr) << e.what() << std::endl;
                return { tupal::make_error_code(tupal::error_code::system_error), nullptr };
            }
        }

        virtual tupal::result_type get(const std::string & id) const {
            try {
                tupal::competition comp;
                comp.id = std::string { id };
                std::string date_str;

                *soci_session << "select id, date, title from competition where id = :id",
                    soci::into(comp.id), soci::into(date_str), soci::into(comp.title), soci::use(comp.id);
                if (soci_session->got_data())
                    return { ok, tupal::to_json(comp) };

                return { tupal::make_error_code(tupal::error_code::unknown_key), nullptr };
            }

            catch (const soci::soci_error & e) {
                TUPAL_MESSAGE(std::cerr) << e.what() << std::endl;
                return { tupal::make_error_code(tupal::error_code::system_error), nullptr };
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
                const auto ec = handle_soci_error(soci_session->get_backend_name(), e);
                if (ec == tupal::make_error_condition(tupal::error_code::system_error)) {
                    TUPAL_MESSAGE(std::cerr) << "SOCI system error: " << e.what() << std::endl;
                }

                return { ec, nullptr };
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
                    case 0: return { tupal::make_error_code(tupal::error_code::unknown_key), nullptr };
                    case 1: break;
                    default: return { tupal::make_error_code(tupal::error_code::duplicate_key), nullptr };
                }
                trx.commit();

                return { ok, tupal::to_json(comp) };
            }

            catch (const soci::soci_error & e) {
                TUPAL_MESSAGE(std::cerr) << e.what() << std::endl;
                return { tupal::make_error_code(tupal::error_code::system_error), nullptr };
            }
        }

        virtual tupal::result_type remove(const std::string & id) {
            try {
                soci::transaction trx(*soci_session);
                soci::statement stmt = (soci_session->prepare << "delete from competition where id=:id", soci::use(id));
                stmt.execute(true);
                switch (stmt.get_affected_rows()) {
                    case 0: return { tupal::make_error_code(tupal::error_code::unknown_key), nullptr };
                    case 1: break;
                    default: return { tupal::make_error_code(tupal::error_code::constraint_violation), nullptr };
                }
                trx.commit();
                return { ok, boost::json::object { {"id", id} } };
            }

            catch (const soci::soci_error & e) {
                TUPAL_MESSAGE(std::cerr) << e.what() << std::endl;
                return { tupal::make_error_code(tupal::error_code::system_error), nullptr };
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

    private:
        std::shared_ptr<soci::session> soci_session;
        std::shared_ptr<tupal::StartGroupManager> start_group_manager;
        std::shared_ptr<tupal::CompetitionClassManager> competition_class_manager;
        std::shared_ptr<tupal::CompetitorManager> competitor_manager;
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

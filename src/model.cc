#include <stdexcept>
#include "boost/uuid/uuid.hpp"
#include "boost/uuid/uuid_generators.hpp"
#include "boost/uuid/uuid_io.hpp"
#include "model.hh"

namespace {

    inline char get_char(char *& p) { char ch = *p; ++p; return ch; }

    inline void unget(char *& p) { --p; }

    inline void expect_separator(char *& p, char ch) {
        if (get_char(p) != ch)
            throw std::invalid_argument("expecting separator");
    }

    inline uint16_t get_digit(char *& p) { 
        char ch = get_char(p);
        if (!std::isdigit(ch))
            throw std::invalid_argument("expecting digit");
        return ch - '0';
    }

    inline uint16_t parse4(char *& p) {
        return get_digit(p) * 1000 + get_digit(p) * 100 + get_digit(p) * 10 + get_digit(p); }

    inline uint16_t parse3(char *& p) { return get_digit(p) * 100 + get_digit(p) * 10 + get_digit(p); }

    inline uint16_t parse2(char *& p) { return get_digit(p) * 10 + get_digit(p); }

    std::time_t parse_time_t(char *& p) {
        struct tm tm { .tm_isdst = -1 };
        tm.tm_year = parse4(p) - 1900;            
        expect_separator(p, '-');
        tm.tm_mon = parse2(p) - 1;
        expect_separator(p, '-');
        tm.tm_mday = parse2(p);
        char ch = get_char(p);

        if (ch == '\0') {
            tm.tm_hour = 0;
            tm.tm_min = 0;
            tm.tm_sec = 0;
            return std::mktime(&tm);
        } else if (ch != 'T') {
            throw std::invalid_argument("expecting 'T' or end of string");
        } else {
            tm.tm_hour = parse2(p);
            expect_separator(p, ':');
            tm.tm_min = parse2(p);
            expect_separator(p, ':');
            tm.tm_sec = parse2(p);

            return std::mktime(&tm);
        }
    }

    inline uint16_t parse_milliseconds(char *& p) {
        const char ch = get_char(p);
        return ch == '.' ? parse3(p) : 0;
    }

    inline std::time_t parse_seconds(char *& p) {
        std::time_t seconds = 0;
        while (std::isdigit(*p)) {
            seconds = seconds * 10 + (*p - '0'); ++p;
        }
        return seconds;
    }

    tupal::duration parse_duration(char *& p) {
        tupal::duration dur { 0, 0 };
        expect_separator(p, 'P');
        expect_separator(p, 'T');
        dur.seconds = parse_seconds(p);
        expect_separator(p, '.');
        dur.milliseconds = parse3(p);
        return dur;
    }

    static boost::uuids::random_generator uuid_generator;

    std::string make_unique_id(const boost::json::value & val) {
        return val.is_null()
            ? boost::lexical_cast<std::string>(uuid_generator())
            : std::string { val.as_string() };
    }

    /**
     * @brief Monad 'optional'
     * @param T object type for enclosing optional. Few requirements for T:
     *   T must be default constructable
     *   T must be copy-constructable.
     *   T must be assignable.
     *
     * @note optional is already supporeted by standard library in most recent c++ stardars!
     * */
    template<class T>
    class optional {
    private:
        bool is_set;
        T value;
    public:
        /** @brief default constructor initalizes an empty 'optional'.  */
        optional() : is_set(false), value() {}

        /** @brief destructor */
        ~optional() noexcept = default;

        /** @brief copy-constror, initializes optional with given val of type T */
        optional(const T & val) : is_set(true), value(val) {}

        /** @brief Returns enclosed object if set, othervise val. */
        inline const T & or_else(const T & val) const { return is_set ? value : val; }

        /**
         * @brief Mapper.
         * Converts enclosed value to type U. If value is not set, returns an
         * empty optional of type U. U must support the same features as T.
         * @param mapper is a function that takes reference to const T (current enclosed value)
         *  and returns object of type U. I.e. it can make conversion routines from T to U.
         */
        template <class U>
        inline optional<U> map(U (*mapper)(const T&)) const {
            return is_set ? optional<U>(mapper(value)) : optional<U>();
        }
    };

    /** @brief Predicate function returning 'true' if val can be considered non-empty. */
    bool non_empty(const boost::json::value & val) {
        if (val.is_null())
            return false;
        return ((val.is_array() && !val.as_array().empty())
                || (val.is_object() && !val.as_object().empty())
                || (val.is_string() && !val.as_string().empty()));
    }

    /** @brief Predicate function returning 'true' if val is non-null. */
    bool non_null(const boost::json::value & val) {
        return !val.is_null();
    }

    /**
     * @brief Safe accessor to json object attributes. 
     * @param obj const reference to json object.
     * @param name non-empty name of the attribute being accessed.
     * @param validate predicate function to validate the objects's value. Default
     *   is non-null, which means that only attributes having meaningfull value are accepted.
     * @return optional of desired type. Note, that only types supported by json::value are accepted.
     */
    template <class T>
    optional<T> at(const boost::json::object & obj, const char * name,
            bool (*validate)(const boost::json::value&) = non_null) {
        auto it = obj.find(name);
        if (it != obj.end() && validate(it->value())) {
            return optional<T>(boost::json::value_to<T>(it->value()));
        } else {
            return optional<T>();
        }
    }
}

namespace tupal {

    // duration

    duration from_duration_string(const std::string & iso8601_str) {
        try {
            char * p = const_cast<char*>(iso8601_str.c_str());
            return parse_duration(p);
        } catch (const std::invalid_argument & e) {
            throw std::invalid_argument("invalid duration '" + iso8601_str + "': " + e.what());
        }
    }

    std::string to_duration_string(const duration duration) {
        char buffer[20];
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
        std::snprintf(buffer, sizeof(buffer), "PT%lld.%03dS",
            static_cast<long long>(duration.seconds),
            duration.milliseconds);
#pragma GCC diagnostic pop
        return std::string(buffer);
    }

    duration & duration::operator+=(const duration & dur) {
        seconds += dur.seconds;
        milliseconds += dur.milliseconds;
        if (milliseconds >= 1000) {
            ++seconds;
            milliseconds -= 1000;
        }
        return *this;
    }

    duration & duration::operator-=(const duration & dur) {
        seconds -= dur.seconds;
        if (milliseconds < dur.milliseconds) {
            --seconds;
            milliseconds = milliseconds + 1000 - dur.milliseconds;
        } else {
            milliseconds -= dur.milliseconds;
        }
        return *this;
    }

    duration operator+(const duration & d1, const duration & d2) {
        duration result = d1;
        result += d2;
        return result;
    }

    duration operator-(const duration & d1, const duration & d2) {
        duration result = d1;
        result -= d2;
        return result;
    }

    bool operator<(const duration & d1, const duration & d2) {
        return (d1.seconds < d2.seconds) ? true
            : (d1.seconds > d2.seconds) ? false
                : (d1.milliseconds < d2.milliseconds);
    }

    // date_time

    date_time & date_time::operator+=(const duration & dur) {
        seconds_since_epoch += dur.seconds;
        milliseconds += dur.milliseconds;
        if (milliseconds >= 1000) {
            ++seconds_since_epoch;
            milliseconds -= 1000;
        }
        return *this;
    }

    date_time & date_time::operator-=(const duration & dur) {
        seconds_since_epoch -= dur.seconds;
        if (milliseconds < dur.milliseconds) {
            --seconds_since_epoch;
            milliseconds = milliseconds + 1000 - dur.milliseconds;
        } else {
            milliseconds -= dur.milliseconds;
        }
        return *this;
    }

    date_time operator+(const date_time & dt, const duration & dur) {
        date_time result = dt;
        result += dur;
        return result;
    }

    date_time operator-(const date_time & dt, const duration & dur) {
        date_time result = dt;
        result -= dur;
        return result;
    }

    duration operator-(const date_time & dt1, const date_time & dt2) {
        const int32_t msec_diff = static_cast<int32_t>(dt1.milliseconds)
                - static_cast<int32_t>(dt2.milliseconds);
        return msec_diff < 0
            ? duration {
                .seconds = dt1.seconds_since_epoch - dt2.seconds_since_epoch - 1,
                .milliseconds = static_cast<uint16_t>(msec_diff + 1000)
            }
            : duration {
                .seconds = dt1.seconds_since_epoch - dt2.seconds_since_epoch,
                .milliseconds = static_cast<uint16_t>(msec_diff)
            };
    }

    date_time from_date_string(const std::string & iso8601_str) {
        try {
            char * p = const_cast<char*>(iso8601_str.c_str());
            return {
                .seconds_since_epoch = parse_time_t(p),
                .milliseconds = parse_milliseconds(p)
            };
        }
        catch (const std::invalid_argument & e) {
            throw std::invalid_argument("invalid date_time '" + iso8601_str + "': " + e.what());
        }
    }

    bool operator<(const date_time & dt1, const date_time & dt2) {
        return (dt1.seconds_since_epoch < dt2.seconds_since_epoch) ? true
            : (dt1.seconds_since_epoch > dt2.seconds_since_epoch) ? false
                : (dt1.milliseconds < dt2.milliseconds);
    }

    std::string to_date_string(const date_time & dt) {
        if (dt.seconds_since_epoch == 0)
            return "1970-01-01T00:00:00.000Z";

        struct tm * tm = std::localtime(&dt.seconds_since_epoch);
        char buffer[30];

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
        std::snprintf(buffer, sizeof(buffer), "%04d-%02d-%02dT%02d:%02d:%02d.%03dZ",
            tm->tm_year + 1900,
            tm->tm_mon + 1,
            tm->tm_mday,
            tm->tm_hour,
            tm->tm_min,
            tm->tm_sec,
            dt.milliseconds);
#pragma GCC diagnostic pop
        return std::string(buffer);
    }

    // key

    const boost::json::object to_json(const key & k) {
        return boost::json::object {
            { "id", k.id },
            { "comp_id", k.comp_id }
        };
    }

    key to_key(const boost::json::object & obj) {
        return key {
            .id = std::string { obj.at("id").as_string() },
            .comp_id = std::string { obj.at("comp_id").as_string() }
        };
    }

    // competition

    const char * type(const competition &) {
        return "competition";
    }

    const boost::json::object to_json(const competition & comp) {
        return boost::json::object {
            { "id", comp.id },
            { "date", to_date_string(comp.date) },
            { "title", comp.title }
        };
    }

    competition to_competition(const boost::json::object & obj) {
        return competition {
            .id = make_unique_id( obj.at("id") ),
            .date = at<std::string>(obj, "date", non_empty)
                .map<tupal::date_time>(tupal::from_date_string)
                .or_else(date_time{0, 0}),
            .title = std::string { obj.at("title").as_string() }
        };
    }

    // start_group

    const char * type(const start_group &) {
        return "start_group";
    }

    const boost::json::object to_json(const start_group & sg) {
        return boost::json::object {
            { "id", sg.id.id },
            { "comp_id", sg.id.comp_id },
            { "title", sg.title },
            { "first_start_time", to_date_string(sg.first_start_time) },
            { "first_bib", static_cast<int64_t>(sg.first_bib) }
        };
    }

    start_group to_start_group(const boost::json::object & obj) {
        auto sg = start_group {
            .id {
                .id = make_unique_id( obj.at("id") ),
                .comp_id = at<std::string>(obj, "comp_id").or_else("")
            },
            .title = std::string { obj.at("title").as_string() },
            .first_start_time = from_date_string( std::string { obj.at("first_start_time").as_string() } ),
            .first_bib = static_cast<uint16_t>(at<int64_t>(obj, "first_bib").or_else(0))
        };
        return sg;
    }

    // competition_class

    const char * type(const competition_class &) {
        return "competition_class";
    }

    const boost::json::object to_json(const competition_class & cc) {
        return boost::json::object {
            { "id", cc.id.id },
            { "comp_id", cc.id.comp_id },
            { "title", cc.title },
            { "start_group_id", cc.start_group_id }
        };
    }

    competition_class to_competition_class(const boost::json::object & obj) {
        return competition_class {
            .id {
                .id = make_unique_id( obj.at("id") ),
                .comp_id = at<std::string>(obj, "comp_id").or_else("")
            },
            .title = std::string { obj.at("title").as_string() },
            .start_group_id = std::string { obj.at("start_group_id").as_string() }
        };
    }

    // competitor

    competitor_status to_competitor_status(int64_t val) {
        switch (val) {
            case 0: return competitor_status::NA;
            case 1: return competitor_status::DNS;
            case 2: return competitor_status::DNF;
            case 3: return competitor_status::DSQ;
            default:
                throw std::invalid_argument("invalid competitor status value");
        }
    }

    const boost::json::value to_json(const competitor_status & status) {
        return boost::json::value(static_cast<int64_t>(status));
    }

    const char * type(const competitor &) {
        return "competitor";
    }

    competitor to_competitor(const boost::json::object & obj) {
        return competitor {
            .id {
                .id = make_unique_id( obj.at("id").as_string() ),
                .comp_id = at<std::string>(obj, "comp_id").or_else("")
            },
            .bib = static_cast<uint16_t>(at<int64_t>(obj, "bib").or_else(0)),
            .name = std::string { obj.at("name").as_string() },
            .start_time_offset = from_duration_string( std::string { obj.at("start_time_offset").as_string() } ),
            .finish_time = at<std::string>(obj, "finish_time", non_empty)
                .map<tupal::date_time>(tupal::from_date_string)
                .or_else(date_time{0, 0}),
            .status = to_competitor_status( obj.at("status").as_int64() ),
            .comp_class_id = std::string { obj.at("comp_class_id").as_string() }
        };
    }

    const boost::json::object to_json(const competitor & competitor) {
        return boost::json::object {
            { "id", competitor.id.id },
            { "comp_id", competitor.id.comp_id },
            { "bib", static_cast<int64_t>(competitor.bib) },
            { "name", competitor.name },
            { "start_time_offset", to_duration_string(competitor.start_time_offset) },
            { "finish_time", to_date_string(competitor.finish_time) },
            { "status", to_json(competitor.status) },
            { "comp_class_id", competitor.comp_class_id }
        };
    }   
};

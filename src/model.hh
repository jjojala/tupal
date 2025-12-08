#ifndef TUPAL_MODEL_HH
#define TUPAL_MODEL_HH

#include <string>
#include <ctime>
#include "boost/json.hpp"
#include "boost/date_time.hpp"

namespace tupal {

    /** @brief duration time representation */
    struct duration {
        /** @brief duration seconds */
        std::time_t seconds;
        /** @brief duration milliseconds beyond seconds */
        uint16_t milliseconds;

        duration & operator+=(const duration & dur);
        duration & operator-=(const duration & dur);
    };

    duration operator+(const duration & d1, const duration & d2);
    duration operator-(const duration & d1, const duration & d2);

    inline bool operator==(const duration & d1, const duration & d2) {
        return d1.seconds == d2.seconds && d1.milliseconds == d2.milliseconds;
    }
    inline bool operator!=(const duration & d1, const duration & d2) { return !(d1 == d2); }
    bool operator<(const duration & d1, const duration & d2);
    inline bool operator<=(const duration & d1, const duration & d2) { return (d1 < d2) || (d1 == d2); }
    inline bool operator>(const duration & d1, const duration & d2) { return !(d1 <= d2); }
    inline bool operator>=(const duration & d1, const duration & d2) { return !(d1 < d2); }

    /** @brief date time representation */
    struct date_time {
        /** @brief seconds since epoch (local time) */
        std::time_t seconds_since_epoch;
        /** @brief milliseconds beyond seconds */
        uint16_t milliseconds;

        date_time & operator+=(const duration & dur);

        date_time & operator-=(const duration & dur);
    };

    date_time operator+(const date_time & dt, const duration & dur);
    date_time operator-(const date_time & dt, const duration & dur);

    duration operator-(const date_time & dt1, const date_time & dt2);

    inline bool operator==(const date_time & dt1, const date_time & dt2) {
        return dt1.seconds_since_epoch == dt2.seconds_since_epoch &&
               dt1.milliseconds == dt2.milliseconds;
    }
    inline bool operator!=(const date_time & dt1, const date_time & dt2) { return !(dt1 == dt2); }
    bool operator<(const date_time & dt1, const date_time & dt2);
    inline bool operator<=(const date_time & dt1, const date_time & dt2) { return (dt1 < dt2) || (dt1 == dt2); }
    inline bool operator>(const date_time & dt1, const date_time & dt2) { return !(dt1 <= dt2); }
    inline bool operator>=(const date_time & dt1, const date_time & dt2) { return !(dt1 < dt2); }

    /** @brief a competition, the root of data */
    struct competition {
        /** @brief Unique, non-empty id */
        std::string id;

        /** @brief Date of the competition. Informal only, e.g. for listings etc. */
        date_time date;

        /** @brief Title of the competition */
        std::string title;
    };

    /** @brief key of subentities (within the competition) */
    struct key {
        /** @brief unique, non-empty identifier, global within competition for the specified entity type. */
        std::string id;

        /** @brief ref to root competition. Must refer to existing comp.id */
        std::string comp_id;
    };

    /** @brief start group (within the competition) */
    struct start_group {
        /** @brief identity of this start group */
        key id;

        /** @brief title */
        std::string title;

        /** @brief exact time of first start within this start group (local time) */
        date_time first_start_time;

        /** @brief first bib within this start group. 0-not used */
        uint16_t first_bib;
    };

    /** @brief competition class (within the competition) */
    struct competition_class {
        /** @brief identity of this competition class */
        key id;

        /** @brief title */
        std::string title;

        /** @brief ref. to start group */
        std::string start_group_id;
    };

    /** @brief competitor status */
    enum competitor_status : uint8_t {
        /** @brief nothing special */
        NA = 0,

        /** @brief did not start */
        DNS = 1,

        /** @brief did not finished */
        DNF = 2,

        /** @brief disqualified */
        DSQ = 3
    };

    /** @brief competitor */
    struct competitor {
        /** @brief identity of this competitor */
        key id;

        /** @brief unique bib of this competitor within the start group, 0-unspecified/not in use */
        uint16_t bib;

        /** @brief name */
        std::string name;

        /** @brief start time offset (absolute start time is start_group.first_start_time + start_time_offset) */
        duration start_time_offset;

        /** @brief absolute finish time (local time) */
        date_time finish_time;
    
        /** @brief competitor status */
        competitor_status status;
        std::string comp_class_id;
    };

    date_time from_date_string(const std::string & iso8601_str);
    std::string to_date_string(const date_time & dt);

    duration from_duration_string(const std::string & iso8601_str);
    std::string to_duration_string(const duration duration);

    const boost::json::value to_json(const competition &);
    const boost::json::value to_json(const start_group &);
    const boost::json::value to_json(const competition_class &);
    const boost::json::value to_json(const competitor_status &);
    const boost::json::value to_json(const competitor &);
    const boost::json::value to_json(const key &);
    const boost::json::value to_json(const std::string &);

    competition to_competition(const boost::json::value &);
    start_group to_start_group(const boost::json::value &);
    competition_class to_competition_class(const boost::json::value &);
    competitor_status to_competitor_status(const boost::json::value &);
    competitor to_competitor(const boost::json::value &);
    key to_key(const boost::json::value &);

    const char * type(const competition &);
    const char * type(const start_group &);
    const char * type(const competition_class &);
    const char * type(const competitor &);
};

#endif
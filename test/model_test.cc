#include <string>
#include <ctime>
#include "doctest/doctest.h"
#include "model.hh"

TEST_CASE("iso8601 date --> date_time") {
    const std::string date_str = "2026-02-27";
    const std::tm tm = {
        .tm_sec = 0,
        .tm_min = 0,
        .tm_hour = 0,
        .tm_mday = 27,
        .tm_mon = 02 - 1,
        .tm_year = 2026 - 1900,
        .tm_isdst = -1
    };

    const auto dt = tupal::from_date_string(date_str);
    CHECK(dt.seconds_since_epoch == std::mktime(const_cast<std::tm*>(&tm)));
    CHECK(dt.milliseconds == 0);

    const std::string result = tupal::to_date_string(dt);
    CHECK(result == "2026-02-27T00:00:00.000Z");
}

TEST_CASE("iso8601 --> date_time") {
    const std::string date_str = "2025-12-31T23:59:59.123Z";
    const std::tm tm = {
        .tm_sec = 59,
        .tm_min = 59,
        .tm_hour = 23,
        .tm_mday = 31,
        .tm_mon = 12 - 1,        
        .tm_year = 2025 - 1900,
        .tm_isdst = -1
    };

    const auto dt = tupal::from_date_string(date_str);
    CHECK(dt.seconds_since_epoch == std::mktime(const_cast<std::tm*>(&tm)));
    CHECK(dt.milliseconds == 123);
}

TEST_CASE("date_time --> iso8601") {
    const std::string date_str = "2025-12-31T23:59:59.123Z";
    const std::tm tm = {
        .tm_sec = 59,
        .tm_min = 59,
        .tm_hour = 23,
        .tm_mday = 31,
        .tm_mon = 12 - 1,        
        .tm_year = 2025 - 1900,
        .tm_isdst = -1
    };

    const tupal::date_time dt = {
        .seconds_since_epoch = std::mktime(const_cast<std::tm*>(&tm)),
        .milliseconds = 123
    };

    const auto str = tupal::to_date_string(dt);
    CHECK(str == date_str);
}

TEST_CASE("iso8601 duration --> duration") {
    const std::string duration_str = "PT3661.456S";

    const auto dur = tupal::from_duration_string(duration_str);
    CHECK(dur.seconds == 3661);
    CHECK(dur.milliseconds == 456);
}

TEST_CASE("duration --> iso8601 duration") {
    const std::string duration_str = "PT3661.456S";

    const tupal::duration dur = {
        .seconds = 3661,
        .milliseconds = 456
    };

    const auto str = tupal::to_duration_string(dur);
    CHECK(str == duration_str);
}

TEST_CASE("duration addition and subtraction") {
    const tupal::duration dur1 = { .seconds = 10, .milliseconds = 500 };
    const tupal::duration dur2 = { .seconds = 5, .milliseconds = 750 };

    const auto dur_sum = dur1 + dur2;
    CHECK(dur_sum.seconds == 16);
    CHECK(dur_sum.milliseconds == 250);

    const auto dur_diff = dur1 - dur2;
    CHECK(dur_diff.seconds == 4);
    CHECK(dur_diff.milliseconds == 750);
}

TEST_CASE("duration operator+= and operator-=") {
    tupal::duration dur1 = { .seconds = 10, .milliseconds = 500 };
    const tupal::duration dur2 = { .seconds = 5, .milliseconds = 750 };

    dur1 += dur2;
    CHECK(dur1.seconds == 16);
    CHECK(dur1.milliseconds == 250);

    dur1 -= dur2;
    CHECK(dur1.seconds == 10);
    CHECK(dur1.milliseconds == 500);
}

TEST_CASE("duration comparison operators") {
    const tupal::duration dur1 = { .seconds = 10, .milliseconds = 500 };
    const tupal::duration dur2 = { .seconds = 10, .milliseconds = 600 };
    const tupal::duration dur3 = { .seconds = 11, .milliseconds = 400 };

    CHECK(dur1 < dur2);
    CHECK(dur2 < dur3);
    CHECK(dur1 < dur3);

    CHECK(dur1 <= dur2);
    CHECK(dur2 <= dur3);
    CHECK(dur1 <= dur3);
    CHECK(dur1 <= dur1);

    CHECK(dur2 > dur1);
    CHECK(dur3 > dur2);
    CHECK(dur3 > dur1);

    CHECK(dur2 >= dur1);
    CHECK(dur3 >= dur2);
    CHECK(dur3 >= dur1);
    CHECK(dur1 >= dur1);

    CHECK(dur1 != dur2);
    CHECK(dur1 == dur1);
}

TEST_CASE("date_time addition and subtraction") {
    const tupal::date_time dt = { .seconds_since_epoch = 1000000, .milliseconds = 500 };
    const tupal::duration dur = { .seconds = 500, .milliseconds = 750 };

    const auto dt_sum = dt + dur;
    CHECK(dt_sum.seconds_since_epoch == 1000501);
    CHECK(dt_sum.milliseconds == 250);

    const auto dt_diff = dt - dur;
    CHECK(dt_diff.seconds_since_epoch == 999499);
    CHECK(dt_diff.milliseconds == 750);
}

TEST_CASE("date_time operator+= and operator-=") {
    tupal::date_time dt = { .seconds_since_epoch = 1000000, .milliseconds = 500 };
    const tupal::duration dur = { .seconds = 500, .milliseconds = 750 };

    dt += dur;
    CHECK(dt.seconds_since_epoch == 1000501);
    CHECK(dt.milliseconds == 250);

    dt -= dur;
    CHECK(dt.seconds_since_epoch == 1000000);
    CHECK(dt.milliseconds == 500);
}

TEST_CASE("date_time comparison operators") {

    std::tm tm = {
        .tm_sec = 30,
        .tm_min = 29,
        .tm_hour = 13,
        .tm_mday = 1,
        .tm_mon = 11,        
        .tm_year = 1970 - 1900,
        .tm_isdst = -1
    };
    const tupal::date_time dt1 = { .seconds_since_epoch = std::mktime(&tm), .milliseconds = 500 };
    const tupal::date_time dt2 = { .seconds_since_epoch = std::mktime(&tm), .milliseconds = 600 };
    const tupal::date_time dt3 = { .seconds_since_epoch = std::mktime(&tm)+1, .milliseconds = 400 };

    MESSAGE("dt1: ", tupal::to_date_string(dt1));
    MESSAGE("dt2: ", tupal::to_date_string(dt2));
    MESSAGE("dt3: ", tupal::to_date_string(dt3));

    CHECK(dt1 < dt2);
    CHECK(dt2 < dt3);
    CHECK(dt1 < dt3);

    CHECK(dt1 <= dt2);
    CHECK(dt2 <= dt3);
    CHECK(dt1 <= dt3);
    CHECK(dt1 <= dt1);

    CHECK(dt2 > dt1);
    CHECK(dt3 > dt2);
    CHECK(dt3 > dt1);

    CHECK(dt2 >= dt1);
    CHECK(dt3 >= dt2);
    CHECK(dt3 >= dt1);
    CHECK(dt1 >= dt1);

    CHECK(dt1 != dt2);
    CHECK(dt1 == dt1);
}

TEST_CASE("competition JSON serialization/deserialization") {
    tupal::competition comp {
        .id = "comp1",
        .date = tupal::from_date_string("2025-12-31T00:00:00.000Z"),
        .title = "New Year Competition"
    };

    const auto json_val = tupal::to_json(comp);
    const auto comp_deserialized = tupal::to_competition(json_val);

    CHECK(comp.id == comp_deserialized.id);
    CHECK(comp.date == comp_deserialized.date);
    CHECK(comp.title == comp_deserialized.title);
}

TEST_CASE("start_group JSON serialization/deserialization") {
    tupal::start_group sg {
        .id = { .id = "sg1", .comp_id = "comp1" },
        .title = "Morning Group",
        .first_start_time = tupal::from_date_string("2025-12-31T09:00:00.000Z"),
        .first_bib = 101
    };

    const auto json_val = tupal::to_json(sg);
    const auto sg_deserialized = tupal::to_start_group(json_val);

    CHECK(sg.id.id == sg_deserialized.id.id);
    CHECK(sg.id.comp_id == sg_deserialized.id.comp_id);
    CHECK(sg.title == sg_deserialized.title);
    CHECK(sg.first_start_time == sg_deserialized.first_start_time);
    CHECK(sg.first_bib == sg_deserialized.first_bib);
}

TEST_CASE("competition_class JSON serialization/deserialization") {
    tupal::competition_class cc {
        .id = { .id = "cc1", .comp_id = "comp1" },
        .title = "Elite Class",
        .start_group_id = "sg1"
    };

    const auto json_val = tupal::to_json(cc);
    const auto cc_deserialized = tupal::to_competition_class(json_val);

    CHECK(cc.id.id == cc_deserialized.id.id);
    CHECK(cc.id.comp_id == cc_deserialized.id.comp_id);
    CHECK(cc.title == cc_deserialized.title);
    CHECK(cc.start_group_id == cc_deserialized.start_group_id);
}

TEST_CASE("competitor JSON serialization/deserialization") {
    tupal::competitor comp {
        .id = { .id = "comp1", .comp_id = "comp1" },
        .bib = 42,
        .name = "John Doe",
        .start_time_offset = tupal::from_duration_string("PT3600.500S"),
        .finish_time = tupal::from_date_string("2025-12-31T10:00:00.500Z"),
        .status = tupal::competitor_status::NA,
        .comp_class_id = "cc1"
    };

    const auto json_val = tupal::to_json(comp);
    const auto comp_deserialized = tupal::to_competitor(json_val);

    CHECK(comp.id.id == comp_deserialized.id.id);
    CHECK(comp.id.comp_id == comp_deserialized.id.comp_id);
    CHECK(comp.bib == comp_deserialized.bib);
    CHECK(comp.name == comp_deserialized.name);
    CHECK(comp.start_time_offset == comp_deserialized.start_time_offset);
    CHECK(comp.finish_time == comp_deserialized.finish_time);
    CHECK(comp.status == comp_deserialized.status);
    CHECK(comp.comp_class_id == comp_deserialized.comp_class_id);
}

TEST_CASE("competitor_status JSON serialization/deserialization") {
    tupal::competitor_status status = tupal::competitor_status::DNF;

    const auto json_val = tupal::to_json(status);
    const auto status_deserialized = tupal::to_competitor_status(json_val.as_int64());

    CHECK(status == status_deserialized);
}

TEST_CASE("key JSON serialization/deserialization") {
    tupal::key k {
        .id = "comp1",
        .comp_id = "root_comp"
    };

    const auto json_val = tupal::to_json(k);
    const auto k_deserialized = tupal::to_key(json_val);

    CHECK(k.id == k_deserialized.id);
    CHECK(k.comp_id == k_deserialized.comp_id);
}
#include <string>
#include <ctime>
#include "doctest/doctest.h"
#include "model.hh"

namespace {
    std::tm make_tm(int second, int minute, int hour, int day, int month, int year) {
        std::tm value{};
        value.tm_sec = second;
        value.tm_min = minute;
        value.tm_hour = hour;
        value.tm_mday = day;
        value.tm_mon = month;
        value.tm_year = year;
        value.tm_isdst = -1;
        return value;
    }
}

TEST_CASE("competitor model generates an id when none is provided") {
    const auto competitor = tupal::to_competitor(boost::json::object {
        { "id", nullptr },
        { "comp_class_id", "class-1" },
        { "bib", 1 },
        { "start_time_offset", "PT0.000S" },
        { "finish_time", "" },
        { "status", 0 },
        { "name", "Taylor, Alex" }
    });

    CHECK(!competitor.id.id.empty());
}

TEST_CASE("iso8601 date --> date_time") {
    const std::string date_str = "2026-02-27";
    std::tm tm = make_tm(0, 0, 0, 27, 02 - 1, 2026 - 1900);

    const auto dt = tupal::from_date_string(date_str);
    CHECK(dt.seconds_since_epoch == std::mktime(const_cast<std::tm*>(&tm)));
    CHECK(dt.milliseconds == 0);

    const std::string result = tupal::to_date_string(dt);
    CHECK(result == "2026-02-27T00:00:00.000Z");
}

TEST_CASE("iso8601 --> date_time") {
    const std::string date_str = "2025-12-31T23:59:59.123Z";
    std::tm tm = make_tm(59, 59, 23, 31, 12 - 1, 2025 - 1900);

    const auto dt = tupal::from_date_string(date_str);
    CHECK(dt.seconds_since_epoch == std::mktime(const_cast<std::tm*>(&tm)));
    CHECK(dt.milliseconds == 123);
}

TEST_CASE("date_time --> iso8601") {
    const std::string date_str = "2025-12-31T23:59:59.123Z";
    std::tm tm = make_tm(59, 59, 23, 31, 12 - 1, 2025 - 1900);

    const tupal::date_time dt { std::mktime(&tm), 123 };

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

    const tupal::duration dur { 3661, 456 };

    const auto str = tupal::to_duration_string(dur);
    CHECK(str == duration_str);
}

TEST_CASE("duration addition and subtraction") {
    const tupal::duration dur1 { 10, 500 };
    const tupal::duration dur2 { 5, 750 };

    const auto dur_sum = dur1 + dur2;
    CHECK(dur_sum.seconds == 16);
    CHECK(dur_sum.milliseconds == 250);

    const auto dur_diff = dur1 - dur2;
    CHECK(dur_diff.seconds == 4);
    CHECK(dur_diff.milliseconds == 750);
}

TEST_CASE("duration operator+= and operator-=") {
    tupal::duration dur1 { 10, 500 };
    const tupal::duration dur2 { 5, 750 };

    dur1 += dur2;
    CHECK(dur1.seconds == 16);
    CHECK(dur1.milliseconds == 250);

    dur1 -= dur2;
    CHECK(dur1.seconds == 10);
    CHECK(dur1.milliseconds == 500);
}

TEST_CASE("duration comparison operators") {
    const tupal::duration dur1 { 10, 500 };
    const tupal::duration dur2 { 10, 600 };
    const tupal::duration dur3 { 11, 400 };

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
    const tupal::date_time dt { 1000000, 500 };
    const tupal::duration dur { 500, 750 };

    const auto dt_sum = dt + dur;
    CHECK(dt_sum.seconds_since_epoch == 1000501);
    CHECK(dt_sum.milliseconds == 250);

    const auto dt_diff = dt - dur;
    CHECK(dt_diff.seconds_since_epoch == 999499);
    CHECK(dt_diff.milliseconds == 750);
}

TEST_CASE("date_time operator+= and operator-=") {
    tupal::date_time dt { 1000000, 500 };
    const tupal::duration dur { 500, 750 };

    dt += dur;
    CHECK(dt.seconds_since_epoch == 1000501);
    CHECK(dt.milliseconds == 250);

    dt -= dur;
    CHECK(dt.seconds_since_epoch == 1000000);
    CHECK(dt.milliseconds == 500);
}

TEST_CASE("date_time comparison operators") {

    std::tm tm = make_tm(30, 29, 13, 1, 11, 1970 - 1900);
    const tupal::date_time dt1 { std::mktime(&tm), 500 };
    const tupal::date_time dt2 { std::mktime(&tm), 600 };
    const tupal::date_time dt3 { std::mktime(&tm) + 1, 400 };

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
        "comp1", tupal::from_date_string("2025-12-31T00:00:00.000Z"), "New Year Competition"
    };

    const auto json_val = tupal::to_json(comp);
    const auto comp_deserialized = tupal::to_competition(json_val);

    CHECK(comp.id == comp_deserialized.id);
    CHECK(comp.date == comp_deserialized.date);
    CHECK(comp.title == comp_deserialized.title);
}

TEST_CASE("start_group JSON serialization/deserialization") {
    tupal::start_group sg {
        { "sg1", "comp1" },
        "Morning Group",
        tupal::from_date_string("2025-12-31T09:00:00.000Z"),
        101,
        tupal::from_duration_string("PT30S")
    };

    const auto json_val = tupal::to_json(sg);
    const auto sg_deserialized = tupal::to_start_group(json_val);

    CHECK(sg.id.id == sg_deserialized.id.id);
    CHECK(sg.id.comp_id == sg_deserialized.id.comp_id);
    CHECK(sg.title == sg_deserialized.title);
    CHECK(sg.first_start_time == sg_deserialized.first_start_time);
    CHECK(sg.first_bib == sg_deserialized.first_bib);
    CHECK(sg.start_interval == sg_deserialized.start_interval);
}

TEST_CASE("competition_class JSON serialization/deserialization") {
    tupal::competition_class cc {
        { "cc1", "comp1" },
        "Elite Class",
        "sg1"
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
        { "comp1", "comp1" },
        42,
        "John Doe",
        tupal::from_duration_string("PT3600.500S"),
        tupal::from_date_string("2025-12-31T10:00:00.500Z"),
        tupal::competitor_status::NA,
        "cc1"
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

TEST_CASE("finish_time_entry JSON serialization/deserialization") {
    const tupal::finish_time_entry entry {
        { "time-1", "comp-1" },
        tupal::from_date_string("2025-12-31T10:00:00.500Z"),
        tupal::finish_entry_status::uncertain
    };

    const auto json_val = tupal::to_json(entry);
    const auto deserialized = tupal::to_finish_time_entry(json_val);

    CHECK(entry.id.id == deserialized.id.id);
    CHECK(entry.id.comp_id == deserialized.id.comp_id);
    CHECK(entry.finish_time == deserialized.finish_time);
    CHECK(entry.status == deserialized.status);
}

TEST_CASE("finish_bib_entry JSON serialization/deserialization preserves nullable fields") {
    const tupal::finish_bib_entry entry {
        { "bib-1", "comp-1" },
        3,
        42,
        tupal::finish_entry_status::confirmed,
        "time-1"
    };

    const auto json_val = tupal::to_json(entry);
    const auto deserialized = tupal::to_finish_bib_entry(json_val);

    CHECK(entry.id.id == deserialized.id.id);
    CHECK(entry.id.comp_id == deserialized.id.comp_id);
    CHECK(entry.sequence_no == deserialized.sequence_no);
    REQUIRE(deserialized.bib.has_value());
    CHECK(*entry.bib == *deserialized.bib);
    CHECK(entry.status == deserialized.status);
    REQUIRE(deserialized.confirmed_time_entry_id.has_value());
    CHECK(*entry.confirmed_time_entry_id == *deserialized.confirmed_time_entry_id);

    const tupal::finish_bib_entry empty_entry {
        { "bib-2", "comp-1" },
        4,
        std::nullopt,
        tupal::finish_entry_status::missing,
        std::nullopt
    };
    const auto empty_json = tupal::to_json(empty_entry);
    CHECK(empty_json.at("bib").is_null());
    CHECK(empty_json.at("confirmed_time_entry_id").is_null());

    const auto empty_deserialized = tupal::to_finish_bib_entry(empty_json);
    CHECK(!empty_deserialized.bib.has_value());
    CHECK(!empty_deserialized.confirmed_time_entry_id.has_value());
    CHECK(empty_entry.status == empty_deserialized.status);
}

TEST_CASE("finish entry status rejects values outside its defined range") {
    CHECK(tupal::to_finish_entry_status(0) == tupal::finish_entry_status::confirmed);
    CHECK(tupal::to_finish_entry_status(1) == tupal::finish_entry_status::uncertain);
    CHECK(tupal::to_finish_entry_status(2) == tupal::finish_entry_status::incorrect);
    CHECK(tupal::to_finish_entry_status(3) == tupal::finish_entry_status::missing);
    CHECK_THROWS_AS(tupal::to_finish_entry_status(4), std::invalid_argument);
}

TEST_CASE("key JSON serialization/deserialization") {
    tupal::key k {
        "comp1",
        "root_comp"
    };

    const auto json_val = tupal::to_json(k);
    const auto k_deserialized = tupal::to_key(json_val);

    CHECK(k.id == k_deserialized.id);
    CHECK(k.comp_id == k_deserialized.comp_id);
}
#include "doctest/doctest.h"
#include "manager.hh"

namespace {
    std::shared_ptr<tupal::CompetitionManager> make_competition_manager() {
        auto manager = tupal::CompetitionManager::new_competition_manager("sqlite3://:memory:");
        const auto [ec, result] = manager->create(boost::json::object {
            { "id", "comp-001" },
            { "date", "2024-01-01" },
            { "title", "New Year Competition" }
        });
        REQUIRE(!ec);
        return manager;
    }
}

TEST_CASE("finish time entry manager CRUD and ordering") {
    auto competition_manager = make_competition_manager();
    auto manager = competition_manager->getFinishTimeEntryManager();

    const auto [late_ec, late] = manager->create("comp-001", boost::json::object {
        { "id", "time-002" },
        { "finish_time", "2024-01-01T10:02:00.000Z" },
        { "status", 1 }
    });
    REQUIRE(!late_ec);
    CHECK(late.as_object().at("id").as_string() == "time-002");
    CHECK(late.as_object().at("comp_id").as_string() == "comp-001");

    const auto [early_ec, early] = manager->create("comp-001", boost::json::object {
        { "id", "time-001" },
        { "finish_time", "2024-01-01T10:01:00.000Z" }
    });
    REQUIRE(!early_ec);
    CHECK(early.as_object().at("status").as_int64() == 0);

    const auto [list_ec, entries] = manager->list("comp-001");
    REQUIRE(!list_ec);
    REQUIRE(entries.is_array());
    REQUIRE(entries.as_array().size() == 2);
    CHECK(entries.as_array()[0].as_object().at("id").as_string() == "time-001");
    CHECK(entries.as_array()[1].as_object().at("id").as_string() == "time-002");

    const auto [get_ec, fetched] = manager->get("comp-001", "time-002");
    REQUIRE(!get_ec);
    CHECK(fetched.as_object().at("finish_time").as_string() == "2024-01-01T10:02:00.000Z");
    CHECK(fetched.as_object().at("status").as_int64() == 1);

    const auto [update_ec, updated] = manager->update("comp-001", boost::json::object {
        { "id", "time-002" },
        { "finish_time", "2024-01-01T10:03:00.000Z" }
    });
    REQUIRE(!update_ec);
    CHECK(updated.as_object().at("finish_time").as_string() == "2024-01-01T10:03:00.000Z");
    CHECK(updated.as_object().at("status").as_int64() == 1);

    const auto [remove_ec, removed] = manager->remove("comp-001", "time-002");
    CHECK(!remove_ec);
    CHECK(removed.as_object().at("id").as_string() == "time-002");

    const auto [missing_ec, missing] = manager->get("comp-001", "time-002");
    CHECK(missing_ec == tupal::make_error_code(tupal::error_code::unknown_key));
    CHECK(missing.is_object());
}

TEST_CASE("finish time entry manager rejects invalid data and unknown competition") {
    auto competition_manager = make_competition_manager();
    auto manager = competition_manager->getFinishTimeEntryManager();

    const auto [missing_comp_ec, missing_comp] = manager->list("comp-missing");
    CHECK(missing_comp_ec == tupal::make_error_code(tupal::error_code::unknown_key));
    CHECK(missing_comp.is_object());

    const auto [no_time_ec, no_time] = manager->create("comp-001", boost::json::object {
        { "id", "time-001" }
    });
    CHECK(no_time_ec == tupal::make_error_code(tupal::error_code::invalid_arguments));
    CHECK(no_time.is_object());

    const auto [bad_status_ec, bad_status] = manager->create("comp-001", boost::json::object {
        { "id", "time-001" },
        { "finish_time", "2024-01-01T10:01:00.000Z" },
        { "status", 4 }
    });
    CHECK(bad_status_ec == tupal::make_error_code(tupal::error_code::invalid_arguments));
    CHECK(bad_status.is_object());
}

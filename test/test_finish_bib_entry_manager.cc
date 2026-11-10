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

    boost::json::value create_time_entry(
            const std::shared_ptr<tupal::FinishTimeEntryManager> & manager, const std::string & id) {
        const auto [ec, result] = manager->create("comp-001", boost::json::object {
            { "id", id },
            { "finish_time", "2024-01-01T10:01:00.000Z" }
        });
        REQUIRE(!ec);
        return result;
    }
}

TEST_CASE("finish bib entry manager CRUD and ordering") {
    auto competition_manager = make_competition_manager();
    auto time_manager = competition_manager->getFinishTimeEntryManager();
    auto manager = competition_manager->getFinishBibEntryManager();
    create_time_entry(time_manager, "time-001");

    const auto [later_ec, later] = manager->create("comp-001", boost::json::object {
        { "id", "bib-002" },
        { "sequence_no", 2 },
        { "bib", nullptr }
    });
    REQUIRE(!later_ec);
    CHECK(later.as_object().at("bib").is_null());
    CHECK(later.as_object().at("confirmed_time_entry_id").is_null());

    const auto [earlier_ec, earlier] = manager->create("comp-001", boost::json::object {
        { "id", "bib-001" },
        { "sequence_no", 1 },
        { "bib", 42 },
        { "status", 0 },
        { "confirmed_time_entry_id", "time-001" }
    });
    REQUIRE(!earlier_ec);
    CHECK(earlier.as_object().at("confirmed_time_entry_id").as_string() == "time-001");

    const auto [list_ec, entries] = manager->list("comp-001");
    REQUIRE(!list_ec);
    REQUIRE(entries.is_array());
    REQUIRE(entries.as_array().size() == 2);
    CHECK(entries.as_array()[0].as_object().at("id").as_string() == "bib-001");
    CHECK(entries.as_array()[1].as_object().at("id").as_string() == "bib-002");

    const auto [get_ec, fetched] = manager->get("comp-001", "bib-001");
    REQUIRE(!get_ec);
    CHECK(fetched.as_object().at("sequence_no").as_int64() == 1);
    CHECK(fetched.as_object().at("bib").as_int64() == 42);
    CHECK(fetched.as_object().at("confirmed_time_entry_id").as_string() == "time-001");

    const auto [update_ec, updated] = manager->update("comp-001", boost::json::object {
        { "id", "bib-001" },
        { "sequence_no", 3 }
    });
    REQUIRE(!update_ec);
    CHECK(updated.as_object().at("sequence_no").as_int64() == 3);
    CHECK(updated.as_object().at("bib").as_int64() == 42);
    CHECK(updated.as_object().at("confirmed_time_entry_id").as_string() == "time-001");

    const auto [remove_ec, removed] = manager->remove("comp-001", "bib-001");
    CHECK(!remove_ec);
    CHECK(removed.as_object().at("id").as_string() == "bib-001");

    const auto [missing_ec, missing] = manager->get("comp-001", "bib-001");
    CHECK(missing_ec == tupal::make_error_code(tupal::error_code::unknown_key));
    CHECK(missing.is_object());

    const auto [remove_time_ec, removed_time] = time_manager->remove("comp-001", "time-001");
    CHECK(!remove_time_ec);
    CHECK(removed_time.as_object().at("id").as_string() == "time-001");
}

TEST_CASE("finish bib entry manager enforces competition and entry constraints") {
    auto competition_manager = make_competition_manager();
    auto manager = competition_manager->getFinishBibEntryManager();

    const auto [missing_comp_ec, missing_comp] = manager->list("comp-missing");
    CHECK(missing_comp_ec == tupal::make_error_code(tupal::error_code::unknown_key));
    CHECK(missing_comp.is_object());

    const auto [created_ec, created] = manager->create("comp-001", boost::json::object {
        { "id", "bib-001" },
        { "sequence_no", 1 },
        { "bib", 42 }
    });
    REQUIRE(!created_ec);
    CHECK(created.as_object().at("id").as_string() == "bib-001");

    const auto [duplicate_ec, duplicate] = manager->create("comp-001", boost::json::object {
        { "id", "bib-002" },
        { "sequence_no", 1 }
    });
    CHECK(duplicate_ec == tupal::make_error_code(tupal::error_code::duplicate_key));
    CHECK(duplicate.is_object());

    const auto [bad_reference_ec, bad_reference] = manager->create("comp-001", boost::json::object {
        { "id", "bib-003" },
        { "sequence_no", 2 },
        { "confirmed_time_entry_id", "time-missing" }
    });
    CHECK(bad_reference_ec == tupal::make_error_code(tupal::error_code::constraint_violation));
    CHECK(bad_reference.is_object());

    const auto [bad_sequence_ec, bad_sequence] = manager->create("comp-001", boost::json::object {
        { "id", "bib-004" },
        { "sequence_no", -1 }
    });
    CHECK(bad_sequence_ec == tupal::make_error_code(tupal::error_code::invalid_arguments));
    CHECK(bad_sequence.is_object());
}

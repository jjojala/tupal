#include "doctest/doctest.h"
#include "manager.hh"

TEST_CASE("competition class manager list (no competition)") {
    auto competition_manager = tupal::CompetitionManager::new_competition_manager("sqlite3://:memory:");
    auto competition_class_manager = competition_manager->getCompetitionClassManager();
    auto [ec, competition_classes]  = competition_class_manager->list("comp-001");
    CHECK(ec == tupal::make_error_condition(tupal::error_code::unknown_key));
}

TEST_CASE("competition class manager list (no start groups)") {
    auto competition_manager = tupal::CompetitionManager::new_competition_manager("sqlite3://:memory:");
    auto competition_class_manager = competition_manager->getCompetitionClassManager();

    auto [comp_create_ec, created_competition] = competition_manager->create(
        boost::json::object {
            { "id", "comp-001" },
            { "date", "2024-01-01" },
            { "title", "New Year Competition" }
        });
    CHECK(!comp_create_ec);

    auto [ec, competition_classes]  = competition_class_manager->list("comp-001");
    CHECK(ec == tupal::make_error_condition(tupal::error_code::unknown_key));
}

TEST_CASE("competition class manager list (empty)") {
    auto competition_manager = tupal::CompetitionManager::new_competition_manager("sqlite3://:memory:");
    auto start_group_manager = competition_manager->getStartGroupManager();
    auto competition_class_manager = competition_manager->getCompetitionClassManager();

    auto [comp_create_ec, created_competition] = competition_manager->create(
        boost::json::object {
            { "id", "comp-001" },
            { "date", "2024-01-01" },
            { "title", "New Year Competition" }
        });
    CHECK(!comp_create_ec);

    auto [sg_create_ec, created_start_group] = start_group_manager->create("comp-001",
        boost::json::object {
            { "id", "sg-001" },
            { "title", "Start Group 1" },
            { "start_time", "2024-01-01T10:00:00Z" },
            { "first_bib", 100 }
        });
    CHECK(!sg_create_ec);

    auto [list_ec, competition_classes]  = competition_class_manager->list("comp-001");
    CHECK(list_ec == std::error_code{});
    CHECK(competition_classes.is_array());
    CHECK(competition_classes.as_array().size() == 0);
}

TEST_CASE("competition class manager list (with data)") {
    auto competition_manager = tupal::CompetitionManager::new_competition_manager("sqlite3://:memory:");
    auto start_group_manager = competition_manager->getStartGroupManager();
    auto competition_class_manager = competition_manager->getCompetitionClassManager();

    auto [comp_create_ec, created_competition] = competition_manager->create(
        boost::json::object {
            { "id", "comp-001" },
            { "date", "2024-01-01" },
            { "title", "New Year Competition" }
        });
    CHECK(!comp_create_ec);

    auto [sg_create_ec, created_start_group] = start_group_manager->create("comp-001",
        boost::json::object {
            { "id", "sg-001" },
            { "title", "Start Group 1" },
            { "start_time", "2024-01-01T10:00:00Z" },
            { "first_bib", 100 }
        });
    CHECK(!sg_create_ec);

    auto [cc_create_ec, created_competition_class] = competition_class_manager->create("comp-001",
        boost::json::object {
            { "id", "class-001" },
            { "title", "P10" },
            { "start_group_id", "sg-001" }
        });
    CHECK(!cc_create_ec);

    auto [list_ec, competition_classes]  = competition_class_manager->list("comp-001");
    CHECK(!list_ec);
    CHECK(competition_classes.is_array());
    CHECK(competition_classes.as_array().size() == 1);
}
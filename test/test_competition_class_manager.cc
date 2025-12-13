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
            { "comp_id", "comp-001" },
            { "title", "Start Group 1" },
            { "first_start_time", "2024-01-01T10:00:00Z" },
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
            { "first_start_time", "2024-01-01T10:00:00.000Z" },
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

TEST_CASE("competition class manager get (unknown)") {
    auto competition_manager = tupal::CompetitionManager::new_competition_manager("sqlite3://:memory:");
    auto competition_class_manager = competition_manager->getCompetitionClassManager();

    auto [get_ec, fetched_competition_class] = competition_class_manager->get("comp-001", "class-001");
    CHECK(get_ec == tupal::make_error_condition(tupal::error_code::unknown_key));
}

TEST_CASE("competition class manager get (existing)") {
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
            { "first_start_time", "2024-01-01T10:00:00Z" }
        });
    CHECK(!sg_create_ec);

    auto [cc_create_ec, created_competition_class] = competition_class_manager->create("comp-001",
        boost::json::object {
            { "id", "class-001" },
            { "title", "P10" },
            { "start_group_id", "sg-001" }
        });
    CHECK(!cc_create_ec);

    auto [get_ec, fetched_competition_class] = competition_class_manager->get("comp-001", "class-001");
    CHECK(!get_ec);
    CHECK(fetched_competition_class.is_object());
    CHECK(fetched_competition_class.as_object().at("id").as_string() == "class-001");
}

TEST_CASE("competition class manager create (comp not found)") {
    auto competition_manager = tupal::CompetitionManager::new_competition_manager("sqlite3://:memory:");
    auto competition_class_manager = competition_manager->getCompetitionClassManager();

    auto new_competition_class = boost::json::object {
        { "id", "class-001" },
        { "title", "P10" },
        { "start_group_id", "sg-001" }
    };

    auto [create_ec, created_competition_class] = competition_class_manager->create("comp-001", new_competition_class);
    CHECK(create_ec == tupal::make_error_condition(tupal::error_code::unknown_key));
}

TEST_CASE("competition class manager create and duplicate") {
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
            { "first_start_time", "2024-01-01T10:00:00Z" },
            { "first_bib", 100 }
        });
    CHECK(!sg_create_ec);

    boost::json::object new_competition_class = {
        { "id", "class-001" },
        { "title", "P10" },
        { "start_group_id", "sg-001" }
    };

    auto [create_ec, created_competition_class] = competition_class_manager->create("comp-001", new_competition_class);
    CHECK(!create_ec);
    CHECK(created_competition_class.is_object());
    CHECK(created_competition_class.as_object().at("id").as_string() == "class-001");

    auto [create_ec2, created_competition2] = competition_class_manager->create("comp-001", new_competition_class);
    CHECK(create_ec2 == tupal::make_error_condition(tupal::error_code::duplicate_key));
}

TEST_CASE("competition class manager update (unknown)") {
    auto competition_manager = tupal::CompetitionManager::new_competition_manager("sqlite3://:memory:");
    auto competition_class_manager = competition_manager->getCompetitionClassManager();

    boost::json::object updated_competition_class = {
        { "id", "class-001" },
        { "title", "P12" },
        { "start_group_id", "sg-001" }
    };

    auto [update_ec, updated_result] = competition_class_manager->update("comp-001", updated_competition_class);
    CHECK(update_ec == tupal::make_error_condition(tupal::error_code::unknown_key));
}

TEST_CASE("competition class manager update (existing)") {
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
            { "first_start_time", "2024-01-01T10:00:00Z" },
            { "first_bib", 100 }
        });
    CHECK(!sg_create_ec);

    boost::json::object new_competition_class = {
        { "id", "class-001" },
        { "title", "P10" },
        { "start_group_id", "sg-001" }
    };

    auto [create_ec, created_competition_class] = competition_class_manager->create("comp-001", new_competition_class);
    CHECK(!create_ec);
    CHECK(created_competition_class.is_object());
    CHECK(created_competition_class.as_object().at("id").as_string() == "class-001");

    boost::json::object updated_competition_class = {
        { "id", "class-001" },
        { "title", "P12" },
        { "start_group_id", "sg-001" }
    };
    auto [update_ec, updated_result] = competition_class_manager->update("comp-001", updated_competition_class);
    CHECK(!update_ec);
    CHECK(updated_result.is_object());
    CHECK(updated_result.as_object().at("title").as_string() == "P12");

    auto [get_ec, fetched_competition_class] = competition_class_manager->get("comp-001", "class-001");
    CHECK(!get_ec);
    CHECK(fetched_competition_class.is_object());
    CHECK(fetched_competition_class.as_object().at("title").as_string() == "P12");
}

TEST_CASE("competition class manager remove (unknown)") {
    auto competition_manager = tupal::CompetitionManager::new_competition_manager("sqlite3://:memory:");
    auto competition_class_manager = competition_manager->getCompetitionClassManager();

    auto [ remove_ec, removed_competition_class ] = competition_class_manager->remove("comp-001", "class-001");
    CHECK(remove_ec == tupal::make_error_condition(tupal::error_code::unknown_key));
}

TEST_CASE("competition class manager remove (existing)") {
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
            { "first_start_time", "2024-01-01T10:00:00Z" },
            { "first_bib", 100 }
        });
    CHECK(!sg_create_ec);

    boost::json::object new_competition_class = {
        { "id", "class-001" },
        { "title", "P10" },
        { "start_group_id", "sg-001" }
    };

    auto [create_ec, created_competition_class] = competition_class_manager->create("comp-001", new_competition_class);
    CHECK(!create_ec);
    CHECK(created_competition_class.is_object());
    CHECK(created_competition_class.as_object().at("id").as_string() == "class-001");

    auto [ remove_ec, removed_competition_class ] = competition_class_manager->remove("comp-001", "class-001");
    CHECK(remove_ec == std::error_code {} );
    auto [get_ec, fetched_competition_class] = competition_class_manager->get("comp-001", "class-001");
    CHECK(get_ec == tupal::make_error_condition(tupal::error_code::unknown_key));
}
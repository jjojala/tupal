#include "doctest/doctest.h"
#include "manager.hh"

TEST_CASE("start group manager list (competition not found)") {
    auto competition_manager = tupal::CompetitionManager::new_competition_manager("sqlite3://:memory:");
    auto start_group_manager = competition_manager->getStartGroupManager();
    auto [ec, start_groups]  = start_group_manager->list("comp-001");
    CHECK(ec); // should return error since competition does not exist
    CHECK(ec == tupal::make_error_code(tupal::error_code::unknown_key));
    CHECK(start_groups.is_null());
}

TEST_CASE("start group manager list (empty)") {
    auto competition_manager = tupal::CompetitionManager::new_competition_manager("sqlite3://:memory:");
    auto start_group_manager = competition_manager->getStartGroupManager();
    auto [comp_create_ec, created_competition] = competition_manager->create(
        boost::json::object {
            { "id", "comp-001" },
            { "date", "2024-01-01" },
            { "title", "New Year Competition" }
        });
    CHECK(!comp_create_ec);

    auto [ec, start_groups]  = start_group_manager->list("comp-001");
    CHECK(!ec); // should return error since competition does not exist
    CHECK(start_groups.is_array());
    CHECK(start_groups.as_array().size() == 0);
}

TEST_CASE("start group manager list with entries") {
    auto competition_manager = tupal::CompetitionManager::new_competition_manager("sqlite3://:memory:");
    auto start_group_manager = competition_manager->getStartGroupManager();

    auto [comp_create_ec, created_competition] = competition_manager->create(
        boost::json::object {
            { "id", "comp-001" },
            { "date", "2024-01-01" },
            { "title", "New Year Competition" }
        });
    CHECK(!comp_create_ec);

    auto new_start_group1 = boost::json::object {
        { "id", "sg-001" },
        { "title", "Start Group 1" },
        { "first_start_time", "2024-01-01T10:00:00.000Z" },
        { "first_bib", 100 }
    };

    auto new_start_group2 = boost::json::object {
        { "id", "sg-002" },
        { "title", "Start Group 2" },
        { "first_start_time", "2024-01-01T11:00:00.000Z" },
        { "first_bib", 200 }
    };

    auto [create_ec1, created_start_group1] = start_group_manager->create("comp-001", new_start_group1);
    CHECK(!create_ec1);
    auto [create_ec2, created_start_group2] = start_group_manager->create("comp-001", new_start_group2);
    CHECK(!create_ec2);

    auto [list_ec, start_groups]  = start_group_manager->list("comp-001");
    CHECK(!list_ec);
    CHECK(start_groups.is_array());
    CHECK(start_groups.as_array().size() == 2);
}

TEST_CASE("start group manager create (comp not found)") {
    auto competition_manager = tupal::CompetitionManager::new_competition_manager("sqlite3://:memory:");
    auto start_group_manager = competition_manager->getStartGroupManager();

    auto new_start_group = boost::json::object {
        { "id", "sg-001" },
        { "title", "Start Group 1" },
        { "first_start_time", "2024-01-01T10:00:00.000Z" },
        { "first_bib", 100 }
    };

    auto [create_ec, created_start_group] = start_group_manager->create("comp-001", new_start_group);
    CHECK(create_ec == tupal::make_error_code(tupal::error_code::unknown_key));
}

TEST_CASE("start group manager create and get") {
    auto competition_manager = tupal::CompetitionManager::new_competition_manager("sqlite3://:memory:");
    auto start_group_manager = competition_manager->getStartGroupManager();

    auto [comp_create_ec, created_competition] = competition_manager->create(
        boost::json::object {
            { "id", "comp-001" },
            { "date", "2024-01-01" },
            { "title", "New Year Competition" }
        });
    CHECK(!comp_create_ec);

    auto new_start_group = boost::json::object {
        { "id", "sg-001" },
        { "title", "Start Group 1" },
        { "first_start_time", "2024-01-01T10:00:00.000Z" },
        { "first_bib", 100 }
    };

    auto [create_ec, created_start_group] = start_group_manager->create("comp-001", new_start_group);
    CHECK(!create_ec);
    CHECK(created_start_group.is_object());
    CHECK(created_start_group.as_object().at("id").as_string() == "sg-001");

    auto [get_ec, fetched_start_group] = start_group_manager->get("comp-001", "sg-001");
    CHECK(!get_ec);
    CHECK(fetched_start_group.is_object());
    CHECK(fetched_start_group.as_object().at("title").as_string() == "Start Group 1");
}

TEST_CASE("start group manager get (unknown)") {
    auto competition_manager = tupal::CompetitionManager::new_competition_manager("sqlite3://:memory:");
    auto start_group_manager = competition_manager->getStartGroupManager();

    auto [get_ec, fetched_start_group] = start_group_manager->get("comp-001", "sg-001");
    CHECK(get_ec == tupal::make_error_code(tupal::error_code::unknown_key));
}

TEST_CASE("start group manager create (duplicate)") {
    auto competition_manager = tupal::CompetitionManager::new_competition_manager("sqlite3://:memory:");
    auto start_group_manager = competition_manager->getStartGroupManager();

    auto [comp_create_ec, created_competition] = competition_manager->create(
        boost::json::object {
            { "id", "comp-001" },
            { "date", "2024-01-01" },
            { "title", "New Year Competition" }
        });
    CHECK(!comp_create_ec);

    auto new_start_group = boost::json::object {
        { "id", "sg-001" },
        { "title", "Start Group 1" },
        { "first_start_time", "2024-01-01T10:00:00.000Z" },
        { "first_bib", 100 }
    };

    auto [create_ec1, created_start_group1] = start_group_manager->create("comp-001", new_start_group);
    CHECK(!create_ec1);

    auto [create_ec2, created_start_group2] = start_group_manager->create("comp-001", new_start_group);
    CHECK(create_ec2 == tupal::make_error_code(tupal::error_code::duplicate_key));
}

TEST_CASE("start group manager update (unknown)") {
    auto competition_manager = tupal::CompetitionManager::new_competition_manager("sqlite3://:memory:");
    auto start_group_manager = competition_manager->getStartGroupManager();

    auto updated_start_group = boost::json::object {
        { "id", "sg-001" },
        { "title", "Updated Start Group" },
        { "first_start_time", "2024-01-01T12:00:00.000Z" },
        { "first_bib", 150 }
    };

    auto [update_ec, updated_result] = start_group_manager->update("comp-001", updated_start_group);
    CHECK(update_ec == tupal::make_error_code(tupal::error_code::unknown_key));
}

TEST_CASE("start group manager create and update") {
    auto competition_manager = tupal::CompetitionManager::new_competition_manager("sqlite3://:memory:");
    auto start_group_manager = competition_manager->getStartGroupManager();

    auto [comp_create_ec, created_competition] = competition_manager->create(
        boost::json::object {
            { "id", "comp-001" },
            { "date", "2024-01-01" },
            { "title", "New Year Competition" }
        });
    CHECK(!comp_create_ec);

    auto new_start_group = boost::json::object {
        { "id", "sg-001" },
        { "title", "Start Group 1" },
        { "first_start_time", "2024-01-01T10:00:00.000Z" },
        { "first_bib", 100 }
    };

    auto [create_ec, created_start_group] = start_group_manager->create("comp-001", new_start_group);
    CHECK(!create_ec);

    auto updated_start_group = boost::json::object {
        { "id", "sg-001" },
        { "title", "Updated Start Group" },
        { "first_start_time", "2024-01-01T12:00:00.000Z" },
        { "first_bib", 150 }
    };

    auto [update_ec, updated_result] = start_group_manager->update("comp-001", updated_start_group);
    //CHECK(!update_ec);
    CHECK(update_ec == std::error_code{});
    CHECK(updated_result.is_object());
    CHECK(updated_result.as_object().at("title").as_string() == "Updated Start Group");

    auto [get_ec, fetched_start_group] = start_group_manager->get("comp-001", "sg-001");
    CHECK(!get_ec);
    CHECK(fetched_start_group.is_object());
    CHECK(fetched_start_group.as_object().at("first_start_time").as_string() == "2024-01-01T12:00:00.000Z");
    CHECK(fetched_start_group.as_object().at("first_bib").as_int64() == 150);
}

TEST_CASE("start group manager remove (unknown)") {
    auto competition_manager = tupal::CompetitionManager::new_competition_manager("sqlite3://:memory:");
    auto start_group_manager = competition_manager->getStartGroupManager();

    auto [ remove_ec, removed_start_group ] = start_group_manager->remove("comp-001", "sg-001");
    CHECK(remove_ec == tupal::make_error_code(tupal::error_code::unknown_key));
}

TEST_CASE("start group manager create and remove") {
    auto competition_manager = tupal::CompetitionManager::new_competition_manager("sqlite3://:memory:");
    auto start_group_manager = competition_manager->getStartGroupManager();

    auto [comp_create_ec, created_competition] = competition_manager->create(
        boost::json::object {
            { "id", "comp-001" },
            { "date", "2024-01-01" },
            { "title", "New Year Competition" }
        });
    CHECK(!comp_create_ec);

    auto new_start_group = boost::json::object {
        { "id", "sg-001" },
        { "title", "Start Group 1" },
        { "first_start_time", "2024-01-01T10:00:00.000Z" },
        { "first_bib", 100 }
    };

    auto [create_ec, created_start_group] = start_group_manager->create("comp-001", new_start_group);
    CHECK(!create_ec);

    auto [remove_ec, removed_start_group] = start_group_manager->remove("comp-001", "sg-001");
    CHECK(remove_ec == std::error_code {});

    auto [get_ec, fetched_start_group] = start_group_manager->get("comp-001", "sg-001");
    CHECK(get_ec == tupal::make_error_code(tupal::error_code::unknown_key));
}   
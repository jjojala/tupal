#include "doctest/doctest.h"
#include "manager.hh"


TEST_CASE("competition manager list (empty)") {
    auto competition_manager = tupal::CompetitionManager::new_competition_manager("sqlite3://:memory:");
    auto [ec, competitions]  = competition_manager->list();
    CHECK(!ec);
    CHECK(competitions.is_array());
    CHECK(competitions.as_array().size() == 0);
}

TEST_CASE("competition manager create and get") {
    auto competition_manager = tupal::CompetitionManager::new_competition_manager("sqlite3://:memory:");

    auto new_competition = boost::json::object {
        { "id", "comp-001" },
        { "date", "2024-01-01" },
        { "title", "New Year Competition" }
    };

    auto [create_ec, created_competition] = competition_manager->create(new_competition);
    CHECK(!create_ec);
    CHECK(created_competition.is_object());
    CHECK(created_competition.as_object().at("id").as_string() == "comp-001");

    auto [get_ec, fetched_competition] = competition_manager->get("comp-001");
    CHECK(!get_ec);
    CHECK(fetched_competition.is_object());
    CHECK(fetched_competition.as_object().at("title").as_string() == "New Year Competition");
}

TEST_CASE("competition manager update and remove") {
    auto competition_manager = tupal::CompetitionManager::new_competition_manager("sqlite3://:memory:");

    auto new_competition = boost::json::object {
        { "id", "comp-002" },
        { "date", "2024-02-14" },
        { "title", "Valentine's Day Competition" }
    };

    auto [create_ec, created_competition] = competition_manager->create(new_competition);
    CHECK(!create_ec);

    auto updated_competition = boost::json::object {
        { "id", "comp-002" },
        { "date", "2024-02-15" }, // changed date
        { "title", "Valentine's Day Competition" }
    };

    auto [update_ec, updated_result] = competition_manager->update(updated_competition);
    CHECK(!update_ec);
    CHECK(updated_result.is_object());
    CHECK(updated_result.as_object().at("date").as_string() == "2024-02-15T00:00:00.000Z");

    auto [ remove_ec, removed_competition ] = competition_manager->remove("comp-002");
    CHECK(remove_ec == std::error_code {} );

    auto [get_ec, fetched_competition] = competition_manager->get("comp-002");
    CHECK(get_ec); // should return error since it's removed

    auto [list_ec, competitions] = competition_manager->list();
    CHECK(!list_ec);
    CHECK(competitions.is_array());
    CHECK(competitions.as_array().size() == 0);
}

TEST_CASE("competition manager duplicate create") {
    auto competition_manager = tupal::CompetitionManager::new_competition_manager("sqlite3://:memory:");

    auto new_competition = boost::json::object {
        { "id", "comp-003" },
        { "date", "2024-03-17" },
        { "title", "St. Patrick's Day Competition" }
    };

    auto [create_ec1, created_competition1] = competition_manager->create(new_competition);
    CHECK(!create_ec1);

    auto [create_ec2, created_competition2] = competition_manager->create(new_competition);
    CHECK(create_ec2); // should return error for duplicate key
    CHECK(create_ec2 == tupal::make_error_code(tupal::error_code::duplicate_key));

    auto [list_ec, competitions] = competition_manager->list();
    CHECK(!list_ec);
    CHECK(competitions.is_array());
    CHECK(competitions.as_array().size() == 1);
}

TEST_CASE("competition manager get unknown") {
    auto competition_manager = tupal::CompetitionManager::new_competition_manager("sqlite3://:memory:");

    auto [get_ec, fetched_competition] = competition_manager->get("non-existent-id");
    CHECK(get_ec); // should return error since it doesn't exist
    CHECK(get_ec == tupal::make_error_code(tupal::error_code::unknown_key));
}

TEST_CASE("competition manager remove unknown") {
    auto competition_manager = tupal::CompetitionManager::new_competition_manager("sqlite3://:memory:");

    auto [ remove_ec, removed_competition ] = competition_manager->remove("non-existent-id");
    CHECK(remove_ec == tupal::make_error_code(tupal::error_code::unknown_key));
}

TEST_CASE("competition manager update unknown") {
    auto competition_manager = tupal::CompetitionManager::new_competition_manager("sqlite3://:memory:");

    auto updated_competition = boost::json::object {
        { "id", "non-existent-id" },
        { "date", "2024-12-31" },
        { "title", "Non Existent Competition" }
    };

    auto [update_ec, updated_result] = competition_manager->update(updated_competition);
    CHECK(update_ec); // should return error since it doesn't exist
    CHECK(update_ec == tupal::make_error_code(tupal::error_code::unknown_key));
}

TEST_CASE("competition manager list after multiple creates") {
    auto competition_manager = tupal::CompetitionManager::new_competition_manager("sqlite3://:memory:");

    for (int i = 1; i <= 5; ++i) {
        auto new_competition = boost::json::object {
            { "id", std::string("comp-") + std::to_string(i) },
            { "date", std::string("2024-04-") + (i < 10 ? "0" : "") + std::to_string(i) },
            { "title", std::string("Competition ") + std::to_string(i) }
        };

        auto [create_ec, created_competition] = competition_manager->create(new_competition);
        CHECK(!create_ec);
    }

    auto [list_ec, competitions] = competition_manager->list();
    CHECK(!list_ec);
    CHECK(competitions.is_array());
    CHECK(competitions.as_array().size() == 5);
}
#include "doctest/doctest.h"
#include "manager.hh"

TEST_CASE("competition manager list (no comp)") {
    auto competition_manager = tupal::CompetitionManager::new_competition_manager("sqlite3://:memory:");
    auto competitor_manager = competition_manager->getCompetitorManager();

    auto [ec, competitions]  = competitor_manager->list("comp-001");
    CHECK(ec == tupal::make_error_code(tupal::error_code::unknown_key));
}

TEST_CASE("competitor manager list (no comp class)") {
    auto competition_manager = tupal::CompetitionManager::new_competition_manager("sqlite3://:memory:");
    auto competitor_manager = competition_manager->getCompetitorManager();

    auto [comp_create_ec, created_competition] = competition_manager->create(
        boost::json::object {
            { "id", "comp-001" },
            { "date", "2024-01-01" },
            { "title", "New Year Competition" }
        });
    CHECK(!comp_create_ec);

    auto [ec, competitions]  = competitor_manager->list("comp-001");
    CHECK(ec == tupal::make_error_code(tupal::error_code::unknown_key));
}

TEST_CASE("competitor manager list (no data)") {
    auto competition_manager = tupal::CompetitionManager::new_competition_manager("sqlite3://:memory:");
    auto start_group_manager = competition_manager->getStartGroupManager();
    auto competition_class_manager = competition_manager->getCompetitionClassManager();
    auto competitor_manager = competition_manager->getCompetitorManager();

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

    auto [list_ec, competitors]  = competitor_manager->list("comp-001");
    CHECK(!list_ec);
    CHECK(competitors.is_array());
    CHECK(competitors.as_array().size() == 0);
}

TEST_CASE("competitor manager list (with data)") {
    auto competition_manager = tupal::CompetitionManager::new_competition_manager("sqlite3://:memory:");
    auto start_group_manager = competition_manager->getStartGroupManager();
    auto competition_class_manager = competition_manager->getCompetitionClassManager();
    auto competitor_manager = competition_manager->getCompetitorManager();

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

    boost::json::value new_competitor1 = boost::json::object {
        { "id", "competitor-001" },
        { "comp_class_id", "class-001" },
        { "bib", 101 },
        { "start_time_offset", "00:10:00" },
        { "finish_time", "" },
        { "status", 0 },
        { "name", "Alice" }
    };
    boost::json::value new_competitor2 = boost::json::object {
        { "id", "competitor-002" },
        { "comp_class_id", "class-001" },
        { "bib", 102 },
        { "start_time_offset", "00:15:00" },
        { "finish_time", "" },
        { "status", 0 },
        { "name", "Bob" }
    };
    auto [create_ec1, created_competitor1] = competitor_manager->create("comp-001", new_competitor1);
    CHECK(!create_ec1);
    auto [create_ec2, created_competitor2] = competitor_manager->create("comp-001", new_competitor2);
    CHECK(!create_ec2); 
    auto [list_ec, competitors]  = competitor_manager->list("comp-001");
    CHECK(!list_ec);
    CHECK(competitors.is_array());
    CHECK(competitors.as_array().size() == 2);
}

TEST_CASE("competitor manager create (no comp)") {
    auto competition_manager = tupal::CompetitionManager::new_competition_manager("sqlite3://:memory:");
    auto competitor_manager = competition_manager->getCompetitorManager();

    boost::json::value new_competitor = boost::json::object {
        { "id", "competitor-001" },
        { "comp_class_id", "class-001" },
        { "bib", 101 },
        { "start_time_offset", "00:10:00" },
        { "finish_time", "" },
        { "status", 0 },
        { "name", "Alice" }
    };

    auto [create_ec, created_competitor] = competitor_manager->create("comp-001", new_competitor);
    CHECK(create_ec == tupal::make_error_code(tupal::error_code::unknown_key));
}

TEST_CASE("competitor manager create (no start group)") {
    auto competition_manager = tupal::CompetitionManager::new_competition_manager("sqlite3://:memory:");
    auto competitor_manager = competition_manager->getCompetitorManager();

    auto [comp_create_ec, created_competition] = competition_manager->create(
        boost::json::object {
            { "id", "comp-001" },
            { "date", "2024-01-01" },
            { "title", "New Year Competition" }
        });
    CHECK(!comp_create_ec);

    boost::json::value new_competitor = boost::json::object {
        { "id", "competitor-001" },
        { "comp_class_id", "class-001" },
        { "bib", 101 },
        { "start_time_offset", "00:10:00" },
        { "finish_time", "" },
        { "status", 0 },
        { "name", "Alice" }
    };

    auto [create_ec, created_competitor] = competitor_manager->create("comp-001", new_competitor);
    CHECK(create_ec == tupal::make_error_code(tupal::error_code::unknown_key));
}

TEST_CASE("competitor manager create (no comp class)") {
    auto competition_manager = tupal::CompetitionManager::new_competition_manager("sqlite3://:memory:");
    auto start_group_manager = competition_manager->getStartGroupManager();
    auto competitor_manager = competition_manager->getCompetitorManager();

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

    boost::json::value new_competitor = boost::json::object {
        { "id", "competitor-001" },
        { "comp_class_id", "class-001" },
        { "bib", 101 },
        { "start_time_offset", "00:10:00" },
        { "finish_time", "" },
        { "status", 0 },
        { "name", "Alice" }
    };
    auto [create_ec, created_competitor] = competitor_manager->create("comp-001", new_competitor);
    CHECK(create_ec == tupal::make_error_code(tupal::error_code::unknown_key));
}

TEST_CASE("competitor manager create and get") {
    auto competition_manager = tupal::CompetitionManager::new_competition_manager("sqlite3://:memory:");
    auto start_group_manager = competition_manager->getStartGroupManager();
    auto competition_class_manager = competition_manager->getCompetitionClassManager();
    auto competitor_manager = competition_manager->getCompetitorManager();

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

    boost::json::value new_competitor = boost::json::object {
        { "id", "competitor-001" },
        { "comp_class_id", "class-001" },
        { "bib", 101 },
        { "start_time_offset", "00:10:00" },
        { "finish_time", "" },
        { "status", 0 },
        { "name", "Alice" }
    };
    auto [create_ec, created_competitor] = competitor_manager->create("comp-001", new_competitor);
    CHECK(!create_ec);
    CHECK(created_competitor.is_object());
    CHECK(created_competitor.as_object().at("id").as_string() == "competitor-001");
    CHECK(created_competitor.as_object().at("comp_class_id").as_string() == "class-001");
    CHECK(created_competitor.as_object().at("bib").as_int64() == 101);
    CHECK(created_competitor.as_object().at("start_time_offset").as_string() == "00:10:00");
    CHECK(created_competitor.as_object().at("finish_time").as_string() == "");
    CHECK(created_competitor.as_object().at("status").as_int64() == 0);
    CHECK(created_competitor.as_object().at("name").as_string() == "Alice");

    auto [get_ec, fetched_competitor] = competitor_manager->get("comp-001", "competitor-001");
    CHECK(!get_ec);
    CHECK(fetched_competitor.is_object());
    CHECK(fetched_competitor.as_object().at("name").as_string() == "Alice");
}

TEST_CASE("competitor manager get (unknown)") {
    auto competition_manager = tupal::CompetitionManager::new_competition_manager("sqlite3://:memory:");
    auto competitor_manager = competition_manager->getCompetitorManager();

    auto [get_ec, fetched_competitor] = competitor_manager->get("comp-001", "competitor-001");
    CHECK(get_ec == tupal::make_error_code(tupal::error_code::unknown_key));
}

TEST_CASE("competitor manager update (unknown)") {
    auto competition_manager = tupal::CompetitionManager::new_competition_manager("sqlite3://:memory:");
    auto competitor_manager = competition_manager->getCompetitorManager();

    boost::json::value updated_competitor = boost::json::object {
        { "id", "competitor-001" },
        { "comp_class_id", "class-001" },
        { "bib", 101 },
        { "start_time_offset", "00:10:00" },
        { "finish_time", "" },
        { "status", 0 },
        { "name", "Alice" }
    };

    auto [update_ec, updated_result] = competitor_manager->update("comp-001", updated_competitor);
    CHECK(update_ec == tupal::make_error_code(tupal::error_code::unknown_key));
}

TEST_CASE("competitor manager update (existing)") {
    auto competition_manager = tupal::CompetitionManager::new_competition_manager("sqlite3://:memory:");
    auto start_group_manager = competition_manager->getStartGroupManager();
    auto competition_class_manager = competition_manager->getCompetitionClassManager();
    auto competitor_manager = competition_manager->getCompetitorManager();

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

    boost::json::value new_competitor = boost::json::object {
        { "id", "competitor-001" },
        { "comp_class_id", "class-001" },
        { "bib", 101 },
        { "start_time_offset", "00:10:00" },
        { "finish_time", "" },
        { "status", 0 },
        { "name", "Alice" }
    };
    auto [create_ec, created_competitor] = competitor_manager->create("comp-001", new_competitor);
    CHECK(!create_ec);
    CHECK(created_competitor.is_object());
    CHECK(created_competitor.as_object().at("id").as_string() == "competitor-001");

    boost::json::value updated_competitor = boost::json::object {
        { "id", "competitor-001" },
        { "comp_class_id", "class-001" },
        { "bib", 102 },
        { "start_time_offset", "00:12:00" },
        { "finish_time", "01:00:00" },
        { "status", 1 },
        { "name", "Alice Smith" }
    };
    auto [update_ec, updated_result] = competitor_manager->update("comp-001", updated_competitor);
    CHECK(!update_ec);
    CHECK(updated_result.is_object());
    CHECK(updated_result.as_object().at("bib").as_int64() == 102);
    CHECK(updated_result.as_object().at("start_time_offset").as_string() == "00:12:00");
    CHECK(updated_result.as_object().at("finish_time").as_string() == "01:00:00");
    CHECK(updated_result.as_object().at("status").as_int64() == 1);
    CHECK(updated_result.as_object().at("name").as_string() == "Alice Smith");

    auto [get_ec, fetched_competitor] = competitor_manager->get("comp-001", "competitor-001");
    CHECK(!get_ec);
    CHECK(fetched_competitor.is_object());
    CHECK(fetched_competitor.as_object().at("bib").as_int64() == 102);
    CHECK(fetched_competitor.as_object().at("start_time_offset").as_string() == "00:12:00");
    CHECK(fetched_competitor.as_object().at("finish_time").as_string() == "01:00:00");
    CHECK(fetched_competitor.as_object().at("status").as_int64() == 1);
    CHECK(fetched_competitor.as_object().at("name").as_string() == "Alice Smith");
}

TEST_CASE("competitor manager remove (unknown)") {
    auto competition_manager = tupal::CompetitionManager::new_competition_manager("sqlite3://:memory:");
    auto competitor_manager = competition_manager->getCompetitorManager();

    auto [ remove_ec, removed_competitor ] = competitor_manager->remove("comp-001", "competitor-001");
    CHECK(remove_ec == tupal::make_error_code(tupal::error_code::unknown_key));
}

TEST_CASE("competitor manager create and remove") {
    auto competition_manager = tupal::CompetitionManager::new_competition_manager("sqlite3://:memory:");
    auto start_group_manager = competition_manager->getStartGroupManager();
    auto competition_class_manager = competition_manager->getCompetitionClassManager();
    auto competitor_manager = competition_manager->getCompetitorManager();

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

    boost::json::value new_competitor = boost::json::object {
        { "id", "competitor-001" },
        { "comp_class_id", "class-001" },
        { "bib", 101 },
        { "start_time_offset", "00:10:00" },
        { "finish_time", "" },
        { "status", 0 },
        { "name", "Alice" }
    };
    auto [create_ec, created_competitor] = competitor_manager->create("comp-001", new_competitor);
    CHECK(!create_ec);

    auto [ remove_ec, removed_competitor ] = competitor_manager->remove("comp-001", "competitor-001");
    CHECK(remove_ec == std::error_code {} );
    auto [get_ec, fetched_competitor] = competitor_manager->get("comp-001", "competitor-001");
    CHECK(get_ec == tupal::make_error_code(tupal::error_code::unknown_key));
}
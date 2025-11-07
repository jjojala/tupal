#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

#include "manager.hh"

// defer creation until first use to avoid static init order / early dlopen problems
static std::shared_ptr<tupal::CompetitionManager> get_competition_manager()
{
    static auto mgr = tupal::CompetitionManager::new_competition_manager("sqlite3://:memory:");
    return mgr;
}

TEST_CASE("sanity") {
    CHECK(1 + 1 == 2);
}

TEST_CASE("competition manager list") {
    auto competition_manager = get_competition_manager();
    auto result = competition_manager->list();
//    CHECK(tupal::ec(result) == tupal::make_error_condition(tupal::error_code::ok));
//    auto json_value = tupal::json(result);
//    CHECK(json_value.is_array());
}

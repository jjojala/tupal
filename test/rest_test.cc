#include "doctest/doctest.h"
#include "boost/process.hpp"
#include "boost/asio.hpp"
#include "boost/json.hpp"
#include "beauty/beauty.hpp"
#include <iostream>
#include <thread>
#include <chrono>


TEST_CASE("rest tests (smoke)") {

    boost::process::child daemon(boost::process::search_path("tupald"));
    std::this_thread::sleep_for(std::chrono::seconds(2));

    beauty::client client;

    {
        auto [ec, response] = client.get("http://localhost:8085/rest/competition/");
        CHECK(ec == std::error_code {});
        CHECK(response.status() == boost::beast::http::status::ok);
        const auto result = boost::json::parse(response.body());
        CHECK(result.is_array());
        CHECK(result.as_array().size() == 0);
    }

    {
        auto [ec, response] = client.post("http://localhost:8085/rest/competition/", 
            R"({ "id": "comp-1", "title": "Competition 1", "date": "2025-11-11" })");
        CHECK(ec == std::error_code {});
        CHECK(response.status() == boost::beast::http::status::created);
        const auto result = boost::json::parse(response.body());
        CHECK(result.is_object());
    }

    {
        auto [ec, response] = client.get("http://localhost:8085/rest/competition/comp-1");
        CHECK(ec == std::error_code {});
        CHECK(response.status() == boost::beast::http::status::ok);
        const auto result = boost::json::parse(response.body());
        CHECK(result.is_object());
    }

    {
        auto [ec, response] = client.get("http://localhost:8085/rest/competition/");
        CHECK(ec == std::error_code {});
        CHECK(response.status() == boost::beast::http::status::ok);
        auto result = boost::json::parse(response.body());
        CHECK(result.is_array());
        CHECK(result.as_array().size() == 1);
    }

    {
        auto [ec, response] = client.get("http://localhost:8085/rest/competition/comp-1/start_group/");
        CHECK(ec == std::error_code {});
        MESSAGE(response.body());
        CHECK(response.status() == boost::beast::http::status::ok);
        auto result = boost::json::parse(response.body());
        CHECK(result.is_array());
        CHECK(result.as_array().size() == 0);
    }

    {
        auto [ec, response] = client.post("http://localhost:8085/rest/competition/comp-1/start_group/", 
            R"({ 
                "id": "sg-1",
                "title": "Start group 1",
                "first_start_time": "2025-11-11T18:00:00.000Z",
                "first_bib": 1
            })");
        CHECK(ec == std::error_code {});
        CHECK(response.status() == boost::beast::http::status::created);
        const auto result = boost::json::parse(response.body());
        CHECK(result.is_object());
    }

    {
        auto [ec, response] = client.get("http://localhost:8085/rest/competition/comp-1/start_group/");
        CHECK(ec == std::error_code {});
        CHECK(response.status() == boost::beast::http::status::ok);
        const auto result = boost::json::parse(response.body());
        CHECK(result.is_array());
        CHECK(result.as_array().size() == 1);
    }


    {
        auto [ec, response] = client.get("http://localhost:8085/rest/competition/comp-1/start_group/sg-1");
        CHECK(ec == std::error_code {});
        CHECK(response.status() == boost::beast::http::status::ok);
        const auto result = boost::json::parse(response.body());
        CHECK(result.is_object());
    }

    {
        auto [ec, response] = client.get("http://localhost:8085/rest/competition/comp-1/start_group/");
        CHECK(ec == std::error_code {});
        auto result = boost::json::parse(response.body());
        CHECK(result.is_array());
        CHECK(result.as_array().size() == 1);
    }

    {
        auto [ec, response] = client.del("http://localhost:8085/rest/competition/comp-1/start_group/sg-1");
        CHECK(ec == std::error_code {});
        CHECK(response.status() == boost::beast::http::status::ok);
        auto result = boost::json::parse(response.body());
        MESSAGE(result);
    }
}   
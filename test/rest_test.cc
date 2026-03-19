#include "doctest/doctest.h"
#include "boost/process.hpp"
#include "boost/asio.hpp"
#include "boost/json.hpp"
#include "beauty/beauty.hpp"
#include <iostream>
#include <chrono>

namespace {
    const std::string base_url = "http://localhost:8085";

    /** @see definition below */
    bool contains(const boost::json::value &, const boost::json::value &);

    bool contains(const boost::json::object & obj, const boost::json::value & item) {
        if (item.is_object()) {
            bool status = true;

            const auto item_object = item.as_object();
            for (const auto & [key, value] : obj) {
                if (item_object.contains(key)) {
                    if (!contains(value, item_object.at(key)))
                        status = false;
                } else {
                    MESSAGE("Object '", item, "' expected to have key '", key, "' but it was not found");
                    status = false;
                }
            }
            return status;
        }

        MESSAGE("Item '", item, "' expected to be object, but it was not");
        return false;
    }

    bool contains(const boost::json::value & val, const boost::json::value & item) {
        if (val.is_object())
            return contains(val.as_object(), item);

        else if (val.is_array()) {
            bool status = true;
            for (const auto values: val.as_array())
                if (!contains(values, item))
                    status = false;
            return status;
        }

        if (val == item)
            return true;

        MESSAGE("Value '", item, "' expected to be '", val, "' but it was not");
        return false;
    }

    /** @brief This is a doctest fixture for test cases that do not use ws or want to set it up by themselves. */
    class daemon_fixture {
    private:
        boost::process::child daemon;
    protected:
        beauty::client client;
    public:
        typedef std::vector<std::string> ws_messages_type;

        daemon_fixture() {
            daemon = boost::process::child(boost::process::search_path("tupald"));

            beauty::client client;
            while (true) {
                const auto [ec, resp] = client.get(base_url + "/rest/competition/");
                if (!ec && resp.status() == boost::beast::http::status::ok && resp.body() == "[]")
                    break;
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
            MESSAGE("Daemon is up and running");
        }

        void init_ws(const std::string & comp_id, ws_messages_type & ws_messages) {
            client.ws(base_url + "/ws/" + comp_id, beauty::ws_handler {
                .on_receive = [&ws_messages](const beauty::ws_context &, const char * data, std::size_t size, bool) {
                    ws_messages.push_back(std::string { data, size });
                }
            });

            while(ws_messages.empty()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
            MESSAGE("WebSocket connection established");
        }
    };

    /** @brief This is a doctest fixture that set up a ws for 'comp-1' */
    class ws_daemon_fixture : public daemon_fixture {
    protected:
        std::vector<std::string> ws_messages;
    public:
        ws_daemon_fixture() : daemon_fixture() {
            init_ws("comp-1", ws_messages);
            ws_messages.clear();
        }
    };
}

}

TEST_CASE_FIXTURE(ws_daemon_fixture, "rest tests (smoke)") {

    MESSAGE("WebSocket client set up, starting REST tests...");
    CHECK(ws_messages.size() == 1);
    CHECK(ws_messages.front() == R"({})");
    ws_messages.clear();

    {
        auto [ec, response] = client.get(base_url + "/rest/competition/");
        CHECK(ec == std::error_code {});
        CHECK(response.status() == boost::beast::http::status::ok);
        const auto result = boost::json::parse(response.body());
        CHECK(result.is_array());
        CHECK(result.as_array().size() == 0);
    }

    {
        const auto data = R"({ "id": "comp-1", "title": "Competition 1", "date": "2025-11-11T00:00:00.000Z" })";

        {
            auto [ec, response] = client.post(base_url + "/rest/competition/", data);
            CHECK(ec == std::error_code {});
            CHECK(response.status() == boost::beast::http::status::created);
            const auto result = boost::json::parse(response.body());
            CHECK(contains(boost::json::parse(data), result));

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            CHECK(ws_messages.size() == 0);
        }

        {
            auto [ec, response] = client.get(base_url + "/rest/competition/comp-1");
            CHECK(ec == std::error_code {});
            CHECK(response.status() == boost::beast::http::status::ok);
            const auto result = boost::json::parse(response.body());
            CHECK(contains(boost::json::parse(data), result));
            MESSAGE("\nSent    : ", boost::json::parse(data), "\nReceived: ", result);
        }

        {
            auto [ec, response] = client.get(base_url + "/rest/competition/");
            CHECK(ec == std::error_code {});
            CHECK(response.status() == boost::beast::http::status::ok);
            auto result = boost::json::parse(response.body());
            CHECK(result.is_array());
            CHECK(result.as_array().size() == 1);
            CHECK(contains(boost::json::parse(data), result.as_array().front()));
        }
    }

    {
        const auto data = R"({ 
                    "id": "sg-1",
                    "title": "Start group 1",
                    "first_start_time": "2025-11-11T18:00:00.000Z",
                    "first_bib": 1
                })";
        {
            auto [ec, response] = client.get(base_url + "/rest/competition/comp-1/start_group/");
            CHECK(ec == std::error_code {});
            MESSAGE(response.body());
            CHECK(response.status() == boost::beast::http::status::ok);
            auto result = boost::json::parse(response.body());
            CHECK(result.is_array());
            CHECK(result.as_array().size() == 0);
        }

        {
            auto [ec, response] = client.post(base_url + "/rest/competition/comp-1/start_group/", data);
            CHECK(ec == std::error_code {});
            CHECK(response.status() == boost::beast::http::status::created);
            const auto result = boost::json::parse(response.body());
            CHECK(contains(boost::json::parse(data), result));
            CHECK(result.as_object().at("comp_id").as_string() == "comp-1");
            CHECK(result.as_object().size() == (boost::json::parse(data).as_object().size() + 1));
            MESSAGE(result);

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            CHECK(ws_messages.size() == 1);
            const auto msg = boost::json::parse(ws_messages.front());

            CHECK(msg.is_object());
            CHECK(msg.as_object().size() == 3);
            CHECK(msg.as_object().at("op").as_string() == "created");
            CHECK(msg.as_object().at("type").as_string() == "start_group");
            CHECK(contains(boost::json::parse(data), msg.as_object().at("item")));
            CHECK(msg.as_object().at("item").as_object().at("comp_id").as_string() == "comp-1");
            CHECK(msg.as_object().at("item").as_object().size() == (boost::json::parse(data).as_object().size() + 1));

            ws_messages.clear();
        }

        {
            auto [ec, response] = client.get(base_url + "/rest/competition/comp-1/start_group/");
            CHECK(ec == std::error_code {});
            CHECK(response.status() == boost::beast::http::status::ok);
            const auto result = boost::json::parse(response.body());
            CHECK(result.is_array());
            CHECK(result.as_array().size() == 1);
            const auto & item = result.as_array().front();
            CHECK(contains(boost::json::parse(data), item));
            CHECK(item.as_object().at("comp_id").as_string() == "comp-1");
            CHECK(item.as_object().size() == (boost::json::parse(data).as_object().size() + 1));
        }

        {
            auto [ec, response] = client.get(base_url + "/rest/competition/comp-1/start_group/sg-1");
            CHECK(ec == std::error_code {});
            CHECK(response.status() == boost::beast::http::status::ok);
            const auto result = boost::json::parse(response.body());
            CHECK(result.is_object());
            CHECK(contains(boost::json::parse(data), result));
            CHECK(result.as_object().at("comp_id").as_string() == "comp-1");
            CHECK(result.as_object().size() == (boost::json::parse(data).as_object().size() + 1));
        }
    }

    // competition class
    {
        const auto data = R"({ 
                    "id": "cc-1",
                    "title": "Competition class 1",
                    "start_group_id": "sg-1"
                })";
        {
            auto [ec, response] = client.get(base_url + "/rest/competition/comp-1/competition_class/");
            CHECK(ec == std::error_code {});
            CHECK(response.status() == boost::beast::http::status::ok);
            const auto result = boost::json::parse(response.body());
            CHECK(result.is_array());
            CHECK(result.as_array().size() == 0);
        }

        {
            auto [ec, response] = client.post(base_url + "/rest/competition/comp-1/competition_class/", data);
            CHECK(ec == std::error_code {});
            CHECK(response.status() == boost::beast::http::status::created);

            const auto result = boost::json::parse(response.body());
            CHECK(contains(boost::json::parse(data), result));
            CHECK(result.as_object().at("comp_id").as_string() == "comp-1");
            CHECK(result.as_object().size() == (boost::json::parse(data).as_object().size() + 1));

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            CHECK(ws_messages.size() == 1);
            const auto msg = boost::json::parse(ws_messages.front());
            CHECK(msg.as_object().size() == 3);
            CHECK(msg.as_object().at("op").as_string() == "created");
            CHECK(msg.as_object().at("type").as_string() == "competition_class");
            CHECK(contains(boost::json::parse(data), msg.as_object().at("item")));
            CHECK(msg.as_object().at("item").as_object().at("comp_id").as_string() == "comp-1");
            CHECK(msg.as_object().at("item").as_object().size() == (boost::json::parse(data).as_object().size() + 1));

            ws_messages.clear();
        }

        {
            auto [ec, response] = client.get(base_url + "/rest/competition/comp-1/competition_class/");
            CHECK(ec == std::error_code {});
            CHECK(response.status() == boost::beast::http::status::ok);
            const auto result = boost::json::parse(response.body());
            CHECK(result.is_array());
            CHECK(result.as_array().size() == 1);
            const auto & item = result.as_array().front();
            CHECK(contains(boost::json::parse(data), item));
            CHECK(item.as_object().at("comp_id").as_string() == "comp-1");
            CHECK(item.as_object().size() == (boost::json::parse(data).as_object().size() + 1));
        }

        {
            auto [ec, response] = client.get(base_url + "/rest/competition/comp-1/competition_class/cc-1");
            CHECK(ec == std::error_code {});
            CHECK(response.status() == boost::beast::http::status::ok);
            const auto result = boost::json::parse(response.body());
            CHECK(contains(boost::json::parse(data), result));
            CHECK(result.as_object().at("comp_id").as_string() == "comp-1");
            CHECK(result.as_object().size() == (boost::json::parse(data).as_object().size() + 1));
        }
    }

    {
        const auto data = R"({ "id": "c-1",
                "comp_class_id": "cc-1",
                "bib": 101,
                "start_time_offset": "PT600.000S",
                "finish_time": "2024-01-01T10:59:11.231Z",
                "status": 0,
                "name": "Alice"
            })";
        {
            auto [ec, response] = client.get(base_url + "/rest/competition/comp-1/competitor/");
            CHECK(ec == std::error_code {});
            CHECK(response.status() == boost::beast::http::status::ok);
            const auto result = boost::json::parse(response.body());
            CHECK(result.is_array());
            CHECK(result.as_array().size() == 0);
        }

        {
            auto [ec, response] = client.post(base_url + "/rest/competition/comp-1/competitor/", data);
            CHECK(ec == std::error_code {});
            CHECK(response.status() == boost::beast::http::status::created);
            const auto result = boost::json::parse(response.body());
            CHECK(contains(boost::json::parse(data), result));
            CHECK(result.as_object().at("comp_id").as_string() == "comp-1");
            CHECK(result.as_object().size() == (boost::json::parse(data).as_object().size() + 1));

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            CHECK(ws_messages.size() == 1);
            const auto msg = boost::json::parse(ws_messages.front());
            CHECK(msg.is_object());
            CHECK(msg.as_object().size() == 3);
            CHECK(msg.as_object().at("op").as_string() == "created");
            CHECK(msg.as_object().at("type").as_string() == "competitor");

            const auto & item = msg.as_object().at("item");
            CHECK(contains(boost::json::parse(data), item));
            CHECK(item.as_object().at("comp_id").as_string() == "comp-1");
            CHECK(item.as_object().size() == (boost::json::parse(data).as_object().size() + 1));

            ws_messages.clear();
        }
    }

    // cleanup

    {
        auto [ec, response] = client.del(base_url + "/rest/competition/comp-1/competitor/c-1");
        CHECK(ec == std::error_code {});
        CHECK(response.status() == boost::beast::http::status::ok);
        const auto result = boost::json::parse(response.body());
        CHECK(result.is_object());
        CHECK(result.as_object().at("id").as_string() == "c-1");
        CHECK(result.as_object().at("comp_id").as_string() == "comp-1");
        MESSAGE(result);

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        CHECK(ws_messages.size() == 1);
        const auto msg = boost::json::parse(ws_messages.front());
        MESSAGE(msg);
        CHECK(msg.is_object());
        CHECK(msg.as_object().size() == 3);
        CHECK(msg.as_object().at("op").as_string() == "removed");
        CHECK(msg.as_object().at("type").as_string() == "competitor");
        CHECK(msg.as_object().at("item").as_object().size() == 2);
        CHECK(msg.as_object().at("item").as_object().at("id").as_string() == "c-1");
        CHECK(msg.as_object().at("item").as_object().at("comp_id").as_string() == "comp-1");

        ws_messages.clear();
    }

    {
        auto [ec, response] = client.del(base_url + "/rest/competition/comp-1/competition_class/cc-1");
        CHECK(ec == std::error_code {});
        CHECK(response.status() == boost::beast::http::status::ok);
        auto result = boost::json::parse(response.body());
        CHECK(result.is_object());
        CHECK(result.as_object().at("id").as_string() == "cc-1");
        CHECK(result.as_object().at("comp_id").as_string() == "comp-1");

        ws_messages.clear();
    }

    {
        auto [ec, response] = client.del(base_url + "/rest/competition/comp-1/start_group/sg-1");
        CHECK(ec == std::error_code {});
        CHECK(response.status() == boost::beast::http::status::ok);
        auto result = boost::json::parse(response.body());
        MESSAGE(result);

        CHECK(result.is_object());
        CHECK(result.as_object().size() == 2);
        CHECK(result.as_object().at("id").as_string() == "sg-1");
        CHECK(result.as_object().at("comp_id").as_string() == "comp-1");

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        CHECK(ws_messages.size() == 1);
        const auto msg = boost::json::parse(ws_messages.front());

        CHECK(msg.is_object());
        CHECK(msg.as_object().size() == 3);
        CHECK(msg.as_object().at("op").as_string() == "removed");
        CHECK(msg.as_object().at("type").as_string() == "start_group");
        CHECK(msg.as_object().at("item").as_object().size() == 2);
        CHECK(msg.as_object().at("item").as_object().at("id").as_string() == "sg-1");
        CHECK(msg.as_object().at("item").as_object().at("comp_id").as_string() == "comp-1");

        ws_messages.clear();        
    }

    std::this_thread::sleep_for(std::chrono::seconds(1));
}
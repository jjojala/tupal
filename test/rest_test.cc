#include "doctest/doctest.h"
#include "boost/process.hpp"
#include "boost/asio.hpp"
#include "boost/json.hpp"
#include "beauty/beauty.hpp"
#include <iostream>
#include <chrono>

namespace {
    const std::string base_url = "http://localhost:8085";

    /**{ declarations, @see definitions below */
    bool contains(const boost::json::value &, const boost::json::array &);
    bool contains(const boost::json::value &, const boost::json::value &);
    /**} */

    /** @brief returns true, if all fields of `obj` are contained by `item`. */
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

    /** @brief returns true, if `val` is included in array `items`. */
    bool contains(const boost::json::value & val, const boost::json::array & items) {
        for (const auto item: items)
            if (contains(val, item))
                return true;
        return false;
    }

    /** @brief returns true, if `val` is included in `item` */
    bool contains(const boost::json::value & val, const boost::json::value & item) {
        if (val.is_object())
            return contains(val.as_object(), item);

        else if (val.is_array()) {
            if (!item.is_array()) {
                MESSAGE("Item '", item, "' expected to be an array, but it is not");
                return false;
            }

            bool status = true;
            for (const auto v: val.as_array())
                if (!contains(v, item.as_array()))
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

boost::json::value to_json(const char * str) { return boost::json::parse(str); }
boost::json::value to_json(const std::string & str) { return boost::json::parse(str); }

std::string to_list(const char * str) { return boost::json::serialize(boost::json::array { to_json(str) }); }

std::string update_json(const char * str, void (*update)(boost::json::object & val)) {
    auto json = to_json(str).as_object();
    update(json);
    return boost::json::serialize(json);
}

#define TUPAL_EXPECT_NOTIFICATION(messages, initialCount) { \
    while (messages.size() <= initialCount) { \
        std::this_thread::sleep_for(std::chrono::milliseconds(50)); \
    } \
}

#define TUPAL_CHECK_STATUS(f, expectedStatus) { \
    auto [ec, response] = f(); \
    CHECK(ec == std::error_code {}); \
    CHECK(response.status() == expectedStatus); \
}

#define TUPAL_TEST_GET(listF, expected) { \
    auto [ec, response] = listF(); \
    CHECK(ec == std::error_code {}); \
    CHECK(response.status() == boost::beast::http::status::ok); \
    MESSAGE("\nExpeced: ", expected, "\nActual : ", to_json(response.body())); \
    CHECK(contains(to_json(expected), to_json(response.body()))); \
}

#define TUPAL_TEST_CREATE_NO_NOTIFICATION(createF, data) { \
    auto [ec, response] = createF(); \
    CHECK(ec == std::error_code {}); \
    CHECK(response.status() == boost::beast::http::status::created); \
    CHECK(contains(to_json(data), to_json(response.body()))); \
}

#define TUPAL_TEST_CREATE_WITH_NOTIFICATIONS(createF, type, data, messages) { \
    const auto msgCount = messages.size(); \
    TUPAL_TEST_CREATE_NO_NOTIFICATION(createF, data); \
    TUPAL_EXPECT_NOTIFICATION(messages, msgCount); \
    const auto expected_notification_body { std::string { R"({"op": "created", "type": ")" } + type + R"("})" }; \
    CHECK(contains(to_json(expected_notification_body), to_json(messages.back()))); \
    CHECK(contains(to_json(data), to_json(messages.back()).as_object()["item"])); \
}

#define TUPAL_TEST_UPDATE_NO_NOTIFICATION(updateF, data) { \
    auto [ec, response] = updateF(); \
    CHECK(ec == std::error_code {}); \
    CHECK(response.status() == boost::beast::http::status::ok); \
    CHECK(contains(to_json(data), to_json(response.body()))); \
}

#define TUPAL_TEST_UPDATE_WITH_NOTIFICATION(updateF, type, data, messages) { \
    const auto msgCount = messages.size(); \
    TUPAL_TEST_UPDATE_NO_NOTIFICATION(updateF, data); \
    TUPAL_EXPECT_NOTIFICATION(messages, msgCount); \
    const auto expected_notification_body { std::string { R"({"op": "updated", "type": ")" } + type + R"("})" }; \
    CHECK(contains(to_json(expected_notification_body), to_json(messages.back()))); \
    CHECK(contains(to_json(data), to_json(messages.back()).as_object()["item"])); \
}

#define TUPAL_TEST_REMOVE_NO_NOTIFICATION(deleteF, dataStr) { \
    auto [ec, response] = deleteF(); \
    CHECK(ec == std::error_code {}); \
    CHECK(response.status() == boost::beast::http::status::ok); \
}

#define TUPAL_TEST_REMOVE_WITH_NOTIFICATION(deleteF, typeStr, dataStr, messages) { \
    const auto msgCount = messages.size(); \
    TUPAL_TEST_REMOVE_NO_NOTIFICATION(deleteF, dataStr); \
    TUPAL_EXPECT_NOTIFICATION(messages, msgCount); \
    CHECK(contains(to_json(update_json(R"({ "op": "removed" })", [](boost::json::object & val) { \
        val["type"] = typeStr; \
        val["item"] = boost::json::object { { "id", dataStr } }; \
    })), to_json(messages.back()))); \
}

#define EXPECT_NOTIFICATION(messages) \
    while (messages.empty()) std::this_thread::sleep_for(std::chrono::milliseconds(50));

TEST_CASE_FIXTURE(daemon_fixture, "WS connect response") {
    const auto comp_data = R"({ "id": "comp-2", "title": "Competition 1", "date": "2025-11-11T00:00:00.000Z" })";
    const std::string comp_id { boost::json::parse(comp_data).as_object().at("id").as_string().c_str() };
    const auto comp_url = base_url + "/rest/competition/" + comp_id;

    const auto start_group_data { R"({ "id": "sg-1", "title": "Start group 1", "first_start_time": "2025-11-11T18:00:00.000Z",
            "first_bib": 1 })" };
    const auto comp_class_data { R"({ "id": "cc-1", "title": "Competition class 1", "start_group_id": "sg-1" })" };
    const auto competitor_data { R"({ "id": "c-1", "comp_class_id": "cc-1", "bib": 101,
            "start_time_offset": "PT600.000S", "finish_time": "2024-01-01T10:59:11.231Z", "status": 0, "name": "Alice" })" };

    TUPAL_TEST_CREATE_NO_NOTIFICATION([&]() { return client.post(base_url + "/rest/competition/", comp_data); }, comp_data);
    TUPAL_TEST_CREATE_NO_NOTIFICATION([&]() { return client.post(comp_url + "/start_group/", start_group_data); }, start_group_data);
    TUPAL_TEST_CREATE_NO_NOTIFICATION([&]() { return client.post(comp_url + "/competition_class/", comp_class_data); }, comp_class_data);
    TUPAL_TEST_CREATE_NO_NOTIFICATION([&]() { return client.post(comp_url + "/competitor/", competitor_data); }, competitor_data);

    ws_messages_type ws_messages;
    init_ws(comp_id, ws_messages);
    CHECK(ws_messages.size() == 1);

    const auto msg = boost::json::parse(ws_messages.front()).as_object();
    MESSAGE(msg);

    CHECK(contains(boost::json::parse(comp_data), msg));
    CHECK(contains(boost::json::parse(start_group_data), msg.at("start_groups").as_array().front()));
    CHECK(contains(boost::json::parse(comp_class_data), msg.at("classes").as_array().front()));
    CHECK(contains(boost::json::parse(competitor_data), msg.at("competitors").as_array().front()));
}

TEST_CASE_FIXTURE(ws_daemon_fixture, "Create, read, update and delete a competition") {
    TUPAL_TEST_GET([&]() { return client.get(base_url + "/rest/competition/"); }, "[]");

#if 0 // #44
    // Get non-existing competition (--> not_found)
    TUPAL_CHECK_STATUS([&]() { return client.get(base_url + "/rest/competition/comp-1"); }, boost::beast::http::status::not_found);
#endif

    const auto data = R"({ "id": "comp-1", "title": "Competition 1", "date": "2025-11-11T00:00:00.000Z" })";
    TUPAL_TEST_CREATE_NO_NOTIFICATION([&]() { return client.post(base_url + "/rest/competition/", data); }, data);
    TUPAL_TEST_GET([&]() { return client.get(base_url + "/rest/competition/"); }, to_list(data));
    TUPAL_TEST_GET([&]() { return client.get(base_url + "/rest/competition/comp-1"); }, std::string { data });

    const auto updated_data = update_json(data, [](boost::json::object & val) { val["title"] = "New competition 1"; });
    TUPAL_TEST_UPDATE_WITH_NOTIFICATION([&]() { 
        return client.put(base_url + "/rest/competition/comp-1", updated_data.c_str()); },
        "competition", updated_data.c_str(), ws_messages);

#if 0 // #45  
// responds with "ok" in case the url contains non-existent id, but the body of the mssage
// refers to existing id (this is basically "bad data"). However, if message body refers also to non-existent id,
// one get not_found as expected.
    TUPAL_CHECK_STATUS([&]() { return client.put(base_url + "/rest/competition/comp-not-fond", updated_data.c_str()); },
            boost::beast::http::status::not_found);
#endif

    // Updating non-existent competition (--> not_found)
    TUPAL_CHECK_STATUS([&]() {
            return client.put(base_url + "/rest/competition/comp-not-found",
                R"({
                    "id": "comp-not-found",
                    "title": "Competition 1",
                    "date": "2025-11-11T00:00:00.000Z"
                })"); }, boost::beast::http::status::not_found);

    // Removing existing competition
    TUPAL_TEST_REMOVE_WITH_NOTIFICATION([&]() { return client.del(base_url + "/rest/competition/comp-1"); },
            "competition", "comp-1", ws_messages);

    // Removing non-existent competition (--> not_found)
    TUPAL_CHECK_STATUS([&]() { return client.del(base_url + "/rest/competition/comp-non-existent"); },
                boost::beast::http::status::not_found);
}

TEST_CASE_FIXTURE(ws_daemon_fixture, "Create, read, update and delete a start group") {

    const auto comp_data = R"({ "id": "comp-1", "title": "Competition 1", "date": "2025-11-11T00:00:00.000Z" })";
    TUPAL_TEST_CREATE_NO_NOTIFICATION([&]() { return client.post(base_url + "/rest/competition/", comp_data); }, comp_data);

    const auto sg_url = base_url + "/rest/competition/comp-1/start_group/";
    TUPAL_TEST_GET([&]() { return client.get(sg_url); }, "[]");

#if 0 // #44
    TUPAL_CHECK_STATUS([&]() { return client.get(sg_url + "sg-1"); }, boost::beast::http::status::not_found);
#endif

    const auto data = R"({ "id": "sg-1", "title": "Start Group 1", "first_start_time": "2025-11-11T18:00:00.000Z", "first_bib": 1 })";
    TUPAL_TEST_CREATE_WITH_NOTIFICATIONS([&]() { return client.post(sg_url, data); }, "start_group", data, ws_messages);
    TUPAL_TEST_GET([&]() { return client.get(sg_url); }, to_list(data) );
    TUPAL_TEST_GET([&]() { return client.get(sg_url + "sg-1"); }, std::string { data });

    const auto updated_data = update_json(data, [](boost::json::object & val) { val["title"] = "New Start Group 1"; });
    TUPAL_TEST_UPDATE_WITH_NOTIFICATION([&]() { return client.put(sg_url + "sg-1", updated_data.c_str()); },
        "start_group", updated_data.c_str(), ws_messages);

#if 0 // #45  
// responds with "ok" in case the url contains non-existent id, but the body of the mssage
// refers to existing id (this is basically "bad data"). However, if message body refers also to non-existent id,
// one get not_found as expected.
    TUPAL_CHECK_STATUS([&]() { return client.put(sg_url + "sg-not-fond", updated_data.c_str()); },
            boost::beast::http::status::not_found);
#endif

    // Updating non-existent competition (--> not_found)
    TUPAL_CHECK_STATUS([&]() { return client.put(sg_url + "sg-not-found",
            R"({ "id": "sg-not-found", "title": "New Start Group 1",
                    "first_start_time": "2025-11-11T18:00:00.000Z", "first_bib": 1 })"); },
        boost::beast::http::status::not_found);

    // Removing existing competition
    TUPAL_TEST_REMOVE_WITH_NOTIFICATION([&]() { return client.del(sg_url + "sg-1"); },
            "start_group", "sg-1", ws_messages);

    // Removing non-existent competition (--> not_found)
    TUPAL_CHECK_STATUS([&]() { return client.del(sg_url + "sg-non-existent"); },
                boost::beast::http::status::not_found);
}

TEST_CASE_FIXTURE(ws_daemon_fixture, "Create, read, update and delete a competition class") {

    const auto comp_data = R"({ "id": "comp-1", "title": "Competition 1", "date": "2025-11-11T00:00:00.000Z" })";
    TUPAL_TEST_CREATE_NO_NOTIFICATION([&]() { return client.post(base_url + "/rest/competition/", comp_data); }, comp_data);

    const auto sg_data = R"({ "id": "sg-1", "title": "Start Group 1", "first_start_time": "2025-11-11T18:00:00.000Z", "first_bib": 1 })";
    TUPAL_TEST_CREATE_WITH_NOTIFICATIONS([&]() { return client.post(base_url + "/rest/competition/comp-1/start_group/", sg_data); },
            "start_group", sg_data, ws_messages);


    const auto cc_url = base_url + "/rest/competition/comp-1/competition_class/";
    TUPAL_TEST_GET([&]() { return client.get(cc_url); }, "[]");

#if 0 // #44
    TUPAL_CHECK_STATUS([&]() { return client.get(cc_url + "cc-not_found"); }, boost::beast::http::status::not_found);
#endif

    const auto data = R"({ "id": "cc-1", "title": "Competition class 1", "start_group_id": "sg-1" })";
    TUPAL_TEST_CREATE_WITH_NOTIFICATIONS([&]() { return client.post(cc_url, data); }, "competition_class", data, ws_messages);
    TUPAL_TEST_GET([&]() { return client.get(cc_url); }, to_list(data) );
    TUPAL_TEST_GET([&]() { return client.get(cc_url + "cc-1"); }, std::string { data });

    const auto updated_data = update_json(data, [](boost::json::object & val) { val["title"] = "New Competition Class 1"; });
    TUPAL_TEST_UPDATE_WITH_NOTIFICATION([&]() { return client.put(cc_url + "cc-1", updated_data.c_str()); },
        "competition_class", updated_data.c_str(), ws_messages);

#if 0 // #45
// responds with "ok" in case the url contains non-existent id, but the body of the mssage
// refers to existing id (this is basically "bad data"). However, if message body refers also to non-existent id,
// one get not_found as expected.
    TUPAL_CHECK_STATUS([&]() { return client.put(cc_url + "cc-not_fond", updated_data.c_str()); },
            boost::beast::http::status::not_found);
#endif

    // Updating non-existent competition (--> not_found)
    TUPAL_CHECK_STATUS([&]() { return client.put(cc_url + "sg-not_found",
            R"({ "id": "sg-not_found", "title": "New Competition class 1", "start_group_id": "sg-1" })"); },
        boost::beast::http::status::not_found);

    // Removing existing competition
    TUPAL_TEST_REMOVE_WITH_NOTIFICATION([&]() { return client.del(cc_url + "cc-1"); },
            "competition_class", "cc-1", ws_messages);

    // Removing non-existent competition (--> not_found)
    TUPAL_CHECK_STATUS([&]() { return client.del(cc_url + "sg-not_found"); },
                boost::beast::http::status::not_found);
}

TEST_CASE_FIXTURE(ws_daemon_fixture, "rest tests (smoke)") {

#if 0
    CHECK(ws_messages.size() == 1);
    CHECK(ws_messages.front() == R"({})");
    ws_messages.clear();
#endif
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
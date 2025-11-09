#include <string>
#include <memory>
#include <sstream>
#include <system_error>

#include <beauty/beauty.hpp>
#include <boost/json.hpp>
#include <cxxopts.hpp>   // added include

#include "manager.hh"
#include "json_helper.hh"

namespace {

	typedef std::unordered_map<std::string, std::weak_ptr<beauty::websocket_session>> sessions_type;

	template <typename T>
	void and_then(std::optional<T> optional, std::function<void(const T&)> f) {
		if (optional)
			return f(*optional);
	}

	std::string param(const beauty::request & req, const char * name) {
		return req.a(name).as_string();
	}

	boost::json::object message(const char * const op, const char * const type, const boost::json::value & data) {
		return { {"op", op}, {"type", type}, {"item", boost::json::value(data) } };
	}

	void notify(const sessions_type & sessions, const boost::json::value & data) {
		const auto message = boost::json::serialize(data);
		for (const auto session: sessions) {
			if (const auto s = session.second.lock()) {
				s->send(std::string { message });
			}
		}
	}

	std::function<void(const boost::json::value &)> make_notifier(const sessions_type & sessions, const char * op, const char * type) {
		return [&sessions,op,type](const boost::json::value & data) { 
			notify(sessions, message(op, type, data)); };
	}

	void handle_list(const tupal::result_type result, beauty::response & resp) {
		if (tupal::ec(result)) {
			resp.result(boost::beast::http::status::internal_server_error);
		} else {
			resp.result(boost::beast::http::status::ok);
			resp.body() = boost::json::serialize(tupal::json(result));
		}
	}

	void handle_get(const tupal::result_type result, beauty::response & resp) {
		if (tupal::ec(result)) {
			resp.result(boost::beast::http::status::internal_server_error);
		} else if (tupal::json(result).is_null()) {
			resp.result(boost::beast::http::status::not_found);
		} else {
			resp.result(boost::beast::http::status::ok);
			resp.body() = boost::json::serialize(tupal::json(result));
		}
	}

	std::optional<boost::json::value> handle_create(const tupal::result_type result, beauty::response & resp) {
		if (tupal::ec(result)) {
			if (tupal::ec(result) == tupal::make_error_condition(tupal::error_code::duplicate_key))
				resp.result(boost::beast::http::status::conflict);
			else
				resp.result(boost::beast::http::status::internal_server_error);
		} else {
			resp.result(boost::beast::http::status::created);
			resp.body() = boost::json::serialize(tupal::json(result));
			return tupal::json(result);
		}
		return std::nullopt;
	}

	std::optional<boost::json::value> handle_update(const tupal::result_type result, beauty::response & resp) {
		if (tupal::ec(result)) {
			if (tupal::ec(result) == tupal::make_error_condition(tupal::error_code::unknown_key))
				resp.result(boost::beast::http::status::not_found);
			else
				resp.result(boost::beast::http::status::internal_server_error);
		} else {
			resp.result(boost::beast::http::status::ok);
			resp.body() = boost::json::serialize(tupal::json(result));
			return tupal::json(result);
		}
		return std::nullopt;
	}

	std::optional<bool> handle_remove(const std::error_code ec, beauty::response & resp) {
		if (ec) {
			if (ec == tupal::make_error_condition(tupal::error_code::unknown_key))
				resp.result(boost::beast::http::status::not_found);
			else
				resp.result(boost::beast::http::status::internal_server_error);
		} else {
			resp.result(boost::beast::http::status::ok);
			return true;
		}
		return std::nullopt;
	}

	void enable_swagger(beauty::server & server) {
		server.enable_swagger("/rest/swagger");
		server.get("/rest/swagger/ui", [](const auto& req, auto& res) {
			res.body() = R"(
				<!DOCTYPE html>
				<html lang="en">
				<head>
					<meta charset="UTF-8">
					<meta name="viewport" content="width=device-width, initial-scale=1.0">
					<title>Swagger UI</title>
					<link rel="stylesheet" type="text/css" href="https://cdnjs.cloudflare.com/ajax/libs/swagger-ui/4.1.3/swagger-ui.css">
				</head>
				<body>
					<div id="swagger-ui"></div>
					<script src="https://cdnjs.cloudflare.com/ajax/libs/swagger-ui/4.1.3/swagger-ui-bundle.js"></script>
					<script>
					const ui = SwaggerUIBundle({
						url: '/rest/swagger',
						dom_id: '#swagger-ui',
					});
					</script>
				</body>
				</html>
			)";
			res.set(beauty::http::field::content_type, "text/html");
		});
    }
}

int main(int argc, char** argv) {

	beauty::server server;
	enable_swagger(server);

    cxxopts::Options opts("tupald", "Backend daemon for tupal results management system");
    opts.add_options()
      ("d,db", "Database URL", cxxopts::value<std::string>()->default_value("sqlite3://:memory:"))
      ("h,help", "This usage");

    auto parsed = opts.parse(argc, argv);
    if (parsed.count("help")) {
        std::cout << opts.help() << '\n';
        return 0;
    }

    std::shared_ptr<tupal::CompetitionManager> manager =
        tupal::CompetitionManager::new_competition_manager(parsed["db"].as<std::string>());

    sessions_type sessions;
	
	server.add_route("/").get([](const auto & req, auto & res) { res.body() = "The app...\n"; });

	server.add_route("/ws")
		.ws(beauty::ws_handler {
			.on_connect = [&sessions](const beauty::ws_context & ctx) { sessions[ctx.uuid] = ctx.ws_session; },
			.on_receive = [](const beauty::ws_context & /* ctx */, const char * /* data */, std::size_t /* size*/, bool /* is_text */) {},
			.on_disconnect = [&sessions](const beauty::ws_context & ctx) { sessions.erase(ctx.uuid); },
			.on_error =  [](const boost::system::error_code /* ec */, const char* /* what */) {}
		});

	server.add_route("/rest/competition/")
		.get([&](const auto & req, beauty::response & res) { handle_list(manager->list(), res); })
		.post([&](const beauty::request & req, beauty::response & res) {
			and_then(handle_create(manager->create(boost::json::parse(req.body())), res), 
				make_notifier(sessions, "created", "competition"));	});
	server.add_route("/rest/competition/:competition_id")
		.get([&](const auto & req, auto & res) { handle_get(manager->get(param(req, "competition_id")), res); })
		.put([&](const auto & req, auto & res) {
			and_then(handle_update(manager->update(boost::json::parse(req.body())), res),
				make_notifier(sessions, "updated", "competition")); })
		.del([&](const auto & req, auto & res) { 
			const auto competition_id = param(req, "competition_id");
			and_then(handle_remove(manager->remove(competition_id), res),
				std::function<void(const bool &)> { [&](const bool &) {
					notify(sessions, message("deleted", "competition", { {"id", competition_id} })); }
				});
			});

	server.add_route("/rest/competition/:competition_id/competitor/")
		.get([&](const auto & req, auto & res) {
			handle_list(manager->getCompetitorManager()->list(param(req, "competition_id")), res); })
		.post([&](const auto & req, auto & res) {
			and_then(handle_create(manager->getCompetitorManager()->create(param(req, "competition_id"), boost::json::parse(req.body())), res),
				make_notifier(sessions, "created", "competitor")); });
	server.add_route("/rest/competition/:competition_id/competitor/:competitor_id")
		.get([&](const auto & req, auto & res) { 
			handle_get(manager->getCompetitorManager()->get(param(req, "competition_id"), param(req, "competitor_id")), res); })
		.put([&](const auto & req, auto & res) {
			and_then(handle_update(manager->getCompetitorManager()->update(param(req, "competition_id"), boost::json::parse(req.body())), res),
				make_notifier(sessions, "updated", "competitor")); })
		.del([&](const auto & req, auto & res) {
			const auto competition_id = param(req, "competition_id");
			const auto competitor_id = param(req, "competitor_id");
			and_then(handle_remove(manager->getCompetitorManager()->remove(competition_id, competitor_id), res),
				std::function<void(const bool &)> { [&](const bool &) { 
					notify(sessions, message("deleted", "competitor", { {"id", competitor_id}, { "comp_id", competition_id } })); }
				});
			});

	server.add_route("/rest/competition/:competition_id/competition_class/")
		.get([&](const auto & req, auto & res) {
			handle_list(manager->getCompetitionClassManager()->list(param(req, "competition_id")), res); })
		.post([&](const auto & req, auto & res) {
			and_then(handle_create(manager->getCompetitionClassManager()->create(param(req, "competition_id"), boost::json::parse(req.body())), res),
				make_notifier(sessions, "created", "competition_class")); });
	server.add_route("/rest/competition/:competition_id/competition_class/:competition_class_id")
		.get([&](const auto & req, auto & res) {
			handle_get(manager->getCompetitionClassManager()->get(param(req, "competition_id"), param(req, "competition_class_id")), res); })
		.put([&](const auto & req, auto & res) {
			and_then(handle_update(manager->getCompetitionClassManager()->update(param(req, "competition_id"), boost::json::parse(req.body())), res),
				make_notifier(sessions, "updated", "competition_class")); })
		.del([&](const auto & req, auto & res) {
			const auto competition_id = param(req, "competition_id");
			const auto competition_class_id = param(req, "competition_class_id");
			and_then(handle_remove(manager->getCompetitionClassManager()->remove(competition_id, competition_class_id), res),
				std::function<void(const bool &)> { [&](const bool &) {
					notify(sessions, message("deleted", "competition_class", { {"id", competition_class_id}, { "comp_id", competition_id } })); }
				});
			handle_remove(manager->getCompetitionClassManager()->remove(param(req, "competition_id"), param(req, "competition_class_id")), res); });

	server.add_route("/rest/competition/:competition_id/start_group/")
		.get([&](const auto & req, auto & res) { handle_list(manager->getStartGroupManager()->list(param(req, "competition_id")), res); })
		.post([&](const auto & req, auto & res) {
			and_then(handle_create(manager->getStartGroupManager()->create(param(req, "competition_id"), boost::json::parse(req.body())), res),
				make_notifier(sessions, "created", "start_group")); });
	server.add_route("/rest/competition/:competition_id/start_group/:start_group_id")
		.get([&](const auto & req, auto & res) {
			handle_get(manager->getStartGroupManager()->get(param(req, "competition_id"), param(req, "start_group_id")), res); })
		.put([&](const auto & req, auto & res) {
			and_then(handle_update(manager->getStartGroupManager()->update(param(req, "competition_id"), boost::json::parse(req.body())), res),
				make_notifier(sessions, "updated", "start_group")); })
		.del([&](const auto & req, auto & res) {
			const auto competition_id = param(req, "competition_id");
			const auto start_group_id = param(req, "start_group_id");
			and_then(handle_remove(manager->getStartGroupManager()->remove(competition_id, start_group_id), res),
				std::function<void(const bool &)> { [&](const bool &) {
					notify(sessions, message("deleted", "start_group", { {"id", start_group_id}, { "comp_id", competition_id } })); }
				});	
			});

	server.listen(8085);

	server.wait();
}
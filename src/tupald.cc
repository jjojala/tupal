#include <string>
#include <memory>
#include <sstream>
#include <fstream>
#include <system_error>

#include <beauty/beauty.hpp>
#include <boost/json.hpp>
#include <cxxopts.hpp>

#include "manager.hh"

namespace {

	class sessions {
	private:
		struct session_info {
			std::weak_ptr<beauty::websocket_session> ws_session;
			std::string competition_id;
		};

		std::unordered_map<std::string, session_info> session_list;

	public:
		void register_client(const std::string & uuid, 
				const std::string & competition_id, std::shared_ptr<beauty::websocket_session> ws_session) {
			session_list[uuid] = session_info { ws_session, competition_id };
		}

		bool unregister_client(const std::string & uuid) {
			return session_list.erase(uuid) > 0;
		}

		std::size_t notify_competition(const std::string & competition_id, const std::string & message) const {
			std::size_t count = 0;
			std::size_t session_count = session_list.size();
			for (const auto & session_pair: session_list) {
				const auto & session_info = session_pair.second;
				if (session_info.competition_id == competition_id) {
					if (const auto ws_session = session_info.ws_session.lock()) {
						ws_session->send(std::string { message });
						++count;
					}
				}
			}
			return count;
		}
	};

	template <typename T>
	void and_then(std::optional<T> optional, std::function<void(const T&)> f) {
		if (optional)
			return f(*optional);
	}

	std::string param(const beauty::request & req, const char * name) {
		return req.a(name).as_string();
	}

	boost::json::object notification(const char * const op, const char * const type, const boost::json::value & data) {
		return { {"op", op}, {"type", type}, {"item", boost::json::value(data) } };
	}

	std::function<void(const boost::json::value &)> make_notifier(const sessions & sessions, const std::string & comp_id, 
			const char * op, const char * type) {
		return [&sessions,&comp_id,op,type](const boost::json::value & data) { 
			sessions.notify_competition(comp_id, boost::json::serialize(notification(op, type, data))); };
	}

	boost::beast::http::status to_status(const std::error_code & ec) {
		if (ec == std::error_code {} || ec == tupal::make_error_condition(tupal::error_code::ok))
			return boost::beast::http::status::ok;
		if (ec == tupal::make_error_condition(tupal::error_code::unknown_key))
			return boost::beast::http::status::not_found;
		if (ec == tupal::make_error_condition(tupal::error_code::duplicate_key) 
				|| ec == tupal::make_error_condition(tupal::error_code::constraint_violation))
			return boost::beast::http::status::conflict;
		if (ec == tupal::make_error_condition(tupal::error_code::invalid_arguments))
			return boost::beast::http::status::bad_request;

		return boost::beast::http::status::internal_server_error;
	}

	void handle_list(const tupal::result_type result, beauty::response & resp) {
		resp.result(to_status(tupal::ec(result)));
		resp.body() = boost::json::serialize(tupal::json(result));
		resp.set_header(boost::beast::http::field::content_type, "application/json");
	}

	void handle_get(const tupal::result_type result, beauty::response & resp) {
		resp.result(to_status(tupal::ec(result)));
		resp.body() = boost::json::serialize(tupal::json(result));
		resp.set_header(boost::beast::http::field::content_type, "application/json");
	}

	std::optional<boost::json::value> handle_create(const tupal::result_type result, beauty::response & resp) {
		resp.body() = boost::json::serialize(tupal::json(result));
		resp.set_header(boost::beast::http::field::content_type, "application/json");
		if (!tupal::ec(result)) {
			resp.result(boost::beast::http::status::created);
			return tupal::json(result);
		}
		resp.result(to_status(tupal::ec(result)));
		return std::nullopt;
	}

	std::optional<boost::json::value> handle_update(const tupal::result_type result, beauty::response & resp) {
		resp.body() = boost::json::serialize(tupal::json(result));
		resp.set_header(boost::beast::http::field::content_type, "application/json");
		resp.result(to_status(tupal::ec(result)));
		if (!tupal::ec(result))
			return tupal::json(result);
		return std::nullopt;
	}

	std::optional<boost::json::value> handle_remove(const tupal::result_type result, beauty::response & resp) {
		resp.body() = boost::json::serialize(tupal::json(result));
		resp.set_header(boost::beast::http::field::content_type, "application/json");
		resp.result(to_status(tupal::ec(result)));
		if (!tupal::ec(result))
			return tupal::json(result);
		return std::nullopt;
	}

	boost::json::array to_array(const tupal::result_type result) {
		if (tupal::ec(result)) {
			return {};
		} else {
			return tupal::json(result).as_array();
		}
	}

	boost::json::object get_competition_data(tupal::CompetitionManager & manager, const std::string & competition_id) {
		const auto [ec, competition] = manager.get(competition_id);
		if (!ec) {
			auto competition_data = competition.as_object();
			competition_data["start_groups"] = to_array(manager.getStartGroupManager()->list(competition_id));
			competition_data["competition_classes"] = to_array(manager.getCompetitionClassManager()->list(competition_id));
			competition_data["competitors"] = to_array(manager.getCompetitorManager()->list(competition_id));
			return competition_data;
		}

		return {};
	}
}

int main(int argc, char** argv) {

	try {
		beauty::server server;

		cxxopts::Options opts("tupald", "Backend daemon for tupal results management system");
		opts.add_options()
		("d,db", "Database URL", cxxopts::value<std::string>()->default_value("sqlite3://:memory:"))
		("r,web-root", "Root directory for static web content", cxxopts::value<std::string>()->default_value("./web"))
		("b,bind", "Bind url for the daemon", cxxopts::value<std::string>()->default_value("http://0.0.0.0:8085"))
		("S,swagger", "Enable swagger")
		("h,help", "This usage");

		auto parsed = opts.parse(argc, argv);
		if (parsed.count("help")) {
			std::cout << opts.help() << '\n';
			return 0;
		}

		if (parsed.count("swagger")) {
			server.enable_swagger("/rest/swagger");
		}

		std::shared_ptr<tupal::CompetitionManager> manager =
			tupal::CompetitionManager::new_competition_manager(parsed["db"].as<std::string>());

		sessions sessions;

		server.add_route("/ws/:competition_id")
			.ws(beauty::ws_handler {
				.on_connect = [&sessions, &manager](const beauty::ws_context & ctx) {
					const auto competition_id = ctx.attributes.find("competition_id");
					if (competition_id != ctx.attributes.end()) {
						sessions.register_client(ctx.uuid, competition_id->second.as_string(), ctx.ws_session.lock());

						ctx.ws_session.lock()->send(boost::json::serialize(
							get_competition_data(*manager, competition_id->second.as_string())));
					}
				},
				.on_disconnect = [&sessions](const beauty::ws_context & ctx) {
					sessions.unregister_client(ctx.uuid);
				},
				.on_error =  [](const boost::system::error_code /* ec */, const char* /* what */) {}
			});

		server.add_route("/rest/competition/")
			.get([&](const auto & req, beauty::response & res) { handle_list(manager->list(), res); })
			.post([&](const beauty::request & req, beauty::response & res) {
				handle_create(manager->create(boost::json::parse(req.body()).as_object()), res); });

		server.add_route("/rest/competition/:competition_id")
			.get([&](const auto & req, auto & res) { handle_get(manager->get(param(req, "competition_id")), res); })
			.put([&](const auto & req, auto & res) {
				and_then(handle_update(manager->update(boost::json::parse(req.body()).as_object()), res),
					make_notifier(sessions, param(req, "competition_id"), "updated", "competition")); })
			.del([&](const auto & req, auto & res) {
				and_then(handle_remove(manager->remove(param(req, "competition_id")), res),
					make_notifier(sessions, param(req, "competition_id"), "removed", "competition"));
			});

		server.add_route("/rest/competition/:competition_id/competitor/")
			.get([&](const auto & req, auto & res) {
				handle_list(manager->getCompetitorManager()->list(param(req, "competition_id")), res); })
			.post([&](const auto & req, auto & res) {
				and_then(handle_create(manager->getCompetitorManager()->create(
						param(req, "competition_id"), boost::json::parse(req.body()).as_object()), res),
					make_notifier(sessions, param(req, "competition_id"), "created", "competitor")); });

		server.add_route("/rest/competition/:competition_id/competitor/:competitor_id")
			.get([&](const auto & req, auto & res) {
				handle_get(manager->getCompetitorManager()->get(
					param(req, "competition_id"), param(req, "competitor_id")), res); })
			.put([&](const auto & req, auto & res) {
				and_then(handle_update(manager->getCompetitorManager()->update(
						param(req, "competition_id"), boost::json::parse(req.body()).as_object()), res),
					make_notifier(sessions, param(req, "competition_id"), "updated", "competitor")); })
			.del([&](const auto & req, auto & res) {
				and_then(handle_remove(manager->getCompetitorManager()->remove(
						param(req, "competition_id"), param(req, "competitor_id")), res),
					make_notifier(sessions, param(req, "competition_id"), "removed", "competitor"));
				});

		server.add_route("/rest/competition/:competition_id/competition_class/")
			.get([&](const auto & req, auto & res) {
				handle_list(manager->getCompetitionClassManager()->list(param(req, "competition_id")), res); })
			.post([&](const auto & req, auto & res) {
				and_then(handle_create(manager->getCompetitionClassManager()->create(
						param(req, "competition_id"), boost::json::parse(req.body()).as_object()), res),
					make_notifier(sessions, param(req, "competition_id"), "created", "competition_class")); });

		server.add_route("/rest/competition/:competition_id/competition_class/:competition_class_id")
			.get([&](const auto & req, auto & res) {
				handle_get(manager->getCompetitionClassManager()->get(
					param(req, "competition_id"), param(req, "competition_class_id")), res); })
			.put([&](const auto & req, auto & res) {
				and_then(handle_update(manager->getCompetitionClassManager()->update(
					param(req, "competition_id"), boost::json::parse(req.body()).as_object()), res),
					make_notifier(sessions, param(req, "competition_id"), "updated", "competition_class")); })
			.del([&](const auto & req, auto & res) {
				and_then(handle_remove(manager->getCompetitionClassManager()->remove(
					param(req, "competition_id"), param(req, "competition_class_id")), res),
					make_notifier(sessions, param(req, "competition_id"), "removed", "competition_class"));
				});

		server.add_route("/rest/competition/:competition_id/start_group/")
			.get([&](const auto & req, auto & res) { handle_list(manager->getStartGroupManager()->list(
				param(req, "competition_id")), res); })
			.post([&](const auto & req, auto & res) {
				and_then(handle_create(manager->getStartGroupManager()->create(
					param(req, "competition_id"), boost::json::parse(req.body()).as_object()), res),
					make_notifier(sessions, param(req, "competition_id"), "created", "start_group")); 
			});

		server.add_route("/rest/competition/:competition_id/start_group/:start_group_id")
			.get([&](const auto & req, auto & res) { handle_get(manager->getStartGroupManager()->get(
				param(req, "competition_id"), param(req, "start_group_id")), res); })
			.put([&](const auto & req, auto & res) {
				and_then(handle_update(manager->getStartGroupManager()->update(
					param(req, "competition_id"), boost::json::parse(req.body()).as_object()), res),
					make_notifier(sessions, param(req, "competition_id"), "updated", "start_group"));
			})
			.del([&](const auto & req, auto & res) {
				and_then(handle_remove(manager->getStartGroupManager()->remove(
					param(req, "competition_id"), param(req, "start_group_id")), res),
					make_notifier(sessions, param(req, "competition_id"), "removed", "start_group"));
				});

		auto handle_get_file = [&](const beauty::request & req, beauty::response & resp) {
			const auto filename = parsed["web-root"].as<std::string>()
				+ std::string { req.target().data(), req.target().size() };

			std::ifstream file{filename, std::ios_base::in};
			if (file) {
				resp.body() = std::string { std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };
			} else {
				std::cerr << "Opening file '" << filename << "' failed: " << strerror(errno) << std::endl;
				resp.result(boost::beast::http::status::not_found);
			}
		};

		server.add_route("/:file").get(handle_get_file);
		server.add_route("/:dir2/:file").get(handle_get_file);
		server.add_route("/:dir1/:dir2/:file").get(handle_get_file);

		const beauty::url bind_url { parsed["bind"].as<std::string>() };
		server.listen(bind_url.port(), bind_url.host());

		server.wait();
		std::exit(0);
	}

	catch (const std::exception & e) {
		std::cerr << "tupald failure: " << e.what() << std::endl;
		std::exit(1);
	}
}

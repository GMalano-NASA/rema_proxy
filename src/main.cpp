#include <memory>
#include <string>
#include <cstdlib>
#include <sstream>
#include <fstream>
#include <restbed>
#include <streambuf>
#include <functional>
#include <filesystem>
#include <map>
#include <exception>
#include <chrono>
#include <thread>
#include <iostream>
#include <spdlog/spdlog.h>
#include <boost/asio.hpp>

#include "syslogger.hpp"
#include "json.hpp"
#include "csv.h"
#include "session.hpp"
#include "websocket-server.hpp"
#include "rema.hpp"
#include "restfull_api.hpp"
#include "upload.hpp"


Session current_session;

std::vector<std::shared_ptr<restbed::Session>> sse_sessions;

using namespace std::chrono_literals;

std::map<std::string, std::string> mime_types = { { ".jpg", "image/jpg" }, {
        ".png", "image/png" }, { "svg", "image/svg+xml" },
        { ".css", "text/css" }, { ".js", "text/javascript" }, { ".ico",
                "image/x-icon" } };

void register_event_source_handler(const std::shared_ptr<restbed::Session> &session) {
    const auto headers = std::multimap<std::string, std::string> {
            { "Connection", "keep-alive" }, { "Cache-Control", "no-cache" }, {
                    "Content-Type", "text/event-stream" }, {
                    "Access-Control-Allow-Origin", "*" } //Only required for demo purposes.
    };

    session->yield(restbed::OK, headers, [](const std::shared_ptr<restbed::Session> &rest_session_ptr) {
        sse_sessions.push_back(rest_session_ptr);
    });
}

void event_stream_handler() {
    static auto prev = std::chrono::high_resolution_clock::from_time_t(0);

    REMA &rema_instance = REMA::get_instance();
    static bool hide_sent = false;
    nlohmann::json res;

    try {
        rema_instance.telemetry_client.receive_async([&rema_instance](auto &rx_buffer) {rema_instance.update_telemetry(rx_buffer); });
        struct telemetry ui_telemetry = rema_instance.telemetry;
        Tool tool = rema_instance.get_selected_tool();
        ui_telemetry.coords = current_session.from_rema_to_ui(rema_instance.telemetry.coords, &tool);
        res["TELEMETRY"] = ui_telemetry;

        auto now = std::chrono::high_resolution_clock::now();
        auto elapsed_time = now - prev;
        if (elapsed_time > std::chrono::milliseconds(500)) {
            res["TEMP_INFO"] = rema_instance.temps;
        }
    } catch (std::exception &e) {        
        SPDLOG_ERROR("Telemetry connection lost... {}", e.what());
    }

    if (!rema_instance.command_client.isConnected || !rema_instance.telemetry_client.isConnected) {
        res["SHOW_CONNECT"] = true;
        hide_sent = false;
    } else {
        if (!hide_sent) {
            res["HIDE_CONNECT"] = true;
            hide_sent = true;
        }
    }

    if (current_session.is_loaded() && current_session.is_changed()) {
        current_session.save_to_disk();
        current_session.set_changed(false);
        res["SESSION_MSG"] = "Session Saved";
    }

    if (!res.empty()) {
        sse_sessions.erase(
                std::remove_if(sse_sessions.begin(), sse_sessions.end(),
                        [](const std::shared_ptr<restbed::Session> &rest_session_ptr) {
                            return rest_session_ptr->is_closed();
                        }),
                sse_sessions.end());

        const auto message = "data: " + nlohmann::to_string(res) + "\n\n";
        for (const auto &session : sse_sessions) {
            session->yield(message);
        }
    }
}

void get_HXs_method_handler(const std::shared_ptr<restbed::Session> &session) {
    if (current_session.is_loaded()) {
        const std::string body = current_session.hx.tubesheet_svg;
        std::string content_type = "image/svg+xml"; 

        const std::multimap<std::string, std::string> headers {
                { "Content-Type", content_type }, { "Content-Length",
                        std::to_string(body.length()) } };

        session->close(restbed::OK, body, headers);
    } else {
        session->close(restbed::NOT_FOUND);
    }
}

void get_method_handler(const std::shared_ptr<restbed::Session> &session) {
    const auto request = session->get_request();

    const std::string filename = request->get_path_parameter("filename");

    std::filesystem::path file_path { "./wwwroot/"
            + request->get_path().substr(std::string("/static/").length()) };

    std::ifstream stream(file_path, std::ifstream::in);

    if (stream.is_open()) {
        const std::string body = std::string(std::istreambuf_iterator<char>(stream),
                std::istreambuf_iterator<char>());

        std::string content_type;
        std::string ext = file_path.filename().extension();
        if (auto elem = mime_types.find(ext); elem != mime_types.end()) {
            content_type = (*elem).second;
        } else {
            content_type = "text/html";
        }

        const std::multimap<std::string, std::string> headers {
                { "Content-Type", content_type }, { "Content-Length",
                        std::to_string(body.length()) } };

        session->close(restbed::OK, body, headers);
    } else {
        session->close(restbed::NOT_FOUND);
    }
}

void post_rtu_method_handler(const std::shared_ptr<restbed::Session> &session,
        REMA &rema) {
    const auto request = session->get_request();

    size_t content_length = request->get_header("Content-Length", 0);

    session->fetch(content_length,
            [&](const std::shared_ptr<restbed::Session> &rest_session_ptr,
                    const restbed::Bytes &body) {

                std::string tx_buffer(body.begin(), body.end());

                boost::asio::streambuf rx_buffer;
                try {
                    rema.command_client.send_blocking(tx_buffer);

                    std::string stream = rema.command_client.receive_blocking();
                    if (!stream.empty()) {
                        //stream.pop_back(); // Erase null character at the end of stream response

                        rest_session_ptr->close(restbed::OK, stream, { { "Content-Length",
                                std::to_string(stream.length()) }, {
                                "Content-Type",
                                "application/json; charset=utf-8" } });

                    }
                } catch (std::exception &e) {
                    std::string message = e.what();
                    SPDLOG_ERROR("COMMUNICATIONS ERROR {}", e.what());
                    rest_session_ptr->close(restbed::OK, message, { { "Content-Length",
                            std::to_string(message.length()) }, { "Content-Type",
                            "application/json; charset=utf-8" },
                            { "Cache-Control", "no-store" }
                    });
                }
            });
}

void failed_filter_validation_handler(
        const std::shared_ptr<restbed::Session> &session) {
    const auto request = session->get_request();
    auto headers = request->get_headers();
    SPDLOG_WARN("Invalid: ");
    for (auto header : headers) {
        SPDLOG_WARN("{} - {}", header.first, header.second);
    }

    session->close(400);
}

int main(const int, const char**) {
    spdlog::set_pattern("[%H:%M:%S %z] [%^%L%$] [%g:%#] [thread %t] %v");

    uint16_t rema_proxy_port = 4321;

    REMA &rema_instance = REMA::get_instance();
    rema_proxy_port = static_cast<uint16_t>(rema_instance.config["REMA_PROXY"].value("port", 4321));
    std::string rtu_host = rema_instance.config["REMA"]["network"].value("ip", "192.168.2.20");
    std::string rtu_service = std::to_string(rema_instance.config["REMA"]["network"].value("port", 5020));
    SPDLOG_INFO("REMA Proxy Server running on {}", rema_proxy_port);   

    rema_instance.connect(rtu_host, rtu_service);

    auto resource_rtu = std::make_shared<restbed::Resource>();
    resource_rtu->set_path("/REMA");
    resource_rtu->set_failed_filter_validation_handler(
            failed_filter_validation_handler);
    resource_rtu->set_method_handler("POST", [&rema_instance](const std::shared_ptr<restbed::Session> &session) {
        post_rtu_method_handler(session, rema_instance);
    });

    auto resource_html_file = std::make_shared<restbed::Resource>();

    resource_html_file->set_paths( {
            "/static/{filename: ^.+\\.(html|jpg|png|svg|ico)$}",
            "/static/css/{filename: ^.+\\.(css)$}",
            "/static/js/{filename: ^.+\\.(js)$}",
            "/static/css/images/{filename: ^.+\\.(jpg|png)$}",
            "/static/images/{filename: ^.+\\.(jpg|png)$}", });

    resource_html_file->set_method_handler("GET", get_method_handler);

    auto resource_HXs = std::make_shared<restbed::Resource>();
    resource_HXs->set_path("/HXs_svg");
    resource_HXs->set_failed_filter_validation_handler(
            failed_filter_validation_handler);
    resource_HXs->set_method_handler("GET", get_HXs_method_handler);

    auto settings = std::make_shared<restbed::Settings>();
    settings->set_port(rema_proxy_port);
    //settings->set_default_header("Connection", "close");

    settings->set_default_headers({
            { "Connection", "keep-alive" },
            { "Cache-Control", "no-store" },
            { "Access-Control-Allow-Origin", "*" } //Only required for demo purposes.
    });

    settings->set_worker_limit(std::thread::hardware_concurrency());

    auto resource_server_side_events = std::make_shared<restbed::Resource>();
    resource_server_side_events->set_path("/sse");
    resource_server_side_events->set_method_handler("GET",
            register_event_source_handler);

    restbed::Service service;
    service.publish(resource_rtu);
    service.publish(resource_HXs);
    service.publish(resource_html_file);
    service.publish(resource_server_side_events);
    upload_create_endpoints(service);
    restfull_api_create_endpoints(service);

    service.schedule(event_stream_handler, std::chrono::milliseconds(100));

    service.set_logger(std::make_shared<SyslogLogger>());

//    // Websocket
//    std::thread websocket_thread(websocket_init);

    service.start(settings);

//    websocket_thread.join();
    return 0;
}


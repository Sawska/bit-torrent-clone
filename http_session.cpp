#include "http_session.h"

http_session::http_session(asio::ip::tcp::socket socket, std::vector<std::string>& seeder_ips, Tracker& tracker)
    : socket_(std::move(socket)), seeder_ips_(seeder_ips), tracker_(tracker) {}

void http_session::start() {
    do_read();
}

void http_session::do_read() {
    auto self(shared_from_this());
    http::async_read(socket_, buffer_, request_,
        [self](beast::error_code ec, std::size_t bytes_transferred) {
            if (!ec) {
                self->handle_request();
            }
        });
}

void http_session::handle_request() {
    std::string target = std::string(request_.target());

    if (request_.method() != http::verb::post) {
        send_response(http::status::method_not_allowed, "Method not allowed");
        return;
    }

    if (target == "/list_seeders") {
        handle_list_seeders();
    } else if (target == "/send_torrent_file") {
        handle_send_torrent_file();
    } else if (target == "/available_files") {
        handle_send_available_files();
    } else if (target == "/choosed_file") {
        handle_choosed_file();
    } else if (target == "/become_seeder") {
        handle_become_seeder();
    } else if (target == "/become_peer") {
        handle_become_peer();
    } else if (target == "/unbecome_seeder") {
        handle_unbecome_seeder();
    } else if (target == "/unbecome_peer") {
        handle_unbecome_peer();
    } else {
        send_response(http::status::not_found, "Not Found");
    }
}

void http_session::handle_list_seeders() {
    std::string response_body = "Seeders:\n";
    for (const auto& seeder : seeder_ips_) {
        response_body += "- " + seeder + "\n";
    }
    send_response(http::status::ok, response_body);
}

void http_session::handle_send_torrent_file() {
    TorrentFile torrent = tracker_.get_torrent_file();
    std::string serialized_torrent = torrent.serialize_to_json();
    send_response(http::status::ok, serialized_torrent);
}

void http_session::handle_send_available_files() {
    std::stringstream response_body;
    response_body << "Available Files:\n";
    tracker_.show_available_files(response_body);
    send_response(http::status::ok, response_body.str());
}

void http_session::handle_choosed_file() {
    std::string file_name = request_.body();
    tracker_.peer_choosed_file(file_name);
    send_response(http::status::ok, "File chosen successfully: " + file_name);
}

void http_session::handle_become_seeder() {
    std::string ip = request_.body();
    tracker_.add_seeder(ip);
    send_response(http::status::ok, "Added your IP " + ip + " to seeders");
}

void http_session::handle_become_peer() {
    std::string ip = request_.body();
    tracker_.add_peer(ip);
    send_response(http::status::ok, "Added your IP " + ip + " to peers");
}

void http_session::handle_unbecome_seeder() {
    std::string ip = request_.body();
    tracker_.remove_seeder(ip);
    send_response(http::status::ok, "Removed your IP " + ip + " from seeders");
}

void http_session::handle_unbecome_peer() {
    std::string ip = request_.body();
    tracker_.remove_peer(ip);
    send_response(http::status::ok, "Removed your IP " + ip + " from peers");
}

void http_session::send_response(http::status status, const std::string& body) {
    auto self(shared_from_this());
    http::response<http::string_body> res{status, request_.version()};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, "text/plain");
    res.keep_alive(request_.keep_alive());
    res.body() = body;
    res.prepare_payload();

    http::async_write(socket_, res,
        [self](beast::error_code ec, std::size_t) {
            self->socket_.shutdown(tcp::socket::shutdown_send, ec);
        });
}
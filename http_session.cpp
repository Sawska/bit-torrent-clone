#include "http_session.h"

http_session::http_session(asio::ip::tcp::socket socket, const std::vector<std::string>& seeder_ips, Tracker& tracker)
    : socket(std::move(socket)), seeder_ips(seeder_ips), tracker(tracker) {}

void http_session::start() {
    read_request();
}

void http_session::read_request() {
    auto self = shared_from_this();
    http::async_read(
        socket, buffer, request,
        [self, this](boost::system::error_code ec, std::size_t bytes_transferred) {
            if (!ec) {
                handle_request();
            }
        }
    );
}

void http_session::handle_request() {
    std::string target = std::string(request.target());
    if (target == "/list_seeders") {
        handle_list_seeders();
    }  else if (target == "/")
    {
        handle_become_peer();
    }
    else if (target == "/send_torrent_file") {
        handle_send_torrent_file();
    } else if (target == "/available_files") {
        handle_send_available_files();
    } else if (target == "/choosed_file") {
        handle_choosed_file();
    } else if (target == "/become_seeder")
    {
        handle_become_seeder();
    } else if (target == "/become_peer")
    {
        handle_become_peer();
    }  else if (target == "/unbecome_seeder")
    {
        handle_unbecome_seeder();
    } else if (target == "/unbecome_peer")
    {
        handle_unbecome_peer();
    }
    else {
        send_response(http::status::not_found, "Not Found");
    }
}



void http_session::handle_become_peer() {
    std::string ip = request.body();

    tracker.add_peer(ip);

    send_response(http::status::ok,"Added your ip" + ip + "to peers");
}

void http_session::handle_unbecome_seeder()
{
    std::string ip = request.body();

    tracker.remove_seeder(ip);

    send_response(http::status::ok,"Removed your ip " + ip + "from seeders");
}

void http_session::handle_unbecome_peer()
{
    std::string ip = request.body();

    tracker.remove_peer(ip);

    send_response(http::status::ok, "Removed your ip " + ip + "froom peers");
}

void http_session::handle_become_seeder() {
    std::string ip = request.body();

    tracker.add_seeder(ip);

    send_response(http::status::ok, "Added your ip " + ip + "to seeders");
}


void http_session::handle_choosed_file() {
    
    std::string file_name = request.body();

    
    tracker.peer_choosed_file( file_name);

    
    send_response(http::status::ok, "File chosen successfully: " + file_name);
}

void http_session::handle_list_seeders() {
    std::string response_body = "Seeders:\n";
    for (const auto& seeder : seeder_ips) {
        response_body += "- " + seeder + "\n";
    }
    send_response(http::status::ok, response_body);
}

void http_session::handle_send_torrent_file() {
    TorrentFile torrent = tracker.get_torrent_file();
    
    
    std::string serialized_torrent = torrent.serialize_to_json();
    
    send_response(http::status::ok, serialized_torrent);
}

void http_session::handle_send_available_files() {
    std::stringstream response_body;
    response_body << "Available Files:\n";
    tracker.show_available_files(response_body);
    
    send_response(http::status::ok, response_body.str());
}

void http_session::send_response(http::status status, const std::string& body) {
    http::response<http::string_body> response{status, request.version()};
    response.set(http::field::server, "Tracker Server");
    response.set(http::field::content_type, "text/plain");
    response.body() = body;
    response.prepare_payload();
    auto self = shared_from_this();
    http::async_write(
        socket, response,
        [self, this](boost::system::error_code ec, std::size_t bytes_transferred) {
            socket.shutdown(asio::ip::tcp::socket::shutdown_send, ec);
        }
    );
}

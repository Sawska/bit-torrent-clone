#include "seeder.h"




Peer_Seeder::Peer_Seeder(asio::io_context& io_context)
    : io_context(io_context), socket(io_context) {}


Peer_Seeder::~Peer_Seeder() {
    disconnect();
}

void Peer_Seeder::connect_to_tracker(const std::string& tracker_ip, unsigned short tracker_port) {
    try {
        tcp::resolver resolver(io_context);
        auto endpoints = resolver.resolve(tracker_ip, std::to_string(tracker_port));

        socket.close();
        socket.open(tcp::v4());
        asio::connect(socket, endpoints);

        std::cout << "Connected to tracker at " << tracker_ip << ":" << tracker_port << std::endl;
    } catch (const boost::system::system_error& e) {
        std::cerr << "Boost system error: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Connection to tracker failed: " << e.what() << std::endl;
    }
}

std::string Peer_Seeder::send_request(const std::string& target, const std::string& body) {
    try {
        std::string host = "127.0.0.1:8080";
        http::request<http::string_body> req{http::verb::post, target, 11};
        req.set(http::field::host, host);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        req.set(http::field::content_type, "text/plain");
        req.body() = body;
        req.prepare_payload();

        if (!socket.is_open()) {
            throw std::runtime_error("Socket is not open.");
        }

        
        std::promise<std::string> response_promise;
        auto response_future = response_promise.get_future();

        http::async_write(socket, req,
            [this, &response_promise](boost::system::error_code ec, std::size_t ) {
                if (ec) {
                    response_promise.set_exception(std::make_exception_ptr(std::runtime_error("Error writing request: " + ec.message())));
                    return;
                }

                
                do_async_read(std::move(response_promise));
            });

        
        std::future_status status = response_future.wait_for(std::chrono::seconds(10)); 

        if (status == std::future_status::timeout) {
            throw std::runtime_error("Timeout waiting for response.");
        }

        return response_future.get(); 
    } catch (const std::exception& e) {
        std::cerr << "Exception in send_request: " << e.what() << std::endl;
        return "";
    }
}

void Peer_Seeder::do_async_read(std::promise<std::string> response_promise) {
    http::response<http::string_body> response;
    http::async_read(socket, buffer, response,
        [this, &response, &response_promise](boost::system::error_code ec, std::size_t ) {
            if (ec) {
                response_promise.set_exception(std::make_exception_ptr(std::runtime_error("Error reading response: " + ec.message())));
                return;
            }

            
            response_promise.set_value(response.body());
        });
}




void Peer_Seeder::handle_request() {
    http::read(socket, buffer, request);
    std::string target = std::string(request.target());

    if (target == "/send_file") {
        handle_send_file();
    } else {
        send_response(http::status::not_found, "Not Found");
    }
}

void Peer_Seeder::ask_for_file() {
    ask_for_seeders();

    while (file_parts.size() < torrent_file.hashed_pieces.size()) {
        int index = check_what_part_needed();
        std::string index_str = std::to_string(index);
        bool part_received = false;

        for (const auto& seeder_ip : seeder_ips) {
            connect_to_tracker(seeder_ip, 80);
            std::string target = "/send_file";
            std::string res = send_request(target, index_str);

            if (!res.empty()) {
                std::vector<char> part(res.begin(), res.end());

                if (check_if_part_sended_is_right(part, index)) {
                    file_parts.push_back(part);
                    part_received = true;
                    break;
                } else {
                    std::cerr << "Received file part from " << seeder_ip << " is incorrect." << std::endl;
                }
            } else {
                std::cerr << "Failed to receive file part from " << seeder_ip << std::endl;
            }
        }

        if (!part_received) {
            std::cerr << "Failed to receive the part from any seeder." << std::endl;
            break;
        }
    }
}

void Peer_Seeder::ask_for_seeders() {
    std::string target = "/list_seeders";
    std::string res = send_request(target, ip);
    process_seeder_list(res);
}

void Peer_Seeder::process_seeder_list(const std::string& seeder_list) {
    std::istringstream stream(seeder_list);
    std::string line;
    seeder_ips.clear();

    while (std::getline(stream, line)) {
        if (line.rfind("- ", 0) == 0) { 
            seeder_ips.push_back(line.substr(2)); 
        }
    }

    std::cout << "Updated Seeder List:" << std::endl;
    for (const auto& seeder : seeder_ips) {
        std::cout << seeder << std::endl;
    }
}

void Peer_Seeder::handle_send_file() {
    int index = std::stoi(request.body());

    if (!check_if_part_available(index)) {
        send_response(http::status::ok, "");
        return;
    }

    std::string part_str(file_parts[index].begin(), file_parts[index].end());
    send_response(http::status::ok, part_str);
}

void Peer_Seeder::send_response(http::status status, const std::string& body) {
    http::response<http::string_body> response{status, request.version()};
    response.set(http::field::server, "Peer Seeder");
    response.set(http::field::content_type, "text/plain");
    response.body() = body;
    response.prepare_payload();
    http::write(socket, response);
}

void Peer_Seeder::ask_for_torrent_file() {
    std::string target = "/send_torrent_file";
    std::string res = send_request(target, "");
    torrent_file = deserialize_torrent_file(res);
}

void Peer_Seeder::ask_for_becoming_seeder() {
    std::string target = "/become_seeder";
    send_request(target, ip);
}

void Peer_Seeder::ask_to_unbecome_seeder() {
    std::string target = "/unbecome_seeder";
    send_request(target, ip);
}

void Peer_Seeder::ask_to_unbecome_peer() {
    std::string target = "/unbecome_peer";
    send_request(target, ip);
}

void Peer_Seeder::show_available_files() {
    std::string target = "/available_files";
    std::string res = send_request(target, "");

    std::cout << "Available Files from Tracker:\n" << res << std::endl;

    std::cout << "-1 Leave" << std::endl;

    std::vector<std::string> parts = split(res, ":");
    int num_files = std::stoi(parts[0]);

    for (int i = 0; i < num_files; ++i) {
        std::cout << i << ": " << parts[i * 2 + 1] << std::endl;
    }

    int index = choose_file(0, num_files - 1);

    if (index == -1) {
        std::cout << "User chose to leave." << std::endl;
    } else {
        std::string name = parts[index * 2 + 1];
        std::cout << "User chose file: " << name << std::endl;
    }
}

int Peer_Seeder::choose_file(int first, int last) {
    int index = -1;
    std::cout << "Choose a file by entering its index (0-" << last << "), or -1 to leave: ";
    std::cin >> index;

    while (index < -1 || index > last) {
        std::cout << "Invalid index. Please enter a number between 0 and " << last << ", or -1 to leave: ";
        std::cin >> index;
    }

    return index;
}

void Peer_Seeder::choosed_file(const std::string& file_name) {
    std::string target = "/choosed_file";
    std::string response = send_request(target, file_name);
    std::cout << "Response from server: " << response << std::endl;
}

void Peer_Seeder::disconnect() {
    socket.close();
    std::cout << "Disconnected from tracker" << std::endl;
}

int Peer_Seeder::check_what_part_needed() {
    for (int i = 0; i < file_parts.size(); ++i) {
        if (file_parts[i].empty()) {
            return i;
        }
    }
    return -1;
}

bool Peer_Seeder::check_if_part_available(int index) {
    return !file_parts[index].empty();
}

void Peer_Seeder::compose_file(const std::string& output_file) {
    std::ofstream combined_file(output_file, std::ios::binary);
    if (!combined_file.is_open()) {
        std::cerr << "Failed to create combined file: " << output_file << std::endl;
        return;
    }

    for (const auto& part : file_parts) {
        combined_file.write(part.data(), part.size());
    }

    combined_file.close();
    std::cout << "Combined file created: " << output_file << std::endl;
}

bool Peer_Seeder::check_if_part_sended_is_right(const std::vector<char>& part, int index) {
    
    if (index < 0 || index >= torrent_file.hashed_pieces.size()) {
        std::cerr << "Error: Index out of range." << std::endl;
        return false;
    }

    
    std::string expected_hash = torrent_file.hash_piece(part.data(), part.size(), index);

    
    return torrent_file.hashed_pieces[index] == expected_hash;
}


TorrentFile Peer_Seeder::deserialize_torrent_file(const std::string& json_str) {
    json j = json::parse(json_str);
    TorrentFile torrent;
    torrent.name = j["name"];
    torrent.size = j["size"];
    torrent.location = j["location"];
    torrent.hashed_pieces = j["hashed_pieces"].get<std::vector<std::string>>();
    return torrent;
}

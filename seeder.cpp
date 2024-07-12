#include "seeder.h"






void Peer_Seeder::connect_to_tracker(const std::string& tracker_ip, unsigned short tracker_port) {
    http_client = httplib::Client(tracker_ip.c_str(), tracker_port);
}



Peer_Seeder::~Peer_Seeder()
{
    disconnect();
}

// std::string Peer_Seeder::send_request(std::string target,std::string body)
// {
        
// }




// void Peer_Seeder::handle_request() {
//     http::read(socket, buffer, request);
//     std::string target = std::string(request.target());

//     if (target == "/send_file") {
//         handle_send_file();
//     } else {
//         send_response(http::status::not_found, "Not Found");
//     }
// }



void Peer_Seeder::ask_for_file() {
    ask_for_seeders();

    while (file_parts.size() < torrent_file.hashed_pieces.size()) {
        int index = check_what_part_needed();
        std::string index_str = std::to_string(index);
        bool part_received = false;

        for (const auto& seeder_ip : seeder_ips) {
            connect_to_tracker(seeder_ip, 80);
            std::string target = "/send_file";
            std::string res = send_request_Pos(target, index_str)->body;

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
    httplib::Result res = send_request_get(target);
    process_seeder_list(res->body);
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

// void Peer_Seeder::handle_send_file() {
//     int index = std::stoi(request.body());

//     if (!check_if_part_available(index)) {
//         send_response(http::status::ok, "");
//         return;
//     }

//     std::string part_str(file_parts[index].begin(), file_parts[index].end());
//     // send_response(http::status::ok, part_str);
// }

// void Peer_Seeder::send_response(http::status status, const std::string& body) {
//     http::response<http::string_body> response{status, request.version()};
//     response.set(http::field::server, "Peer Seeder");
//     response.set(http::field::content_type, "text/plain");
//     response.body() = body;
//     response.prepare_payload();
//     http::write(socket, response);
// }

void Peer_Seeder::ask_for_torrent_file() {
    std::string target = "/send_torrent_file";
    httplib::Result res = send_request_get(target);
    torrent_file = deserialize_torrent_file(res->body);
}


void Peer_Seeder::ask_for_becoming_seeder() {
    std::string target = "/become_seeder";
    nlohmann::json request_body;
    request_body["ip"] = ip;

    std::cout << "Request Body: " << request_body << std::endl;

    httplib::Result res = send_request_Pos(target, request_body);

    if (res) {
        std::cout << "Response Status: " << res->status << std::endl;
        if (res->status == 200) {
            std::cout << "Response Body: " << res->body << std::endl;
        } else {
            std::cerr << "Failed to become a seeder, Status Code: " << res->status << std::endl;
        }
    } else {
        std::cerr << "Failed to send request" << std::endl;
    }
}

httplib::Result Peer_Seeder::send_request_Pos(std::string target, nlohmann::json request_body) {
    return http_client.Post(target.c_str(), request_body.dump(), "application/json");
}

httplib::Result Peer_Seeder::send_request_get(std::string target)
{
    httplib::Result res = http_client.Get(target);
    return httplib::Result();
}


void Peer_Seeder::ask_to_unbecome_seeder() {
    std::string target = "/unbecome_seeder";
    nlohmann::json request_body;
    request_body["ip"] = ip;
   httplib::Result res = send_request_Pos(target, ip);

   if (res && res->status == 200) {
        std::cout << res->body << std::endl;
    } else {
        std::cerr << "Failed to become a seeder" << std::endl;
    }
}

void Peer_Seeder::ask_to_unbecome_peer() {
    std::string target = "/unbecome_peer";
    nlohmann::json request_body;
    request_body["ip"] = ip;
    httplib::Result res = send_request_Pos(target, ip);

    if (res && res->status == 200) {
        std::cout << res->body << std::endl;
    } else {
        std::cerr << "Failed to become a seeder" << std::endl;
    }
}

void Peer_Seeder::show_available_files() {
    std::string target = "/available_files";
    httplib::Result res_http = send_request_get(target);
    std::string res = res_http->body;


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
    std::string response = send_request_Pos(target, file_name)->body;
    std::cout << "Response from server: " << response << std::endl;
}

void Peer_Seeder::disconnect() {
    http_client = httplib::Client(""); 
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

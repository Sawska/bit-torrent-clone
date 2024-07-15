#include "seeder.h"





void Peer_Seeder::connect_to_tracker(const std::string& tracker_ip, unsigned short tracker_port) {
    http_client = httplib::Client(tracker_ip, tracker_port);
}



Peer_Seeder::~Peer_Seeder()
{
    disconnect();
}






void Peer_Seeder::ask_for_file() {
    ask_for_seeders();

    if (seeder_ips.empty()) {
        std::cerr << "No seeders available." << std::endl;
        return;
    }

    initialize_file_parts(); // Initialize file_parts to the correct size

    auto first_seeder_ip = seeder_ips[0];
    auto res_s = split(first_seeder_ip, ":");

    if (res_s.size() != 2) {
        std::cerr << "Invalid seeder IP format: " << first_seeder_ip << std::endl;
        return;
    }

    connect_to_tracker(res_s[0], std::stoi(res_s[1]));

    while (true) {
        int index = check_what_part_needed();

        if (index == -1) {
            std::cerr << "All parts are already received." << std::endl;
            break;
        }

        nlohmann::json request_body;
        request_body["index"] = index;

        std::string target = "/send_file";
        httplib::Result res = send_request_Pos(target, request_body);

        if (res && res->status == 200) {
            std::string body = res->body;

            if (body.empty()) {
                std::cerr << "Seeder " << first_seeder_ip << " does not have the requested part." << std::endl;
                continue;
            }

            std::vector<char> part(body.begin(), body.end());

            std::string str(part.begin(), part.end());
            file_parts[index] = str; // Store part in the correct index
            std::cout << "Received part " << index << " from " << first_seeder_ip << std::endl;
        } else {
            std::cerr << "Failed to receive file part from " << first_seeder_ip << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::seconds(1)); // Add a delay before the next request
    }

    if (file_parts.size() == torrent_file.hashed_pieces.size() &&
        std::all_of(file_parts.begin(), file_parts.end(), [](const std::string& part) { return !part.empty(); })) {
        std::cout << "All parts received successfully." << std::endl;
    } else {
        std::cerr << "Failed to receive all parts." << std::endl;
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



void Peer_Seeder::ask_for_torrent_file() {
    std::string target = "/send_torrent_file";

    httplib::Result res = send_request_get(target);
    std::cout << "here4" << std::endl;
    std::cout << res->body << std::endl;
    
    torrent_file = deserialize_torrent_file(res->body);
}

void Peer_Seeder::be_seeder(std::string ip,int port) {
      CROW_ROUTE(app, "/send_file")
    .methods(crow::HTTPMethod::POST)
    ([this](const crow::request& req) {
        try {
             auto json_req = nlohmann::json::parse(req.body);
            int index = json_req.at("index").get<int>();

            if (!check_if_part_available(index)) {
                return crow::response(404, "Part not available");
            }

            std::string part_str(file_parts[index].begin(), file_parts[index].end());
            return crow::response(part_str);
        } catch (const std::exception& e) {
            return crow::response(400, "Invalid request");
        }
    });

}


void Peer_Seeder::ask_for_becoming_seeder() {
    std::string target = "/become_seeder";
    nlohmann::json request_body;
    request_body["ip"] = ip;
    request_body["port"] = std::to_string(port);
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
    std::cout << request_body.dump() << std::endl;
    return http_client.Post(target, request_body.dump(), "application/json");
}

httplib::Result Peer_Seeder::send_request_get(std::string target)
{
    httplib::Result res = http_client.Get(target);
    return res;
}


void Peer_Seeder::ask_to_unbecome_seeder() {
    std::string target = "/unbecome_seeder";
    nlohmann::json request_body;
    request_body["ip"] = ip;
    request_body["port"] = std::to_string(port);
   httplib::Result res = send_request_Pos(target, request_body);

   if (res && res->status == 200) {
        std::cout << res->body << std::endl;
    } else {
        std::cerr << "Failed to become a seeder" << std::endl;
    }
}

void Peer_Seeder::main_exchange()
{
    std::string target = "/";
    nlohmann::json request_body;
    request_body["ip"] = ip;
    request_body["port"] =  std::to_string(port);
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

void Peer_Seeder::ask_to_unbecome_peer() {
    std::string target = "/unbecome_peer";
    nlohmann::json request_body;
    request_body["ip"] = ip;
    request_body["port"] = std::to_string(port);
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

void Peer_Seeder::show_available_files() {
    std::string target = "/available_files";
    httplib::Result res_http = send_request_get(target);
    std::string res = res_http->body;


    std::cout << "Available Files from Tracker:\n" << res << std::endl;


    std::vector<std::string> parts = split(res, ":");
    int num_files = std::stoi(parts[0]);

    for (int i = 0; i < num_files; ++i) {
        std::cout << i << ": " << parts[i * 2 + 1] << std::endl;
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
    nlohmann::json request_body;
    request_body["name"] = file_name;
    std::string response = send_request_Pos(target, request_body)->body;
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

void Peer_Seeder::initialize_file_parts() {
    
    file_parts.resize(torrent_file.hashed_pieces.size(), "");
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



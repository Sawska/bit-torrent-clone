#include "seeder.h"


void Peer_Seeder::connect_to_tracker(const std::string& tracker_ip, unsigned short tracker_port) {
    tcp::resolver resolver(io_context);
    auto endpoints = resolver.resolve(tracker_ip, std::to_string(tracker_port));
    
    asio::connect(socket, endpoints);
    std::cout << "Connected to tracker at " << tracker_ip << ":" << tracker_port << std::endl;
}
std::string Peer_Seeder::send_request(const std::string& target, const std::string& body) {
    http::request<http::string_body> req{http::verb::post, target, 11};
    req.set(http::field::host, ip);
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    req.set(http::field::content_type, "text/plain");
    req.body() = body;
    req.prepare_payload();

    http::write(socket, req);

    beast::flat_buffer buffer;
    http::response<http::string_body> res;
    http::read(socket, buffer, res);

    std::cout << "Response code: " << res.result_int() << std::endl;
    std::cout << "Response body: " << res.body() << std::endl;

    return res.body();
}

void Peer_Seeder::handle_request() {
        http::read(socket, buffer, request);
        std::string target = std::string(request.target());

        if (target == "/send_file") {
            //
        } else {
            send_response(http::status::not_found, "Not Found");
        }
}

void Peer_Seeder::ask_for_file() 
{
    
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
     std::string res = send_request(target,"");

    torrent_file =  deserialize_torrent_file(res);


}

void Peer_Seeder::ask_for_becoming_seeder()
{
    std::string target = "/become_seeder";
    send_request(target,ip);
}

void Peer_Seeder::ask_to_unbecome_seeder()
{
    std::string target = "/unbecome_seeder";
    send_request(target,ip);

}

void Peer_Seeder::ask_to_unbecome_peer()
{
    std::string target = "/unbecome_peer";
    send_request(target,ip);
}

void Peer_Seeder::show_available_files() {
    std::string target = "/available_files";
    std::string res = send_request(target,"");

    std::cout << "Available Files from Tracker:\n" << res << std::endl;

    std::cout << "-1 Leave" << std::endl;

    
    std::vector<std::string> parts = split(res, ":");
    int num_files = std::stoi(parts[0]); 

    
    for (int i = 0; i < num_files; ++i) {
        std::cout << i << ": " << parts[i*2 + 1] << std::endl;
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

bool Peer_Seeder::check_if_part_sended_is_right(const std::vector<char>& part, int index, TorrentFile& tor_file) {
    return tor_file.hashed_pieces[index] == tor_file.hash_piece(part.data(), part.size(), index);
}

TorrentFile Peer_Seeder::deserialize_torrent_file(const std::string &json_str)
{
     json j = json::parse(json_str);

    
    TorrentFile torrent;
    torrent.name = j["name"];
    torrent.size = j["size"];
    torrent.location = j["location"];
    torrent.hashed_pieces = j["hashed_pieces"].get<std::vector<std::string>>();

    return torrent;
}

#ifndef SEEDER_H
#define SEEDER_H

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include "tracker.h"
#include "torrent.h"
#include <boost/algorithm/string.hpp>

namespace asio = boost::asio;
namespace beast = boost::beast;
using tcp = asio::ip::tcp;
using json = nlohmann::json;

class Tracker; 

class Peer_Seeder {
public:
    std::string ip;
    std::vector<std::vector<char>> file_parts; 
    asio::io_context io_context;
    tcp::socket socket{io_context};
    http::request<http::string_body> request;
    TorrentFile torrent_file;

     std::vector<std::string> split(const std::string& s, const std::string& delimiter) {
        std::vector<std::string> parts;
        boost::algorithm::split(parts, s, boost::algorithm::is_any_of(delimiter));
        return parts;
    }

    Peer_Seeder() : socket(io_context) {}

    void connect_to_tracker(const std::string &tracker_ip, unsigned short tracker_port);

    std::string send_request(const std::string& target, const std::string& body);

    void handle_request();
    void send_response(http::status status, const std::string& body);


    void ask_for_torrent_file();
    void ask_for_becoming_seeder();
    void ask_to_unbecome_seeder();
    void ask_to_unbecome_peer();
    

    void show_available_files();

    void choosed_file(const std::string& file_name);

    void disconnect();

    int check_what_part_needed();
    bool check_if_part_available(int index);
    int choose_file(int first, int last);
    void compose_file(const std::string& output_file);
    bool check_if_part_sended_is_right(const std::vector<char>& part, int index, TorrentFile& tor_file);
    TorrentFile deserialize_torrent_file(const std::string& json_str);

    private:
     beast::flat_buffer buffer;
};

#endif 

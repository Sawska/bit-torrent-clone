#ifndef SEEDER_H
#define SEEDER_H

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <mutex>
#include <condition_variable>
#include "tracker.h"
#include "torrent.h"
#include <boost/algorithm/string.hpp>
#include <nlohmann/json.hpp>
#include "httplib.h"

class Tracker;

class Peer_Seeder {
public:
    std::string ip;
    std::vector<std::string> file_parts;
    std::vector<std::string> seeder_ips;
    httplib::Client http_client;  
    crow::SimpleApp app;
    int port;
    
    Peer_Seeder(const std::string& ip,int port)
        : ip(ip), http_client(""),port(port) {}

    TorrentFile torrent_file;

    ~Peer_Seeder();

    void connect_to_tracker(const std::string& tracker_ip, unsigned short tracker_port);
    void main_exchange();
    void process_seeder_list(const std::string& seeder_list);
    void ask_for_torrent_file();
    void ask_for_becoming_seeder();
    httplib::Result send_request_get(std::string target);
    httplib::Result send_request_Pos(std::string target,  nlohmann::json request_body);
    void ask_to_unbecome_seeder();
    void be_seeder(std::string ip,int port);
    void ask_to_unbecome_peer();
    void ask_for_file();
    void ask_for_seeders();
    void show_available_files();
    void choosed_file(const std::string& file_name);
    void disconnect();
    int check_what_part_needed();
    bool check_if_part_available(int index);
    int choose_file(int first, int last);
    void compose_file(const std::string& output_file);
    bool check_if_part_sended_is_right(const std::vector<char> &part, int index);
    TorrentFile deserialize_torrent_file(const std::string& json_str);

private:
    std::mutex mutex_;
    std::vector<std::string> split(const std::string& s, const std::string& delimiter) {
        std::vector<std::string> parts;
        boost::algorithm::split(parts, s, boost::algorithm::is_any_of(delimiter));
        return parts;
    }
};

#endif

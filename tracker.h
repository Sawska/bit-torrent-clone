#ifndef TRACKER_H
#define TRACKER_H

#include <iostream>
#include <vector>
#include <string>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <sqlite3.h>
#include <mutex>
#include "torrent.h"
#include "crow.h"
#include <fstream>



class Tracker {
public:
    std::vector<std::string> seeder_ips;
    std::vector<std::string> peer_ips;
    TorrentFile torrent_file;
    std::string ip;
    sqlite3* db;
    crow::SimpleApp app;

    
    ~Tracker();
    std::string list_seeders();
    std::string list_peer();
    
    void define_routes();
    
    void add_seeder(const std::string& seeder_ip);
    void add_peer(const std::string& peer_ip);
    void remove_seeder(const std::string& seeder_ip);
    void remove_peer(const std::string& peer_ip);
    TorrentFile get_torrent_file() const;
    std::string select_file(const std::string& name);
    void peer_choosed_file(const std::string& name);
    void show_available_files(std::stringstream& response_body) const;
    void delete_file(const std::string& name);
    void add_file(const std::string& name, const std::string& path);
    bool openDatabase(const std::string& dbName);
    void closeDatabase();
    void if_db_not_created();
    void create_example_file(const std::string& filename, std::size_t size_kb);
private:
};

#endif // TRACKER_H

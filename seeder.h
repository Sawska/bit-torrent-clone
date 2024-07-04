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

namespace asio = boost::asio;
namespace beast = boost::beast;
using tcp = asio::ip::tcp;

class Tracker; 

class Peer_Seeder {
public:
    std::string ip;
    std::vector<std::vector<char>> file_parts; 
    asio::io_context io_context;
    tcp::socket socket{io_context};

    Peer_Seeder() : socket(io_context) {}

    
    int check_what_part_needed();
    bool check_if_part_available(int index);
    void compose_file(const std::string& output_file);
    bool check_if_part_sended_is_right(const std::vector<char>& part, int index, TorrentFile& tor_file);
};

#endif 

#ifndef TORRENT_H
#define TORRENT_H

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <openssl/sha.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class TorrentFile {
public:
    std::string name;
    long long size;
    std::string location;
    std::vector<std::string> hashed_pieces;

    void create_torrent_file(const std::string& filepath, int piece_size);

    std::string hash_piece(const char* data, size_t size, int piece_index);

    json serialize_to_json() const;

private:
    int num_pieces(int piece_size);
};

#endif // TORRENT_H

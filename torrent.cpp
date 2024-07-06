#include "torrent.h"

void TorrentFile::create_torrent_file(const std::string& filepath, int piece_size) {
    size_t last_slash = filepath.find_last_of("/\\");
    if (last_slash != std::string::npos) {
        name = filepath.substr(last_slash + 1);
    } else {
        name = filepath;
    }

    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filepath << std::endl;
        return;
    }

    file.seekg(0, std::ios::end);
    size = file.tellg();
    file.seekg(0, std::ios::beg);

    hashed_pieces.clear();

    char buffer[piece_size];
    for (int i = 0; i < num_pieces(piece_size); ++i) {
        file.read(buffer, piece_size);
        int read_size = file.gcount();

        std::string hash_str = hash_piece(buffer, read_size, i);
        hashed_pieces.push_back(hash_str);
    }

    file.close();
    std::cout << "Torrent file created for: " << filepath << std::endl;
}

std::string TorrentFile::hash_piece(const char* data, size_t size, int piece_index) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((const unsigned char*)data, size, hash);

    char hex_hash[2 * SHA256_DIGEST_LENGTH + 1];
    for (int j = 0; j < SHA256_DIGEST_LENGTH; ++j) {
        sprintf(hex_hash + 2 * j, "%02x", hash[j]);
    }
    hex_hash[2 * SHA256_DIGEST_LENGTH] = '\0';

    std::string hash_str = std::string(hex_hash) + std::to_string(piece_index);
    return hash_str;
}

json TorrentFile::serialize_to_json() const {
    json j;
    j["name"] = name;
    j["size"] = size;
    j["location"] = location;
    j["hashed_pieces"] = hashed_pieces;
    return j;
}

int TorrentFile::num_pieces(int piece_size) {
    return (size + piece_size - 1) / piece_size;
}

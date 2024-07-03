#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <openssl/sha.h>

class TorrentFile {
public:
    std::string name;
    long long size;  
    std::string location;  
    std::vector<std::string> hashed_pieces;

    void create_torrent_file(const std::string& filepath, int piece_size) {
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

    std::string hash_piece(const char* data, size_t size, int piece_index) {
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

    int num_pieces(int piece_size) {
        return (size + piece_size - 1) / piece_size;
    }
};


class Peer_Seeder {
public:
    std::string ip;
    std::vector<std::vector<char>> file_parts;  

    void receive_file(const std::string& peer_ip) {
        std::cout << "Receiving file from peer at IP: " << peer_ip << std::endl;
        // Implementation to receive file from peer
    }

    void send_file_to(const std::string& peer_ip) {
        std::cout << "Sending file to peer at IP: " << peer_ip << std::endl;
        // Implementation to send file to peer
    }

    void become_seeder() {
        std::cout << "Becoming a seeder.\n";
        // Implementation to become a seeder
    }

    void become_peer() {
        std::cout << "Becoming a peer.\n";
        // Implementation to become a peer
    }

    int check_what_part_needed() {
        for (int i = 0; i < file_parts.size(); ++i) {
            if (file_parts[i].empty()) {
                return i;
            }
        }
        return -1;
    }

    bool check_if_part_available(int index) {
        return !file_parts[index].empty();
    }

    void compose_file(const std::string& output_file) {
        std::ofstream combined_file(output_file, std::ios::binary);
        if (!combined_file.is_open()) {
            std::cerr << "Failed to create combined file: " << output_file << std::endl;
            return;
        }

        for (int i = 0; i < file_parts.size(); ++i) {
            combined_file.write(file_parts[i].data(), file_parts[i].size());
        }

        combined_file.close();
        std::cout << "Combined file created: " << output_file << std::endl;
    }

    bool check_if_part_sended_is_right(const std::vector<char>& part, int index, TorrentFile& tor_file) {
        return tor_file.hashed_pieces[index] == tor_file.hash_piece(part.data(), part.size(), index);
    }
};

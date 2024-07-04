#include "seeder.h"




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

#include <thread>
#include <iostream>
#include "seeder.h"
#include "tracker.h"

void run_tracker(Tracker& tracker) {
    sqlite3* db = nullptr;
    tracker.db = db;
    tracker.openDatabase("tracker.db");
    tracker.define_routes();
    tracker.create_example_file("example1.txt", 20);
    tracker.app.bindaddr("127.0.0.1").port(8080).multithreaded().run();
}

void run_seeder(Peer_Seeder& seeder) {
    seeder.file_parts.resize(4, ""); 
    seeder.file_parts[0] = "AAAAA";
    seeder.file_parts[1] = "AAAAA";
    seeder.file_parts[2] = "AAAAA";
    seeder.file_parts[3] = "AAAAA";
    seeder.be_seeder("127.0.0.1", 8081);
    seeder.app.bindaddr(seeder.ip).port(seeder.port).multithreaded().run();
}

int main() {
    try {
        Tracker tracker;
        std::thread tracker_thread(run_tracker, std::ref(tracker));

        std::this_thread::sleep_for(std::chrono::seconds(2));

        Peer_Seeder peer_seeder("127.0.0.1", 8081);
        peer_seeder.connect_to_tracker("127.0.0.1", 8080);
        peer_seeder.main_exchange();
        peer_seeder.show_available_files();

        peer_seeder.choosed_file("example1.txt");
        peer_seeder.ask_for_becoming_seeder();
        peer_seeder.ask_for_torrent_file();
        std::cout << "Torrent file name: " << peer_seeder.torrent_file.name << std::endl;

        std::this_thread::sleep_for(std::chrono::seconds(1));
        Peer_Seeder peer_seeder1("127.0.0.1", 8082);
        peer_seeder1.connect_to_tracker("127.0.0.1", 8080);
        peer_seeder1.main_exchange();
        peer_seeder1.ask_for_torrent_file();

        std::this_thread::sleep_for(std::chrono::seconds(1));

        std::thread seeder_thread(run_seeder, std::ref(peer_seeder));
        peer_seeder1.initialize_file_parts();
        peer_seeder1.ask_for_file();
        peer_seeder1.compose_file("new_example.txt");

        tracker_thread.join();
        seeder_thread.join();
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown exception occurred" << std::endl;
    }

    return 0;
}
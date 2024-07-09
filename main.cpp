#include <iostream>
#include <thread>
#include <sqlite3.h>
#include <boost/asio.hpp>
#include "tracker.h"
#include "seeder.h"
#include "http_session.h"

// void run_tracker(Tracker& tracker) {
//     tracker.io_context.run();
// }

// void run_seeder(Peer_Seeder& seeder, const std::string& tracker_ip, unsigned short tracker_port) {
//     try {
//         seeder.connect_to_tracker(tracker_ip, tracker_port);
//         seeder.ask_for_becoming_seeder();
//         seeder.ask_for_seeders();
//         seeder.disconnect();
//     } catch (const std::exception& e) {
//         std::cerr << "Seeder error: " << e.what() << std::endl;
//     }
// }





int main() {
    sqlite3* db = nullptr;
    Tracker tracker;
    tracker.db = db;

    Peer_Seeder seeder(tracker.io_context);
    seeder.ip = "127.0.0.2";

    try {
        seeder.connect_to_tracker("127.0.0.1", 8080);

        std::thread io_thread([&tracker]() {
            tracker.io_context.run();
        });

        seeder.ask_for_becoming_seeder();
        seeder.ask_for_seeders();
        seeder.disconnect();

        io_thread.join(); 

    } catch (const boost::system::system_error& e) {
        std::cerr << "Boost system error: " << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

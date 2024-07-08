#include <iostream>
#include <sqlite3.h>
#include "tracker.h"
#include "seeder.h"




int main() {
    sqlite3 *db = nullptr;
    Tracker tracker;
    tracker.db = db;
    tracker.ip = "127.0.0.1";

    Peer_Seeder seeder;
    seeder.ip = "127.0.0.2";

    try {
        // Initialize and start the Tracker
        asio::io_context io_context;

        // Check endpoint binding for Tracker
        asio::ip::tcp::endpoint endpoint(asio::ip::tcp::v4(), 8080);
        // tracker.acceptor.open(endpoint.protocol());
        tracker.acceptor.bind(endpoint);
        tracker.acceptor.listen();

        std::cout << "Tracker initialized and listening on port 8080." << std::endl;

        
        seeder.connect_to_tracker("127.0.0.1", 8080);
        
        

    } catch (const boost::system::system_error& e) {
        std::cerr << "Boost system error: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }


        
    // seeder.ask_for_becoming_seeder();
    // seeder.ask_for_seeders();

    // for(int i =0;i<seeder.seeder_ips.size();i++)
    // {
    //     std::cout << seeder.seeder_ips[i] << std::endl;
    // }

    // seeder.ask_to_unbecome_seeder();

    // seeder.ask_for_seeders();

    // for(int i =0;i<seeder.seeder_ips.size();i++)
    // {
    //     std::cout << seeder.seeder_ips[i] << std::endl;
    // }
    seeder.disconnect();

    tracker.closeDatabase();

    return 0;
}
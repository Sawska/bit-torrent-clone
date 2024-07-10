#include <iostream>
#include <thread>
#include <sqlite3.h>
#include <boost/asio.hpp>
#include "tracker.h"
#include "seeder.h"

int main() {
    try {
        // Create io_context object
        boost::asio::io_context io_context;

        // Initialize the Tracker
        Tracker tracker;

         tracker.define_routes();
        std::thread tracker_thread([&]() {
            // tracker.start_accept();
            io_context.run();
        });

        // Initialize Peer_Seeder
        Peer_Seeder peer_seeder(io_context);

        // Connect Peer_Seeder to the Tracker
        peer_seeder.connect_to_tracker("127.0.0.1", 8080);

        // Example of sending a request from Peer_Seeder to Tracker
        std::string response = peer_seeder.send_request("/ask_for_list_of_seeders", "");
        std::cout << "Response from tracker: " << response << std::endl;

        // Make sure the tracker thread completes before exiting
        tracker_thread.join();
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}

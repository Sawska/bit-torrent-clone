#include <boost/asio.hpp>
#include "seeder.h"
#include "tracker.h"


int main() {
    asio::io_context io_context;

    Tracker tracker;
    sqlite3* db = nullptr;
    tracker.db = db;
    tracker.openDatabase("tracker.db");
    tracker.define_routes();

    Peer_Seeder peer_seeder(io_context);
    peer_seeder.connect_to_tracker("127.0.0.1", 8080);
    peer_seeder.send_request("/", "");

    return 0;


    
}

#include <thread>
#include "seeder.h"
#include "tracker.h"

// Function to run the tracker
void run_tracker(Tracker& tracker) {
    sqlite3* db = nullptr;
    tracker.db = db;
    tracker.openDatabase("tracker.db");
    tracker.define_routes();
    
    tracker.define_routes();
}

int main() {
    
    Tracker tracker;

    
    std::thread tracker_thread(run_tracker, std::ref(tracker));

    
    Peer_Seeder peer_seeder("127.0.0.1");
    peer_seeder.connect_to_tracker("127.0.0.1", 8080);
    nlohmann::json request_body;
    request_body["ip"] = peer_seeder.ip;

    // peer_seeder.send_request_Pos("/",request_body);

    peer_seeder.ask_for_becoming_seeder();
    // peer_seeder.ask_for_seeders();
    // peer_seeder.ask_to_unbecome_seeder();

    
    tracker_thread.join();

    return 0;
}

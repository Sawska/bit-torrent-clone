#include <thread>
#include "seeder.h"
#include "tracker.h"

// Function to run the tracker
void run_tracker(Tracker& tracker) {
    sqlite3* db = nullptr;
    tracker.db = db;
    tracker.openDatabase("tracker.db");
    tracker.add_file("exampleLizok.txt","exampleLizok.txt");
    
    tracker.define_routes();
    tracker.app.bindaddr("127.0.0.1").port(8080).multithreaded().run();
}

int main() {
    Tracker tracker;
    std::thread tracker_thread(run_tracker, std::ref(tracker));

    
    std::this_thread::sleep_for(std::chrono::seconds(1));

    
    Peer_Seeder peer_seeder("127.0.0.1");

    
    peer_seeder.connect_to_tracker("127.0.0.1", 8080);

    
    peer_seeder.main_exchange();


    peer_seeder.show_available_files();
    
    peer_seeder.choosed_file("exampleLizok.txt");

    peer_seeder.ask_for_torrent_file();



    std::cout << peer_seeder.torrent_file.name << std::endl;
    
    
    
    tracker_thread.join();

    return 0;
}

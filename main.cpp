#include <iostream>
#include <sqlite3.h>
#include "tracker.hpp"
#include "seeder.hpp"



int main() {
    sqlite3 *db = nullptr;
    Tracker tracker;
    tracker.db = db;
    tracker.ip = "127.0.0.1";

    Peer_Seeder seeder;

    seeder.ip = "127.0.0.2";

    seeder.connect_to_tracker("127.0.0.1",80);
        
    seeder.ask_for_seeders();
    seeder.ask_for_becoming_seeder();

    for(int i =0;i<seeder.seeder_ips.size();i++)
    {
        std::cout << seeder.seeder_ips[i] << std::endl;
    }

    seeder.ask_to_unbecome_seeder();

    seeder.ask_for_seeders();

    for(int i =0;i<seeder.seeder_ips.size();i++)
    {
        std::cout << seeder.seeder_ips[i] << std::endl;
    }
    seeder.disconnect();

    tracker.closeDatabase();

    return 0;
}
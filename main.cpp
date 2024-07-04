#include <iostream>
#include <sqlite3.h>
#include "tracker.h"

int main() {
    sqlite3 *db;
    if (!openDatabase("tracker.db", &db)) {
        std::cerr << "Failed to open database." << std::endl;
        return 1;
    }

    Tracker tracker;
    tracker.ip = "127.0.0.1";

    

    closeDatabase(db);

    return 0;
}

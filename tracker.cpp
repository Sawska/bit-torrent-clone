#include "tracker.h"

void Tracker::add_seeder(const std::string& seeder_ip) {
    seeder_ips.push_back(seeder_ip);
    std::cout << "Seeder added: " << seeder_ip << std::endl;
}

void Tracker::add_peer(const std::string& peer_ip) {
    peer_ips.push_back(peer_ip);
    std::cout << "Peer added: " << peer_ip << std::endl;
}

void Tracker::remove_seeder(const std::string& seeder_ip) {
    auto it = std::find(seeder_ips.begin(), seeder_ips.end(), seeder_ip);
    if (it != seeder_ips.end()) {
        seeder_ips.erase(it);
        std::cout << "Seeder removed: " << seeder_ip << std::endl;
    } else {
        std::cerr << "Seeder not found: " << seeder_ip << std::endl;
    }
}

void Tracker::remove_peer(const std::string& peer_ip) {
    auto it = std::find(peer_ips.begin(), peer_ips.end(), peer_ip);
    if (it != peer_ips.end()) {
        peer_ips.erase(it);
        std::cout << "Peer removed: " << peer_ip << std::endl;
    } else {
        std::cerr << "Peer not found: " << peer_ip << std::endl;
    }
}

std::string Tracker::list_seeders() {
    std::stringstream body;
    body << "Seeders:\n";
    for (const auto& seeder : seeder_ips) {
        body << "- " << seeder << "\n";
    }
    return body.str();  
}
std::string Tracker::list_peer() {
    std::stringstream body;
    body << "Seeders:\n";
    for (const auto& peer : peer_ips) {
        body << "- " << peer << "\n";
    }
    return body.str();  
}


TorrentFile Tracker::get_torrent_file() const {
    return torrent_file;
}

std::string Tracker::select_file(const std::string& name) {
    std::string path;

    std::cout <<  "file name " + name << std::endl;

    std::string sql = "SELECT path FROM files WHERE name = ?";
    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        return "";
    }

    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        const unsigned char* path_result = sqlite3_column_text(stmt, 0);
        if (path_result) {
            path = reinterpret_cast<const char*>(path_result);
        }
    } else {
        std::cerr << "Error selecting file: " << sqlite3_errmsg(db) << std::endl;
    }

    sqlite3_finalize(stmt);
    return path;
}

void Tracker::peer_choosed_file(const std::string& name) {
    std::string path = select_file(name);

    if (!path.empty()) {
        torrent_file.create_torrent_file(path, 5);
    } else {
        std::cerr << "File not found: " << name << std::endl;
    }
}

void Tracker::show_available_files(std::stringstream& response_body) const {
    std::string sql = "SELECT name FROM files";
    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        return;
    }

    int index = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char* name = sqlite3_column_text(stmt, 0);
        if (name) {
            response_body << index++ << ": " << name << "\n";
        }
    }

    sqlite3_finalize(stmt);
}

void Tracker::delete_file(const std::string& name) {
    std::string sql = "DELETE FROM files WHERE name = ?";
    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        return;
    }

    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_DONE) {
        std::cout << "Deleted file: " << name << std::endl;
    } else {
        std::cerr << "Error deleting file: " << sqlite3_errmsg(db) << std::endl;
    }

    sqlite3_finalize(stmt);
}

void Tracker::add_file(const std::string& name, const std::string& path) {
    std::string sql = "INSERT INTO files (name, path) VALUES (?, ?)";
    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        return;
    }

    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, path.c_str(), -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_DONE) {
        std::cout << "Added file successfully: " << name << " at path: " << path << std::endl;
    } else {
        std::cerr << "Failed to add file: " << sqlite3_errmsg(db) << std::endl;
    }

    sqlite3_finalize(stmt);
}

bool Tracker::openDatabase(const std::string& dbName) {
    return sqlite3_open(dbName.c_str(), &db) == SQLITE_OK;
}

void Tracker::closeDatabase() {
    if (db) {
        sqlite3_close(db);
        db = nullptr;
    }
}

void Tracker::if_db_not_created() {
    std::string sql = 
        "CREATE TABLE IF NOT EXISTS files ("
        "id INTEGER PRIMARY KEY, "
        "name TEXT NOT NULL, "
        "path TEXT NOT NULL"
        ");";

    char* errMsg = nullptr;

    int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg);

    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
    } else {
        std::cout << "Table created successfully or already exists." << std::endl;
    }
}
Tracker::~Tracker() {
    closeDatabase();
}



void Tracker::define_routes() {
    CROW_ROUTE(app, "/")
    .methods(crow::HTTPMethod::POST) 
    ([this](const crow::request& req) {
        
        std::string ip = crow::json::load(req.body)["ip"].s();
        std::string  port = crow::json::load(req.body)["port"].s();
        std::cout << ip << std::endl;
        add_peer(ip+":"+ port);
        return crow::response("added your IP to peers");
    });

    CROW_ROUTE(app, "/list_seeders")
    .methods(crow::HTTPMethod::Get) 
    ([this]() {
        return crow::response(list_seeders()); 
    });
    CROW_ROUTE(app, "/list_peers")
    .methods(crow::HTTPMethod::Get) 
    ([this]() {
        return crow::response(list_seeders()); 
    });

    CROW_ROUTE(app, "/become_seeder")
    .methods(crow::HTTPMethod::Post)
    ([this](const crow::request& req) {
        auto json_body = crow::json::load(req.body);
        std::cout << req.body << std::endl;
        if (!json_body) {
            return crow::response(400, "Invalid JSON");
        }

        std::string ip = json_body["ip"].s();
        std::string port = crow::json::load(req.body)["port"].s();
        this->add_seeder(ip+":"+port); 
        
        std::string response_message = "added as seeder " + ip;
        return crow::response(200, response_message);
    });



    CROW_ROUTE(app, "/unbecome_seeder")
    .methods(crow::HTTPMethod::Post)
    ([this](const crow::request& req) {
        std::string ip = crow::json::load(req.body)["ip"].s();
        std::string port = crow::json::load(req.body)["port"].s();
        remove_seeder(ip+":"+port);
        return crow::response("removed from seeders");
    });

    CROW_ROUTE(app, "/unbecome_peer")
    .methods(crow::HTTPMethod::Post)
    ([this](const crow::request& req) {
        std::string ip = crow::json::load(req.body)["ip"].s();
        std::string port = crow::json::load(req.body)["port"].s();
        remove_peer(ip+":"+port);
        return crow::response("removed from peers");
    });

    CROW_ROUTE(app, "/send_torrent_file")
    .methods(crow::HTTPMethod::Get) 
    ([this]() {
        return crow::response(get_torrent_file().serialize_to_json().dump()); 
    });

    CROW_ROUTE(app, "/available_files")
    .methods(crow::HTTPMethod::Get) 
    ([this]() {
        std::stringstream response_body;
        show_available_files(response_body); 
        return crow::response(response_body.str());
    });

    CROW_ROUTE(app, "/choosed_file")
    .methods(crow::HTTPMethod::Post)
    ([this](const crow::request& req) {

        std::string name = crow::json::load(req.body)["name"].s();
        std::cout <<  "from Crow " + req.body << std::endl;
        std::cout <<  "from Crow name" + name << std::endl;
        peer_choosed_file(name);
        return crow::response("file chosen");
    });

    
}




void Tracker::create_example_file(const std::string& filename, std::size_t size_bytes) {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to create file: " << filename << std::endl;
        return;
    }

    std::vector<char> buffer(size_bytes, 'A');

    file.write(buffer.data(), size_bytes);
    if (!file) {
        std::cerr << "Failed to write to file: " << filename << std::endl;
    }

    file.close();
    if (!file.good()) {
        std::cerr << "Error occurred while closing the file: " << filename << std::endl;
    }

    add_file(filename, "./" + filename);
    std::cout << "Created file: " << filename << " with size: " << size_bytes << " bytes" << std::endl;
}

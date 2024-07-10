#include "tracker.h"

Tracker::Tracker() : acceptor(io_context) {
    tcp::endpoint endpoint(asio::ip::make_address("127.0.0.1"), 8080);

    boost::system::error_code ec;
    acceptor.open(endpoint.protocol(), ec);
    if (ec) {
        std::cerr << "Open error: " << ec.message() << std::endl;
        return;
    }

    acceptor.set_option(asio::socket_base::reuse_address(true), ec);
    if (ec) {
        std::cerr << "Set option error: " << ec.message() << std::endl;
        return;
    }

    acceptor.bind(endpoint, ec);
    if (ec) {
        std::cerr << "Bind error: " << ec.message() << std::endl;
        return;
    }

    acceptor.listen(asio::socket_base::max_listen_connections, ec);
    if (ec) {
        std::cerr << "Listen error: " << ec.message() << std::endl;
        return;
    }

    if (!openDatabase("tracker.db")) {
        std::cerr << "Failed to open database." << std::endl;
        return;
    }

    if_db_not_created();
    start_accept();
}


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

std::stringstream Tracker::list_seeders() {


        std::stringstream body;
        body << "Seeders:\n";
        for (const auto& seeder : seeder_ips) {
            body << "- " << seeder << "\n";
        }
            body;
}

TorrentFile Tracker::get_torrent_file() const {
    return torrent_file;
}

std::string Tracker::select_file(const std::string& name) {
    std::string path;

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
        torrent_file.create_torrent_file(path, 5); // Example chunk size: 5 bytes
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
    .methods("POST"_method)
    ([this](const crow::request& req) {
        auto ip = crow::json::load(req.body)["ip"].s();
        add_peer(ip);
        return crow::response("added your IP to peers");
    });

    CROW_ROUTE(app, "/list_seeders")
    ([this]() {
        return crow::response(list_seeders());
    });

    CROW_ROUTE(app, "/become_seeder")
    .methods("POST"_method)
    ([this](const crow::request& req) {
        auto ip = crow::json::load(req.body)["ip"].s();
        add_seeder(ip);
        return crow::response("added as seeder");
    });

    CROW_ROUTE(app, "/unbecome_seeder")
    .methods("POST"_method)
    ([this](const crow::request& req) {
        auto ip = crow::json::load(req.body)["ip"].s();
        remove_seeder(ip);
        return crow::response("removed from seeders");
    });

    CROW_ROUTE(app, "/send_torrent_file")
    ([this]() {
        return crow::response(get_torrent_file());
    });

    CROW_ROUTE(app, "/available_files")
    ([this]() {
        std::stringstream response_body;
        return crow::response(show_available_files(response_body));
    });

    CROW_ROUTE(app, "/choosed_file")
    .methods("POST"_method)
    ([this](const crow::request& req) {
        auto name = crow::json::load(req.body)["name"].s();
        peer_choosed_file(name);
        return crow::response("file chosen");
    });

    app.port(8080).multithreaded().run();
}


void Tracker::start_accept() {
    acceptor.async_accept(
        [this](boost::system::error_code ec, tcp::socket socket) {
            if (!ec) {
                std::cout << "Accepted connection from: " << socket.remote_endpoint().address().to_string() << std::endl;
                
                
                beast::flat_buffer buffer;
                http::request<http::string_body> req;
                http::read(socket, buffer, req);

                
                handle_request(std::move(req), socket);
            } else {
                std::cerr << "Accept error: " << ec.message() << std::endl;
            }
            start_accept(); 
        }
    );
}

void Tracker::handle_request(http::request<http::string_body> req, tcp::socket& socket) {
    
    http::response<http::string_body> res{http::status::ok, req.version()};
    res.set(http::field::server, "Tracker Server");
    res.set(http::field::content_type, "text/plain");

    if (req.method() == http::verb::get && req.target() == "/list_seeders") {
        std::stringstream body;
        body << "Seeders:\n";
        for (const auto& seeder : seeder_ips) {
            body << "- " << seeder << "\n";
        }
        res.body() = body.str();
    } else {
        res.body() = "Invalid request.";
    }

    res.prepare_payload();
    http::write(socket, res);

    beast::error_code ec;
    socket.shutdown(tcp::socket::shutdown_both, ec);
    if (ec && ec != beast::errc::not_connected) {
        std::cerr << "Shutdown error: " << ec.message() << std::endl;
    }
}
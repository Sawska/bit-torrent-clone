#include "tracker.h"
#include "http_session.h"

Tracker::Tracker()
    : acceptor(io_context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), 8080)) {
    start_accept();
}

void Tracker::start_accept() {
    acceptor.async_accept(
        [this](boost::system::error_code ec, asio::ip::tcp::socket socket) {
            if (!ec) {
                std::cout << "Accepted connection from: " << socket.remote_endpoint().address().to_string() << std::endl;
                std::make_shared<http_session>(std::move(socket), seeder_ips, *this)->start();
            }
            start_accept();
        }
    );
}

void Tracker::add_seeder(const std::string& seeder_ip) {
    seeder_ips.push_back(seeder_ip);
    std::cout << "Seeder added: " << seeder_ip << std::endl;
}

void Tracker::add_peer(const std::string& peer_ip) {
    peer_ips.push_back(peer_ip);
    std::cout << "Peer added: " << peer_ip << std::endl;
}

void Tracker::remove_seeder(const std::string &seeder_ip) {
    auto it = std::find(seeder_ips.begin(), seeder_ips.end(), seeder_ip);
    if (it != seeder_ips.end()) {
        seeder_ips.erase(it);
        std::cout << "Seeder removed: " << seeder_ip << std::endl;
    } else {
        std::cerr << "Seeder not found: " << seeder_ip << std::endl;
    }
}

void Tracker::remove_peer(const std::string &peer_ip)
{
    auto it = std::find(peer_ips.begin(),peer_ips.end(),peer_ip);
    if (it != peer_ips.end()) {
        peer_ips.erase(it);
        std::cout << "Peer removed:" << peer_ip << std::endl;
    } else {
        std::cerr <<  "Peer not found" << peer_ip << std::endl;
    }
}

void Tracker::list_seeders(const std::string& client_ip, const std::string& port) {
    std::cout << "Handling list seeders request from IP: " << client_ip << std::endl;

    try {
        asio::io_context io_context;
        tcp::resolver resolver(io_context);
        beast::tcp_stream stream(io_context);

        auto const results = resolver.resolve(client_ip, port);
        stream.connect(results);

        std::stringstream body;
        body << "Seeders:\n";
        for (const auto& seeder : seeder_ips) {
            body << "- " << seeder << "\n";
        }

        http::response<http::string_body> res{http::status::ok, 11};
        res.set(http::field::server, "Tracker Server");
        res.set(http::field::content_type, "text/plain");
        res.body() = body.str();
        res.prepare_payload();

        http::write(stream, res);

        beast::error_code ec;
        stream.socket().shutdown(tcp::socket::shutdown_both, ec);
        if (ec && ec != beast::errc::not_connected) {
            throw beast::system_error{ec};
        }
    } catch (std::exception& e) {
        std::cerr << "Error handling list seeders request: " << e.what() << std::endl;
    }
}

TorrentFile Tracker::get_torrent_file() const {
    return torrent_file;
}

std::string Tracker::select_file( const std::string &name) {
    std::string path;

    std::string sql = "SELECT path FROM files WHERE name = ?";
    sqlite3_stmt *stmt = nullptr;

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

void Tracker::show_available_files( std::stringstream& response_body) const {
    std::string sql = "SELECT name FROM files";
    sqlite3_stmt *stmt = nullptr;

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
    sqlite3_stmt *stmt = nullptr;

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
    sqlite3_stmt *stmt = nullptr;

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



bool openDatabase(const std::string& dbName, sqlite3** db) {
    return sqlite3_open(dbName.c_str(), db) == SQLITE_OK;
}

void closeDatabase(sqlite3* db) {
    sqlite3_close(db);
}

void if_db_not_created(sqlite3* db) {
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

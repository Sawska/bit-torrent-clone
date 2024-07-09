#ifndef HTTP_SESSION_H
#define HTTP_SESSION_H

#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <memory>
#include <vector>
#include "tracker.h"

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;

class http_session : public std::enable_shared_from_this<http_session> {
public:
    explicit http_session(asio::ip::tcp::socket socket, std::vector<std::string>& seeder_ips, Tracker& tracker);
    void start();

private:
    void do_read();
    void handle_request();
    void handle_choosed_file();
    void handle_list_seeders();
    void handle_send_torrent_file();
    void handle_send_available_files();
    void send_response(http::status status, const std::string& body);
    void handle_become_seeder();
    void handle_become_peer();
    void handle_unbecome_seeder();
    void handle_unbecome_peer();

    asio::ip::tcp::socket socket_;
    beast::flat_buffer buffer_;
    http::request<http::string_body> request_;
    std::vector<std::string>& seeder_ips_;
    Tracker& tracker_;
};

#endif

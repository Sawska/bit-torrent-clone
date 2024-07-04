#ifndef HTTP_SESSION_H
#define HTTP_SESSION_H

#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <memory>
#include <vector>
#include "tracker.h"

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = boost::beast::http;

class http_session : public std::enable_shared_from_this<http_session> {
public:
    http_session(asio::ip::tcp::socket socket, const std::vector<std::string>& seeder_ips, Tracker& tracker);

    void start();

private:
    void read_request();
    void handle_request();
    void handle_choosed_file();
    void handle_list_seeders();
    void handle_send_torrent_file();
    void handle_send_available_files();
    void send_response(http::status status, const std::string& body);

    asio::ip::tcp::socket socket;
    beast::flat_buffer buffer;
    http::request<http::string_body> request;
    std::vector<std::string> seeder_ips;
    Tracker& tracker;
};

#endif // HTTP_SESSION_H

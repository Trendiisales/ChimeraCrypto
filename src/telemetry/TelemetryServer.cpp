#include "telemetry/TelemetryServer.hpp"
#include "engine/InstitutionalEngine.hpp"
#include <sstream>
#include <cstring>
#include <netinet/in.h>
#include <unistd.h>

namespace chimera {

TelemetryServer::TelemetryServer(InstitutionalEngine& engine,
                                 int port)
    : engine_(engine),
      port_(port)
{}

TelemetryServer::~TelemetryServer() {
    stop();
}

void TelemetryServer::start()
{
    running_ = true;
    server_thread_ = std::thread(&TelemetryServer::run, this);
}

void TelemetryServer::stop()
{
    running_ = false;
    if (server_thread_.joinable())
        server_thread_.join();
}

std::string TelemetryServer::build_json()
{
    std::ostringstream oss;
    oss << "{";
    oss << "\"equity\":" << engine_.get_equity();
    oss << "}";
    return oss.str();
}

void TelemetryServer::run()
{
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
        return;

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_);

    int opt = 1;
    setsockopt(server_fd,
               SOL_SOCKET,
               SO_REUSEADDR | SO_REUSEPORT,
               &opt,
               sizeof(opt));

    if (bind(server_fd,
             (struct sockaddr*)&address,
             sizeof(address)) < 0)
        return;

    if (listen(server_fd, 10) < 0)
        return;

    while (running_)
    {
        sockaddr_in client_addr{};
        socklen_t addrlen = sizeof(client_addr);

        int client =
            accept(server_fd,
                   (struct sockaddr*)&client_addr,
                   &addrlen);

        if (client < 0)
            continue;

        std::string body = build_json();

        std::ostringstream response;
        response << "HTTP/1.1 200 OK\r\n";
        response << "Content-Type: application/json\r\n";
        response << "Content-Length: "
                 << body.size() << "\r\n\r\n";
        response << body;

        std::string resp_str = response.str();

        send(client,
             resp_str.c_str(),
             resp_str.size(),
             0);

        close(client);
    }

    close(server_fd);
}

}

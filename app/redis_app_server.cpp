#include <base_log.h>
#include <redis_server.h>

#include <cstdlib>

using xin::base::log;
using xin::redis::Server;

int main(int argc, char* argv[])
{
    constexpr Server::PortType port = 16379;
    log::set_level(xin::base::LogLevel::Warning);
    Server server{ port };

    try {
        server.start();
    }
    catch (const std::exception& e) {
        log::error("Server error: {}", e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

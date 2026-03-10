#include <base_log.h>
#include <redis_application_context.h>
#include <redis_server.h>

#include <cstdlib>
#include <thread>

using xin::base::log;
using xin::redis::application_context;
using xin::redis::Server;

int main(int argc, char* argv[])
{
    constexpr Server::Port port = 26379;
    log::set_level(xin::base::LogLevel::Warning);

    auto thread_count =
        std::thread::hardware_concurrency() == 0 ? 1 : std::thread::hardware_concurrency();
    Server server{ port, thread_count };

    try {
        server.start();
    }
    catch (const std::exception& e) {
        log::error("Server error: {}", e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

#ifndef XIN_REDIS_APPLICATION_CONTEXT_H
#define XIN_REDIS_APPLICATION_CONTEXT_H

#include <asio.hpp>
#include <asio/io_context.hpp>
#include <base_aof_logger.h>
#include <redis_storage.h>

#include <cstddef>
#include <string_view>

namespace xin::redis {

// 这是一个全局上下文，包含了运行时需要访问的全局资源，如数据库实例、日志系统等。
struct application_context {
    application_context() = delete;

    using Port = std::uint16_t;

    static constexpr Port DEFAULT_PORT = 16379;

    static constexpr std::size_t DB_COUNT = 16;

    static constexpr std::string_view AOF_FILE_PATH = "appendonly.aof";

    static Port port;

    static std::array<Database, DB_COUNT> databases;

    static base::AOFLogger aof_logger;

    // replay 期间设为 true，防止重放的写命令被二次写入 AOF
    static bool replaying_aof;

    // 从 AOF 文件重放所有命令，返回成功执行的命令数
    static auto load_aof() -> std::size_t;

    static auto db(std::size_t index) -> Database&;
};

} // namespace xin::redis

#endif // XIN_REDIS_APPLICATION_CONTEXT_H

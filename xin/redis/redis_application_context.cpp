#include "redis_application_context.h"

#include <base_aof_logger.h>
#include <base_log.h>
#include <redis_command.h>
#include <redis_serialization_protocl.h>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <span>

namespace xin::redis {

std::array<Database, application_context::DB_COUNT> application_context::databases{};

application_context::PortType application_context::port = application_context::DEFAULT_PORT;

base::AOFLogger application_context::aof_logger{ application_context::AOF_FILE_PATH };

bool application_context::replaying_aof = false;

auto application_context::load_aof() -> std::size_t
{
    std::ifstream file{ std::string(AOF_FILE_PATH), std::ios::binary };
    if (!file.is_open())
        return 0;

    std::string content{ std::istreambuf_iterator<char>{ file }, std::istreambuf_iterator<char>{} };
    if (content.empty())
        return 0;

    replaying_aof = true;

    std::span<const char> buf{ content.data(), content.size() };
    RESPParser parser;
    std::size_t succeed_count = 0;
    std::size_t failed_count = 0;
    std::size_t index = 0;

    while (!buf.empty()) {
        auto result = parser.parse(buf);
        if (result) {
            commands::dispatch(index, *result);
            ++succeed_count;
            parser.reset();
        }
        else if (result.error() == RESPParser::Error::Waiting) {
            base::log::warning("AOF: 文件末尾有不完整的命令，已忽略");
            ++failed_count;
            break;
        }
        else {
            base::log::warning("AOF: 解析命令失败，终止重放（已执行 {} 条命令，失败 {} 条命令）",
                               succeed_count, failed_count);
            ++failed_count;
            break;
        }
    }

    replaying_aof = false;
    base::log::info("AOF: 重放完成，共执行 {} 条命令，失败 {} 条命令", succeed_count, failed_count);
    return succeed_count;
}

auto application_context::db(std::size_t index) -> Database& { return databases[index]; }

} // namespace xin::redis

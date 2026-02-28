#ifndef XIN_BASE_LOG_H
#define XIN_BASE_LOG_H

#include "base_formats.h"

#include <cstdint>
#include <string_view>

namespace xin::base {

enum class LogLevel : std::uint8_t { Trace, Debug, Info, Warning, Error, Critical };

class Logger {
public:
    Logger() = default;

    Logger(const Logger&) = delete;
    auto operator=(const Logger&) -> Logger& = delete;

    Logger(Logger&&) = default;
    auto operator=(Logger&&) -> Logger& = default;

    virtual ~Logger() = default;

    template<typename... Args>
    void trace(const std::string_view str, Args&&...args)
    {
        log(LogLevel::Trace, xformat(str, std::forward<Args>(args)...));
    }

    template<typename... Args>
    void debug(const std::string_view str, Args&&...args)
    {
        log(LogLevel::Debug, xformat(str, std::forward<Args>(args)...));
    }

    template<typename... Args>
    void info(const std::string_view str, Args&&...args)
    {
        log(LogLevel::Info, xformat(str, std::forward<Args>(args)...));
    }

    template<typename... Args>
    void warning(const std::string_view str, Args&&...args)
    {
        log(LogLevel::Warning, xformat(str, std::forward<Args>(args)...));
    }

    template<typename... Args>
    void error(const std::string_view str, Args&&...args)
    {
        log(LogLevel::Error, xformat(str, std::forward<Args>(args)...));
    }

    template<typename... Args>
    void critical(const std::string_view str, Args&&...args)
    {
        log(LogLevel::Critical, xformat(str, std::forward<Args>(args)...));
    }

    void set_level(LogLevel level) noexcept
    {
        level_ = level;
        set_level_impl(level);
    }

    [[nodiscard]]
    constexpr auto level() const noexcept -> LogLevel
    {
        return level_;
    }

private:
    LogLevel level_;

    virtual void log(LogLevel level, std::string_view message) = 0;

    virtual void set_level_impl(LogLevel level) noexcept {}
};

struct log {
    log() = delete;

    static void set_level(LogLevel level) { logger().set_level(level); }

    static auto level() noexcept -> LogLevel { return logger().level(); }

    static void set_default_logger(std::unique_ptr<Logger> logger)
    {
        default_logger = std::move(logger);
    }

    template<typename... Args>
    static void trace(const std::string_view str, Args&&...args)
    {
        logger().trace(str, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void debug(const std::string_view str, Args&&...args)
    {
        logger().debug(str, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void info(const std::string_view str, Args&&...args)
    {
        logger().info(str, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void warning(const std::string_view str, Args&&...args)
    {
        logger().warning(str, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void error(const std::string_view str, Args&&...args)
    {
        logger().error(str, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void critical(const std::string_view str, Args&&...args)
    {
        logger().critical(str, std::forward<Args>(args)...);
    }

private:
    static std::unique_ptr<Logger> default_logger;

    static auto logger() -> Logger& { return *default_logger; }
};

} // namespace xin::base

#endif // XIN_BASE_LOG_H
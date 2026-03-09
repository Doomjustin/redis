# Xin Redis

一个使用 Modern C++ (C++23) 实现的 Redis 风格服务器，定位是学习与工程实践项目，而不是 Redis 的完整替代。

项目当前具备：

- RESP 命令解析（支持增量解析与半包）
- 基于 `asio` 协程的 TCP 服务端
- 内存数据库（String / Hash / List / Sorted Set）
- TTL 过期、周期清理、`FLUSHDB ASYNC`
- AOF 追加写入与启动重放
- `doctest` + `ctest` 测试体系

## 快速开始

### 1. 环境要求

- CMake `>= 4.0`
- C++23 编译器
- vcpkg（用于依赖管理）

依赖声明见 `vcpkg.json`：

- `fmt`
- `spdlog`
- `doctest`
- `asio`
- `ms-gsl`

说明：构建脚本会尝试链接 `jemalloc`（可选，缺失时不影响核心功能）。

### 2. 配置与编译

```bash
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build
```

主要产物：

- 服务端：`build/app/xin-redis-server`
- 客户端占位程序：`build/app/xin-redis-cli`
- 测试可执行：`build/xin/xin.test`

### 3. 启动与连接

启动服务端（默认端口 `16379`）：

```bash
./build/app/xin-redis-server
```

连接测试：

```bash
redis-cli -p 16379
```

说明：`app/redis_app_client.cpp` 目前是占位实现，建议使用 `redis-cli`。

## 当前命令支持

按 `xin/redis/redis_command.cpp` 中注册表，当前已实现以下命令：

### Common

- `PING [message]`
- `MGET key [key ...]`
- `KEYS key [key ...]`
- `DBSIZE`
- `FLUSHDB [SYNC|ASYNC]`
- `EXPIRE key seconds`
- `TTL key`
- `PERSIST key`

### String

- `SET key value`
- `SET key value EX seconds`
- `GET key`

### Hash

- `HSET key field value [field value ...]`
- `HGET key field`
- `HGETALL key`

### List

- `LPUSH key element [element ...]`
- `LPOP key`
- `LRANGE key start stop`

### Sorted Set

- `ZADD key score member [score member ...]`
- `ZRANGE key start stop [WITHSCORES]`

## 协议与行为约定

- 命令名大小写不敏感（内部统一转小写分发）。
- 类型不匹配时返回：
`WRONGTYPE Operation against a key holding the wrong kind of value`
- `GET/HGET/LPOP` 未命中返回 Null Bulk String（`$-1\r\n`）。
- `MGET/KEYS/HGETALL/LRANGE/ZRANGE` 返回 RESP Array。
- `TTL` 行为：
- key 不存在时返回 `-2`
- key 存在但无过期时间时返回 `-1`

## 持久化（AOF）

- AOF 文件路径固定为当前工作目录下的 `appendonly.aof`。
- 服务端启动时会先执行 AOF 重放（`application_context::load_aof()`）。
- 写命令分发前会将原始 RESP 命令追加到 AOF。
- AOF 后台线程按容量阈值或定时（约 1s）刷盘。
- 重放期间会关闭二次写入，避免 AOF 自我追加。

## 架构说明

### 网络层

- `Server` 使用 `asio::io_context` 与协程 `co_spawn`。
- 每个连接由 `Session` 协程处理，循环读 socket、解析 RESP、分发命令、回写响应。

### 协议层

- `RESPParser` 采用状态机：`Start -> ArraySize -> BulkPrefix -> BulkSize -> BulkData`。
- 支持跨多次 `read_some` 的增量解析。

### 存储层

- 单进程内存数据库（当前 `db(index)` 返回同一个实例）。
- 过期时间结构与数据分离维护。
- 服务端额外运行过期清理协程，约每 `100ms` 清理一次到期 key。

### 数据结构

- String: `shared_ptr<string>`
- Hash: `unordered_map<string, shared_ptr<string>>`
- List: `list<shared_ptr<string>>`
- Sorted Set: `std::set` + 成员索引 map 的组合结构

## 测试

执行全部测试：

```bash
ctest --test-dir build --output-on-failure
```

或直接运行：

```bash
./build/xin/xin.test
```

测试入口定义在 `xin/unit_test.cpp`，具体用例分布在 `xin/base/*.test.cpp` 与 `xin/redis/*.test.cpp`。

## 目录结构

```text
app/         # 程序入口（server / cli）
xin/base/    # 基础设施：日志、格式化、AOF 等
xin/redis/   # 协议、命令、会话、存储、数据结构
cmake/       # CMake 辅助脚本
```

## 已知差异与限制

- 不是完整 Redis 实现，功能范围聚焦核心命令。
- 当前是单库模型（`db(index)` 未实现多 DB）。
- `KEYS` 目前是“输入 key 列表过滤存在项”，不是通配符模式匹配语义。
- 未实现事务、发布订阅、Lua、ACL、集群等特性。

## 开发路线建议

如果你计划继续扩展，优先级建议：

1. 补齐 `DEL/EXISTS/INCR` 等高频命令。
2. 增加 AOF rewrite 与恢复一致性检查。
3. 将 `KEYS` 扩展为 Redis 风格 pattern 匹配。
4. 细化性能基准和压测回归流程。

# Xin Redis（开发文档）

一个使用 Modern C++ (C++23) 实现的 Redis 风格服务器，定位是学习与工程实践项目，不是 Redis 的完整替代。

当前能力（按源码）：

- RESP 命令解析（支持增量解析与半包）
- 基于 `asio` 协程的 TCP 服务端
- 内存数据库（String / Hash / List / Sorted Set）
- TTL 过期、周期清理、`FLUSHDB ASYNC`
- AOF 追加写入与启动重放
- `doctest` + `ctest` 测试体系

## 当前压力测试

我在 `xin-doom.duckdns.org:16379` 放了 release 版本，以下结果使用手机热点网络测试。

```bash
redis-benchmark -h xin-doom.duckdns.org -p 16379 -t set -n 10000 -c 50

Summary:
  throughput summary: 686.29 requests per second
  latency summary (msec):
          avg       min       p50       p95       p99       max
       72.234    33.664    66.175   109.567   179.839   693.759
```

```bash
redis-benchmark -h xin-doom.duckdns.org -p 16379 -t set -n 10000 -c 50 -P 100

Summary:
  throughput summary: 5733.95 requests per second
  latency summary (msec):
          avg       min       p50       p95       p99       max
      326.397    54.976   215.423  1126.399  1160.191  1205.247
```

```bash
redis-benchmark -h xin-doom.duckdns.org -p 16379 -t set -n 100000 -c 50 -P 100

Summary:
  throughput summary: 20584.60 requests per second
  latency summary (msec):
          avg       min       p50       p95       p99       max
      179.219    57.248   170.879   270.591   327.935   378.367
```

本地测试：

```bash
redis-benchmark -h 127.0.0.1 -p 26379 -t set,get -n 1000000 -c 50 -P 100

====== SET ======
  1000000 requests completed in 8.19 seconds
  50 parallel clients
  3 bytes payload
  keep alive: 1
  multi-thread: no

Summary:
  throughput summary: 122040.52 requests per second
  latency summary (msec):
          avg       min       p50       p95       p99       max
        0.296     0.056     0.255     0.583     1.247     2.943

====== GET ======
  1000000 requests completed in 8.19 seconds
  50 parallel clients
  3 bytes payload
  keep alive: 1
  multi-thread: no

Summary:
  throughput summary: 122100.12 requests per second
  latency summary (msec):
          avg       min       p50       p95       p99       max
        0.250     0.048     0.215     0.511     0.847     2.431
```

## 快速开始

### 1. 环境要求

- CMake `>= 4.0`
- C++23 编译器
- vcpkg（依赖管理）

依赖见 `vcpkg.json`：

- `fmt`
- `spdlog`
- `doctest`
- `asio`
- `ms-gsl`

说明：构建会尝试链接 `jemalloc`（可选，未找到不影响核心功能）。

### 2. 配置与编译

先准备 `vcpkg`（若已安装可跳过）：

```bash
git clone https://github.com/microsoft/vcpkg.git
./vcpkg/bootstrap-vcpkg.sh
```

设置环境变量：

```bash
export VCPKG_ROOT=/path/to/vcpkg
```

Debug 构建：

```bash
cmake -S . -B build \
  -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake \
  -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

Release 构建：

```bash
cmake -S . -B build-release \
  -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build-release
```

主要产物：

- 服务端：`build/app/xin-redis-server`
- 客户端占位程序：`build/app/xin-redis-cli`
- 测试程序：`build/xin/xin.test`

### 3. 启动与连接

启动服务端：

```bash
./build/app/xin-redis-server
```

当前 `app/redis_app_server.cpp` 将端口写死为 `26379`。

使用 `redis-cli` 连接：

```bash
redis-cli -p 26379
```

建议在仓库根目录启动服务。AOF 默认写到当前工作目录下的 `appendonly.aof`。

说明：`app/redis_app_client.cpp` 当前是占位实现（直接返回成功），建议使用 `redis-cli`。

### 4. 快速验收

```bash
redis-cli -p 26379 PING
redis-cli -p 26379 SET k v EX 10
redis-cli -p 26379 GET k
redis-cli -p 26379 TTL k
redis-cli -p 26379 ZADD z 1 a 2 b
```

## 命令支持（按 `xin/redis/redis_command.cpp`）

### Common

- `PING [message]`
- `SELECT index`
- `DEL key [key ...]`
- `EXISTS key [key ...]`
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

## 协议与行为说明

- 命令名大小写不敏感（内部统一转小写分发）。
- `GET`/`HGET`/`LPOP` 未命中返回 Null Bulk String（`$-1\r\n`）。
- `MGET`/`KEYS`/`HGETALL`/`LRANGE`/`ZRANGE` 返回 RESP Array。
- `TTL`：key 不存在返回 `-2`，key 存在但无过期时间返回 `-1`。
- `FLUSHDB` 无参数时等价于 `FLUSHDB SYNC`。
- `EXPIRE` seconds 参数必须是整数；非法时返回 `ERR value is not an integer or out of range`。
- 类型不匹配时返回 `WRONGTYPE Operation against a key holding the wrong kind of value`。

## 持久化（AOF）

- AOF 文件路径固定为当前工作目录下的 `appendonly.aof`。
- 服务端启动时先执行 `application_context::load_aof()`，完成后再开始监听端口。
- 写命令会追加原始 RESP 命令到 AOF。
- 当写命令切换 DB 时，会先写入 `SELECT <index>`，再写写命令，保证重放时 DB 语义一致。
- AOF 后台线程按阈值或定时（约 1 秒）刷盘。
- 重放期间 `replaying_aof=true`，避免重放写入再次追加到 AOF。
- 如果 AOF 尾部是不完整命令，当前策略是忽略残缺尾部并继续启动。

## 测试

运行全部测试：

```bash
ctest --test-dir build --output-on-failure
```

或直接运行：

```bash
./build/xin/xin.test
```

如果使用 Release 目录：

```bash
ctest --test-dir build-release --output-on-failure
./build-release/xin/xin.test
```

测试入口在 `xin/unit_test.cpp`，用例分布在 `xin/base/*.test.cpp` 与 `xin/redis/*.test.cpp`。

## 目录结构

```text
app/         # 程序入口（server / cli）
xin/base/    # 基础设施：日志、格式化、AOF 等
xin/redis/   # 协议、命令、会话、存储、数据结构
cmake/       # CMake 辅助脚本
```

## 已知限制

- 当前不是完整 Redis 实现，功能范围聚焦核心命令。
- `KEYS` 当前不是 Redis 的通配符模式匹配，而是“按输入 key 列表过滤存在项”。
- `SET` 目前仅支持 `EX` 选项，不支持 `NX/XX/PX/KEEPTTL` 等扩展选项。
- `app/redis_app_server.cpp` 里端口是硬编码值（`26379`），尚未提供命令行或配置文件改端口能力。
- 未实现事务、发布订阅、Lua、ACL、集群等能力。

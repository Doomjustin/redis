# Xin Redis

`Xin Redis` 是一个基于 Modern C++ (C++23) 的 Redis 风格服务器实现，目标是学习与工程实践。

它强调：

- 可读、可测的核心实现
- RESP 协议解析与命令分发链路
- AOF 持久化与恢复

详细开发文档见：`docs/README.dev.md`

## 特性

- RESP 增量解析（支持半包）
- `asio` 协程网络模型
- 内存数据结构：String / Hash / List / Sorted Set
- TTL 与周期过期清理
- AOF 追加写入 + 启动重放
- `doctest` + `ctest` 测试

## 快速开始

### 1. 依赖

- CMake `>= 4.0`
- C++23 编译器
- vcpkg

`vcpkg.json` 依赖：`fmt`、`spdlog`、`doctest`、`asio`、`ms-gsl`。

### 2. 构建

```bash
export VCPKG_ROOT=/path/to/vcpkg

cmake -S . -B build \
  -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake \
  -DCMAKE_BUILD_TYPE=Debug

cmake --build build
```

### 3. 运行

```bash
./build/app/xin-redis-server
redis-cli -p 26379
```

说明：当前默认端口为 `26379`（在 `app/redis_app_server.cpp` 中硬编码）。

### 4. 验证

```bash
redis-cli -p 26379 PING
redis-cli -p 26379 SET k v EX 10
redis-cli -p 26379 GET k
redis-cli -p 26379 TTL k
redis-cli -p 26379 ZADD z 1 a 2 b
```

## 命令概览

- Common: `PING` `SELECT` `DEL` `EXISTS` `MGET` `KEYS` `DBSIZE` `FLUSHDB` `EXPIRE` `TTL` `PERSIST`
- String: `SET` `GET`
- Hash: `HSET` `HGET` `HGETALL`
- List: `LPUSH` `LPOP` `LRANGE`
- Sorted Set: `ZADD` `ZRANGE`

完整语义和边界行为见 `docs/README.dev.md`。

## 测试

```bash
ctest --test-dir build --output-on-failure
```

## 项目现状

这是一个聚焦核心能力的实现，不是完整 Redis 替代。

当前未覆盖：事务、发布订阅、Lua、ACL、集群等。

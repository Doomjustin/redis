# Xin Redis（Modern C++）

一个使用 Modern C++（C++23）实现的轻量 Redis 风格服务端。当前聚焦于：

- RESP 协议解析（支持增量解析/半包）
- 内存存储（String / Hash / List / Sorted Set）
- 常用命令分发与响应编码
- 过期时间（TTL）与 `FLUSHDB ASYNC`

> 目标是学习型/实验型实现，不是完整 Redis 替代品。

## 当前实现概览

- 网络层：`asio` 协程 TCP 服务端（默认端口 `16379`）
- 协议层：RESP 解析状态机（`RESPParser`）
- 存储层：进程内单库（`Database`），含过期时间管理
- 命令层：按模块分发（common/string/hash/list/sorted_set）
- 测试：`doctest` + `ctest`

## 依赖

项目通过 `vcpkg.json` 管理依赖：

- `fmt`
- `spdlog`
- `doctest`
- `asio`
- `ms-gsl`

## 构建

### 1) 准备 vcpkg toolchain

确保可用的 vcpkg，并拿到 toolchain 路径，例如：

`/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake`

### 2) 配置与编译

```bash
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build
```

产物（默认）：

- 服务端：`build/app/xin-redis-server`
- 测试程序：`build/xin/xin.test`

## 运行

启动服务端：

```bash
./build/app/xin-redis-server
```

使用 `redis-cli` 连接：

```bash
redis-cli -p 16379
```

> `app/redis_app_client.cpp` 当前是占位实现（直接返回成功），建议使用 `redis-cli` 测试。

## 命令支持矩阵

### 通用

- `PING [message]`
- `MGET key [key ...]`
- `KEYS key [key ...]`
	- 当前实现是“按输入 key 列表过滤存在项”，不是 Redis 的通配符匹配语义。
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

## RESP 与行为说明

- 命令名大小写不敏感（如 `PiNg` 可正常执行）。
- `GET/HGET/LPOP` 未命中返回 Null Bulk String（`$-1\r\n`）。
- `MGET/KEYS/HGETALL/LRANGE/ZRANGE` 返回 RESP Array。
- 类型不匹配返回：
	- `WRONGTYPE Operation against a key holding the wrong kind of value`
- `TTL` 语义：
	- key 不存在：`-2`
	- key 存在但无过期：`-1`

## 快速示例

```text
> SET name xin
+OK

> GET name
$3
xin

> EXPIRE name 10
:1

> TTL name
:9

> HSET user:100 name tom age 10
:2

> LRANGE mylist 0 -1
*0
```

## 测试

运行测试：

```bash
ctest --test-dir build --output-on-failure
```

或直接执行：

```bash
./build/xin/xin.test
```

## 项目结构

```text
app/            # 可执行程序（server / cli）
xin/base/       # 基础模块（日志、格式化、工具类等）
xin/redis/      # 协议、命令、响应、存储、数据结构与测试
cmake/          # CMake 辅助脚本
```

## 已知限制（与 Redis 的差异）

- 单进程内存存储，无 RDB/AOF 持久化
- 单 DB（`db(index)` 当前返回同一个实例）
- `KEYS` 非通配符模式匹配
- 未覆盖事务、发布订阅、Lua、ACL、集群等能力
- 当前 Sorted Set 采用 `std::set` + 索引结构，偏向可读性而非极致性能

## Benchmark（示例）

```bash
redis-benchmark -p 16379 -t set,get -n 1000000 -q -P 16
```

在我机器上的测试结果。
```bash
# redis
redis-benchmark -p 6379 -t set,get -n 1000000 -q -P 16
SET: 1438848.88 requests per second, p50=0.471 msec
GET: 1605136.38 requests per second, p50=0.423 msec

# 我的
redis-benchmark -p 16379 -t set,get -n 1000000 -q -P 16
WARNING: Could not fetch server CONFIG
SET: 1317523.00 requests per second, p50=0.607 msec
GET: 1280409.75 requests per second, p50=0.623 msec
```

## 后续
当前的性能测试结果还行，暂时不会考虑优化调优，依然以功能实现为主

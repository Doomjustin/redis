# Redis using Modern C++

用 Modern C++（C++23）实现的一个轻量 Redis 风格服务端，支持 RESP 协议解析、字符串与哈希结构、过期时间与常用命令。

## 功能特性

- 基于 `asio` 协程的 TCP 服务端。
- RESP 增量解析（支持半包场景）。
- 内存 KV 存储（String + Hash），支持过期时间（TTL）。
- 支持同步/异步清空数据库。
- 使用 `doctest` 编写单元测试。

## 依赖

项目通过 `vcpkg.json` 管理依赖：

- `fmt`
- `spdlog`
- `doctest`
- `asio`
- `ms-gsl`

## 构建

### 1) 准备依赖（vcpkg）

如果你使用 vcpkg，请先确保可用，并准备好 toolchain 文件路径。

### 2) 生成与编译

```bash
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build
```

## 运行

服务端默认监听端口 `16379`。

```bash
./build/app/xin-redis-server
```

可使用 `redis-cli` 验证（RESP 兼容）：

```bash
redis-cli -p 16379
```

## 已支持命令

- `PING [message]`
- `SET key value`
- `SET key value EX seconds`
- `GET key`
- `MGET key [key ...]`
- `KEYS key [key ...]`（当前实现按传入 key 列表过滤存在项，不考虑支持通配符）
- `EXPIRE key seconds`
- `TTL key`
- `PERSIST key`
- `DBSIZE`
- `FLUSHDB [SYNC|ASYNC]`
- `HSET key field value [field value ...]`
- `HGET key field`
- `HGETALL key`

## 响应语义说明

- `GET` 命中时返回单条 Bulk String：`$<len>\r\n<value>\r\n`
- `GET` 未命中返回 Null Bulk String：`$-1\r\n`
- `MGET/KEYS/HGETALL` 返回数组形式（RESP Array）：`*<n> ...`
- `HGET` 命中返回单条 Bulk String，未命中返回 Null Bulk String
- 类型不匹配时返回 `WRONGTYPE Operation against a key holding the wrong kind of value`

## Hash 示例

```text
> HSET user:100 name tom age 10
:2

> HGET user:100 name
$3
tom

> HGETALL user:100
1) "name"
2) "tom"
3) "age"
4) "10"
```

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
```

## 测试

运行单元测试：

```bash
ctest --test-dir build --output-on-failure
```

或直接执行测试程序：

```bash
./build/xin/xin.test
```

## 项目结构

```text
app/            # 可执行程序（server / cli）
xin/base/       # 基础工具模块
xin/redis/      # Redis 核心实现（协议、命令、存储、响应）
cmake/          # CMake 辅助脚本
```

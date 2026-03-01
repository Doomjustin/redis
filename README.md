# Redis using Modern C++
用modern C++重写Redis

## 当前效果
client 使用redis-cli
```bash
└> redis-cli -p 16379
127.0.0.1:16379> set 1
(error) ERR wrong number of arguments for 'set' command
127.0.0.1:16379> set 1 1
OK
127.0.0.1:16379> set 1 2
OK
127.0.0.1:16379> keys 1
1) "2"
127.0.0.1:16379> keys 2
(empty array)
127.0.0.1:16379> keys 1 2 3
1) "2"
127.0.0.1:16379> set 2 2
OK
127.0.0.1:16379> keys 1 2 3
1) "2"
2) "2"
127.0.0.1:16379> mget 1 2 3 4
1) "2"
2) "2"
3) ""
4) ""
127.0.0.1:16379> exit
```

server
```bash
└> /home/doom/code/redis/build/app/xin-redis-server
[2026-03-01 20:29:00.538] [info] Listening on port 16379
[2026-03-01 20:29:10.577] [info] Command: COMMAND
[2026-03-01 20:29:10.577] [info] Command: COMMAND
[2026-03-01 20:29:12.210] [info] Command: set
[2026-03-01 20:29:12.210] [error] SET command requires exactly 2 arguments, but got 1
[2026-03-01 20:29:14.433] [info] Command: set
[2026-03-01 20:29:14.433] [info] SET command executed with key: 1 and value: 1
[2026-03-01 20:29:16.165] [info] Command: set
[2026-03-01 20:29:16.165] [info] SET command executed with key: 1 and value: 2
[2026-03-01 20:29:19.850] [info] Command: keys
[2026-03-01 20:29:19.850] [info] KEYS command executed with pattern: ["1"]
[2026-03-01 20:29:19.850] [info] KEYS command executed with pattern: ["1"], found 1 keys
[2026-03-01 20:29:22.226] [info] Command: keys
[2026-03-01 20:29:22.226] [info] KEYS command executed with pattern: ["2"]
[2026-03-01 20:29:22.226] [info] KEYS command executed with pattern: ["2"], found 0 keys
[2026-03-01 20:29:29.210] [info] Command: keys
[2026-03-01 20:29:29.210] [info] KEYS command executed with pattern: ["1", "2", "3"]
[2026-03-01 20:29:29.210] [info] KEYS command executed with pattern: ["1", "2", "3"], found 1 keys
[2026-03-01 20:29:39.559] [info] Command: set
[2026-03-01 20:29:39.559] [info] SET command executed with key: 2 and value: 2
[2026-03-01 20:29:43.691] [info] Command: keys
[2026-03-01 20:29:43.691] [info] KEYS command executed with pattern: ["1", "2", "3"]
[2026-03-01 20:29:43.691] [info] KEYS command executed with pattern: ["1", "2", "3"], found 2 keys
[2026-03-01 20:29:46.966] [info] Command: mget
[2026-03-01 20:29:46.966] [info] MGET command executed with keys: ["1", "2", "3", "4"]
[2026-03-01 20:29:46.966] [info] MGET command executed with keys: ["1", "2", "3", "4"], found 2 values
[2026-03-01 20:29:48.934] [info] Client disconnected

```
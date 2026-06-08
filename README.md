# Viobot2 SDK Demo

Viobot2 SDK Demo 是一个 Linux CMake 示例工程，用于连接 Viobot/Baton 设备，控制 stereo 算法，并接收位姿数据。

English version: [README.md](README.md).

## 功能

- 通过 Viobot SDK 登录 Viobot/Baton 设备。
- 登录后通过 `Channel=1` 的 `vio_frame_sys_state` 查询当前算法状态。
- 保持后台 `Channel=1` 监听，使心跳输出能反映外部算法控制命令。
- 将心跳、算法状态和位姿频率合并为一行紧凑输出。
- 通过 `GET /Stream?Channel=1` 接收当前实时位姿帧。
- 通过 `/Algorithm/*/4` 启动、停止、重启 Stereo3。
- 通过 `/Algorithm/*/5` 启动、停止、重启 Stereo4。
- 启动 Stereo3 或 Stereo4 后通过 `Channel=1` 接收当前实时位姿。
- CMake 按 CPU 架构自动选择仓库内置的 Linux SDK 动态库。

## 接口概览

| 功能 | 接口 | 说明 |
| --- | --- | --- |
| 实时位姿流 | `GET /Stream?Channel=1` | 接收 `type=26` 位姿和速度帧 |
| 系统状态流 | `GET /Stream?Channel=1` | 接收 `type=8` 的 `vio_frame_sys_state` 状态帧 |
| Stereo3 控制 | `/Algorithm/enable/4`、`/disable/4`、`/reboot/4` | 通过 `net_vio_set_cfg` 发送 |
| Stereo4 控制 | `/Algorithm/enable/5`、`/disable/5`、`/reboot/5` | 通过 `net_vio_set_cfg` 发送 |

`vio_frame_sys_state` 使用 Baton 运行时 `system_status` 状态值：`0=ready`、`7=stereo3_initializing`、`8=stereo3_running`、`9=stereo4_initializing`、`10=stereo4_running`。部分旧运行时可能仍用 `51` 表示 `stereo4_running`，因此 demo 同时兼容 `10` 和 `51`。

实时位姿流中，位置单位为米，时间戳单位为秒，线上四元数顺序为 `x, y, z, w`。示例程序会把每个收到的 `type=26` 位姿帧都计入 `pose fps`，但位姿文本输出限频到 10Hz，每条打印出来的位姿 sample 占一行。


## 终端输出

- 位姿行使用绿色输出，最高 10Hz。
- 操作菜单和控制响应使用黄色输出。
- 心跳行使用白色输出，并包含 `Channel=1` 实际接收 FPS；该频率在位姿打印限频之前统计：

```text
algo system status: stereo4_running, heartbeat status: ok, pose fps: 24.9
```


## 支持的 Linux 架构

CMake 会根据 `CMAKE_SYSTEM_PROCESSOR` 自动选择 SDK 库目录：

| 处理器架构 | SDK 目录 |
| --- | --- |
| `x86_64`, `amd64` | `lib/linux/x86_64` |
| `i386`, `i686`, `x86` | `lib/linux/i386` |
| `aarch64`, `arm64` | `lib/linux/arm` |

`lib/linux/arm/libvio_sdk.so` 是 AArch64 64 位动态库。

## 依赖

- Linux
- GCC/G++
- CMake 3.10 或更高版本
- 能通过网络访问 Viobot/Baton 服务的 `8000` 端口

## 编译

```bash
cmake -S . -B build
cmake --build build -j
```

生成的可执行文件位于：

```bash
build/viobot_demo
```

## 运行

```bash
./build/viobot_demo
```

按提示输入设备 IP。登录成功后，示例程序会先连接一次 `Channel=1`，读取当前 `vio_frame_sys_state` 算法状态，然后启动后台 `Channel=1` 监听并进入操作菜单。心跳事件会合并为紧凑状态行输出，因此程序停在菜单时，外部 `/Algorithm/*` PUT 控制命令也会反映到心跳状态中。对于 Stereo3/Stereo4 菜单操作，示例程序只根据 `vio_frame_sys_state` 确认所选算法；位姿帧只用于位姿输出和 FPS 统计。

| 操作 | 说明 |
| --- | --- |
| `0` | 登出并退出 |
| `1` | 启动 Stereo3 并接收实时位姿 |
| `2` | 重启 Stereo3 并接收实时位姿 |
| `3` | 停止 Stereo3 |
| `4` | 启动 Stereo4 并接收实时位姿 |
| `5` | 重启 Stereo4 并接收实时位姿 |
| `6` | 停止 Stereo4 |

接收位姿时输入 `q` 并回车，可断开位姿流并回到数字菜单。

## 目录结构

```text
.
├── CMakeLists.txt
├── include/vio_sdk.h
├── include/viobot_demo
│   ├── algorithm_status.h
│   ├── channel1_stream.h
│   ├── console.h
│   ├── demo_app.h
│   └── pose.h
├── lib/linux
│   ├── arm/libvio_sdk.so
│   ├── i386/libvio_sdk.so
│   └── x86_64/libvio_sdk.so
├── README.md
├── README.zh-CN.md
└── src
    ├── algorithm_status.cpp
    ├── channel1_stream.cpp
    ├── console.cpp
    ├── demo_app.cpp
    ├── pose.cpp
    └── viobot_demo.cpp
```

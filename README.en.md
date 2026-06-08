# Viobot2 SDK Demo

Viobot2 SDK Demo is a Linux CMake example for connecting to a Viobot/Baton device, controlling stereo algorithms, and receiving pose data.


## Features

- Log in to a Viobot/Baton device through the Viobot SDK.
- Query the current algorithm status from `vio_frame_sys_state` on `Channel=1` after login.
- Keep a background `Channel=1` monitor running so heartbeat output reflects external algorithm control commands.
- Print heartbeat, algorithm status, and pose frequency in one compact line.
- Receive realtime `Channel=1` pose frames from `GET /Stream?Channel=1`.
- Start, stop, and reboot Stereo3 through `/Algorithm/*/4`.
- Start, stop, and reboot Stereo4 through `/Algorithm/*/5`.
- Receive the current realtime pose from `Channel=1` after starting Stereo3 or Stereo4.
- Select the bundled Linux SDK shared library automatically by CPU architecture.

## Interface Summary

| Feature | Interface | Notes |
| --- | --- | --- |
| Realtime pose stream | `GET /Stream?Channel=1` | Receives `type=26` pose and twist frames |
| System status stream | `GET /Stream?Channel=1` | Receives `type=8` `vio_frame_sys_state` frames |
| Stereo3 control | `/Algorithm/enable/4`, `/disable/4`, `/reboot/4` | Sent through `net_vio_set_cfg` |
| Stereo4 control | `/Algorithm/enable/5`, `/disable/5`, `/reboot/5` | Sent through `net_vio_set_cfg` |

`vio_frame_sys_state` follows the Baton runtime `system_status` values: `0=ready`, `7=stereo3_initializing`, `8=stereo3_running`, `9=stereo4_initializing`, and `10=stereo4_running`. Some older runtime builds may still report `51` for `stereo4_running`, so the demo accepts both `10` and `51` as Stereo4 running.

The realtime pose stream provides positions in meters, timestamps in seconds, and quaternions in `x, y, z, w` order on the wire. The demo counts every received `type=26` pose frame for `pose fps`, while pose text output is throttled to 10 Hz and each printed pose sample is one line.

This demo intentionally uses only the `Channel=1` VIO pose stream. `Channel=1` `type=26` frames do not contain a source identifier, so the demo does not infer Stereo3/Stereo4 ownership from pose frames. Algorithm status is reported from `vio_frame_sys_state`; `pose fps` only means the current `Channel=1` pose stream is being received.

## Terminal Output

- Pose lines are green and printed at up to 10 Hz.
- Operation menus and control responses are yellow.
- Heartbeat lines are white and include the actual `Channel=1` receive FPS, counted before pose print throttling:

```text
algo system status: stereo4_running, heartbeat status: ok, pose fps: 24.9
```

The SDK library has been verified against documented interfaces such as `/System/version`, `/System/hardware_status`, `/Baton/config`, `/System/network`, and `/Algorithm/disable/5` through `net_vio_get_cfg` / `net_vio_set_cfg`.

## Supported Linux Architectures

CMake maps `CMAKE_SYSTEM_PROCESSOR` to the SDK library directory:

| Processor | SDK directory |
| --- | --- |
| `x86_64`, `amd64` | `lib/linux/x86_64` |
| `i386`, `i686`, `x86` | `lib/linux/i386` |
| `aarch64`, `arm64` | `lib/linux/arm` |

`lib/linux/arm/libvio_sdk.so` is an AArch64 64-bit shared library.

## Requirements

- Linux
- GCC/G++
- CMake 3.10 or newer
- Network access to the Viobot/Baton service on port `8000`

## Build

```bash
cmake -S . -B build
cmake --build build -j
```

The executable is generated at:

```bash
build/viobot_demo
```

## Run

```bash
./build/viobot_demo
```

Enter the device IP address when prompted. After login, the demo connects `Channel=1` once to read the current `vio_frame_sys_state` algorithm status, then starts a background `Channel=1` monitor before entering the operation menu. Heartbeat events are printed as a compact status line, so external `/Algorithm/*` PUT commands are reflected in the heartbeat status while the demo is waiting at the menu. For Stereo3/Stereo4 menu actions, the demo confirms the selected algorithm only from `vio_frame_sys_state`; pose frames are used only for pose output and FPS counting.

| Operation | Description |
| --- | --- |
| `0` | Log out and exit |
| `1` | Start Stereo3 and receive realtime pose |
| `2` | Reboot Stereo3 and receive realtime pose |
| `3` | Stop Stereo3 |
| `4` | Start Stereo4 and receive realtime pose |
| `5` | Reboot Stereo4 and receive realtime pose |
| `6` | Stop Stereo4 |

When receiving pose data, input `q` and press Enter to disconnect the pose stream and return to the numeric menu.

## Repository Layout

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

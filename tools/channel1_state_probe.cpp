#include "vio_sdk.h"

#include <atomic>
#include <chrono>
#include <cstring>
#include <iostream>
#include <thread>

namespace {

std::atomic<int> g_connect_state(0);
std::atomic<unsigned int> g_pose_count(0);
std::atomic<unsigned int> g_status_count(0);
std::atomic<int> g_last_status(-1);
std::atomic<bool> g_heartbeat(false);

const char* status_name(int status) {
    switch (status) {
        case 0:
            return "ready";
        case 1:
            return "stereo1_initializing";
        case 2:
            return "stereo1_running";
        case 3:
            return "stereo2_initializing";
        case 4:
            return "stereo2_running";
        case 5:
            return "mono1_initializing";
        case 6:
            return "mono1_running";
        case 7:
            return "stereo3_initializing";
        case 8:
            return "stereo3_running";
        case 9:
            return "stereo4_initializing";
        case 10:
            return "stereo4_running";
        case 50:
            return "stereo4_waiting_or_initializing";
        case 51:
            return "stereo4_running";
        default:
            return "unknown";
    }
}

void vio_call connect_callback(int state, void*) {
    g_connect_state.store(state);
    std::cout << "connect_state=" << state << std::endl;
}

void vio_call event_callback(const char* data, int, void*) {
    if (data != nullptr && std::strstr(data, "Heartbeat") != nullptr) {
        g_heartbeat.store(true);
    }
}

void vio_call stream_callback(int,
                              const vio_frame_info_s* frame_info,
                              const char* frame_data,
                              void*) {
    if (frame_info == nullptr || frame_data == nullptr) {
        return;
    }

    if (frame_info->type == vio_frame_pose_and_twist) {
        g_pose_count.fetch_add(1);
    } else if (frame_info->type == vio_frame_sys_state && frame_info->length > 0) {
        const int status =
            static_cast<unsigned char>(frame_data[frame_info->length - 1]);
        g_last_status.store(status);
        const unsigned int count = g_status_count.fetch_add(1) + 1;
        std::cout << "sys_state[" << count << "]=" << status
                  << " (" << status_name(status) << ")" << std::endl;
    } else {
        std::cout << "other_frame_type=" << frame_info->type
                  << " length=" << frame_info->length << std::endl;
    }
}

}  // namespace

int main(int argc, char** argv) {
    const char* ip = argc > 1 ? argv[1] : "10.21.12.131";
    const int seconds = argc > 2 ? std::atoi(argv[2]) : 30;

    net_vio_sdk_init();

    vio_login_info_s login_info;
    std::memset(&login_info, 0, sizeof(login_info));
    std::strncpy(login_info.ipaddr, ip, sizeof(login_info.ipaddr) - 1);
    login_info.port = 8000;
    login_info.connect_cb = connect_callback;
    login_info.event_cb = event_callback;

    HANDLE login_handle = net_vio_login(login_info);
    if (login_handle == nullptr) {
        std::cout << "login failed" << std::endl;
        net_vio_sdk_exit();
        return 1;
    }

    HANDLE stream_handle = net_vio_stream_connect(login_handle, 1, stream_callback);
    if (stream_handle == nullptr) {
        std::cout << "stream connect failed" << std::endl;
        net_vio_logout(login_handle);
        net_vio_sdk_exit();
        return 1;
    }

    std::cout << "probing Channel=1 for " << seconds << " seconds" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(seconds));

    net_vio_stream_disconnect(stream_handle);
    net_vio_logout(login_handle);
    net_vio_sdk_exit();

    const int status = g_last_status.load();
    std::cout << "summary: heartbeat=" << (g_heartbeat.load() ? "ok" : "none")
              << ", pose_count=" << g_pose_count.load()
              << ", sys_state_count=" << g_status_count.load()
              << ", last_status=" << status
              << " (" << status_name(status) << ")" << std::endl;
    return 0;
}

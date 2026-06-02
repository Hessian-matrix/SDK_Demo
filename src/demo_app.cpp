#include "viobot_demo/demo_app.h"

#include "viobot_demo/channel1_stream.h"
#include "viobot_demo/console.h"

#include <cstring>
#include <iostream>
#include <thread>

namespace viobot_demo {
namespace {

constexpr int kPosePrintIntervalMs = 100;

constexpr const char* kStereo3Enable = "/Algorithm/enable/4";
constexpr const char* kStereo3Disable = "/Algorithm/disable/4";
constexpr const char* kStereo3Reboot = "/Algorithm/reboot/4";
constexpr const char* kStereo4Enable = "/Algorithm/enable/5";
constexpr const char* kStereo4Disable = "/Algorithm/disable/5";
constexpr const char* kStereo4Reboot = "/Algorithm/reboot/5";

enum OperationKind {
    operation_start,
    operation_reboot
};

struct StartOperation {
    const char* key;
    const char* uri;
    const char* fallback_start_uri;
    ActiveAlgorithm algorithm;
    OperationKind kind;
};

constexpr StartOperation kStartOperations[] = {
    {"1", kStereo3Enable, kStereo3Enable, active_algorithm_stereo3, operation_start},
    {"2", kStereo3Reboot, kStereo3Enable, active_algorithm_stereo3, operation_reboot},
    {"4", kStereo4Enable, kStereo4Enable, active_algorithm_stereo4, operation_start},
    {"5", kStereo4Reboot, kStereo4Enable, active_algorithm_stereo4, operation_reboot},
};

const char* disable_uri_for(ActiveAlgorithm algorithm) {
    if (algorithm == active_algorithm_stereo3) {
        return kStereo3Disable;
    }
    if (algorithm == active_algorithm_stereo4) {
        return kStereo4Disable;
    }
    return nullptr;
}

const char* algorithm_name(ActiveAlgorithm algorithm) {
    if (algorithm == active_algorithm_stereo3) {
        return "stereo3";
    }
    if (algorithm == active_algorithm_stereo4) {
        return "stereo4";
    }
    return "none";
}

ActiveAlgorithm active_algorithm_from_status(int status) {
    if (status == algo_status_stereo3_initializing ||
        status == algo_status_stereo3_running) {
        return active_algorithm_stereo3;
    }
    if (status == algo_status_stereo4_initializing ||
        status == algo_status_stereo4_running) {
        return active_algorithm_stereo4;
    }
    return active_algorithm_none;
}

}  // namespace

DemoApp::DemoApp()
    : login_handle_(nullptr),
      status_query_deadline_(std::chrono::steady_clock::now()),
      algo_status_(algo_status_unknown),
      active_algorithm_(active_algorithm_none),
      has_heartbeat_(false),
      suppress_heartbeat_output_(false),
      pose_receive_running_(false),
      pose_rate_started_(false),
      query_status_only_(false),
      status_monitor_running_(false),
      status_confirm_algorithm_(active_algorithm_none),
      pose_frame_count_(0),
      status_probe_pose_count_(0),
      status_probe_state_count_(0),
      pose_rate_start_(std::chrono::steady_clock::now()),
      last_pose_print_time_(std::chrono::steady_clock::now()) {}

void vio_call DemoApp::connect_callback(int state, void* user_data) {
    if (user_data != nullptr) {
        static_cast<DemoApp*>(user_data)->on_connect_state(state);
    }
}

void vio_call DemoApp::event_callback(const char* data, int, void* user_data) {
    if (user_data != nullptr) {
        static_cast<DemoApp*>(user_data)->on_event(data);
    }
}

void DemoApp::handle_channel1_frame(unsigned int frame_type,
                                    unsigned int frame_length,
                                    const char* frame_data,
                                    void* user_data) {
    if (user_data != nullptr) {
        static_cast<DemoApp*>(user_data)->on_channel1_frame(
            frame_type, frame_length, frame_data);
    }
}

void DemoApp::handle_status_monitor_frame(unsigned int frame_type,
                                          unsigned int frame_length,
                                          const char* frame_data,
                                          void* user_data) {
    if (user_data != nullptr) {
        static_cast<DemoApp*>(user_data)->on_status_monitor_frame(
            frame_type, frame_length, frame_data);
    }
}

bool DemoApp::should_continue_status_query(void* user_data) {
    DemoApp* app = static_cast<DemoApp*>(user_data);
    return app != nullptr && app->query_status_only_.load() &&
           std::chrono::steady_clock::now() < app->status_query_deadline_ &&
           app->status_probe_state_count_.load() == 0;
}

bool DemoApp::should_continue_status_confirm(void* user_data) {
    DemoApp* app = static_cast<DemoApp*>(user_data);
    if (app == nullptr || !app->query_status_only_.load() ||
        std::chrono::steady_clock::now() >= app->status_query_deadline_) {
        return false;
    }
    const ActiveAlgorithm algorithm =
        static_cast<ActiveAlgorithm>(app->status_confirm_algorithm_.load());
    return !is_algorithm_status(algorithm, app->algo_status_.load());
}

bool DemoApp::should_continue_pose_receive(void* user_data) {
    DemoApp* app = static_cast<DemoApp*>(user_data);
    return app != nullptr && app->pose_receive_running_.load();
}

bool DemoApp::should_continue_status_monitor(void* user_data) {
    DemoApp* app = static_cast<DemoApp*>(user_data);
    return app != nullptr && app->status_monitor_running_.load();
}

void DemoApp::on_connect_state(int state) {
    switch (state) {
        case 1:
            std::cout << "connect ok" << std::endl;
            return;
        case 0:
            std::cout << "disconnected" << std::endl;
            return;
        case -1:
            std::cout << "connect error" << std::endl;
            return;
        case -2:
            std::cout << "keepalive thread exit" << std::endl;
            return;
        default:
            std::cout << "unknown connection state: " << state << std::endl;
    }
}

void DemoApp::on_event(const char* data) {
    if (data != nullptr && std::strstr(data, "Heartbeat") != nullptr) {
        has_heartbeat_.store(true);
        if (!suppress_heartbeat_output_.load()) {
            print_current_heartbeat_line();
        }
    }
}

void DemoApp::on_channel1_frame(unsigned int frame_type,
                                unsigned int frame_length,
                                const char* frame_data) {
    if (frame_data == nullptr) {
        return;
    }

    if (frame_type == vio_frame_pose_and_twist) {
        if (query_status_only_.load()) {
            status_probe_pose_count_.fetch_add(1);
            return;
        }
        on_pose_payload(static_cast<int>(frame_length), frame_data);
    } else if (frame_type == vio_frame_sys_state) {
        update_system_status(frame_data, frame_length);
        status_probe_state_count_.fetch_add(1);
    }
}

void DemoApp::on_status_monitor_frame(unsigned int frame_type,
                                      unsigned int frame_length,
                                      const char* frame_data) {
    if (frame_data == nullptr) {
        return;
    }
    if (frame_type == vio_frame_sys_state) {
        update_system_status(frame_data, frame_length);
    } else if (frame_type == vio_frame_pose_and_twist &&
               !pose_receive_running_.load()) {
        count_pose_frame();
    }
}

void DemoApp::update_system_status(const char* frame_data,
                                   unsigned int frame_length) {
    if (frame_data == nullptr || frame_length == 0) {
        return;
    }
    const int status = static_cast<unsigned char>(frame_data[frame_length - 1]);
    algo_status_.store(normalize_algorithm_status(status));
}

void DemoApp::count_pose_frame() {
    if (!pose_rate_started_.exchange(true)) {
        std::lock_guard<std::mutex> lock(pose_rate_mutex_);
        pose_frame_count_.store(0);
        pose_rate_start_ = std::chrono::steady_clock::now();
    }
    pose_frame_count_.fetch_add(1);
}

void DemoApp::on_pose_payload(int length, const char* frame_data) {
    int offset = 0;
    PoseAndTwistSample sample;
    while (read_pose_and_twist_sample(frame_data, length, &offset, &sample)) {
        count_pose_frame();

        if (should_print_pose_sample()) {
            print_pose_and_twist(sample);
        }
    }
}

bool DemoApp::login() {
    vio_login_info_s login_info;
    std::memset(&login_info, 0, sizeof(login_info));
    std::strncpy(login_info.ipaddr, device_ip_.c_str(), sizeof(login_info.ipaddr) - 1);
    login_info.port = kDevicePort;
    login_info.userData = this;
    login_info.connect_cb = connect_callback;
    login_info.event_cb = event_callback;

    login_handle_ = net_vio_login(login_info);
    if (login_handle_ == nullptr) {
        std::cout << "login fail" << std::endl;
        return false;
    }

    std::cout << "login success" << std::endl;
    return true;
}

void DemoApp::logout() {
    stop_status_monitor();
    if (login_handle_ != nullptr) {
        net_vio_logout(login_handle_);
        login_handle_ = nullptr;
    }
}

bool DemoApp::handle_operation(const std::string& operation) {
    if (operation == "0") {
        return false;
    }
    if (operation == "3") {
        stop_algorithm(kStereo3Disable);
    } else if (operation == "6") {
        stop_algorithm(kStereo4Disable);
    } else {
        bool handled = false;
        for (const auto& item : kStartOperations) {
            if (operation == item.key) {
                handled = true;
                if (start_or_reboot_algorithm(
                        item.uri,
                        item.fallback_start_uri,
                        item.algorithm,
                        item.kind == operation_reboot)) {
                    receive_pose_until_q();
                }
                break;
            }
        }
        if (!handled) {
            std::cout << kColorYellow << "unknown operation: " << operation
                      << kColorReset << std::endl;
        }
    }

    print_current_menu_status_line();
    print_menu();
    return true;
}

bool DemoApp::send_algorithm_command(const char* uri) {
    char buffer[] = "{}";
    const BOOL ret = net_vio_set_cfg(login_handle_, uri, buffer, std::strlen(buffer));
    std::cout << kColorYellow << uri << " ret: " << ret << kColorReset << std::endl;
    return ret == 1;
}

bool DemoApp::start_or_reboot_algorithm(const char* uri,
                                        const char* fallback_start_uri,
                                        ActiveAlgorithm algorithm,
                                        bool reboot) {
    query_initial_system_status();
    const ActiveAlgorithm current_algorithm =
        active_algorithm_from_status(algo_status_.load());
    if (reboot && current_algorithm != algorithm) {
        uri = fallback_start_uri;
    }
    if (!reboot && current_algorithm == algorithm) {
        std::cout << kColorYellow << algorithm_name(algorithm)
                  << " is already active" << kColorReset << std::endl;
        return true;
    }
    if (current_algorithm != active_algorithm_none) {
        const char* disable_uri = disable_uri_for(current_algorithm);
        std::cout << kColorYellow << "switching from "
                  << algorithm_name(current_algorithm) << " to "
                  << algorithm_name(algorithm) << kColorReset << std::endl;
        if (disable_uri == nullptr || !send_algorithm_command(disable_uri)) {
            return false;
        }
        if (!wait_for_algorithm_status(active_algorithm_none,
                                       std::chrono::seconds(8))) {
            std::cout << kColorYellow
                      << "cannot switch algorithm, device status is "
                      << algo_status_name() << kColorReset << std::endl;
            return false;
        }
    }

    if (!send_algorithm_command(uri)) {
        return false;
    }

    if (!wait_for_algorithm_status(algorithm, std::chrono::seconds(8))) {
        std::cout << kColorYellow
                  << "algorithm command accepted, but device status is "
                  << algo_status_name() << kColorReset << std::endl;
        return false;
    }
    active_algorithm_.store(algorithm);
    print_current_menu_status_line();
    return true;
}

void DemoApp::stop_algorithm(const char* uri) {
    if (send_algorithm_command(uri)) {
        active_algorithm_.store(active_algorithm_none);
        query_initial_system_status();
        print_current_menu_status_line();
    }
}

bool DemoApp::query_system_status_for(std::chrono::seconds timeout,
                                      bool stop_on_first_state) {
    algo_status_.store(algo_status_unknown);
    status_probe_pose_count_.store(0);
    status_probe_state_count_.store(0);
    suppress_heartbeat_output_.store(true);
    query_status_only_.store(true);

    Channel1Stream stream;
    if (!stream.connect(device_ip_)) {
        suppress_heartbeat_output_.store(false);
        query_status_only_.store(false);
        return false;
    }

    std::cout << kColorYellow << "querying system status from Channel=1 ..."
              << kColorReset << std::endl;
    status_query_deadline_ = std::chrono::steady_clock::now() + timeout;
    stream.read_loop(handle_channel1_frame,
                     stop_on_first_state ? should_continue_status_query
                                         : should_continue_status_confirm,
                     this);
    if (algo_status_.load() == algo_status_unknown) {
        std::cout << kColorYellow
                  << "system status query timeout: no vio_frame_sys_state received"
                  << kColorReset << std::endl;
    }
    std::cout << kColorYellow
              << "status probe summary: pose_frames="
              << status_probe_pose_count_.load()
              << ", sys_state_frames=" << status_probe_state_count_.load()
              << kColorReset << std::endl;
    query_status_only_.store(false);
    suppress_heartbeat_output_.store(false);
    return algo_status_.load() != algo_status_unknown;
}

void DemoApp::query_initial_system_status() {
    status_confirm_algorithm_.store(active_algorithm_none);
    query_system_status_for(std::chrono::seconds(30), true);
}

bool DemoApp::wait_for_algorithm_status(ActiveAlgorithm algorithm,
                                        std::chrono::seconds timeout) {
    status_confirm_algorithm_.store(algorithm);
    query_system_status_for(timeout, false);
    status_confirm_algorithm_.store(active_algorithm_none);
    return is_algorithm_status(algorithm, algo_status_.load());
}

void DemoApp::start_status_monitor() {
    if (status_monitor_running_.exchange(true)) {
        return;
    }
    status_monitor_thread_ = std::thread(&DemoApp::status_monitor_loop, this);
}

void DemoApp::stop_status_monitor() {
    status_monitor_running_.store(false);
    if (status_monitor_thread_.joinable()) {
        status_monitor_thread_.join();
    }
}

void DemoApp::status_monitor_loop() {
    while (status_monitor_running_.load()) {
        Channel1Stream stream;
        if (stream.connect(device_ip_)) {
            stream.read_loop(handle_status_monitor_frame,
                             should_continue_status_monitor,
                             this);
        }
        if (status_monitor_running_.load()) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
}

void DemoApp::pose_receive_input_loop() {
    std::string input;
    while (pose_receive_running_.load() && std::cin >> input) {
        if (input == "q" || input == "Q") {
            pose_receive_running_.store(false);
            pose_receive_cv_.notify_one();
            break;
        }
        std::cout << kColorYellow << "receiving pose, input q to return menu"
                  << kColorReset << std::endl;
    }
}

void DemoApp::receive_pose_until_q() {
    Channel1Stream stream;
    if (!stream.connect(device_ip_)) {
        return;
    }

    query_status_only_.store(false);
    pose_receive_running_.store(true);
    reset_pose_frequency_counter();
    reset_pose_print_timer();
    std::cout << kColorYellow
              << "receiving realtime pose, input q to return menu"
              << kColorReset << std::endl;

    std::thread input_thread(&DemoApp::pose_receive_input_loop, this);
    std::thread stream_thread([this, &stream] {
        stream.read_loop(handle_channel1_frame, should_continue_pose_receive, this);
        pose_receive_running_.store(false);
        pose_receive_cv_.notify_one();
    });

    {
        std::unique_lock<std::mutex> lock(pose_receive_mutex_);
        pose_receive_cv_.wait(lock, [this] { return !pose_receive_running_.load(); });
    }

    stream.shutdown_and_close();
    if (input_thread.joinable()) {
        input_thread.join();
    }
    if (stream_thread.joinable()) {
        stream_thread.join();
    }
}

const char* DemoApp::heartbeat_status() const {
    return has_heartbeat_.load() ? "ok" : "unknown";
}

const char* DemoApp::algo_status_name() const {
    return system_status_name(algo_status_.load());
}

bool DemoApp::should_print_pose_sample() {
    std::lock_guard<std::mutex> lock(pose_print_mutex_);
    const auto now = std::chrono::steady_clock::now();
    const auto elapsed_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            now - last_pose_print_time_)
            .count();
    if (elapsed_ms < kPosePrintIntervalMs) {
        return false;
    }
    last_pose_print_time_ = now;
    return true;
}

void DemoApp::reset_pose_print_timer() {
    std::lock_guard<std::mutex> lock(pose_print_mutex_);
    last_pose_print_time_ = std::chrono::steady_clock::now() -
                            std::chrono::milliseconds(kPosePrintIntervalMs);
}

void DemoApp::reset_pose_frequency_counter() {
    std::lock_guard<std::mutex> lock(pose_rate_mutex_);
    pose_frame_count_.store(0);
    pose_rate_started_.store(false);
    pose_rate_start_ = std::chrono::steady_clock::now();
}

double DemoApp::consume_pose_frequency_hz() {
    std::lock_guard<std::mutex> lock(pose_rate_mutex_);
    const auto now = std::chrono::steady_clock::now();
    const double dt_s = std::chrono::duration<double>(now - pose_rate_start_).count();
    const unsigned int count = pose_frame_count_.exchange(0);
    pose_rate_start_ = now;
    if (dt_s <= 0.0) {
        return 0.0;
    }
    return static_cast<double>(count) / dt_s;
}

void DemoApp::print_current_heartbeat_line() {
    print_heartbeat_line(algo_status_name(), heartbeat_status(), consume_pose_frequency_hz());
}

void DemoApp::print_current_menu_status_line() {
    print_menu_status_line(algo_status_name(), heartbeat_status());
}

int DemoApp::run() {
    std::cout << "sdk version: 0x" << std::hex << net_vio_sdk_version()
              << std::dec << std::endl;
    net_vio_sdk_init();

    std::cout << "Please input device ip: ";
    std::cin >> device_ip_;

    if (!login()) {
        net_vio_sdk_exit();
        return 0;
    }

    query_initial_system_status();
    start_status_monitor();
    print_current_menu_status_line();
    print_menu();

    std::string operation;
    while (std::cin >> operation && handle_operation(operation)) {}

    logout();
    net_vio_sdk_exit();
    std::cout << "ByeBye" << std::endl;
    return 0;
}

}  // namespace viobot_demo

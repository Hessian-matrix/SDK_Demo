#include "viobot_demo/console.h"

#include <iostream>

namespace viobot_demo {

const char* const kColorReset = "\033[0m";
const char* const kColorGreen = "\033[32m";
const char* const kColorYellow = "\033[33m";
const char* const kColorWhite = "\033[37m";

void print_heartbeat_line(const char* algo_status, const char* heartbeat_status, double pose_hz) {
    std::cout << kColorWhite << "algo system status: " << algo_status
              << ", heartbeat status: " << heartbeat_status
              << ", pose fps: " << pose_hz << kColorReset << std::endl;
}

void print_menu_status_line(const char* algo_status, const char* heartbeat_status) {
    std::cout << kColorYellow << "algo system status: " << algo_status
              << ", heartbeat status: " << heartbeat_status
              << kColorReset << std::endl;
}

void print_pose_and_twist(const PoseAndTwistSample& sample) {
    const Vector3d euler = quat_to_euler_angles(sample.quaternion);
    std::cout << kColorGreen
              << "pose position=(" << sample.position.x << ","
              << sample.position.y << "," << sample.position.z << ")"
              << " quaternion_xyzw=(" << sample.quaternion.x << ","
              << sample.quaternion.y << "," << sample.quaternion.z << ","
              << sample.quaternion.w << ")"
              << " euler_rad=(" << euler.x << "," << euler.y << ","
              << euler.z << ")"
              << " linear=(" << sample.linear_velocity.x << ","
              << sample.linear_velocity.y << "," << sample.linear_velocity.z << ")"
              << " angular=(" << sample.angular_velocity.x << ","
              << sample.angular_velocity.y << "," << sample.angular_velocity.z << ")"
              << kColorReset << std::endl;
}

void print_menu() {
    std::cout << kColorYellow << "\nOperations:" << std::endl;
    std::cout << "  0: logout and exit" << std::endl;
    std::cout << "  1: start stereo3 and receive realtime pose" << std::endl;
    std::cout << "  2: reboot stereo3 and receive realtime pose" << std::endl;
    std::cout << "  3: stop stereo3" << std::endl;
    std::cout << "  4: start stereo4 and receive realtime pose" << std::endl;
    std::cout << "  5: reboot stereo4 and receive realtime pose" << std::endl;
    std::cout << "  6: stop stereo4" << std::endl;
    std::cout << "Please input operation: " << kColorReset;
}

}  // namespace viobot_demo

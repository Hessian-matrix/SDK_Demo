#include "viobot_demo/pose.h"

#include <cmath>
#include <cstring>

namespace viobot_demo {
namespace {

constexpr int kPoseAndTwistSize = 52;

#ifndef M_PI
constexpr double M_PI = 3.14159265358979323846;
#endif

template <typename T>
T read_le_value(const char* data) {
    T value;
    std::memcpy(&value, data, sizeof(T));
    return value;
}

Vector3d read_vector3f(const char* data, int* offset) {
    Vector3d value = {
        read_le_value<float>(data + *offset),
        read_le_value<float>(data + *offset + 4),
        read_le_value<float>(data + *offset + 8)};
    *offset += 12;
    return value;
}

Quaterniond read_quaternion_xyzw(const char* data, int* offset) {
    const float qx = read_le_value<float>(data + *offset);
    const float qy = read_le_value<float>(data + *offset + 4);
    const float qz = read_le_value<float>(data + *offset + 8);
    const float qw = read_le_value<float>(data + *offset + 12);
    *offset += 16;
    return Quaterniond{qw, qx, qy, qz};
}

}  // namespace

Vector3d quat_to_euler_angles(const Quaterniond& quat) {
    Vector3d angles;

    const double sinr_cosp = 2.0 * (quat.w * quat.x + quat.y * quat.z);
    const double cosr_cosp = 1.0 - 2.0 * (quat.x * quat.x + quat.y * quat.y);
    angles.x = std::atan2(sinr_cosp, cosr_cosp);

    const double sinp = 2.0 * (quat.w * quat.y - quat.z * quat.x);
    if (std::abs(sinp) >= 1.0) {
        angles.y = std::copysign(M_PI / 2.0, sinp);
    } else {
        angles.y = std::asin(sinp);
    }

    const double siny_cosp = 2.0 * (quat.w * quat.z + quat.x * quat.y);
    const double cosy_cosp = 1.0 - 2.0 * (quat.y * quat.y + quat.z * quat.z);
    angles.z = std::atan2(siny_cosp, cosy_cosp);
    return angles;
}

bool read_pose_and_twist_sample(const char* frame_data,
                                int length,
                                int* offset,
                                PoseAndTwistSample* sample) {
    if (frame_data == nullptr || offset == nullptr || sample == nullptr) {
        return false;
    }
    if (*offset + kPoseAndTwistSize > length) {
        return false;
    }

    sample->position = read_vector3f(frame_data, offset);
    sample->quaternion = read_quaternion_xyzw(frame_data, offset);
    sample->linear_velocity = read_vector3f(frame_data, offset);
    sample->angular_velocity = read_vector3f(frame_data, offset);
    return true;
}

}  // namespace viobot_demo

#include <iostream>
#include <cstring>
#include <string>

using namespace std;
#include "vio_sdk.h"

// 根据操作系统选择不同的 sleep 函数
#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#define sleep(x) Sleep(x * 1000)// 将秒转换为毫秒
#else
#include <unistd.h>
#endif

#define cfg_max     1024
//可通过操作1测试
#define cfg_version "/System/version"//硬件版本
#define cfg_network "/System/network"//网络信息
#define cfg_viobot  "/Viobot/config"
#define cfg_stereo2 "/Stereo2/config"
#define cfg_loop    "/loop/config"
#define cfg_user    "/User/config"
#define ctrl_smart_status   "/Config/smart"
//可通过操作2测试
//可直接用net_vio_set_cfg
#define system_shutdown     "/System/shutdown"
#define system_savekeyframe "/Smart/saveKeyFrame"
#define system_addkeyframe  "/Smart/addKeyFrame"
#define system_reboot       "/System/reboot"
//用net_vio_set_cfg 得加算法数字 例如
#define algorithm_enable    "/Algorithm/enable/2" ///Algorithm/enable/2
#define algorithm_disable   "/Algorithm/disable/2"///Algorithm/disable/2
#define algorithm_reboot    "/Algorithm/reboot/2"///Algorithm/reboot/2
#define algorithm_reset     "/Algorithm/reset/2"///Algorithm/reset/2

typedef enum {
    ready = 0,
    stereo1_initializing,
    stereo1_running,
    stereo2_initializing,
    stereo2_running,
    mono1_initializing,
    mono1_running,
    stereo3_initializing,
    stereo3_running
}system_status;

system_status sys_status = ready;//define the status of system
#define M_PI (3.14159265358979323846)
union float_byte {
    float d_float;
    char d_char[4];
};

struct QVector4D {
    double w, x, y, z;
};

struct QVector3D {
    double x, y, z;
};

QVector3D byte2Point(char* str, int* start, int length){
    int num = *start;
    float_byte data_charge[3];
    memset(data_charge, 0x00, 3 * 4);
    if (length >= 3 * 4)
    {
        for (int j = 0; j < 3; j++)
        {
            for (int k = 0; k < 4; k++)
            {
                data_charge[j].d_char[k] = str[num++];
            }
        }
    }
    *start = num;
    return QVector3D{ data_charge[0].d_float,data_charge[1].d_float,data_charge[2].d_float };
}

QVector4D byte2Quat(char* str, int* start, int length){
    int num = *start;
    QVector4D* quat;
    float_byte data_charge[4];
    memset(data_charge, 0x00, 4 * 4);
    if (length >= 4 * 4)
    {
        for (int j = 0; j < 4; j++)
        {
            for (int k = 0; k < 4; k++)
            {
                data_charge[j].d_char[k] = str[num++];
            }
        }
    }
    *start = num;
    return QVector4D{ data_charge[0].d_float ,data_charge[1].d_float ,data_charge[2].d_float,data_charge[3].d_float };
}

QVector3D quat2EulerAngles(QVector4D quat){
    QVector3D angles;

    // roll (x-axis rotation)
    double sinr_cosp = 2 * (quat.w * quat.x + quat.y * quat.z);
    double cosr_cosp = 1 - 2 * (quat.x * quat.x + quat.y * quat.y);
    angles.x = (std::atan2(sinr_cosp, cosr_cosp));

    // pitch (y-axis rotation)
    double sinp = 2 * (quat.w * quat.y - quat.z * quat.x);
    if (std::abs(sinp) >= 1)
        angles.y = (std::copysign(M_PI / 2, sinp)); // use 90 degrees if out of range
    else
        angles.y = (std::asin(sinp));

    // yaw (z-axis rotation)
    double siny_cosp = 2 * (quat.w * quat.z + quat.x * quat.y);
    double cosy_cosp = 1 - 2 * (quat.y * quat.y + quat.z * quat.z);
    angles.z = (std::atan2(siny_cosp, cosy_cosp));
    return angles;
}

void vio_call connect_callback(int state, void* userData){
    if (state == 1)
        printf("connect ok !\n");
    else if (state == 0)
        printf("disconnected !\n");
    else if(state == -1)
        printf("connect error !\n");
    else if (state == -2)
        printf("keepalive thread exit !\n");
    else
        printf("unknown !\n");
}

void vio_call event_callback(const char* data, int length, void* userData){
    printf("event_callback:%s",data);
    printf("\n");
}

void pose_data_print(int length, const char* frameData) {
    printf("pose_data:\n");
    int st = 0;
    int pointLen = 3 * 4;
    int quatLen = 4 * 4;
    QVector3D pt;
    QVector4D quat;
    QVector3D angles;
    while (st <= (length - pointLen - quatLen)){
        pt = byte2Point((char*)frameData, &st, length);
        std::cout << "\t Position:" << pt.x << "," << pt.y << "," << pt.z << "\n";
        quat = byte2Quat((char*)frameData, &st, length);
        std::cout << "\t Quaternion:" << quat.x << "," << quat.y << "," << quat.z << "," << quat.w << "\n";
        angles = quat2EulerAngles(quat);
        std::cout << "\t Euler_angles:" << angles.x << "," << angles.y << "," << angles.z << "\n";
    }
}

void tiwst_data_print(int length,const char* frameData) {
    printf("twist_data:\n");
    QVector3D pt;
    int st = 0;
    int pointLen = 3 * 4;
    while (st <= (length - pointLen)){//linear.x、linear.y、linear.z、angular.x、angular.y、angular.z
        pt = byte2Point((char*)frameData, &st, length);
        std::cout << pt.x << "," << pt.y << "," << pt.z << "\n";
    }
}

void print_frame_info(int channel, const vio_frame_info_s* frameInfo) {
    printf("channel:%d\n", channel);
    printf("type:%u\n", frameInfo->type);
    printf("timestamp:%f\n", frameInfo->timestamp);
    printf("seq:%u\n", frameInfo->seq);
    printf("width:%u\n", frameInfo->width);
    printf("height:%u\n", frameInfo->height);
    printf("length:%u\n", frameInfo->length);
}

void vio_call stream_callback(int channel, const vio_frame_info_s* frameInfo, const char* frameData, void* userData){
    if (frameInfo->type == vio_frame_loop_pose) {
        print_frame_info(channel, frameInfo);
        pose_data_print(frameInfo->length,frameData);
    }
    else if (frameInfo->type == vio_frame_twist) {
        print_frame_info(channel, frameInfo);
        tiwst_data_print(frameInfo->length,frameData);
    }
    else if (frameInfo->type == vio_frame_sys_state) {//get the status of system
        print_frame_info(channel, frameInfo);
        //printf("sys_status:%d\n", frameData[frameInfo->length - 1]);
        sys_status = static_cast<system_status>(frameData[frameInfo->length - 1]);
    }
    else
        return;
    printf("==================\n");
}

vio_login_info_s login_info = {
    "10.21.10.21",
    8000,
    "",
    "",
    NULL,
    connect_callback,
    event_callback
};

int main(int argc, char* argv[]){
    int ret;
    HANDLE streamHandle1 = NULL;
    printf("sdk version : 0x%08lx\n", net_vio_sdk_version());
    net_vio_sdk_init();

    cout << "Please input device ip : ";
    cin >> login_info.ipaddr;

    HANDLE loginHandle = net_vio_login(login_info);
    if (!loginHandle){
        cout << "login fail\n";
        net_vio_sdk_exit();
        cout << "ByeBye\n";
        return 0;
    }
    std::cout << "login success\n";
    cout << "Please input the operation[0-3] : ";   

    int v;
    while (cin >> v){
        if (v == 0) {
            net_vio_logout(loginHandle);
            break;
        }
        else if (v == 1) {
            if (streamHandle1 == NULL) {//Establish a connection for flow channel 1
                streamHandle1 = net_vio_stream_connect(loginHandle, 1, stream_callback);
            }
            else {//disconnect
                net_vio_stream_disconnect(streamHandle1);
                streamHandle1 = NULL;
            }
        }
        else if (v == 2){//start or stop
            char buffer[] = "{}";
            if (sys_status == ready) {//The system is currently in a ready state and can be started directly
                ret = net_vio_set_cfg(loginHandle, "/Algorithm/enable/2", buffer, strlen(buffer));
            }
            else if (sys_status == stereo2_running) {//The system is currently in a running state and can be stoped
                ret = net_vio_set_cfg(loginHandle, "/Algorithm/disable/2", buffer, strlen(buffer));
            }
        }
        else if (v == 3) {//algo restart
            char buffer[]="{}";
            if (sys_status == stereo2_running) {//The stereo2 algorithm is currently in a running state and can be restart
                ret = net_vio_set_cfg(loginHandle, "/Algorithm/restart/2", buffer, strlen(buffer));
            }
        }
    }
    net_vio_sdk_exit();
    cout << "ByeBye\n";
    return 0;
}

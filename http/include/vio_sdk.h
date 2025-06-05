//#pragma once
#ifndef _vio_sdk_h_
#define _vio_sdk_h_

#if (defined(_WIN32)) //windows
#ifdef VIOSDK_EXPORTS
#define NET_VIO_API  extern "C" __declspec(dllexport)
#else
#define NET_VIO_API  extern "C" __declspec(dllimport)
#endif
#else
#define NET_VIO_API extern "C"
#endif

#ifndef _WINDOWS_
#if (defined(_WIN32) || defined(_WIN64))
#include <winsock2.h>
#include <windows.h>    
#endif
#endif

#ifdef __linux__
typedef int BOOL;
typedef long LONG;
typedef void* HANDLE;
#define __stdcall
#endif

#define vio_call __stdcall

/**
 * @brief 连接回调
 * @param [out] state:连接状态,1-已连接，0-已断开，-1-错误
 * @param [out] userData:用户自定义数据
 * @return 无
 */
typedef void(vio_call* vio_connect_callback)(int state, void* userData);

/**
 * @brief 事件回调
 * @param [out] data:数据内容
 * @param [out] length:数据长度
 * @param [out] userData:用户自定义数据
 * @return 无
 */
typedef void(vio_call* vio_event_callback)(const char* data, int length, void* userData);

//登录信息
typedef struct
{
	char	ipaddr[32];		
	int		port;
	char	username[32];
	char	password[32];
	void* userData;
	vio_connect_callback connect_cb;
	vio_event_callback event_cb;
}vio_login_info_s;

//帧类型
typedef enum
{
	vio_frame_imu = 1,
	vio_frame_image_gray,
	vio_frame_image_deep,
	vio_frame_image_amp,
	vio_frame_algo_pose,
	vio_frame_loop_pose,
	vio_frame_map_cloud,
	vio_frame_sys_state,
	wio_frame_tof_cloud,
	wio_frame_image_gray_right,
	vio_frame_global_pose,
	vio_frame_global_point,
	vio_frame_twist,
	vio_frame_rdf_pose,
	vio_frame_rdf_points,
        vio_frame_pose_and_twist = 26,
}vio_frame_type_e;

//帧信息
typedef struct
{
	unsigned int 	type;
	double			timestamp;
	unsigned int	seq;
	unsigned int 	width;
	unsigned int 	height;
	unsigned int 	length;
}vio_frame_info_s;


/**
 * @brief 获取SDK版本信息
 * @param 无
 * @return SDK版本信息，2个高字节表示主版本，2个低字节表示次版本。如0x00030000:表示版本为3.0
 */
NET_VIO_API LONG vio_call net_vio_sdk_version();

/**
 * @brief SDK初始化，程序开始后调用一次即可
 * @param 无
 * @return 成功返回1，失败返回错误码
 */
NET_VIO_API BOOL vio_call net_vio_sdk_init();

/**
 * @brief SDK退出，程序结束前调用一次即可
 * @param 无
 * @return 成功返回1，失败返回错误码
 */
NET_VIO_API BOOL vio_call net_vio_sdk_exit();

/**
 * @brief 设备登录
 * @param [in] login_info:登录信息
 * @return 成功返回登录句柄，失败返回NULL
 */
NET_VIO_API HANDLE vio_call net_vio_login(vio_login_info_s loginInfo);

/**
 * @brief 设备登出
 * @param [in] loginHandle:登录句柄
 * @return 成功返回1，失败返回错误码
 */
NET_VIO_API BOOL vio_call net_vio_logout(HANDLE loginHandle);

/**
 * @brief 流数据回调
 * @param [out] channel:通道号
 * @param [out] frameInfo:帧信息
 * @param [out] frame:帧数据
 * @param [out] userData:用户自定义数据
 * @return 无
 */
typedef void(vio_call *vio_stream_callback)(int channel, const vio_frame_info_s* frameInfo, const char* frameData, void* userData);

/**
 * @brief 流数据连接
 * @param [in] loginHandle:登录句柄
 * @param [in] channel:通道号，1-IMU+回环位姿+算法位姿；2-可见光灰度图；3-深度图+幅度图；4-建图点云;5-tof点云;6-右目图像;
 * @param [in] cb:流数据回调
 * @return 成功返回流句柄，失败返回NULL
 */
NET_VIO_API HANDLE vio_call net_vio_stream_connect(HANDLE loginHandle, int channel, vio_stream_callback cb);

/**
 * @brief 流数据断开
 * @param [in] streamHandle:流句柄
 * @return 成功返回1，失败返回错误码
 */
NET_VIO_API BOOL vio_call net_vio_stream_disconnect(HANDLE streamHandle);

/**
 * @brief 获取参数
 * @param [in] loginHandle:登录句柄
 * @param [in] uri:接口访问地址
 * @param [out] dataBuff:数据缓冲区
 * @param [in] buffSize:缓冲区大小
 * @return 成功返回1，失败返回错误码
 */
NET_VIO_API BOOL vio_call net_vio_get_cfg(HANDLE loginHandle, const char* uri, char* dataBuff, int buffSize);

/**
 * @brief 设置参数
 * @param [in] loginHandle:登录句柄
 * @param [in] uri:接口访问地址
 * @param [in] data:输入参数
 * @param [in] length:参数长度
 * @return 成功返回1，失败返回错误码
 */
NET_VIO_API BOOL vio_call net_vio_set_cfg(HANDLE loginHandle, const char* uri, const char* data, int length);


#endif

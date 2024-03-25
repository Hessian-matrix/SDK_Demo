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
 * @brief ���ӻص�
 * @param [out] state:����״̬,1-�����ӣ�0-�ѶϿ���-1-����
 * @param [out] userData:�û��Զ�������
 * @return ��
 */
typedef void(vio_call* vio_connect_callback)(int state, void* userData);

/**
 * @brief �¼��ص�
 * @param [out] data:��������
 * @param [out] length:���ݳ���
 * @param [out] userData:�û��Զ�������
 * @return ��
 */
typedef void(vio_call* vio_event_callback)(const char* data, int length, void* userData);

//��¼��Ϣ
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

//֡����
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
}vio_frame_type_e;

//֡��Ϣ
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
 * @brief ��ȡSDK�汾��Ϣ
 * @param ��
 * @return SDK�汾��Ϣ��2�����ֽڱ�ʾ���汾��2�����ֽڱ�ʾ�ΰ汾����0x00030000:��ʾ�汾Ϊ3.0
 */
NET_VIO_API LONG vio_call net_vio_sdk_version();

/**
 * @brief SDK��ʼ��������ʼ�����һ�μ���
 * @param ��
 * @return �ɹ�����1��ʧ�ܷ��ش�����
 */
NET_VIO_API BOOL vio_call net_vio_sdk_init();

/**
 * @brief SDK�˳����������ǰ����һ�μ���
 * @param ��
 * @return �ɹ�����1��ʧ�ܷ��ش�����
 */
NET_VIO_API BOOL vio_call net_vio_sdk_exit();

/**
 * @brief �豸��¼
 * @param [in] login_info:��¼��Ϣ
 * @return �ɹ����ص�¼�����ʧ�ܷ���NULL
 */
NET_VIO_API HANDLE vio_call net_vio_login(vio_login_info_s loginInfo);

/**
 * @brief �豸�ǳ�
 * @param [in] loginHandle:��¼���
 * @return �ɹ�����1��ʧ�ܷ��ش�����
 */
NET_VIO_API BOOL vio_call net_vio_logout(HANDLE loginHandle);

/**
 * @brief �����ݻص�
 * @param [out] channel:ͨ����
 * @param [out] frameInfo:֡��Ϣ
 * @param [out] frame:֡����
 * @param [out] userData:�û��Զ�������
 * @return ��
 */
typedef void(vio_call *vio_stream_callback)(int channel, const vio_frame_info_s* frameInfo, const char* frameData, void* userData);

/**
 * @brief ����������
 * @param [in] loginHandle:��¼���
 * @param [in] channel:ͨ���ţ�1-IMU+�ػ�λ��+�㷨λ�ˣ�2-�ɼ���Ҷ�ͼ��3-���ͼ+����ͼ��4-��ͼ����;5-tof����;6-��Ŀͼ��;
 * @param [in] cb:�����ݻص�
 * @return �ɹ������������ʧ�ܷ���NULL
 */
NET_VIO_API HANDLE vio_call net_vio_stream_connect(HANDLE loginHandle, int channel, vio_stream_callback cb);

/**
 * @brief �����ݶϿ�
 * @param [in] streamHandle:�����
 * @return �ɹ�����1��ʧ�ܷ��ش�����
 */
NET_VIO_API BOOL vio_call net_vio_stream_disconnect(HANDLE streamHandle);

/**
 * @brief ��ȡ����
 * @param [in] loginHandle:��¼���
 * @param [in] uri:�ӿڷ��ʵ�ַ
 * @param [out] dataBuff:���ݻ�����
 * @param [in] buffSize:��������С
 * @return �ɹ�����1��ʧ�ܷ��ش�����
 */
NET_VIO_API BOOL vio_call net_vio_get_cfg(HANDLE loginHandle, const char* uri, char* dataBuff, int buffSize);

/**
 * @brief ���ò���
 * @param [in] loginHandle:��¼���
 * @param [in] uri:�ӿڷ��ʵ�ַ
 * @param [in] data:�������
 * @param [in] length:��������
 * @return �ɹ�����1��ʧ�ܷ��ش�����
 */
NET_VIO_API BOOL vio_call net_vio_set_cfg(HANDLE loginHandle, const char* uri, const char* data, int length);


#endif

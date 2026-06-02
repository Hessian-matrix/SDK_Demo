#include "vio_sdk.h"

#include <cstring>
#include <iostream>
#include <string>
#include <vector>

namespace {

void vio_call connect_callback(int state, void*) {
    std::cout << "connect_state=" << state << std::endl;
}

void vio_call event_callback(const char*, int, void*) {}

bool get_cfg(HANDLE login_handle, const char* uri) {
    std::vector<char> buffer(8192, '\0');
    const BOOL ret = net_vio_get_cfg(login_handle, uri, buffer.data(),
                                     static_cast<int>(buffer.size()));
    std::cout << "GET " << uri << " ret=" << ret;
    if (ret == 1) {
        std::cout << " body=" << buffer.data();
    }
    std::cout << std::endl;
    return ret == 1;
}

bool set_cfg(HANDLE login_handle, const char* uri) {
    char body[] = "{}";
    const BOOL ret = net_vio_set_cfg(login_handle, uri, body, std::strlen(body));
    std::cout << "PUT " << uri << " ret=" << ret << std::endl;
    return ret == 1;
}

}  // namespace

int main(int argc, char** argv) {
    const char* ip = argc > 1 ? argv[1] : "10.21.12.131";

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

    get_cfg(login_handle, "/System/version");
    get_cfg(login_handle, "/System/hardware_status");
    get_cfg(login_handle, "/Baton/config");
    get_cfg(login_handle, "/System/network");

    set_cfg(login_handle, "/Algorithm/disable/5");

    net_vio_logout(login_handle);
    net_vio_sdk_exit();
    return 0;
}

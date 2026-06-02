#include "viobot_demo/algorithm_status.h"

namespace viobot_demo {

const char* system_status_name(int status) {
    switch (status) {
        case ready:
            return "ready";
        case stereo1_initializing:
            return "stereo1_initializing";
        case stereo1_running:
            return "stereo1_running";
        case stereo2_initializing:
            return "stereo2_initializing";
        case stereo2_running:
            return "stereo2_running";
        case mono1_initializing:
            return "mono1_initializing";
        case mono1_running:
            return "mono1_running";
        case stereo3_initializing:
            return "stereo3_initializing";
        case stereo3_running:
            return "stereo3_running";
        case stereo4_initializing:
            return "stereo4_initializing";
        case stereo4_running:
        case algo_status_stereo4_running_legacy:
            return "stereo4_running";
        default:
            return "unknown";
    }
}

int normalize_algorithm_status(int system_status) {
    if (system_status == ready) {
        return algo_status_ready;
    }
    if (system_status == stereo3_initializing) {
        return algo_status_stereo3_initializing;
    }
    if (system_status == stereo3_running) {
        return algo_status_stereo3_running;
    }
    if (system_status == stereo4_initializing) {
        return algo_status_stereo4_initializing;
    }
    if (system_status == stereo4_running ||
        system_status == algo_status_stereo4_running_legacy) {
        return algo_status_stereo4_running;
    }
    return algo_status_unknown;
}

bool is_algorithm_status(ActiveAlgorithm algorithm, int status) {
    if (algorithm == active_algorithm_stereo3) {
        return status == algo_status_stereo3_initializing ||
               status == algo_status_stereo3_running;
    }
    if (algorithm == active_algorithm_stereo4) {
        return status == algo_status_stereo4_initializing ||
               status == algo_status_stereo4_running;
    }
    return status == algo_status_ready;
}

}  // namespace viobot_demo

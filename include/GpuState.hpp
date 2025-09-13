#pragma once
#include "nlohmann/json.hpp"
using json = nlohmann::json;

struct GpuState {
    struct GpuTuning {
        int minFreq;
        int maxFreq;
        int voltage;
    } gpuTuning;

    struct VramTuning {
        std::string memoryTiming;
        int maxFreq;
    } vramTuning;

    struct FanTuning {
        bool zeroRpm;
        std::vector<int> speeds;
        std::vector<int> temperatures;
    } fanTuning;

    struct PowerTuning {
        int powerLimit;
    } powerTuning;
};

// clang-format off
#define json_field(X) j[#X] = state.##X
static void to_json(json& j, const GpuState::GpuTuning& state) { 
    json_field(minFreq);
    json_field(maxFreq);
    json_field(voltage);
}
static void to_json(json& j, const GpuState::VramTuning& state) { 
    json_field(memoryTiming);
    json_field(maxFreq);
}
static void to_json(json& j, const GpuState::FanTuning& state) { 
    json_field(zeroRpm);
    json_field(speeds);
    json_field(temperatures);
}
static void to_json(json& j, const GpuState::PowerTuning& state) { 
    json_field(powerLimit);
}
static void to_json(json& j, const GpuState& state) {
    json_field(gpuTuning);
    json_field(vramTuning);
    json_field(fanTuning);
    json_field(powerTuning);
}
#undef json_field
// clang-format on
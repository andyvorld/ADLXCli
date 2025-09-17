#pragma once
#pragma warning disable
#include <nlohmann/json.hpp>
#pragma warning restore
using json = nlohmann::json;

#define GPU_TUNING_FIELDS     \
    json_field(int, minFreq); \
    json_field(int, maxFreq); \
    json_field(int, voltage)

#define VRAM_TUNING_FIELDS                 \
    json_field(std::string, memoryTiming); \
    json_field(int, maxFreq)

#define FAN_TUNING_FIELDS                 \
    json_field(bool, zeroRpm);            \
    json_field(std::vector<int>, speeds); \
    json_field(std::vector<int>, temperatures)

#define POWER_TUNING_FIELDS json_field(int, powerLimit)

#define GPU_STATE_FIELDS                \
    json_field(GpuTuning, gpuTuning);   \
    json_field(VramTuning, vramTuning); \
    json_field(FanTuning, fanTuning);   \
    json_field(PowerTuning, powerTuning)

#define JSON_TYPES                               \
    json_type(GpuTuning, GPU_TUNING_FIELDS);     \
    json_type(VramTuning, VRAM_TUNING_FIELDS);   \
    json_type(FanTuning, FAN_TUNING_FIELDS);     \
    json_type(PowerTuning, POWER_TUNING_FIELDS); \
    json_type(GpuState, GPU_STATE_FIELDS)

#pragma region struct def
#define json_type(TYPE, FIELDS) \
    struct TYPE {               \
        FIELDS;                 \
    }
#define json_field(TYPE, X) TYPE X = {}

JSON_TYPES;

#undef json_field
#undef json_type
#pragma endregion

#pragma region to_json
#define json_type(TYPE, FIELDS) \
    static void to_json(json& j, const TYPE& state) { FIELDS; }
#define json_field(TYPE, X) j[#X] = state.##X

JSON_TYPES;

#undef json_field
#undef json_type
#pragma endregion

#pragma region from_json
#define json_type(TYPE, FIELDS) \
    static void from_json(const json& j, TYPE& state) { FIELDS; }
#define json_field(TYPE, X) j.at(#X).get_to(state.##X)

JSON_TYPES;

#undef json_field
#undef json_type
#pragma endregion
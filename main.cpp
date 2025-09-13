#include <format>
#include <functional>
#include <iostream>
#include <sstream>

#include "SDK/ADLXHelper/Windows/Cpp/ADLXHelper.h"
#include "SDK/Include/IGPUManualFanTuning.h"
#include "SDK/Include/IGPUManualGFXTuning.h"
#include "SDK/Include/IGPUManualPowerTuning.h"
#include "SDK/Include/IGPUManualVRAMTuning.h"
#include "SDK/Include/IGPUPresetTuning.h"
#include "SDK/Include/IGPUTuning.h"

// Use ADLX namespace
using namespace adlx;

static void _CHECK_ADLX(ADLX_RESULT val, const char* str) {
    if (!ADLX_SUCCEEDED(val)) throw std::runtime_error(std::format("Failed {}", str));
}
#define CHECK_ADLX(val) _CHECK_ADLX(val, #val)

template <class T>
static IADLXInterfacePtr_T<T> _GetService(ADLX_RESULT (IADLXSystem::*F)(T**), const char* Fstr) {
    IADLXInterfacePtr_T<T> servicePtr;

    auto _f = std::bind(F, g_ADLX.GetSystemServices(), std::placeholders::_1);
    auto res = _f(&servicePtr);
    if (!ADLX_SUCCEEDED(res)) throw std::runtime_error(std::format("Failed to obtain {}.", (Fstr + 1)));

    return servicePtr;
}
#define GetService(X) _GetService(X, #X)

static auto GpuIfaceGetter(IADLXGPUTuningServicesPtr gpuTuningService, IADLXGPUPtr gpu) {
    return [gpu, gpuTuningService](auto F) {
        IADLXInterfacePtr ifacePtr;

        auto _f = std::bind(F, gpuTuningService, std::placeholders::_1, std::placeholders::_2);
        auto res = _f(gpu, &ifacePtr);
        if (!ADLX_SUCCEEDED(res)) throw std::runtime_error(std::format("Failed to obtain {}.", typeid(F).name()));

        return ifacePtr;
    };
}

static int CheckRange(int val, const ADLX_IntRange& range) {
    int _val = (val / range.step) * range.step;
    if ((_val < range.minValue) || (range.maxValue < _val)) {
        throw std::runtime_error(
            std::format("val: {}, outside valid range [{}, {}]", val, range.minValue, range.maxValue));
    }

    return _val;
}

static void _SetIfaceValue(auto tuningIface, auto RangeF, auto SetF, std::string Fname, int val, std::string valUnit) {
    ADLX_IntRange range;
    auto _rangeF = std::bind(RangeF, tuningIface, std::placeholders::_1);
    _rangeF(&range);

    int _val = (val / range.step) * range.step;
    if ((_val < range.minValue) || (range.maxValue < _val)) {
        throw std::runtime_error(
            std::format("val: {}, outside valid range [{}, {}]", val, range.minValue, range.maxValue));
    }

    auto _setF = std::bind(SetF, tuningIface, std::placeholders::_1);
    _setF(_val);
#if DEBUG
    std::cout << std::format("Set {} to {} {}", Fname, _val, valUnit) << '\n';
#endif
}
#define SetIfaceValue(X, Y, Z, Zu)                                                     \
    _SetIfaceValue(X, &std::remove_pointer<decltype(X.GetPtr())>::type::Get##Y##Range, \
                   &std::remove_pointer<decltype(X.GetPtr())>::type::Set##Y, #Y, Z, Zu)

template <typename T>
static T GetTuningPtr(auto iface) {
    T ptr(iface);
    if (ptr == nullptr) {
        throw std::runtime_error(std::format("Failed to obtain '{}' tuning ptr", typeid(T).name()));
    }
    return ptr;
}

static auto TryCheck(auto ptr) {
    return [ptr](auto F, adlx_bool _default = false) {
        adlx_bool ret;
        auto _f = std::bind(F, ptr, std::placeholders::_1);
        ADLX_RESULT res = _f(&ret);
        if (ADLX_SUCCEEDED(res)) return ret;
        return _default;
    };
}

static ADLX_MEMORYTIMING_DESCRIPTION StringToMemoryTiming(std::string timing) {
#define ENTRY(X) \
    if (timing == #X) return X
    ENTRY(MEMORYTIMING_DEFAULT);
    ENTRY(MEMORYTIMING_FAST_TIMING);
    ENTRY(MEMORYTIMING_FAST_TIMING_LEVEL_2);
    ENTRY(MEMORYTIMING_AUTOMATIC);
    ENTRY(MEMORYTIMING_MEMORYTIMING_LEVEL_1);
    ENTRY(MEMORYTIMING_MEMORYTIMING_LEVEL_2);
#undef ENTRY
    throw std::runtime_error(std::format("Unknown memory timing setting '{}'", timing));
}

struct GpuState {
    struct GpuTuning {
        int MinFreq;
        int MaxFreq;
        int Voltage;
    } gpuTuning;

    struct VramTuning {
        std::string MemoryTiming;
        int MaxFreq;
    } vramTuning;

    struct FanTuning {
        bool ZeroRpm;
        std::vector<int> Speeds;
        std::vector<int> Temperatures;
    } fanTuning;

    struct PowerTuning {
        int PowerLimit;
    } powerTuning;
};

static int _main(void) {
    // clang-format off
    GpuState gpuState = { 
        .gpuTuning {
            .MinFreq = 2400,
            .MaxFreq = 2500,
            .Voltage = 1125
        },
        .vramTuning {
            .MemoryTiming = "MEMORYTIMING_DEFAULT",
            .MaxFreq = 2614
        },
        .fanTuning {
            .ZeroRpm = true,
            .Speeds = { 23, 30, 41, 54, 80 },
            .Temperatures = { 55, 70, 79, 88, 95 }
        },
        .powerTuning {
            .PowerLimit = 15
        }
    };
    // clang-format on

    // Initialize ADLX
    CHECK_ADLX(g_ADLX.Initialize());

    auto gpuTuningService = GetService(&IADLXSystem::GetGPUTuningServices);
    auto gpus = GetService(&IADLXSystem::GetGPUs);

    IADLXGPUPtr oneGPU;
    CHECK_ADLX(gpus->At(0, &oneGPU));

    auto gpuIfaceGetter = GpuIfaceGetter(gpuTuningService, oneGPU);
    {
        auto gpuPresetTuningIfc = gpuIfaceGetter(&IADLXGPUTuningServices::GetPresetTuning);
        auto gpuPresetTuningPtr = GetTuningPtr<IADLXGPUPresetTuningPtr>(gpuPresetTuningIfc);

        auto _check = TryCheck(gpuPresetTuningPtr);
#define check(x) _check(&IADLXGPUPresetTuning::IsCurrent##x)
        if (check(PowerSaver) || check(Quiet) || check(Balanced) || check(Turbo) || check(Rage)) {
            CHECK_ADLX(gpuTuningService->ResetToFactory(oneGPU));
        }
#undef check
    }

    {
        auto manualGFXTuningIfc = gpuIfaceGetter(&IADLXGPUTuningServices::GetManualGFXTuning);
        auto manualGFXTuning2Ptr = GetTuningPtr<IADLXManualGraphicsTuning2Ptr>(manualGFXTuningIfc);
        SetIfaceValue(manualGFXTuning2Ptr, GPUMinFrequency, gpuState.gpuTuning.MinFreq, "MHz");
        SetIfaceValue(manualGFXTuning2Ptr, GPUMaxFrequency, gpuState.gpuTuning.MaxFreq, "MHz");
        SetIfaceValue(manualGFXTuning2Ptr, GPUVoltage, gpuState.gpuTuning.Voltage, "mV");
    }

    {
        auto manualVramTuningIfc = gpuIfaceGetter(&IADLXGPUTuningServices::GetManualVRAMTuning);
        auto manualVramTuning2Ptr = GetTuningPtr<IADLXManualVRAMTuning2Ptr>(manualVramTuningIfc);

        IADLXMemoryTimingDescriptionListPtr timingList;
        CHECK_ADLX(manualVramTuning2Ptr->GetSupportedMemoryTimingDescriptionList(&timingList));
        auto CheckTimingSupported = [timingList =
                                         std::as_const(timingList)](ADLX_MEMORYTIMING_DESCRIPTION val) -> bool {
            for (int i = timingList->Begin(); i != timingList->End(); ++i) {
                IADLXMemoryTimingDescriptionPtr ptr;
                CHECK_ADLX(timingList->At(i, &ptr));

                ADLX_MEMORYTIMING_DESCRIPTION desc;
                CHECK_ADLX(ptr->GetDescription(&desc));

                if (val == desc) return true;
            }

            return false;
        };
        auto newTiming = StringToMemoryTiming(gpuState.vramTuning.MemoryTiming);
        if (!CheckTimingSupported(newTiming)) {
            throw std::runtime_error(std::format("{} is not supported by the gpu", gpuState.vramTuning.MemoryTiming));
        }
        CHECK_ADLX(manualVramTuning2Ptr->SetMemoryTimingDescription(newTiming));
        std::cout << std::format("Set MemoryTiming to {}", gpuState.vramTuning.MemoryTiming) << '\n';

        SetIfaceValue(manualVramTuning2Ptr, MaxVRAMFrequency, gpuState.vramTuning.MaxFreq, "MHz");
    }

    {
        auto manualFanTuningIfc = gpuIfaceGetter(&IADLXGPUTuningServices::GetManualFanTuning);
        auto manualFanTuningPtr = GetTuningPtr<IADLXManualFanTuningPtr>(manualFanTuningIfc);

        auto _check = TryCheck(manualFanTuningPtr);
        if (_check(&IADLXManualFanTuning::IsSupportedZeroRPM)) {
            CHECK_ADLX(manualFanTuningPtr->SetZeroRPMState(true));
        }

        IADLXManualFanTuningStateListPtr states;
        CHECK_ADLX(manualFanTuningPtr->GetFanTuningStates(&states));

        ADLX_IntRange speedRange, TemperatureRange;
        CHECK_ADLX(manualFanTuningPtr->GetFanTuningRanges(&speedRange, &TemperatureRange));

        std::stringstream oss;
        for (auto i = states->Begin(); i != states->End(); ++i) {
            IADLXManualFanTuningStatePtr state;
            CHECK_ADLX(states->At(i, &state));

            adlx_int speed = 0, temperature = 0;
            CHECK_ADLX(state->GetFanSpeed(&speed));
            CHECK_ADLX(state->GetTemperature(&temperature));

            int newSpeed = CheckRange(gpuState.fanTuning.Speeds.at(i), speedRange);
            int newTemp = CheckRange(gpuState.fanTuning.Temperatures.at(i), TemperatureRange);
            CHECK_ADLX(state->SetFanSpeed(newSpeed));
            CHECK_ADLX(state->SetTemperature(newTemp));

            oss << std::format("Set FanState[{}] -> ({}%, {} degC)", i, newSpeed, newTemp) << '\n';
        }

        manualFanTuningPtr->SetFanTuningStates(states);
        std::cout << oss.str();
    }

    {
        auto manualPowerTuningIfc = gpuIfaceGetter(&IADLXGPUTuningServices::GetManualPowerTuning);
        auto manualPowerTuningPtr = GetTuningPtr<IADLXManualPowerTuningPtr>(manualPowerTuningIfc);

        SetIfaceValue(manualPowerTuningPtr, PowerLimit, gpuState.powerTuning.PowerLimit, "%");

        ADLX_IntRange range;
        manualPowerTuningPtr->GetTDCLimitRange(&range);
    }
}

int main(void) {
    try {
        return _main();
    } catch (const std::runtime_error& e) {
        std::cout << e.what() << '\n';
    }
}
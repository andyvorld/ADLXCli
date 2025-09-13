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

int main(void) {
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
        SetIfaceValue(manualGFXTuning2Ptr, GPUMinFrequency, 2400, "MHz");
        SetIfaceValue(manualGFXTuning2Ptr, GPUMaxFrequency, 2500, "MHz");
        SetIfaceValue(manualGFXTuning2Ptr, GPUVoltage, 1125, "mV");
    }

    {
        auto manualVramTuningIfc = gpuIfaceGetter(&IADLXGPUTuningServices::GetManualVRAMTuning);
        auto vramTuning2Ptr = GetTuningPtr<IADLXManualVRAMTuning2Ptr>(manualVramTuningIfc);
        SetIfaceValue(vramTuning2Ptr, MaxVRAMFrequency, 2614, "MHz");
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
        int speeds[] = {23, 30, 41, 54, 80};
        int temps[] = {55, 70, 79, 88, 95};
        for (auto i = states->Begin(); i != states->End(); ++i) {
            IADLXManualFanTuningStatePtr state;
            CHECK_ADLX(states->At(i, &state));

            adlx_int speed = 0, temperature = 0;
            CHECK_ADLX(state->GetFanSpeed(&speed));
            CHECK_ADLX(state->GetTemperature(&temperature));

            int newSpeed = CheckRange(speeds[i], speedRange);
            int newTemp = CheckRange(temps[i], TemperatureRange);
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

        SetIfaceValue(manualPowerTuningPtr, PowerLimit, 15, "%");

        ADLX_IntRange range;
        manualPowerTuningPtr->GetTDCLimitRange(&range);
    }
}
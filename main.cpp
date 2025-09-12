#include "SDK/ADLXHelper/Windows/Cpp/ADLXHelper.h"
#include "SDK/Include/IGPUManualGFXTuning.h"
#include "SDK/Include/IGPUTuning.h"
#include <format>
#include <functional> 
#include <iostream>

// Use ADLX namespace
using namespace adlx;

void _CHECK_ADLX(ADLX_RESULT val, const char* str) {
    if (!ADLX_SUCCEEDED(val))
        throw std::runtime_error(std::format("Failed {}", str));
}
#define CHECK_ADLX(val) _CHECK_ADLX(val, #val)

template<class T>
IADLXInterfacePtr_T<T> _GetService(ADLX_RESULT (IADLXSystem::*F)(T**), const char* Fstr) {
    IADLXInterfacePtr_T<T> servicePtr;

    auto _f = std::bind(F, g_ADLX.GetSystemServices(), std::placeholders::_1);
    auto res = _f(&servicePtr);
    if (!ADLX_SUCCEEDED(res))
        throw std::runtime_error(std::format("Failed to obtain {}.", (Fstr + 1)));

    return servicePtr;
}
#define GetService(X) _GetService(X, #X)

auto _GetGpuIface(IADLXGPUTuningServicesPtr gpuTuningService, auto F, const char* Fstr) {
    return [F, &gpuTuningService, Fstr](IADLXGPUPtr gpu) {
        IADLXInterfacePtr ifacePtr;

        auto _f = std::bind(F, gpuTuningService, std::placeholders::_1, std::placeholders::_2);
        auto res = _f(gpu, &ifacePtr);
        if (!ADLX_SUCCEEDED(res))
            throw std::runtime_error(std::format("Failed to obtain {}.", (Fstr + 1)));

        return ifacePtr;
    };
}
#define GetGpuIface(X, Y) _GetGpuIface(X, Y, #Y)

void _SetIfaceValue(auto tuningIface, auto RangeF, auto SetF, int val) {
    ADLX_IntRange range;
    auto _rangeF = std::bind(RangeF, tuningIface, std::placeholders::_1);
    _rangeF(&range);

    int _val = (val / range.step) * range.step;
    if ((_val < range.minValue) || (range.maxValue < _val)) {
        throw std::runtime_error(std::format("val: {}, outside valid range [{}, {}]", val, range.minValue, range.maxValue));
    }

    auto _setF = std::bind(SetF, tuningIface, std::placeholders::_1);
    _setF(val);
}
#define SetIfaceValue(X, Y, Z) _SetIfaceValue( \
    X, \
    &std::remove_pointer<decltype(X.GetPtr())>::type::Get##Y##Range, \
    &std::remove_pointer<decltype(X.GetPtr())>::type::Set##Y, \
    Z \
)

int main(void) {

    // Initialize ADLX
    CHECK_ADLX(g_ADLX.Initialize());

    auto gpuTuningService = GetService(&IADLXSystem::GetGPUTuningServices);
    auto gpus = GetService(&IADLXSystem::GetGPUs);

    IADLXGPUPtr oneGPU;
    CHECK_ADLX(gpus->At(0, &oneGPU));

    auto manualGFXTuningIfc = GetGpuIface(gpuTuningService, &IADLXGPUTuningServices::GetManualGFXTuning)(oneGPU);

    //IADLXManualGraphicsTuning2* manualGFXTuning2 = IADLXManualGraphicsTuning2Ptr(manualGFXTuningIfc).GetPtr();
    IADLXManualGraphicsTuning2Ptr manualGFXTuning2(manualGFXTuningIfc);

    //ADLX_IntRange range;
    //manualGFXTuning2->GetGPUMaxFrequencyRange(&range);
    //range.

    //auto asdfasddfd = &std::remove_pointer<decltype(manualGFXTuning2.GetPtr())>::type::GetGPUMaxFrequency;
    SetIfaceValue(manualGFXTuning2, GPUMaxFrequency, 2500);
}
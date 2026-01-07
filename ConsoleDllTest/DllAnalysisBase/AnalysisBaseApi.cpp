#include "pch.h"

#include "../Common/NativeInterfaces.h"
#include "../Common/AnalysisEngineBase.h"

#include <algorithm>
#include <memory>
#include <numeric>
#include <string>
#include <vector>
#include <sstream>
#include <iostream>

#pragma comment(lib, "gdcmCommon.lib")
#ifdef _DEBUG
#pragma comment(lib, "glew32d.lib")
#else
#pragma comment(lib, "glew32.lib")
#endif
#pragma comment(lib, "OpenGL32.lib")

static thread_local std::string g_lastError;

static void SetLastError(const std::string& message);

// Base Analysis Engine - 继承自AnalysisEngineBase
class BaseAnalysisEngine : public AnalysisEngineBase
{
public:
    BaseAnalysisEngine() = default;

    // 重写抽象方法
    NativeResult RunAnalysis() override
    {
        if (!initialized_)
        {
            SetLastError("Engine not initialized");
            return NATIVE_E_NOT_INITIALIZED;
        }

        if (voxels_.empty())
        {
            SetLastError("No volume data loaded");
            return NATIVE_E_NOT_INITIALIZED;
        }

        // 基础分析逻辑
        std::cout << "[BaseAnalysisEngine] Running base analysis..." << std::endl;
        // TODO: 实际的基础分析算法
        return NATIVE_OK;
    }
};

static void SetLastError(const std::string& message)
{
    g_lastError = message;
}

static NativeResult ValidateHandle(EngineHandle handle, BaseAnalysisEngine*& outContext)
{
    if (handle == nullptr)
    {
        SetLastError("Engine handle is null");
        return NATIVE_E_INVALID_ARGUMENT;
    }

    outContext = static_cast<BaseAnalysisEngine*>(handle);
    return NATIVE_OK;
}

extern "C" __declspec(dllexport) const char* Engine_GetLastError()
{
    return g_lastError.c_str();
}

extern "C" __declspec(dllexport) EngineHandle Engine_Create()
{
    try
    {
        BaseAnalysisEngine* context = new BaseAnalysisEngine();
        return static_cast<EngineHandle>(context);
    }
    catch (const std::bad_alloc&)
    {
        SetLastError("Failed to allocate engine context");
        return nullptr;
    }
}

extern "C" __declspec(dllexport) void Engine_Destroy(EngineHandle handle)
{
    if (handle == nullptr)
    {
        return;
    }

    BaseAnalysisEngine* context = nullptr;
    if (ValidateHandle(handle, context) != NATIVE_OK)
    {
        return;
    }

    delete context;
}

extern "C" __declspec(dllexport) NativeResult Engine_Initialize(EngineHandle handle)
{
    std::cout << "[AnalysisBase DLL] Engine_Initialize called" << std::endl;
    BaseAnalysisEngine* context = nullptr;
    NativeResult result = ValidateHandle(handle, context);
    if (result != NATIVE_OK)
    {
        std::cout << "[AnalysisBase DLL] Engine_Initialize: invalid handle" << std::endl;
        return result;
    }

    result = context->Initialize();
    std::cout << "[AnalysisBase DLL] Engine_Initialize success" << std::endl;
    return result;
}

extern "C" __declspec(dllexport) NativeResult Engine_LoadSyntheticVolume(
    EngineHandle handle,
    std::uint32_t width,
    std::uint32_t height,
    std::uint32_t depth,
    double spacing)
{
    BaseAnalysisEngine* context = nullptr;
    NativeResult result = ValidateHandle(handle, context);
    if (result != NATIVE_OK)
    {
        return result;
    }

    return context->LoadSyntheticVolume(width, height, depth, spacing);
}

extern "C" __declspec(dllexport) NativeResult Engine_GetVolumeStats(EngineHandle handle, EngineVolumeStats* outStats)
{
    if (outStats == nullptr)
    {
        SetLastError("Volume stats output pointer is null");
        return NATIVE_E_INVALID_ARGUMENT;
    }

    BaseAnalysisEngine* context = nullptr;
    NativeResult result = ValidateHandle(handle, context);
    if (result != NATIVE_OK)
    {
        return result;
    }

    return context->GetVolumeStats(outStats);
}

extern "C" __declspec(dllexport) NativeResult Engine_RunSelfTest(EngineVolumeStats* outStats)
{
    
    EngineHandle handle = Engine_Create();
    if (handle == nullptr)
    {
        return NATIVE_E_INTERNAL_ERROR;
    }

    const NativeResult initResult = Engine_Initialize(handle);
    if (initResult != NATIVE_OK)
    {
        Engine_Destroy(handle);
        return initResult;
    }

    const NativeResult loadResult = Engine_LoadSyntheticVolume(handle, 32, 32, 32, 0.7);
    if (loadResult != NATIVE_OK)
    {
        Engine_Destroy(handle);
        return loadResult;
    }

    NativeResult statsResult = Engine_GetVolumeStats(handle, outStats);
    Engine_Destroy(handle);
    return statsResult;
}

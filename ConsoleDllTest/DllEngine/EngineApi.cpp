#include "pch.h"

#include "../Common/NativeInterfaces.h"

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


struct EngineContext
{
    bool initialized = false;
    std::vector<double> voxels;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::uint32_t depth = 0;
    double spacing = 1.0;
};

static thread_local std::string g_lastError;

static void SetLastError(const std::string& message)
{
    g_lastError = message;
}

static NativeResult ValidateHandle(EngineHandle handle, EngineContext*& outContext)
{
    if (handle == nullptr)
    {
        SetLastError("Engine handle is null");
        return NATIVE_E_INVALID_ARGUMENT;
    }

    outContext = static_cast<EngineContext*>(handle);
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
        EngineContext* context = new EngineContext();
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

    EngineContext* context = nullptr;
    if (ValidateHandle(handle, context) != NATIVE_OK)
    {
        return;
    }

    delete context;
}

extern "C" __declspec(dllexport) NativeResult Engine_Initialize(EngineHandle handle)
{
    std::cout << "[Engine DLL] Engine_Initialize called" << std::endl;
    EngineContext* context = nullptr;
    NativeResult result = ValidateHandle(handle, context);
    if (result != NATIVE_OK)
    {
        std::cout << "[Engine DLL] Engine_Initialize: invalid handle" << std::endl;
        return result;
    }

    context->initialized = true;
    std::cout << "[Engine DLL] Engine_Initialize success" << std::endl;
    return NATIVE_OK;
}

extern "C" __declspec(dllexport) NativeResult Engine_LoadSyntheticVolume(
    EngineHandle handle,
    std::uint32_t width,
    std::uint32_t height,
    std::uint32_t depth,
    double spacing)
{
    EngineContext* context = nullptr;
    NativeResult result = ValidateHandle(handle, context);
    if (result != NATIVE_OK)
    {
        return result;
    }

    if (!context->initialized)
    {
        SetLastError("Engine not initialized");
        return NATIVE_E_NOT_INITIALIZED;
    }

    if (width == 0 || height == 0 || depth == 0 || spacing <= 0.0)
    {
        SetLastError("Invalid synthetic volume parameters");
        return NATIVE_E_INVALID_ARGUMENT;
    }

    const std::size_t voxelCount = static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * static_cast<std::size_t>(depth);

    try
    {
        context->voxels.resize(voxelCount);
    }
    catch (const std::bad_alloc&)
    {
        SetLastError("Failed to allocate synthetic volume data");
        return NATIVE_E_INTERNAL_ERROR;
    }

    for (std::size_t z = 0; z < depth; ++z)
    {
        for (std::size_t y = 0; y < height; ++y)
        {
            for (std::size_t x = 0; x < width; ++x)
            {
                const std::size_t index = (z * static_cast<std::size_t>(height) + y) * static_cast<std::size_t>(width) + x;
                const double value = static_cast<double>((x + y + z) % 4096);
                context->voxels[index] = value;
            }
        }
    }

    context->width = width;
    context->height = height;
    context->depth = depth;
    context->spacing = spacing;

    return NATIVE_OK;
}

extern "C" __declspec(dllexport) NativeResult Engine_GetVolumeStats(EngineHandle handle, EngineVolumeStats* outStats)
{
    if (outStats == nullptr)
    {
        SetLastError("Volume stats output pointer is null");
        return NATIVE_E_INVALID_ARGUMENT;
    }

    EngineContext* context = nullptr;
    NativeResult result = ValidateHandle(handle, context);
    if (result != NATIVE_OK)
    {
        return result;
    }

    if (!context->initialized)
    {
        SetLastError("Engine not initialized");
        return NATIVE_E_NOT_INITIALIZED;
    }

    if (context->voxels.empty())
    {
        SetLastError("No volume has been loaded");
        return NATIVE_E_INVALID_ARGUMENT;
    }

    const auto minmax = std::minmax_element(context->voxels.begin(), context->voxels.end());
    const double sum = std::accumulate(context->voxels.begin(), context->voxels.end(), 0.0);

    outStats->width = context->width;
    outStats->height = context->height;
    outStats->depth = context->depth;
    outStats->spacing = context->spacing;
    outStats->minimumValue = *minmax.first;
    outStats->maximumValue = *minmax.second;
    outStats->meanValue = sum / static_cast<double>(context->voxels.size());

    return NATIVE_OK;
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

#pragma once

#include <cstdint>
#include <cstddef>

#ifdef __cplusplus
extern "C" {
#endif

/* Shared result codes for all native components. */
enum NativeResult : int {
    NATIVE_OK = 0,
    NATIVE_E_INVALID_ARGUMENT = 1,
    NATIVE_E_NOT_INITIALIZED = 2,
    NATIVE_E_INTERNAL_ERROR = 3,
    NATIVE_E_NOT_IMPLEMENTED = 4,
    NATIVE_USER_CANCELLED = 5,
    NATIVE_E_NOT_FOUND = 6,
    NATIVE_E_ALREADY_EXISTS = 7,
    NATIVE_E_FAIL = 8,
    NATIVE_E_NO_DATA = 9,
    NATIVE_E_NO_CHANGE = 10
};

/* Common Handle types */
typedef void* EngineHandle;
typedef void* RendererHandle;
typedef void* AlgorithmHandle;

/* Core module types */
typedef void* ThreadPoolHandle;
typedef void* TimerHandle;
typedef void* ConfigHandle;

/* DICOM module types */
typedef void* DicomHandle;
typedef void* DicomReaderHandle;
typedef void* DicomWriterHandle;
typedef void* DicomSeriesHandle;
typedef void* VolumeHandle;

/* Visualization module types */
typedef void* APRRendererHandle;
typedef void* MPRRendererHandle;
typedef void* VolumeRendererHandle;
typedef void* TransferFunctionHandle;
typedef void* WindowHandle;

/* Additional visualization type aliases */
typedef void* APRHandle;
typedef void* MPRHandle;
typedef void* Volume3DHandle;

/* Offscreen rendering types */
typedef struct {
    int width;
    int height;
    unsigned char* pixels;  // RGBA format
    size_t pixelCount;
} FrameBuffer;

/* Image Processing module types */
typedef void* MaskHandle;
typedef void* ROIHandle;

/* Analysis module types */
typedef void* AnalysisContextHandle;
typedef void* AnalysisResultHandle;

typedef struct EngineVolumeStats {
    std::uint32_t width;
    std::uint32_t height;
    std::uint32_t depth;
    double spacing;
    double minimumValue;
    double maximumValue;
    double meanValue;
} EngineVolumeStats;

typedef EngineHandle (__cdecl* PFN_Engine_Create)();
typedef void (__cdecl* PFN_Engine_Destroy)(EngineHandle);
typedef NativeResult (__cdecl* PFN_Engine_Initialize)(EngineHandle);
typedef NativeResult (__cdecl* PFN_Engine_LoadSyntheticVolume)(EngineHandle, std::uint32_t, std::uint32_t, std::uint32_t, double);
typedef NativeResult (__cdecl* PFN_Engine_GetVolumeStats)(EngineHandle, EngineVolumeStats*);
typedef NativeResult (__cdecl* PFN_Engine_RunSelfTest)(EngineVolumeStats*);
typedef const char* (__cdecl* PFN_Engine_GetLastError)();

/* Renderer types */
typedef void* RendererHandle;

typedef struct RendererDiagnostics {
    std::uint32_t surfaceWidth;
    std::uint32_t surfaceHeight;
    float lastSliceLocation;
    std::uint32_t renderedFrameCount;
} RendererDiagnostics;

typedef RendererHandle (__cdecl* PFN_Renderer_Create)();
typedef void (__cdecl* PFN_Renderer_Destroy)(RendererHandle);
typedef NativeResult (__cdecl* PFN_Renderer_Initialize)(RendererHandle, std::uint32_t, std::uint32_t);
typedef NativeResult (__cdecl* PFN_Renderer_RenderSlice)(RendererHandle, float);
typedef NativeResult (__cdecl* PFN_Renderer_RunSelfTest)(RendererDiagnostics*);
typedef const char* (__cdecl* PFN_Renderer_GetLastError)();

/* Algorithm types */
typedef void* AlgorithmHandle;

typedef struct AlgorithmScalarReport {
    double minimumValue;
    double maximumValue;
    double meanValue;
    double standardDeviation;
} AlgorithmScalarReport;

typedef AlgorithmHandle (__cdecl* PFN_Algorithm_Create)();
typedef void (__cdecl* PFN_Algorithm_Destroy)(AlgorithmHandle);
typedef NativeResult (__cdecl* PFN_Algorithm_LoadScalarBuffer)(AlgorithmHandle, const double*, std::size_t);
typedef NativeResult (__cdecl* PFN_Algorithm_ComputeStatistics)(AlgorithmHandle, AlgorithmScalarReport*);
typedef NativeResult (__cdecl* PFN_Algorithm_RunSelfTest)(AlgorithmScalarReport*);
typedef const char* (__cdecl* PFN_Algorithm_GetLastError)();

#ifdef __cplusplus
}
#endif

#include "visualization_wrapper.h"
#include "utils.h"
#include "DicomApi.h"
#include "VisualizationApi.h"
#include "VolumeData.h"

// Compatibility for ConsoleDllTest Common VolumeData definition
typedef VolumeContext VolumeData;

#include <napi.h>
#include <string>
#include <map>
#include <vector>
#include <filesystem>
#include <iostream>
#include <chrono>
#include <fstream>
#include <cstring>
#include "FatAnalysis.h"
#include "VascularAnalysis.h"

#ifdef _WIN32
#include <vector>
#include <limits>
#include <algorithm>
#endif

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

// Fallback stubs to satisfy missing mask operations when the DLL does not export them.
static NativeResult MPR_MaskMorphology2D_fallback(const char* sessionId, int maskId, MorphologyOperation op, int kernelSize, int connectivity) {
    (void)sessionId;
    (void)maskId;
    (void)op;
    (void)kernelSize;
    (void)connectivity;
    return NATIVE_OK;
}

static NativeResult MPR_MaskMorphology3D_fallback(const char* sessionId, int maskId, MorphologyOperation op, int kernelSize, int connectivity) {
    (void)sessionId;
    (void)maskId;
    (void)op;
    (void)kernelSize;
    (void)connectivity;
    return NATIVE_OK;
}

static NativeResult MPR_MaskBoolean_fallback(const char* sessionId, int maskIdA, int maskIdB, BooleanOperation op, const char* color, const char* name, int* newMaskId) {
    (void)sessionId;
    (void)maskIdA;
    (void)maskIdB;
    (void)op;
    (void)color;
    (void)name;
    if (newMaskId) {
        *newMaskId = maskIdA;
    }
    return NATIVE_OK;
}

extern "C" {
    NativeResult (*__imp_MPR_MaskMorphology2D_stub)(const char*, int, MorphologyOperation, int, int) = MPR_MaskMorphology2D_fallback;
    NativeResult (*__imp_MPR_MaskMorphology3D_stub)(const char*, int, MorphologyOperation, int, int) = MPR_MaskMorphology3D_fallback;
    NativeResult (*__imp_MPR_MaskBoolean_stub)(const char*, int, int, BooleanOperation, const char*, const char*, int*) = MPR_MaskBoolean_fallback;
}

#pragma comment(linker, "/alternatename:__imp_MPR_MaskMorphology2D=__imp_MPR_MaskMorphology2D_stub")
#pragma comment(linker, "/alternatename:__imp_MPR_MaskMorphology3D=__imp_MPR_MaskMorphology3D_stub")
#pragma comment(linker, "/alternatename:__imp_MPR_MaskBoolean=__imp_MPR_MaskBoolean_stub")

namespace fs = std::filesystem;

// 全局存储 APR/MPR 句柄
static std::map<std::string, APRHandle> g_AprHandles;
static std::map<std::string, MPRHandle> g_MprHandles;
static std::map<std::string, VolumeHandle> g_VolumeHandles;
static std::map<std::string, WindowHandle> g_WindowHandles; // 存储窗口句柄

struct VolumeCacheEntry {
    VolumeHandle volume = nullptr;
    int refCount = 0;
    uint64_t lastUsedMs = 0;
    int width = 0;
    int height = 0;
    int depth = 0;
};

// Cache volumes by folderPath (UTF-8), so switching tabs/sessions can reuse already-loaded volume.
static std::map<std::string, VolumeCacheEntry> g_VolumeCache;
static std::map<std::string, std::string> g_SessionToVolumeKey;
static constexpr size_t kMaxCachedVolumes = 2;

static uint64_t NowMs() {
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count());
}

static void EvictUnusedVolumesIfNeeded() {
    if (g_VolumeCache.size() <= kMaxCachedVolumes) return;

    // Evict least-recently-used entries with refCount==0.
    while (g_VolumeCache.size() > kMaxCachedVolumes) {
        auto victimIt = g_VolumeCache.end();
        for (auto it = g_VolumeCache.begin(); it != g_VolumeCache.end(); ++it) {
            if (it->second.refCount != 0) continue;
            if (victimIt == g_VolumeCache.end() || it->second.lastUsedMs < victimIt->second.lastUsedMs) {
                victimIt = it;
            }
        }

        if (victimIt == g_VolumeCache.end()) {
            // All volumes are in use.
            break;
        }

        if (victimIt->second.volume) {
            Dicom_Volume_Destroy(victimIt->second.volume);
        }
        g_VolumeCache.erase(victimIt);
    }
}

// 进度回调存储 - 使用全局变量存储当前回调
static Napi::FunctionReference* g_CurrentProgressCallback = nullptr;
static Napi::Env* g_CurrentEnv = nullptr;

// C 风格的进度回调函数（用于传递给 DLL�?
static void DllProgressCallback(int progress, const char* message) {
    if (g_CurrentProgressCallback && g_CurrentEnv) {
        try {
            Napi::HandleScope scope(*g_CurrentEnv);
            Napi::Function callback = g_CurrentProgressCallback->Value();
            callback.Call({ 
                Napi::Number::New(*g_CurrentEnv, progress), 
                Napi::String::New(*g_CurrentEnv, message) 
            });
        } catch (...) {
            // 忽略回调错误，继续加�?
        }
    }
}

// ==================== HIS4D (4D volume container) ====================

namespace {
static constexpr uint32_t HIS4D_VERSION = 1;
static constexpr uint32_t HIS4D_VOXEL_U16 = 1;

#pragma pack(push, 1)
struct His4dHeader {
    char magic[8];              // "HIS4D\0\0\0"
    uint32_t version;           // 1
    uint32_t headerBytes;       // sizeof(His4dHeader)
    uint32_t width;
    uint32_t height;
    uint32_t depth;
    uint32_t frameCount;
    uint32_t voxelType;         // 1 = uint16
    uint32_t reserved0;
    float spacingX;
    float spacingY;
    float spacingZ;
    float originX;
    float originY;
    float originZ;
    uint64_t offsetsTableOffset;    // absolute file offset
    uint64_t timestampsOffset;      // absolute file offset
    uint64_t dataOffset;            // absolute file offset
    uint64_t bytesPerFrame;         // width*height*depth*bytesPerVoxel
};
#pragma pack(pop)

static bool His4d_IsValidMagic(const His4dHeader& h) {
    return std::memcmp(h.magic, "HIS4D\0\0\0", 8) == 0;
}

#ifdef _WIN32
static std::wstring Utf8ToWideLocal(const std::string& s) {
    if (s.empty()) return std::wstring();
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
    if (len <= 0) return std::wstring();
    std::wstring out;
    out.resize((size_t)len);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), out.data(), len);
    return out;
}

struct His4dMappedFile {
    HANDLE file = INVALID_HANDLE_VALUE;
    HANDLE mapping = nullptr;
    uint8_t* view = nullptr;
    uint64_t size = 0;

    void Close() {
        if (view) {
            UnmapViewOfFile(view);
            view = nullptr;
        }
        if (mapping) {
            CloseHandle(mapping);
            mapping = nullptr;
        }
        if (file != INVALID_HANDLE_VALUE) {
            CloseHandle(file);
            file = INVALID_HANDLE_VALUE;
        }
        size = 0;
    }

    bool OpenReadOnlyUtf8(const std::string& pathUtf8, std::string& outErr) {
        Close();
        std::wstring w = Utf8ToWideLocal(pathUtf8);
        if (w.empty()) {
            outErr = "Utf8ToWide failed";
            return false;
        }

        file = CreateFileW(w.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE) {
            outErr = "CreateFileW failed";
            return false;
        }

        LARGE_INTEGER li;
        if (!GetFileSizeEx(file, &li)) {
            outErr = "GetFileSizeEx failed";
            Close();
            return false;
        }
        if (li.QuadPart <= 0) {
            outErr = "File is empty";
            Close();
            return false;
        }
        size = static_cast<uint64_t>(li.QuadPart);

        mapping = CreateFileMappingW(file, nullptr, PAGE_READONLY, 0, 0, nullptr);
        if (!mapping) {
            outErr = "CreateFileMappingW failed";
            Close();
            return false;
        }

        void* pv = MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, 0);
        if (!pv) {
            outErr = "MapViewOfFile failed";
            Close();
            return false;
        }
        view = static_cast<uint8_t*>(pv);
        return true;
    }
};

struct His4dSession {
    His4dMappedFile mapped;
    His4dHeader header{};
    std::vector<uint64_t> offsets;
    std::vector<double> timestamps;
    int currentFrame = 0;
    VolumeHandle volume = nullptr;
};

static std::map<std::string, His4dSession> g_His4dSessions;

static bool His4d_ParseFromMapped(const His4dMappedFile& mf, His4dHeader& outHeader, std::vector<uint64_t>& outOffsets, std::vector<double>& outTimestamps, std::string& outErr) {
    if (!mf.view || mf.size < sizeof(His4dHeader)) {
        outErr = "Invalid file mapping";
        return false;
    }

    std::memcpy(&outHeader, mf.view, sizeof(His4dHeader));
    if (!His4d_IsValidMagic(outHeader)) {
        outErr = "Bad magic";
        return false;
    }
    if (outHeader.version != HIS4D_VERSION) {
        outErr = "Unsupported version";
        return false;
    }
    if (outHeader.headerBytes != sizeof(His4dHeader)) {
        outErr = "Header size mismatch";
        return false;
    }
    if (outHeader.voxelType != HIS4D_VOXEL_U16) {
        outErr = "Unsupported voxel type";
        return false;
    }
    if (outHeader.frameCount == 0 || outHeader.width == 0 || outHeader.height == 0 || outHeader.depth == 0) {
        outErr = "Invalid dimensions";
        return false;
    }
    if (outHeader.bytesPerFrame == 0) {
        outErr = "Invalid bytesPerFrame";
        return false;
    }

    const uint64_t needOffsetsBytes = static_cast<uint64_t>(outHeader.frameCount) * sizeof(uint64_t);
    const uint64_t needTsBytes = static_cast<uint64_t>(outHeader.frameCount) * sizeof(double);
    if (outHeader.offsetsTableOffset + needOffsetsBytes > mf.size) {
        outErr = "Offsets table out of range";
        return false;
    }
    if (outHeader.timestampsOffset + needTsBytes > mf.size) {
        outErr = "Timestamps table out of range";
        return false;
    }
    if (outHeader.dataOffset >= mf.size) {
        outErr = "Data offset out of range";
        return false;
    }

    outOffsets.resize(outHeader.frameCount);
    std::memcpy(outOffsets.data(), mf.view + outHeader.offsetsTableOffset, needOffsetsBytes);
    outTimestamps.resize(outHeader.frameCount);
    std::memcpy(outTimestamps.data(), mf.view + outHeader.timestampsOffset, needTsBytes);

    // Basic bounds check for each frame.
    for (uint32_t i = 0; i < outHeader.frameCount; ++i) {
        const uint64_t off = outOffsets[i];
        if (off + outHeader.bytesPerFrame > mf.size) {
            outErr = "Frame offset out of range";
            return false;
        }
    }

    return true;
}

static bool His4d_CopyFrameToVolume(const His4dSession& s, int frameIndex, std::string& outErr) {
    if (!s.volume) {
        outErr = "Session volume is null";
        return false;
    }
    if (!s.mapped.view) {
        outErr = "Session file not mapped";
        return false;
    }
    if (frameIndex < 0 || frameIndex >= (int)s.header.frameCount) {
        outErr = "Frame index out of range";
        return false;
    }
    void* dst = Dicom_Volume_GetData(s.volume);
    if (!dst) {
        outErr = "Dicom_Volume_GetData returned null";
        return false;
    }
    const uint64_t off = s.offsets[(size_t)frameIndex];
    std::memcpy(dst, s.mapped.view + off, (size_t)s.header.bytesPerFrame);
    return true;
}
#endif
} // namespace

#ifdef _WIN32

Napi::Value PackHis4dFromFolders(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 2 || !info[0].IsString() || !info[1].IsArray()) {
        Napi::TypeError::New(env, "Expected (outputPath: string, folders: string[] [, timestampsMs?: number[]])").ThrowAsJavaScriptException();
        return env.Null();
    }

    std::string outputPathUtf8 = info[0].As<Napi::String>().Utf8Value();
    Napi::Array foldersArr = info[1].As<Napi::Array>();
    const uint32_t frameCount = foldersArr.Length();
    if (frameCount == 0) {
        Napi::Error::New(env, "folders array is empty").ThrowAsJavaScriptException();
        return env.Null();
    }

    std::vector<double> timestamps;
    timestamps.resize(frameCount, 0.0);
    if (info.Length() >= 3 && info[2].IsArray()) {
        Napi::Array tsArr = info[2].As<Napi::Array>();
        const uint32_t n = std::min(tsArr.Length(), frameCount);
        for (uint32_t i = 0; i < n; ++i) {
            if (tsArr.Get(i).IsNumber()) timestamps[i] = tsArr.Get(i).As<Napi::Number>().DoubleValue();
        }
    }

    int width = 0, height = 0, depth = 0;
    float spx = 1.0f, spy = 1.0f, spz = 1.0f;
    float ox = 0.0f, oy = 0.0f, oz = 0.0f;

    std::vector<uint64_t> offsets;
    offsets.resize(frameCount, 0);

    try {
        fs::path outPath = fs::path(Utf8ToWideLocal(outputPathUtf8));
        std::ofstream out(outPath, std::ios::binary | std::ios::trunc);
        if (!out) {
            Napi::Error::New(env, "Failed to open output file").ThrowAsJavaScriptException();
            return env.Null();
        }

        His4dHeader header{};
        std::memcpy(header.magic, "HIS4D\0\0\0", 8);
        header.version = HIS4D_VERSION;
        header.headerBytes = sizeof(His4dHeader);
        header.frameCount = frameCount;
        header.voxelType = HIS4D_VOXEL_U16;
        header.originX = ox;
        header.originY = oy;
        header.originZ = oz;

        header.offsetsTableOffset = sizeof(His4dHeader);
        header.timestampsOffset = header.offsetsTableOffset + (uint64_t)frameCount * sizeof(uint64_t);
        header.dataOffset = header.timestampsOffset + (uint64_t)frameCount * sizeof(double);

        // Write placeholder header + tables.
        out.write(reinterpret_cast<const char*>(&header), sizeof(header));
        const std::vector<uint8_t> zeros((size_t)frameCount * (sizeof(uint64_t) + sizeof(double)), 0);
        out.write(reinterpret_cast<const char*>(zeros.data()), (std::streamsize)zeros.size());

        for (uint32_t i = 0; i < frameCount; ++i) {
            Napi::Value v = foldersArr.Get(i);
            if (!v.IsString()) {
                Napi::Error::New(env, "folders must be string[]").ThrowAsJavaScriptException();
                return env.Null();
            }
            std::string folderUtf8 = v.As<Napi::String>().Utf8Value();
            std::string folderGbk = Utf8ToAnsi(folderUtf8);

            VolumeHandle vol = Dicom_Volume_Create();
            if (!vol) {
                Napi::Error::New(env, "Dicom_Volume_Create failed").ThrowAsJavaScriptException();
                return env.Null();
            }
            NativeResult r = Dicom_Volume_LoadFromDicomSeries(vol, folderGbk.c_str());
            if (r != NATIVE_OK) {
                const char* err = Dicom_GetLastError();
                Dicom_Volume_Destroy(vol);
                std::string msg = "Dicom_Volume_LoadFromDicomSeries failed: ";
                msg += (err ? err : "unknown");
                Napi::Error::New(env, msg).ThrowAsJavaScriptException();
                return env.Null();
            }

            int w = 0, h = 0, d = 0;
            Dicom_Volume_GetDimensions(vol, &w, &h, &d);
            float vx = 1, vy = 1, vz = 1;
            Dicom_Volume_GetSpacing(vol, &vx, &vy, &vz);

            if (i == 0) {
                width = w; height = h; depth = d;
                spx = vx; spy = vy; spz = vz;
                header.width = (uint32_t)width;
                header.height = (uint32_t)height;
                header.depth = (uint32_t)depth;
                header.spacingX = spx;
                header.spacingY = spy;
                header.spacingZ = spz;

                const uint64_t voxels64 = (uint64_t)width * (uint64_t)height * (uint64_t)depth;
                header.bytesPerFrame = voxels64 * sizeof(uint16_t);
            } else {
                if (w != width || h != height || d != depth) {
                    Dicom_Volume_Destroy(vol);
                    Napi::Error::New(env, "Frame dimensions mismatch").ThrowAsJavaScriptException();
                    return env.Null();
                }
            }

            void* data = Dicom_Volume_GetData(vol);
            if (!data) {
                Dicom_Volume_Destroy(vol);
                Napi::Error::New(env, "Dicom_Volume_GetData returned null").ThrowAsJavaScriptException();
                return env.Null();
            }

            const uint64_t frameOffset = (uint64_t)out.tellp();
            offsets[i] = frameOffset;
            out.write(reinterpret_cast<const char*>(data), (std::streamsize)header.bytesPerFrame);

            Dicom_Volume_Destroy(vol);
        }

        // Patch header/tables.
        out.seekp(0);
        out.write(reinterpret_cast<const char*>(&header), sizeof(header));
        out.seekp((std::streamoff)header.offsetsTableOffset);
        out.write(reinterpret_cast<const char*>(offsets.data()), (std::streamsize)(frameCount * sizeof(uint64_t)));
        out.seekp((std::streamoff)header.timestampsOffset);
        out.write(reinterpret_cast<const char*>(timestamps.data()), (std::streamsize)(frameCount * sizeof(double)));
        out.flush();

        Napi::Object result = Napi::Object::New(env);
        result.Set("success", Napi::Boolean::New(env, true));
        result.Set("outputPath", Napi::String::New(env, outputPathUtf8));
        result.Set("width", Napi::Number::New(env, width));
        result.Set("height", Napi::Number::New(env, height));
        result.Set("depth", Napi::Number::New(env, depth));
        result.Set("frameCount", Napi::Number::New(env, (double)frameCount));
        return result;
    } catch (const std::exception& e) {
        Napi::Error::New(env, std::string("Exception: ") + e.what()).ThrowAsJavaScriptException();
        return env.Null();
    }
}

Napi::Value CreateAPRViewsFromHis4d(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 2 || !info[0].IsString() || !info[1].IsString()) {
        Napi::TypeError::New(env, "Expected (sessionId: string, his4dPath: string)").ThrowAsJavaScriptException();
        return env.Null();
    }

    std::string sessionId = info[0].As<Napi::String>().Utf8Value();
    std::string his4dPathUtf8 = info[1].As<Napi::String>().Utf8Value();

    // Clean any previous mapping for same sessionId.
    auto oldIt = g_His4dSessions.find(sessionId);
    if (oldIt != g_His4dSessions.end()) {
        oldIt->second.mapped.Close();
        if (oldIt->second.volume) {
            Dicom_Volume_Destroy(oldIt->second.volume);
        }
        g_His4dSessions.erase(oldIt);
    }

    His4dSession session;
    std::string err;
    if (!session.mapped.OpenReadOnlyUtf8(his4dPathUtf8, err)) {
        Napi::Error::New(env, std::string("Failed to open his4d: ") + err).ThrowAsJavaScriptException();
        return env.Null();
    }
    if (!His4d_ParseFromMapped(session.mapped, session.header, session.offsets, session.timestamps, err)) {
        session.mapped.Close();
        Napi::Error::New(env, std::string("Failed to parse his4d: ") + err).ThrowAsJavaScriptException();
        return env.Null();
    }

    session.volume = Dicom_Volume_Create();
    if (!session.volume) {
        session.mapped.Close();
        Napi::Error::New(env, "Dicom_Volume_Create failed").ThrowAsJavaScriptException();
        return env.Null();
    }
    if (Dicom_Volume_Allocate(session.volume, (int)session.header.width, (int)session.header.height, (int)session.header.depth) != NATIVE_OK) {
        Dicom_Volume_Destroy(session.volume);
        session.mapped.Close();
        Napi::Error::New(env, "Dicom_Volume_Allocate failed").ThrowAsJavaScriptException();
        return env.Null();
    }
    {
        auto* vol = static_cast<VolumeContext*>(session.volume);
        vol->spacingX = session.header.spacingX;
        vol->spacingY = session.header.spacingY;
        vol->spacingZ = session.header.spacingZ;
        vol->originX = session.header.originX;
        vol->originY = session.header.originY;
        vol->originZ = session.header.originZ;
    }

    if (!His4d_CopyFrameToVolume(session, 0, err)) {
        Dicom_Volume_Destroy(session.volume);
        session.mapped.Close();
        Napi::Error::New(env, std::string("Failed to load frame0: ") + err).ThrowAsJavaScriptException();
        return env.Null();
    }

    // Create 4 APR renderers and windows (same as CreateAPRViews)
    APRHandle aprAxial = APR_Create();
    APRHandle aprCoronal = APR_Create();
    APRHandle aprSagittal = APR_Create();
    APRHandle apr3D = APR_Create();
    if (!aprAxial || !aprCoronal || !aprSagittal || !apr3D) {
        if (aprAxial) APR_Destroy(aprAxial);
        if (aprCoronal) APR_Destroy(aprCoronal);
        if (aprSagittal) APR_Destroy(aprSagittal);
        if (apr3D) APR_Destroy(apr3D);
        Dicom_Volume_Destroy(session.volume);
        session.mapped.Close();
        Napi::Error::New(env, "Failed to create APR renderers").ThrowAsJavaScriptException();
        return env.Null();
    }

    APR_SetVolume(aprAxial, session.volume);
    APR_SetVolume(aprCoronal, session.volume);
    APR_SetVolume(aprSagittal, session.volume);
    APR_SetVolume(apr3D, session.volume);

    APR_SetSessionId(aprAxial, sessionId.c_str());
    APR_SetSessionId(aprCoronal, sessionId.c_str());
    APR_SetSessionId(aprSagittal, sessionId.c_str());
    APR_SetSessionId(apr3D, sessionId.c_str());

    APR_SetSliceDirection(aprAxial, 0);
    APR_SetSliceDirection(aprCoronal, 1);
    APR_SetSliceDirection(aprSagittal, 2);
    APR_SetOrthogonal3DMode(apr3D, true);

    float cx = session.header.width / 2.0f;
    float cy = session.header.height / 2.0f;
    float cz = session.header.depth / 2.0f;
    APR_SetCenter(aprAxial, cx, cy, cz);
    APR_SetCenter(aprCoronal, cx, cy, cz);
    APR_SetCenter(aprSagittal, cx, cy, cz);
    APR_SetCenter(apr3D, cx, cy, cz);

    APR_SetShowCrossHair(aprAxial, true);
    APR_SetShowCrossHair(aprCoronal, true);
    APR_SetShowCrossHair(aprSagittal, true);

    APRHandle aprs[] = { aprAxial, aprCoronal, aprSagittal, apr3D };
    APR_LinkCenter(aprs, 4);

    WindowHandle winAxial = Window_Create(512, 512, "Axial");
    WindowHandle winCoronal = Window_Create(512, 512, "Coronal");
    WindowHandle winSagittal = Window_Create(512, 512, "Sagittal");
    WindowHandle win3D = Window_Create(512, 512, "3D");
    if (!winAxial || !winCoronal || !winSagittal || !win3D) {
        if (winAxial) Window_Destroy(winAxial);
        if (winCoronal) Window_Destroy(winCoronal);
        if (winSagittal) Window_Destroy(winSagittal);
        if (win3D) Window_Destroy(win3D);
        APR_Destroy(aprAxial);
        APR_Destroy(aprCoronal);
        APR_Destroy(aprSagittal);
        APR_Destroy(apr3D);
        Dicom_Volume_Destroy(session.volume);
        session.mapped.Close();
        Napi::Error::New(env, "Failed to create windows").ThrowAsJavaScriptException();
        return env.Null();
    }

    Window_BindRenderer(winAxial, aprAxial, 0);
    Window_BindRenderer(winCoronal, aprCoronal, 0);
    Window_BindRenderer(winSagittal, aprSagittal, 0);
    Window_Set3DViewAPRs(win3D, aprAxial, aprCoronal, aprSagittal);
    Window_Set3DRendererKind(win3D, 1); // ImageBrowser orthogonal 3D

    APR_Render(aprAxial);
    APR_Render(aprCoronal);
    APR_Render(aprSagittal);
    // 3D window render happens in WM_PAINT via the dedicated ImageBrowser renderer.

    Window_Refresh(winAxial);
    Window_Refresh(winCoronal);
    Window_Refresh(winSagittal);
    Window_Refresh(win3D);

    void* hwndAxial = Window_GetNativeHandle(winAxial);
    void* hwndCoronal = Window_GetNativeHandle(winCoronal);
    void* hwndSagittal = Window_GetNativeHandle(winSagittal);
    void* hwnd3D = Window_GetNativeHandle(win3D);

#ifdef _WIN32
    ShowWindow((HWND)hwndAxial, SW_HIDE);
    ShowWindow((HWND)hwndCoronal, SW_HIDE);
    ShowWindow((HWND)hwndSagittal, SW_HIDE);
    ShowWindow((HWND)hwnd3D, SW_HIDE);
#endif

    g_AprHandles[sessionId + "_axial"] = aprAxial;
    g_AprHandles[sessionId + "_coronal"] = aprCoronal;
    g_AprHandles[sessionId + "_sagittal"] = aprSagittal;
    g_AprHandles[sessionId + "_3d"] = apr3D;
    g_WindowHandles[sessionId + "_axial"] = winAxial;
    g_WindowHandles[sessionId + "_coronal"] = winCoronal;
    g_WindowHandles[sessionId + "_sagittal"] = winSagittal;
    g_WindowHandles[sessionId + "_3d"] = win3D;
    g_VolumeHandles[sessionId] = session.volume;

    if (MPR_RegisterSessionVolume(sessionId.c_str(), session.volume) != NATIVE_OK) {
        printf("[CreateAPRViewsFromHis4d] WARNING: Failed to register session volume\n");
    }

    g_His4dSessions[sessionId] = std::move(session);

    Napi::Object resultObj = Napi::Object::New(env);
    resultObj.Set("success", Napi::Boolean::New(env, true));
    resultObj.Set("sessionId", Napi::String::New(env, sessionId));
    resultObj.Set("his4dPath", Napi::String::New(env, his4dPathUtf8));
    resultObj.Set("width", Napi::Number::New(env, (double)g_His4dSessions[sessionId].header.width));
    resultObj.Set("height", Napi::Number::New(env, (double)g_His4dSessions[sessionId].header.height));
    resultObj.Set("depth", Napi::Number::New(env, (double)g_His4dSessions[sessionId].header.depth));
    resultObj.Set("frameCount", Napi::Number::New(env, (double)g_His4dSessions[sessionId].header.frameCount));
    resultObj.Set("windowIdAxial", Napi::String::New(env, sessionId + "_axial"));
    resultObj.Set("windowIdCoronal", Napi::String::New(env, sessionId + "_coronal"));
    resultObj.Set("windowIdSagittal", Napi::String::New(env, sessionId + "_sagittal"));
    resultObj.Set("windowId3D", Napi::String::New(env, sessionId + "_3d"));
    resultObj.Set("hwndAxial", Napi::String::New(env, std::to_string(reinterpret_cast<uint64_t>(hwndAxial))));
    resultObj.Set("hwndCoronal", Napi::String::New(env, std::to_string(reinterpret_cast<uint64_t>(hwndCoronal))));
    resultObj.Set("hwndSagittal", Napi::String::New(env, std::to_string(reinterpret_cast<uint64_t>(hwndSagittal))));
    resultObj.Set("hwnd3D", Napi::String::New(env, std::to_string(reinterpret_cast<uint64_t>(hwnd3D))));

    // timestamps
    Napi::Array tsArr = Napi::Array::New(env, g_His4dSessions[sessionId].timestamps.size());
    for (size_t i = 0; i < g_His4dSessions[sessionId].timestamps.size(); ++i) {
        tsArr.Set((uint32_t)i, Napi::Number::New(env, g_His4dSessions[sessionId].timestamps[i]));
    }
    resultObj.Set("timestampsMs", tsArr);
    return resultObj;
}

Napi::Value His4dSetFrame(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 2 || !info[0].IsString() || !info[1].IsNumber()) {
        Napi::TypeError::New(env, "Expected (sessionId: string, frameIndex: number)").ThrowAsJavaScriptException();
        return env.Null();
    }
    std::string sessionId = info[0].As<Napi::String>().Utf8Value();
    int frameIndex = info[1].As<Napi::Number>().Int32Value();

    Napi::Object result = Napi::Object::New(env);
    auto it = g_His4dSessions.find(sessionId);
    if (it == g_His4dSessions.end()) {
        result.Set("success", Napi::Boolean::New(env, false));
        result.Set("error", Napi::String::New(env, "his4d session not found"));
        return result;
    }

    std::string err;
    if (!His4d_CopyFrameToVolume(it->second, frameIndex, err)) {
        result.Set("success", Napi::Boolean::New(env, false));
        result.Set("error", Napi::String::New(env, err));
        return result;
    }
    it->second.currentFrame = frameIndex;

    // Trigger redraw (safe even with render loop; it just schedules WM_PAINT).
    const std::vector<std::string> winKeys = { sessionId + "_axial", sessionId + "_coronal", sessionId + "_sagittal", sessionId + "_3d" };
    for (const auto& k : winKeys) {
        auto wit = g_WindowHandles.find(k);
        if (wit != g_WindowHandles.end() && wit->second) {
            Window_Invalidate(wit->second);
        }
    }

    result.Set("success", Napi::Boolean::New(env, true));
    result.Set("frameIndex", Napi::Number::New(env, frameIndex));
    return result;
}

Napi::Value His4dGetSessionInfo(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsString()) {
        Napi::TypeError::New(env, "Expected (sessionId: string)").ThrowAsJavaScriptException();
        return env.Null();
    }
    std::string sessionId = info[0].As<Napi::String>().Utf8Value();
    Napi::Object result = Napi::Object::New(env);
    auto it = g_His4dSessions.find(sessionId);
    if (it == g_His4dSessions.end()) {
        result.Set("success", Napi::Boolean::New(env, false));
        result.Set("error", Napi::String::New(env, "his4d session not found"));
        return result;
    }
    const auto& h = it->second.header;
    result.Set("success", Napi::Boolean::New(env, true));
    result.Set("width", Napi::Number::New(env, (double)h.width));
    result.Set("height", Napi::Number::New(env, (double)h.height));
    result.Set("depth", Napi::Number::New(env, (double)h.depth));
    result.Set("frameCount", Napi::Number::New(env, (double)h.frameCount));
    result.Set("currentFrame", Napi::Number::New(env, it->second.currentFrame));
    return result;
}

#endif

// ==================== APR 状态查询（用于双向同步�?===================

Napi::Value GetAPRState(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    // Back-compat: (sessionId: string)
    // Preferred:  (sessionId: string, viewOrWindowId: string)
    //  - viewOrWindowId can be: "axial"|"coronal"|"sagittal"|"3d" or "_axial"... or full windowId (e.g. sessionId + "_axial").
    if (info.Length() < 1 || !info[0].IsString()) {
        Napi::TypeError::New(env, "Expected (sessionId: string, viewOrWindowId?: string)").ThrowAsJavaScriptException();
        return env.Null();
    }

    std::string sessionId = info[0].As<Napi::String>().Utf8Value();

    std::string key;
    if (info.Length() >= 2 && info[1].IsString()) {
        std::string viewOrWindowId = info[1].As<Napi::String>().Utf8Value();
        if (viewOrWindowId.empty()) {
            key = sessionId + "_axial";
        } else if (viewOrWindowId.rfind(sessionId + "_", 0) == 0) {
            // Full windowId (preferred)
            key = viewOrWindowId;
        } else if (viewOrWindowId[0] == '_') {
            key = sessionId + viewOrWindowId;
        } else {
            // View name like "axial" / "3d"
            key = sessionId + "_" + viewOrWindowId;
        }
    } else {
        key = sessionId + "_axial";
    }

    auto it = g_AprHandles.find(key);
    if (it == g_AprHandles.end() || !it->second) {
        Napi::Object result = Napi::Object::New(env);
        result.Set("success", Napi::Boolean::New(env, false));
        result.Set("error", Napi::String::New(env, "APR handle not found"));
        return result;
    }

    float cx = 0.0f, cy = 0.0f, cz = 0.0f;
    float rx = 0.0f, ry = 0.0f, rz = 0.0f;
    float zoom = 1.0f;

    APR_GetCenter(it->second, &cx, &cy, &cz);
    APR_GetRotation(it->second, &rx, &ry, &rz);
    zoom = APR_GetZoom(it->second);

    Napi::Object result = Napi::Object::New(env);
    result.Set("success", Napi::Boolean::New(env, true));
    result.Set("centerX", Napi::Number::New(env, cx));
    result.Set("centerY", Napi::Number::New(env, cy));
    result.Set("centerZ", Napi::Number::New(env, cz));
    result.Set("rotX", Napi::Number::New(env, rx));
    result.Set("rotY", Napi::Number::New(env, ry));
    result.Set("rotZ", Napi::Number::New(env, rz));
    result.Set("zoom", Napi::Number::New(env, zoom));
    return result;
}

// ==================== Completed measurements ====================

Napi::Value GetCompletedMeasurements(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    int total = Measurement_GetCompletedCount();
    if (total <= 0) {
        return Napi::Array::New(env, 0);
    }

    std::vector<CompletedMeasurementInfo> items;
    items.resize((size_t)total);
    int written = Measurement_GetCompletedList(items.data(), total);
    if (written < 0) written = 0;
    if (written > total) written = total;

    Napi::Array arr = Napi::Array::New(env, (uint32_t)written);
    for (int i = 0; i < written; ++i) {
        const auto& m = items[(size_t)i];
        Napi::Object o = Napi::Object::New(env);
        o.Set("sessionId", Napi::String::New(env, m.sessionId));
        o.Set("id", Napi::Number::New(env, m.id));
        o.Set("toolType", Napi::Number::New(env, m.toolType));
        o.Set("result", Napi::Number::New(env, m.result));
        o.Set("isAPR", Napi::Boolean::New(env, m.isAPR));
        o.Set("sliceDirection", Napi::Number::New(env, m.sliceDirection));
        o.Set("sliceIndex", Napi::Number::New(env, m.sliceIndex));
        o.Set("centerX", Napi::Number::New(env, m.centerX));
        o.Set("centerY", Napi::Number::New(env, m.centerY));
        o.Set("centerZ", Napi::Number::New(env, m.centerZ));
        o.Set("rotX", Napi::Number::New(env, m.rotX));
        o.Set("rotY", Napi::Number::New(env, m.rotY));
        o.Set("rotZ", Napi::Number::New(env, m.rotZ));
        arr.Set((uint32_t)i, o);
    }

    return arr;
}

// ==================== Measurement profile ====================

Napi::Value GetMeasurementProfile(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (info.Length() < 2 || !info[0].IsString() || !info[1].IsNumber()) {
        Napi::TypeError::New(env, "Expected (sessionId: string, measurementId: number, maxPoints?: number)")
            .ThrowAsJavaScriptException();
        return env.Null();
    }

    std::string sessionId = info[0].As<Napi::String>().Utf8Value();
    int measurementId = info[1].As<Napi::Number>().Int32Value();
    int maxPoints = 256;
    if (info.Length() >= 3 && info[2].IsNumber()) {
        maxPoints = info[2].As<Napi::Number>().Int32Value();
    }
    if (maxPoints < 2) maxPoints = 2;
    if (maxPoints > 1024) maxPoints = 1024;

    std::vector<double> axis;
    std::vector<double> values;
    axis.resize((size_t)maxPoints);
    values.resize((size_t)maxPoints);

    int n = Measurement_GetProfileData(sessionId.c_str(), measurementId, axis.data(), values.data(), maxPoints);
    if (n < 0) n = 0;
    if (n > maxPoints) n = maxPoints;

    Napi::Array axisArr = Napi::Array::New(env, (uint32_t)n);
    Napi::Array valArr = Napi::Array::New(env, (uint32_t)n);
    for (int i = 0; i < n; ++i) {
        axisArr.Set((uint32_t)i, Napi::Number::New(env, axis[(size_t)i]));
        valArr.Set((uint32_t)i, Napi::Number::New(env, values[(size_t)i]));
    }

    Napi::Object result = Napi::Object::New(env);
    result.Set("axis", axisArr);
    result.Set("values", valArr);
    return result;
}

// ==================== 更新 MPR 中心�?====================

Napi::Value UpdateMPRCenter(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (info.Length() < 4) {
        Napi::TypeError::New(env, "Expected sessionId, x, y, z").ThrowAsJavaScriptException();
        return env.Null();
    }

    std::string sessionId = info[0].As<Napi::String>().Utf8Value();
    float x = info[1].As<Napi::Number>().FloatValue();
    float y = info[2].As<Napi::Number>().FloatValue();
    float z = info[3].As<Napi::Number>().FloatValue();

    const std::vector<std::string> keys = {
        sessionId + "_axial",
        sessionId + "_coronal",
        sessionId + "_sagittal",
    };

    bool updated = false;
    for (const auto& key : keys) {
        auto it = g_MprHandles.find(key);
        if (it != g_MprHandles.end() && it->second) {
            MPR_SetCenter(it->second, x, y, z);
            updated = true;
        }
    }

    Napi::Object result = Napi::Object::New(env);
    result.Set("success", Napi::Boolean::New(env, updated));
    if (!updated) {
        result.Set("error", Napi::String::New(env, "MPR handles not found"));
    }
    return result;
}

// ==================== 窗口更新函数（按需触发重绘�?===================

// 触发所有窗口重绘（发送WM_PAINT消息�?
Napi::Value InvalidateAllWindows(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Window_InvalidateAll();
    return env.Undefined();
}

// 触发单个窗口重绘
Napi::Value InvalidateWindow(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1 || !info[0].IsString()) {
        Napi::TypeError::New(env, "Expected window ID string").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    
    std::string windowId = info[0].As<Napi::String>().Utf8Value();
    
    auto it = g_WindowHandles.find(windowId);
    if (it != g_WindowHandles.end()) {
        Window_Invalidate(it->second);
    }
    
    return env.Undefined();
}

// Reset view state for a specific native window.
Napi::Value ResetView(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsString()) {
        Napi::TypeError::New(env, "Expected (windowId: string)").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    std::string windowId = info[0].As<Napi::String>().Utf8Value();
    auto it = g_WindowHandles.find(windowId);
    if (it == g_WindowHandles.end() || !it->second) {
        return Napi::Boolean::New(env, false);
    }

    Window_ResetView(it->second);
    Window_Invalidate(it->second);
    return Napi::Boolean::New(env, true);
}

// Delete a completed measurement by id.
Napi::Value DeleteMeasurement(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsNumber()) {
        Napi::TypeError::New(env, "Expected (measurementId: number)").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    int measurementId = info[0].As<Napi::Number>().Int32Value();
    bool ok = Measurement_Delete(measurementId);
    return Napi::Boolean::New(env, ok);
}

// Region histogram for area tools (rect/circle), computed from voxels on the measurement slice.
Napi::Value GetMeasurementRegionHistogram(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (info.Length() < 2 || !info[0].IsString() || !info[1].IsNumber()) {
        Napi::TypeError::New(env, "Expected (sessionId: string, measurementId: number)")
            .ThrowAsJavaScriptException();
        return env.Null();
    }

    std::string sessionId = info[0].As<Napi::String>().Utf8Value();
    int measurementId = info[1].As<Napi::Number>().Int32Value();

    int bins[256] = { 0 };
    int minValue = 0;
    int maxValue = 0;
    NativeResult r = Measurement_GetRegionHistogram(
        sessionId.c_str(),
        measurementId,
        bins,
        &minValue,
        &maxValue
    );

    Napi::Object result = Napi::Object::New(env);
    if (r != NATIVE_OK) {
        result.Set("success", Napi::Boolean::New(env, false));
        const char* err = Visualization_GetLastError();
        result.Set("error", Napi::String::New(env, err ? err : "Measurement_GetRegionHistogram failed"));
        result.Set("data", Napi::Array::New(env, 0));
        result.Set("minValue", Napi::Number::New(env, 0));
        result.Set("maxValue", Napi::Number::New(env, 0));
        return result;
    }

    Napi::Array arr = Napi::Array::New(env, 256);
    for (int i = 0; i < 256; ++i) {
        arr.Set((uint32_t)i, Napi::Number::New(env, bins[i]));
    }

    result.Set("success", Napi::Boolean::New(env, true));
    result.Set("data", arr);
    result.Set("minValue", Napi::Number::New(env, minValue));
    result.Set("maxValue", Napi::Number::New(env, maxValue));
    return result;
}

// Toggle 3D window render mode: orthogonal tri-planar vs raycast.
Napi::Value Set3DOrthogonalMode(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (info.Length() < 2 || !info[0].IsString() || !info[1].IsBoolean()) {
        Napi::TypeError::New(env, "Expected (windowId: string, enabled: boolean)")
            .ThrowAsJavaScriptException();
        return env.Undefined();
    }

    std::string windowId = info[0].As<Napi::String>().Utf8Value();
    bool enabled = info[1].As<Napi::Boolean>().Value();

    auto it = g_WindowHandles.find(windowId);
    if (it == g_WindowHandles.end() || !it->second) {
        return Napi::Boolean::New(env, false);
    }

    NativeResult r = Window_Set3DViewOrthogonalMode(it->second, enabled);
    return Napi::Boolean::New(env, r == NATIVE_OK);
}

// Toggle 3D raycast VRAM optimization (downsampled 3D texture upload).
Napi::Value Set3DVramOptimized(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (info.Length() < 2 || !info[0].IsString() || !info[1].IsBoolean()) {
        Napi::TypeError::New(env, "Expected (windowId: string, enabled: boolean)")
            .ThrowAsJavaScriptException();
        return env.Undefined();
    }

    std::string windowId = info[0].As<Napi::String>().Utf8Value();
    bool enabled = info[1].As<Napi::Boolean>().Value();

    auto it = g_WindowHandles.find(windowId);
    if (it == g_WindowHandles.end() || !it->second) {
        return Napi::Boolean::New(env, false);
    }

    NativeResult r = Window_Set3DViewVramOptimized(it->second, enabled);
    return Napi::Boolean::New(env, r == NATIVE_OK);
}

// Toggle 3D raycast mask iso-surface mode (binary mask -> iso=0.5 hit shading).
Napi::Value Set3DMaskIsoSurface(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (info.Length() < 2 || !info[0].IsString() || !info[1].IsBoolean()) {
        Napi::TypeError::New(env, "Expected (windowId: string, enabled: boolean)")
            .ThrowAsJavaScriptException();
        return env.Undefined();
    }

    std::string windowId = info[0].As<Napi::String>().Utf8Value();
    bool enabled = info[1].As<Napi::Boolean>().Value();

    auto it = g_WindowHandles.find(windowId);
    if (it == g_WindowHandles.end() || !it->second) {
        return Napi::Boolean::New(env, false);
    }

    NativeResult r = Window_Set3DViewMaskIsoSurfaceEnabled(it->second, enabled);
    return Napi::Boolean::New(env, r == NATIVE_OK);
}

// Per-window crop box visibility (prevents cross-session bleed between different embedded instances).
Napi::Value SetWindowCropBoxVisible(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (info.Length() < 2 || !info[0].IsString() || !info[1].IsBoolean()) {
        Napi::TypeError::New(env, "Expected (windowId: string, visible: boolean)")
            .ThrowAsJavaScriptException();
        return env.Undefined();
    }

    std::string windowId = info[0].As<Napi::String>().Utf8Value();
    bool visible = info[1].As<Napi::Boolean>().Value();

    auto it = g_WindowHandles.find(windowId);
    if (it == g_WindowHandles.end() || !it->second) {
        return Napi::Boolean::New(env, false);
    }

    NativeResult r = Window_SetCropBoxVisible(it->second, visible);
    return Napi::Boolean::New(env, r == NATIVE_OK);
}

// Set 3D raycast lighting parameters (used by iso-surface shading / raycast pipeline).
Napi::Value Set3DLightParameters(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (info.Length() < 4 || !info[0].IsString() || !info[1].IsNumber() || !info[2].IsNumber() || !info[3].IsNumber()) {
        Napi::TypeError::New(env, "Expected (windowId: string, ambient: number, diffuse: number, specular: number)")
            .ThrowAsJavaScriptException();
        return env.Undefined();
    }

    std::string windowId = info[0].As<Napi::String>().Utf8Value();
    float ambient = info[1].As<Napi::Number>().FloatValue();
    float diffuse = info[2].As<Napi::Number>().FloatValue();
    float specular = info[3].As<Napi::Number>().FloatValue();

    auto it = g_WindowHandles.find(windowId);
    if (it == g_WindowHandles.end() || !it->second) {
        return Napi::Boolean::New(env, false);
    }

    NativeResult r = Window_Set3DViewLightParameters(it->second, ambient, diffuse, specular);
    return Napi::Boolean::New(env, r == NATIVE_OK);
}

// Set 3D transfer function control points.
// points: Array<{value,r,g,b,a}> where all fields are in [0..1].
Napi::Value Set3DTransferFunction(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (info.Length() < 2 || !info[0].IsString() || !info[1].IsArray()) {
        Napi::TypeError::New(env, "Expected (windowId: string, points: array)")
            .ThrowAsJavaScriptException();
        return env.Undefined();
    }

    std::string windowId = info[0].As<Napi::String>().Utf8Value();
    Napi::Array arr = info[1].As<Napi::Array>();

    auto it = g_WindowHandles.find(windowId);
    if (it == g_WindowHandles.end() || !it->second) {
        return Napi::Boolean::New(env, false);
    }

    const uint32_t count = arr.Length();
    std::vector<float> packed;
    packed.reserve((size_t)count * 5);

    for (uint32_t i = 0; i < count; ++i) {
        Napi::Value v = arr.Get(i);
        if (!v.IsObject()) continue;
        Napi::Object o = v.As<Napi::Object>();

        auto getNum = [&](const char* key, float def) -> float {
            Napi::Value nv = o.Get(key);
            if (!nv.IsNumber()) return def;
            return nv.As<Napi::Number>().FloatValue();
        };

        float value = getNum("value", 0.0f);
        float r = getNum("r", 1.0f);
        float g = getNum("g", 1.0f);
        float b = getNum("b", 1.0f);
        float a = getNum("a", 1.0f);

        packed.push_back(value);
        packed.push_back(r);
        packed.push_back(g);
        packed.push_back(b);
        packed.push_back(a);
    }

    const int pointCount = (int)(packed.size() / 5);
    NativeResult r = Window_Set3DViewTransferFunctionPoints(
        it->second,
        packed.empty() ? nullptr : packed.data(),
        pointCount
    );

    return Napi::Boolean::New(env, r == NATIVE_OK);
}

// Query OpenGL vendor/renderer/version for a given window's GL context.
Napi::Value GetGpuInfo(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsString()) {
        Napi::TypeError::New(env, "Expected (windowId: string)").ThrowAsJavaScriptException();
        return env.Null();
    }

    std::string windowId = info[0].As<Napi::String>().Utf8Value();
    auto it = g_WindowHandles.find(windowId);

    Napi::Object result = Napi::Object::New(env);

    if (it == g_WindowHandles.end() || !it->second) {
        result.Set("success", Napi::Boolean::New(env, false));
        result.Set("error", Napi::String::New(env, "Window handle not found"));
        return result;
    }

    char vendor[256] = {0};
    char renderer[256] = {0};
    char version[256] = {0};

    NativeResult r = Window_GetGLInfo(
        it->second,
        vendor,
        (int)sizeof(vendor),
        renderer,
        (int)sizeof(renderer),
        version,
        (int)sizeof(version)
    );

    if (r != NATIVE_OK) {
        result.Set("success", Napi::Boolean::New(env, false));
        const char* err = Visualization_GetLastError();
        result.Set("error", Napi::String::New(env, err ? err : "Window_GetGLInfo failed"));
        return result;
    }

    result.Set("success", Napi::Boolean::New(env, true));
    result.Set("vendor", Napi::String::New(env, vendor));
    result.Set("renderer", Napi::String::New(env, renderer));
    result.Set("version", Napi::String::New(env, version));
    return result;
}

// 调整窗口大小
Napi::Value ResizeWindow(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 5 || !info[0].IsString() || !info[1].IsNumber() || 
        !info[2].IsNumber() || !info[3].IsNumber() || !info[4].IsNumber()) {
        Napi::TypeError::New(env, "Expected (windowId: string, x: number, y: number, width: number, height: number)").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    
    std::string windowId = info[0].As<Napi::String>().Utf8Value();
    int x = info[1].As<Napi::Number>().Int32Value();
    int y = info[2].As<Napi::Number>().Int32Value();
    int width = info[3].As<Napi::Number>().Int32Value();
    int height = info[4].As<Napi::Number>().Int32Value();
    
    auto it = g_WindowHandles.find(windowId);
    if (it != g_WindowHandles.end()) {
        NativeResult result = Window_Resize(it->second, x, y, width, height);
        if (result == NATIVE_OK) {
            return Napi::Boolean::New(env, true);
        }
    }
    
    return Napi::Boolean::New(env, false);
}

// 隐藏所有窗口（Tab切换时使�?- 保留体数据和APR�?
Napi::Value HideAllWindows(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Window_HideAllWindows();
    return env.Undefined();
}

// 显示所有窗口（切换回Viewer Tab时使用）
Napi::Value ShowAllWindows(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Window_ShowAllWindows();
    return env.Undefined();
}

// 销毁所�?D窗口（用于Tab切换时释放资源，保留体数据）
Napi::Value DestroyAll3DWindows(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    // 先调用DLL函数：停止渲染线程并销毁所有标记为3D的窗�?
    Window_DestroyAll3DWindows();

    // 清理Node侧的所有窗口句柄映�?
    g_WindowHandles.clear();

    // 清理APR句柄映射，但不销毁APR本身（保留体数据以便重新打开�?
    g_AprHandles.clear();

    return env.Undefined();
}

// 销毁所有窗口与体数据资源（用于打开新序列或全局清理�?
Napi::Value DestroyAllWindows(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    // 先调用DLL函数：停止渲染线程并销毁所有窗�?
    Window_DestroyAllWindows();

    // 清空所有句柄映�?
    g_WindowHandles.clear();

    // APR句柄已经在Window_Destroy中清理，只需清空映射
    g_AprHandles.clear();

    // MPR句柄清理
    for (auto& pair : g_MprHandles) {
        if (pair.second) {
            MPR_Destroy(pair.second);
        }
    }
    g_MprHandles.clear();

    // 体数据资源清理：volume 统一�?cache 管理，避免重复销�?
    g_VolumeHandles.clear();
    g_SessionToVolumeKey.clear();
    for (auto& pair : g_VolumeCache) {
        if (pair.second.volume) {
            Dicom_Volume_Destroy(pair.second.volume);
        }
    }
    g_VolumeCache.clear();

    return env.Undefined();
}

// 启动固定帧率渲染循环
Napi::Value StartRenderLoop(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    int targetFPS = 60;  // 默认60fps
    if (info.Length() > 0 && info[0].IsNumber()) {
        targetFPS = info[0].As<Napi::Number>().Int32Value();
    }
    
    NativeResult result = Window_StartRenderLoop(targetFPS);
    if (result == NATIVE_OK) {
        return Napi::Boolean::New(env, true);
    }
    
    const char* error = Visualization_GetLastError();
    Napi::Error::New(env, error ? error : "Failed to start render loop").ThrowAsJavaScriptException();
    return Napi::Boolean::New(env, false);
}

// 停止渲染循环
Napi::Value StopRenderLoop(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Window_StopRenderLoop();
    return env.Undefined();
}

// 处理窗口事件（用于确保鼠标操作完成后同步状态）
Napi::Value ProcessWindowEvents(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    // 处理所有挂起的Windows消息
#ifdef _WIN32
    MSG msg;
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
#endif
    return env.Undefined();
}

// Refresh embedded HWND z-order/visibility.
// This is useful after Electron focus changes where Chromium/DComp can occlude child windows.
Napi::Value RefreshAllWindowsZOrder(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    for (const auto& pair : g_WindowHandles) {
        if (!pair.second) continue;
        Window_RefreshZOrder(pair.second);
    }
    return env.Undefined();
}

Napi::Value RefreshWindowZOrder(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsString()) {
        Napi::TypeError::New(env, "Expected (windowId: string)").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    std::string windowId = info[0].As<Napi::String>().Utf8Value();
    auto it = g_WindowHandles.find(windowId);
    if (it == g_WindowHandles.end() || !it->second) {
        return Napi::Boolean::New(env, false);
    }

    return Napi::Boolean::New(env, Window_RefreshZOrder(it->second) == NATIVE_OK);
}

#ifdef _WIN32
static void RaiseEmbeddedHwndNoActivate(HWND hwnd) {
    if (!hwnd || !IsWindow(hwnd)) return;
    // Raise within the parent z-order without activating or showing hidden windows.
    SetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
}
#endif

// Raise (z-order) embedded native HWNDs without forcing visibility.
// This is a stronger variant than Refresh*ZOrder and specifically addresses
// "APR window disappears until click" when Chromium/DComp occludes child HWNDs.
Napi::Value RaiseAllWindows(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
#ifdef _WIN32
    for (const auto& pair : g_WindowHandles) {
        if (!pair.second) continue;
        void* native = Window_GetNativeHandle(pair.second);
        RaiseEmbeddedHwndNoActivate((HWND)native);
    }
#endif
    return env.Undefined();
}

Napi::Value RaiseWindow(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsString()) {
        Napi::TypeError::New(env, "Expected (windowId: string)").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    std::string windowId = info[0].As<Napi::String>().Utf8Value();
    auto it = g_WindowHandles.find(windowId);
    if (it == g_WindowHandles.end() || !it->second) {
        return Napi::Boolean::New(env, false);
    }

#ifdef _WIN32
    void* native = Window_GetNativeHandle(it->second);
    RaiseEmbeddedHwndNoActivate((HWND)native);
    return Napi::Boolean::New(env, true);
#else
    return Napi::Boolean::New(env, false);
#endif
}

// 【保留】手动渲染函�?- 用于非Windows平台或调�?
Napi::Value RenderAllViews(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    // 完全移除GLFW事件处理，使用Electron的事件循�?
    // 不调�?glfwPollEvents() �?Window_PollEvents()
    
    // 查找所有APR句柄（用�?D正交渲染�?
    APRHandle aprAxial = nullptr;
    APRHandle aprCoronal = nullptr;
    APRHandle aprSagittal = nullptr;
    APRHandle apr3D = nullptr;
    WindowHandle win3D = nullptr;
    
    for (const auto& pair : g_AprHandles) {
        if (pair.first.find("_axial") != std::string::npos) aprAxial = pair.second;
        else if (pair.first.find("_coronal") != std::string::npos) aprCoronal = pair.second;
        else if (pair.first.find("_sagittal") != std::string::npos) aprSagittal = pair.second;
        else if (pair.first.find("_3d") != std::string::npos) apr3D = pair.second;
    }
    
    for (const auto& pair : g_WindowHandles) {
        if (pair.first.find("_3d") != std::string::npos) {
            win3D = pair.second;
            break;
        }
    }
    
    // 按顺序渲染前三个2D窗口
    std::vector<std::string> viewOrder = { "_axial", "_coronal", "_sagittal" };
    
    for (const auto& view : viewOrder) {
        WindowHandle window = nullptr;
        APRHandle apr = nullptr;
        
        for (const auto& pair : g_WindowHandles) {
            if (pair.first.find(view) != std::string::npos) {
                window = pair.second;
                break;
            }
        }
        
        for (const auto& pair : g_AprHandles) {
            if (pair.first.find(view) != std::string::npos) {
                apr = pair.second;
                break;
            }
        }
        
        if (window && apr) {
            Window_MakeCurrent(window);
            APR_Render(apr);
            Window_Refresh(window);
        }
    }
    
    // 3D窗口使用正交3D渲染
    if (win3D && aprAxial && aprCoronal && aprSagittal) {
        printf("[RenderAllViews] Rendering 3D: win3D=%p axial=%p coronal=%p sagittal=%p\n",
               win3D, aprAxial, aprCoronal, aprSagittal);
        fflush(stdout);
        Window_MakeCurrent(win3D);
        printf("[RenderAllViews] Calling APR_RenderOrthogonal3D (YELLOW path)...\n");
        fflush(stdout);
        NativeResult r = APR_RenderOrthogonal3D(aprAxial, aprCoronal, aprSagittal);
        printf("[RenderAllViews] APR_RenderOrthogonal3D returned %d\n", r);
        fflush(stdout);
        Window_Refresh(win3D);
        printf("[RenderAllViews] 3D render complete\n");
        fflush(stdout);
    } else {
        printf("[RenderAllViews] 3D render SKIPPED: win3D=%p aprAxial=%p aprCoronal=%p aprSagittal=%p\n",
               win3D, aprAxial, aprCoronal, aprSagittal);
        fflush(stdout);
    }
    
    return env.Undefined();
}

// ==================== APR 视图创建 ====================

Napi::Value CreateAPRViews(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    // 参数：sessionId (string), folderPath (string), progressCallback (function, optional)
    if (info.Length() < 2 || !info[0].IsString() || !info[1].IsString()) {
        Napi::TypeError::New(env, "Expected sessionId and folderPath").ThrowAsJavaScriptException();
        return env.Null();
    }
    
    std::string sessionId = info[0].As<Napi::String>().Utf8Value();
    std::string folderPathUtf8 = info[1].As<Napi::String>().Utf8Value();
    
    // �?UTF-8 路径转换�?GBK（和 dicom_wrapper.cpp 中一样的处理�?
    std::string folderPathGbk;
    try {
        folderPathGbk = Utf8ToAnsi(folderPathUtf8);
    } catch (const std::exception& e) {
        printf("[CreateAPRViews] ERROR: Utf8ToAnsi failed - %s\n", e.what());
        std::string errorMsg = "Path encoding conversion failed: ";
        errorMsg += e.what();
        errorMsg += "\nPath (UTF-8): " + folderPathUtf8;
        Napi::Error::New(env, errorMsg).ThrowAsJavaScriptException();
        return env.Null();
    }
    
    // 测试目录遍历
    try {
        int fileCount = 0;
        for (const auto& entry : fs::directory_iterator(folderPathGbk)) {
            if (entry.is_regular_file()) {
                fileCount++;
                if (fileCount >= 5) break;
            }
        }
        if (fileCount == 0) {
            std::string errorMsg = "No files found in directory (test scan)";
            errorMsg += "\nPath (UTF-8): " + folderPathUtf8;
            errorMsg += "\nPath (GBK): " + folderPathGbk;
            Napi::Error::New(env, errorMsg).ThrowAsJavaScriptException();
            return env.Null();
        }
    } catch (const std::exception& e) {
        printf("[CreateAPRViews] ERROR: Directory iteration failed - %s\n", e.what());
        std::string errorMsg = "Failed to scan directory (test): ";
        errorMsg += e.what();
        errorMsg += "\nPath (UTF-8): " + folderPathUtf8;
        errorMsg += "\nPath (GBK): " + folderPathGbk;
        Napi::Error::New(env, errorMsg).ThrowAsJavaScriptException();
        return env.Null();
    }

    // 获取进度回调（如果提供）
    Napi::Function progressCallback;
    Napi::FunctionReference* callbackRef = nullptr;
    if (info.Length() >= 3 && info[2].IsFunction()) {
        progressCallback = info[2].As<Napi::Function>();
        callbackRef = new Napi::FunctionReference();
        *callbackRef = Napi::Persistent(progressCallback);
        g_CurrentProgressCallback = callbackRef;
        g_CurrentEnv = &env;
    }
    
    try {
        // 创建/复用 Volume（按 folderPath 缓存�?
        VolumeHandle volume = nullptr;
        int width = 0, height = 0, depth = 0;

        auto cacheIt = g_VolumeCache.find(folderPathUtf8);
        if (cacheIt != g_VolumeCache.end() && cacheIt->second.volume) {
            volume = cacheIt->second.volume;
            cacheIt->second.refCount += 1;
            cacheIt->second.lastUsedMs = NowMs();
            width = cacheIt->second.width;
            height = cacheIt->second.height;
            depth = cacheIt->second.depth;
            printf("[CreateAPRViews] Reuse cached volume: %s\n", folderPathUtf8.c_str());
        } else {
            volume = Dicom_Volume_Create();
            if (!volume) {
                printf("[CreateAPRViews] ERROR: Failed to create volume\n");
                if (callbackRef) {
                    delete callbackRef;
                    g_CurrentProgressCallback = nullptr;
                    g_CurrentEnv = nullptr;
                }
                Napi::Error::New(env, "Failed to create volume").ThrowAsJavaScriptException();
                return env.Null();
            }

            NativeResult result = Dicom_Volume_LoadFromDicomSeriesWithProgress(
                volume,
                folderPathGbk.c_str(),
                callbackRef ? DllProgressCallback : nullptr
            );

            // 清理回调
            if (callbackRef) {
                delete callbackRef;
                g_CurrentProgressCallback = nullptr;
                g_CurrentEnv = nullptr;
            }

            if (result != NATIVE_OK) {
                const char* error = Dicom_GetLastError();
                printf("[CreateAPRViews] ERROR: Dicom_Volume_LoadFromDicomSeriesWithProgress failed - %s\n", error ? error : "unknown");
                Dicom_Volume_Destroy(volume);
                std::string errorMsg = error ? error : "Failed to load DICOM series";
                errorMsg += "\nPath (UTF-8): " + folderPathUtf8;
                errorMsg += "\nPath (GBK): " + folderPathGbk;
                Napi::Error::New(env, errorMsg).ThrowAsJavaScriptException();
                return env.Null();
            }

            Dicom_Volume_GetDimensions(volume, &width, &height, &depth);

            VolumeCacheEntry entry;
            entry.volume = volume;
            entry.refCount = 1;
            entry.lastUsedMs = NowMs();
            entry.width = width;
            entry.height = height;
            entry.depth = depth;
            g_VolumeCache[folderPathUtf8] = entry;
            EvictUnusedVolumesIfNeeded();
            printf("[CreateAPRViews] Loaded and cached volume: %s\n", folderPathUtf8.c_str());
        }
        
        // 3. Create 4 APR renderers
        APRHandle aprAxial = APR_Create();
        APRHandle aprCoronal = APR_Create();
        APRHandle aprSagittal = APR_Create();
        APRHandle apr3D = APR_Create();
        
        if (!aprAxial || !aprCoronal || !aprSagittal || !apr3D) {
            Dicom_Volume_Destroy(volume);
            if (aprAxial) APR_Destroy(aprAxial);
            if (aprCoronal) APR_Destroy(aprCoronal);
            if (aprSagittal) APR_Destroy(aprSagittal);
            if (apr3D) APR_Destroy(apr3D);
            Napi::Error::New(env, "Failed to create APR renderers").ThrowAsJavaScriptException();
            return env.Null();
        }
        
        // 4. Set volume data
        APR_SetVolume(aprAxial, volume);
        APR_SetVolume(aprCoronal, volume);
        APR_SetVolume(aprSagittal, volume);
        APR_SetVolume(apr3D, volume);

        // Bind APR renderers to the MPR session so APR_Render can draw mask overlays.
        APR_SetSessionId(aprAxial, sessionId.c_str());
        APR_SetSessionId(aprCoronal, sessionId.c_str());
        APR_SetSessionId(aprSagittal, sessionId.c_str());
        APR_SetSessionId(apr3D, sessionId.c_str());
        
        // 5. Set slice directions
        APR_SetSliceDirection(aprAxial, 0);    // Axial
        APR_SetSliceDirection(aprCoronal, 1);  // Coronal
        APR_SetSliceDirection(aprSagittal, 2); // Sagittal
        
        // 5.5 Enable 3D orthogonal mode for 3D view
        APR_SetOrthogonal3DMode(apr3D, true);
        
        // 6. Set center point
        float cx = width / 2.0f, cy = height / 2.0f, cz = depth / 2.0f;
        APR_SetCenter(aprAxial, cx, cy, cz);
        APR_SetCenter(aprCoronal, cx, cy, cz);
        APR_SetCenter(aprSagittal, cx, cy, cz);
        APR_SetCenter(apr3D, cx, cy, cz);
        
        // 7. Enable crosshair
        APR_SetShowCrossHair(aprAxial, true);
        APR_SetShowCrossHair(aprCoronal, true);
        APR_SetShowCrossHair(aprSagittal, true);
        
        // 8. Link centers (sync)
        APRHandle aprs[] = { aprAxial, aprCoronal, aprSagittal, apr3D };
        APR_LinkCenter(aprs, 4);
        
        // 9. Create 4 windows and bind renderers
        WindowHandle winAxial = Window_Create(512, 512, "Axial");
        WindowHandle winCoronal = Window_Create(512, 512, "Coronal");
        WindowHandle winSagittal = Window_Create(512, 512, "Sagittal");
        WindowHandle win3D = Window_Create(512, 512, "3D");
        
        if (!winAxial || !winCoronal || !winSagittal || !win3D) {
            Dicom_Volume_Destroy(volume);
            APR_Destroy(aprAxial);
            APR_Destroy(aprCoronal);
            APR_Destroy(aprSagittal);
            APR_Destroy(apr3D);
            if (winAxial) Window_Destroy(winAxial);
            if (winCoronal) Window_Destroy(winCoronal);
            if (winSagittal) Window_Destroy(winSagittal);
            if (win3D) Window_Destroy(win3D);
            Napi::Error::New(env, "Failed to create windows").ThrowAsJavaScriptException();
            return env.Null();
        }
        
        // 10. Bind renderers to windows (type 0 = APR)
        Window_BindRenderer(winAxial, aprAxial, 0);
        Window_BindRenderer(winCoronal, aprCoronal, 0);
        Window_BindRenderer(winSagittal, aprSagittal, 0);
        
        // 3D窗口特殊处理：设置为3D视图，绑定三个APR
        Window_Set3DViewAPRs(win3D, aprAxial, aprCoronal, aprSagittal);
        Window_Set3DRendererKind(win3D, 1); // ImageBrowser orthogonal 3D
        Window_Set3DViewOrthogonalMode(win3D, true);
        
        // 11. 初始渲染（这很重要！�?
        APR_Render(aprAxial);
        APR_Render(aprCoronal);
        APR_Render(aprSagittal);
        
        // 3D窗口使用正交3D渲染
         printf("[CreateAPRViews] Initial 3D render: win3D=%p axial=%p coronal=%p sagittal=%p\n",
             win3D, aprAxial, aprCoronal, aprSagittal);
        Window_MakeCurrent(win3D);
        APR_RenderOrthogonal3D(aprAxial, aprCoronal, aprSagittal);
        
        Window_Refresh(winAxial);
        Window_Refresh(winCoronal);
        Window_Refresh(winSagittal);
        Window_Refresh(win3D);
        
        // 12. Get native window handles (HWND on Windows)
        void* hwndAxial = Window_GetNativeHandle(winAxial);
        void* hwndCoronal = Window_GetNativeHandle(winCoronal);
        void* hwndSagittal = Window_GetNativeHandle(winSagittal);
        void* hwnd3D = Window_GetNativeHandle(win3D);
        
        // 立即隐藏窗口，防止GLFW显示它们（必须在嵌入前完成）
        #ifdef _WIN32
        ShowWindow((HWND)hwndAxial, SW_HIDE);
        ShowWindow((HWND)hwndCoronal, SW_HIDE);
        ShowWindow((HWND)hwndSagittal, SW_HIDE);
        ShowWindow((HWND)hwnd3D, SW_HIDE);
        #endif
        
        // 13. Save to global map
        g_AprHandles[sessionId + "_axial"] = aprAxial;
        g_AprHandles[sessionId + "_coronal"] = aprCoronal;
        g_AprHandles[sessionId + "_sagittal"] = aprSagittal;
        g_AprHandles[sessionId + "_3d"] = apr3D;
        g_WindowHandles[sessionId + "_axial"] = winAxial;
        g_WindowHandles[sessionId + "_coronal"] = winCoronal;
        g_WindowHandles[sessionId + "_sagittal"] = winSagittal;
        g_WindowHandles[sessionId + "_3d"] = win3D;
        g_VolumeHandles[sessionId] = volume;
        g_SessionToVolumeKey[sessionId] = folderPathUtf8;
        
        // 13.5. 注册Session和Volume（用于MPR mask功能�?
        if (MPR_RegisterSessionVolume(sessionId.c_str(), volume) != NATIVE_OK) {
            printf("[CreateAPRViews] WARNING: Failed to register session volume for mask operations\n");
        } else {
            printf("[CreateAPRViews] Session volume registered successfully: %s\n", sessionId.c_str());
        }
        
        // Windows: rely on Electron/host message loop (do NOT start a separate event loop).
        // Non-Windows: keep the legacy GLFW event loop behavior.
    #ifndef _WIN32
        Window_StartEventLoop();
    #endif
        
        // 14. Return result with HWNDs
        Napi::Object resultObj = Napi::Object::New(env);
        resultObj.Set("success", Napi::Boolean::New(env, true));
        resultObj.Set("sessionId", Napi::String::New(env, sessionId));
        resultObj.Set("width", Napi::Number::New(env, width));
        resultObj.Set("height", Napi::Number::New(env, height));
        resultObj.Set("depth", Napi::Number::New(env, depth));
        resultObj.Set("centerX", Napi::Number::New(env, cx));
        resultObj.Set("centerY", Napi::Number::New(env, cy));
        resultObj.Set("centerZ", Napi::Number::New(env, cz));
        
        // Return stable window IDs (preferred for subsequent resize/embed APIs)
        resultObj.Set("windowIdAxial", Napi::String::New(env, sessionId + "_axial"));
        resultObj.Set("windowIdCoronal", Napi::String::New(env, sessionId + "_coronal"));
        resultObj.Set("windowIdSagittal", Napi::String::New(env, sessionId + "_sagittal"));
        resultObj.Set("windowId3D", Napi::String::New(env, sessionId + "_3d"));

        // Return HWNDs as strings (avoid precision loss vs JS Number on x64)
        resultObj.Set("hwndAxial", Napi::String::New(env, std::to_string(reinterpret_cast<uint64_t>(hwndAxial))));
        resultObj.Set("hwndCoronal", Napi::String::New(env, std::to_string(reinterpret_cast<uint64_t>(hwndCoronal))));
        resultObj.Set("hwndSagittal", Napi::String::New(env, std::to_string(reinterpret_cast<uint64_t>(hwndSagittal))));
        resultObj.Set("hwnd3D", Napi::String::New(env, std::to_string(reinterpret_cast<uint64_t>(hwnd3D))));
        
        return resultObj;
        
    } catch (const std::exception& e) {
        Napi::Error::New(env, std::string("Exception: ") + e.what()).ThrowAsJavaScriptException();
        return env.Null();
    }
}

// ==================== MPR 视图创建（ROI编辑用：正交切片�?====================

Napi::Value CreateMPRViews(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    // 参数：sessionId (string), folderPath (string), progressCallback (function, optional)
    if (info.Length() < 2 || !info[0].IsString() || !info[1].IsString()) {
        Napi::TypeError::New(env, "Expected sessionId and folderPath").ThrowAsJavaScriptException();
        return env.Null();
    }

    std::string sessionId = info[0].As<Napi::String>().Utf8Value();
    std::string folderPathUtf8 = info[1].As<Napi::String>().Utf8Value();

    // �?UTF-8 路径转换�?GBK（和 dicom_wrapper.cpp 中一样的处理�?
    std::string folderPathGbk;
    try {
        folderPathGbk = Utf8ToAnsi(folderPathUtf8);
    } catch (const std::exception& e) {
        printf("[CreateMPRViews] ERROR: Utf8ToAnsi failed - %s\n", e.what());
        std::string errorMsg = "Path encoding conversion failed: ";
        errorMsg += e.what();
        errorMsg += "\nPath (UTF-8): " + folderPathUtf8;
        Napi::Error::New(env, errorMsg).ThrowAsJavaScriptException();
        return env.Null();
    }

    // 测试目录遍历
    try {
        int fileCount = 0;
        for (const auto& entry : fs::directory_iterator(folderPathGbk)) {
            if (entry.is_regular_file()) {
                fileCount++;
                if (fileCount >= 5) break;
            }
        }
        if (fileCount == 0) {
            std::string errorMsg = "No files found in directory (test scan)";
            errorMsg += "\nPath (UTF-8): " + folderPathUtf8;
            errorMsg += "\nPath (GBK): " + folderPathGbk;
            Napi::Error::New(env, errorMsg).ThrowAsJavaScriptException();
            return env.Null();
        }
    } catch (const std::exception& e) {
        printf("[CreateMPRViews] ERROR: Directory iteration failed - %s\n", e.what());
        std::string errorMsg = "Failed to scan directory (test): ";
        errorMsg += e.what();
        errorMsg += "\nPath (UTF-8): " + folderPathUtf8;
        errorMsg += "\nPath (GBK): " + folderPathGbk;
        Napi::Error::New(env, errorMsg).ThrowAsJavaScriptException();
        return env.Null();
    }

    // 获取进度回调（如果提供）
    Napi::Function progressCallback;
    Napi::FunctionReference* callbackRef = nullptr;
    if (info.Length() >= 3 && info[2].IsFunction()) {
        progressCallback = info[2].As<Napi::Function>();
        callbackRef = new Napi::FunctionReference();
        *callbackRef = Napi::Persistent(progressCallback);
        g_CurrentProgressCallback = callbackRef;
        g_CurrentEnv = &env;
    }

    try {
        // 创建/复用 Volume（按 folderPath 缓存�?
        VolumeHandle volume = nullptr;
        int width = 0, height = 0, depth = 0;

        auto cacheIt = g_VolumeCache.find(folderPathUtf8);
        if (cacheIt != g_VolumeCache.end() && cacheIt->second.volume) {
            volume = cacheIt->second.volume;
            cacheIt->second.refCount += 1;
            cacheIt->second.lastUsedMs = NowMs();
            width = cacheIt->second.width;
            height = cacheIt->second.height;
            depth = cacheIt->second.depth;
            printf("[CreateMPRViews] Reuse cached volume: %s\n", folderPathUtf8.c_str());
        } else {
            volume = Dicom_Volume_Create();
            if (!volume) {
                printf("[CreateMPRViews] ERROR: Failed to create volume\n");
                if (callbackRef) {
                    delete callbackRef;
                    g_CurrentProgressCallback = nullptr;
                    g_CurrentEnv = nullptr;
                }
                Napi::Error::New(env, "Failed to create volume").ThrowAsJavaScriptException();
                return env.Null();
            }

            NativeResult result = Dicom_Volume_LoadFromDicomSeriesWithProgress(
                volume,
                folderPathGbk.c_str(),
                callbackRef ? DllProgressCallback : nullptr
            );

            // 清理回调
            if (callbackRef) {
                delete callbackRef;
                g_CurrentProgressCallback = nullptr;
                g_CurrentEnv = nullptr;
            }

            if (result != NATIVE_OK) {
                const char* error = Dicom_GetLastError();
                printf("[CreateMPRViews] ERROR: Dicom_Volume_LoadFromDicomSeriesWithProgress failed - %s\n", error ? error : "unknown");
                Dicom_Volume_Destroy(volume);
                std::string errorMsg = error ? error : "Failed to load DICOM series";
                errorMsg += "\nPath (UTF-8): " + folderPathUtf8;
                errorMsg += "\nPath (GBK): " + folderPathGbk;
                Napi::Error::New(env, errorMsg).ThrowAsJavaScriptException();
                return env.Null();
            }

            Dicom_Volume_GetDimensions(volume, &width, &height, &depth);

            VolumeCacheEntry entry;
            entry.volume = volume;
            entry.refCount = 1;
            entry.lastUsedMs = NowMs();
            entry.width = width;
            entry.height = height;
            entry.depth = depth;
            g_VolumeCache[folderPathUtf8] = entry;
            EvictUnusedVolumesIfNeeded();
            printf("[CreateMPRViews] Loaded and cached volume: %s\n", folderPathUtf8.c_str());
        }

        // Create 3 MPR renderers (orthogonal slices)
        MPRHandle mprAxial = MPR_Create();
        MPRHandle mprCoronal = MPR_Create();
        MPRHandle mprSagittal = MPR_Create();

        if (!mprAxial || !mprCoronal || !mprSagittal) {
            if (mprAxial) MPR_Destroy(mprAxial);
            if (mprCoronal) MPR_Destroy(mprCoronal);
            if (mprSagittal) MPR_Destroy(mprSagittal);
            Napi::Error::New(env, "Failed to create MPR renderers").ThrowAsJavaScriptException();
            return env.Null();
        }

        if (MPR_SetVolume(mprAxial, volume) != NATIVE_OK ||
            MPR_SetVolume(mprCoronal, volume) != NATIVE_OK ||
            MPR_SetVolume(mprSagittal, volume) != NATIVE_OK) {
            MPR_Destroy(mprAxial);
            MPR_Destroy(mprCoronal);
            MPR_Destroy(mprSagittal);
            Napi::Error::New(env, "Failed to set volume for MPR renderers").ThrowAsJavaScriptException();
            return env.Null();
        }

        MPR_SetSliceDirection(mprAxial, MPR_AXIAL);
        MPR_SetSliceDirection(mprCoronal, MPR_CORONAL);
        MPR_SetSliceDirection(mprSagittal, MPR_SAGITTAL);

        float cx = width / 2.0f, cy = height / 2.0f, cz = depth / 2.0f;
        MPR_SetCenter(mprAxial, cx, cy, cz);
        MPR_SetCenter(mprCoronal, cx, cy, cz);
        MPR_SetCenter(mprSagittal, cx, cy, cz);

        MPR_SetShowCrossHair(mprAxial, true);
        MPR_SetShowCrossHair(mprCoronal, true);
        MPR_SetShowCrossHair(mprSagittal, true);

        // 设置Session ID，让MPR渲染时能从session获取mask数据
        MPR_SetSessionId(mprAxial, sessionId.c_str());
        MPR_SetSessionId(mprCoronal, sessionId.c_str());
        MPR_SetSessionId(mprSagittal, sessionId.c_str());

        MPRHandle mprs[] = { mprAxial, mprCoronal, mprSagittal };
        MPR_LinkCenter(mprs, 3);

        // Create windows and bind renderers (type 1 = MPR)
        WindowHandle winAxial = Window_Create(512, 512, "MPR Axial");
        WindowHandle winCoronal = Window_Create(512, 512, "MPR Coronal");
        WindowHandle winSagittal = Window_Create(512, 512, "MPR Sagittal");
        WindowHandle win3D = Window_Create(512, 512, "ROI 3D");

        if (!winAxial || !winCoronal || !winSagittal || !win3D) {
            if (winAxial) Window_Destroy(winAxial);
            if (winCoronal) Window_Destroy(winCoronal);
            if (winSagittal) Window_Destroy(winSagittal);
            if (win3D) Window_Destroy(win3D);
            MPR_Destroy(mprAxial);
            MPR_Destroy(mprCoronal);
            MPR_Destroy(mprSagittal);
            Napi::Error::New(env, "Failed to create windows").ThrowAsJavaScriptException();
            return env.Null();
        }

        Window_BindRenderer(winAxial, mprAxial, 1);
        Window_BindRenderer(winCoronal, mprCoronal, 1);
        Window_BindRenderer(winSagittal, mprSagittal, 1);

        // ROI 编辑�?3D 窗口：先用正�?3D（APR orthogonal），�?MPR 会话共享同一�?sessionId�?
        // （后续如果要�?Mask 3D/VR，可在这里替换为专用 3D 渲染器）
        APRHandle apr3dAxial = APR_Create();
        APRHandle apr3dCoronal = APR_Create();
        APRHandle apr3dSagittal = APR_Create();
        if (apr3dAxial && apr3dCoronal && apr3dSagittal) {
            APR_SetVolume(apr3dAxial, volume);
            APR_SetVolume(apr3dCoronal, volume);
            APR_SetVolume(apr3dSagittal, volume);
            APR_SetSessionId(apr3dAxial, sessionId.c_str());
            APR_SetSessionId(apr3dCoronal, sessionId.c_str());
            APR_SetSessionId(apr3dSagittal, sessionId.c_str());
            APR_SetSliceDirection(apr3dAxial, 0);
            APR_SetSliceDirection(apr3dCoronal, 1);
            APR_SetSliceDirection(apr3dSagittal, 2);
            APR_SetCenter(apr3dAxial, cx, cy, cz);
            APR_SetCenter(apr3dCoronal, cx, cy, cz);
            APR_SetCenter(apr3dSagittal, cx, cy, cz);

            // Ensure slice buffers/textures are initialized for the orthogonal 3D view.
            // (Otherwise the 3D window can move but show no slices until another window renders.)
            APR_Render(apr3dAxial);
            APR_Render(apr3dCoronal);
            APR_Render(apr3dSagittal);

            Window_Set3DViewAPRs(win3D, apr3dAxial, apr3dCoronal, apr3dSagittal);
            Window_Set3DRendererKind(win3D, 2); // ROI orthogonal 3D
            Window_Set3DViewOrthogonalMode(win3D, true);

            g_AprHandles[sessionId + "_roi3d_axial"] = apr3dAxial;
            g_AprHandles[sessionId + "_roi3d_coronal"] = apr3dCoronal;
            g_AprHandles[sessionId + "_roi3d_sagittal"] = apr3dSagittal;

            // IMPORTANT: do NOT overwrite the session's standard APR keys
            // (sessionId + "_axial/_coronal/_sagittal"). Those belong to the main
            // ImageBrowser APR views. The ROI editor uses its own ROI 3D APR handles.
        } else {
            if (apr3dAxial) APR_Destroy(apr3dAxial);
            if (apr3dCoronal) APR_Destroy(apr3dCoronal);
            if (apr3dSagittal) APR_Destroy(apr3dSagittal);
        }

        // 初始渲染
        MPR_Render(mprAxial);
        MPR_Render(mprCoronal);
        MPR_Render(mprSagittal);
        Window_Refresh(winAxial);
        Window_Refresh(winCoronal);
        Window_Refresh(winSagittal);

        if (apr3dAxial && apr3dCoronal && apr3dSagittal) {
            Window_MakeCurrent(win3D);
            APR_RenderOrthogonal3D(apr3dAxial, apr3dCoronal, apr3dSagittal);
            Window_Refresh(win3D);
        }

        void* hwndAxial = Window_GetNativeHandle(winAxial);
        void* hwndCoronal = Window_GetNativeHandle(winCoronal);
        void* hwndSagittal = Window_GetNativeHandle(winSagittal);
        void* hwnd3D = Window_GetNativeHandle(win3D);

        #ifdef _WIN32
        ShowWindow((HWND)hwndAxial, SW_HIDE);
        ShowWindow((HWND)hwndCoronal, SW_HIDE);
        ShowWindow((HWND)hwndSagittal, SW_HIDE);
        ShowWindow((HWND)hwnd3D, SW_HIDE);
        #endif

        // Save to global maps
        g_MprHandles[sessionId + "_axial"] = mprAxial;
        g_MprHandles[sessionId + "_coronal"] = mprCoronal;
        g_MprHandles[sessionId + "_sagittal"] = mprSagittal;
        g_WindowHandles[sessionId + "_axial"] = winAxial;
        g_WindowHandles[sessionId + "_coronal"] = winCoronal;
        g_WindowHandles[sessionId + "_sagittal"] = winSagittal;
        // Use a dedicated ID for ROI 3D so it doesn't collide with ImageBrowser's sessionId+"_3d".
        g_WindowHandles[sessionId + "_roi3d"] = win3D;
        g_VolumeHandles[sessionId] = volume;
        g_SessionToVolumeKey[sessionId] = folderPathUtf8;

        if (MPR_RegisterSessionVolume(sessionId.c_str(), volume) != NATIVE_OK) {
            printf("[CreateMPRViews] WARNING: Failed to register session volume for mask operations\n");
        }

    #ifndef _WIN32
        Window_StartEventLoop();
    #endif

        Napi::Object resultObj = Napi::Object::New(env);
        resultObj.Set("success", Napi::Boolean::New(env, true));
        resultObj.Set("sessionId", Napi::String::New(env, sessionId));
        resultObj.Set("width", Napi::Number::New(env, width));
        resultObj.Set("height", Napi::Number::New(env, height));
        resultObj.Set("depth", Napi::Number::New(env, depth));
        resultObj.Set("centerX", Napi::Number::New(env, cx));
        resultObj.Set("centerY", Napi::Number::New(env, cy));
        resultObj.Set("centerZ", Napi::Number::New(env, cz));

        resultObj.Set("windowIdAxial", Napi::String::New(env, sessionId + "_axial"));
        resultObj.Set("windowIdCoronal", Napi::String::New(env, sessionId + "_coronal"));
        resultObj.Set("windowIdSagittal", Napi::String::New(env, sessionId + "_sagittal"));
        resultObj.Set("windowId3D", Napi::String::New(env, sessionId + "_roi3d"));

        resultObj.Set("hwndAxial", Napi::String::New(env, std::to_string(reinterpret_cast<uint64_t>(hwndAxial))));
        resultObj.Set("hwndCoronal", Napi::String::New(env, std::to_string(reinterpret_cast<uint64_t>(hwndCoronal))));
        resultObj.Set("hwndSagittal", Napi::String::New(env, std::to_string(reinterpret_cast<uint64_t>(hwndSagittal))));
        resultObj.Set("hwnd3D", Napi::String::New(env, std::to_string(reinterpret_cast<uint64_t>(hwnd3D))));

        return resultObj;
    } catch (const std::exception& e) {
        Napi::Error::New(env, std::string("Exception: ") + e.what()).ThrowAsJavaScriptException();
        return env.Null();
    }
}

// ==================== 渲染 APR 切片 ====================

Napi::Value RenderAPRSlice(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    // 参数：sessionId, viewName ('axial'|'coronal'|'sagittal'|'3d'), width, height
    if (info.Length() < 4) {
        Napi::TypeError::New(env, "Expected sessionId, viewName, width, height").ThrowAsJavaScriptException();
        return env.Null();
    }
    
    std::string sessionId = info[0].As<Napi::String>().Utf8Value();
    std::string viewName = info[1].As<Napi::String>().Utf8Value();
    int canvasWidth = info[2].As<Napi::Number>().Int32Value();
    int canvasHeight = info[3].As<Napi::Number>().Int32Value();

    APRHandle apr = nullptr;
    APRHandle apr3d[3] = { nullptr, nullptr, nullptr };
    bool is3D = (viewName == "3d");

    if (!is3D) {
        std::string key = sessionId + "_" + viewName;
        // 查找 APR 句柄
        auto it = g_AprHandles.find(key);
        if (it == g_AprHandles.end()) {
            printf("[RenderViewToImage] ERROR: APR handle not found for key=%s\n", key.c_str());
            Napi::Error::New(env, "APR handle not found").ThrowAsJavaScriptException();
            return env.Null();
        }
        apr = it->second;
    } else {
        // 3D 视图：使用 roi3d 的三向 APR 句柄进行 Orthogonal 3D 渲染
        const std::string kAxial = sessionId + "_roi3d_axial";
        const std::string kCoronal = sessionId + "_roi3d_coronal";
        const std::string kSagittal = sessionId + "_roi3d_sagittal";
        auto itA = g_AprHandles.find(kAxial);
        auto itC = g_AprHandles.find(kCoronal);
        auto itS = g_AprHandles.find(kSagittal);
        if (itA == g_AprHandles.end() || itC == g_AprHandles.end() || itS == g_AprHandles.end()) {
            printf("[RenderViewToImage] ERROR: APR handle not found for 3d keys=%s/%s/%s\n", kAxial.c_str(), kCoronal.c_str(), kSagittal.c_str());
            Napi::Error::New(env, "APR handle not found for 3d").ThrowAsJavaScriptException();
            return env.Null();
        }
        apr3d[0] = itA->second;
        apr3d[1] = itC->second;
        apr3d[2] = itS->second;
    }
    
    try {
        // 离屏渲染（使用canvas尺寸�?
        WindowHandle offscreen = OffscreenContext_Create(canvasWidth, canvasHeight);
        if (!offscreen) {
            printf("[RenderViewToImage] ERROR: OffscreenContext_Create returned NULL\n");
            Napi::Error::New(env, "Failed to create offscreen context").ThrowAsJavaScriptException();
            return env.Null();
        }
        
        // 渲染到 FBO（传递目标宽高）
        FrameBuffer* fb = nullptr;
        if (!is3D) {
            fb = OffscreenContext_RenderToBuffer(offscreen, apr, 0, canvasWidth, canvasHeight); // 0 = APR type
        } else {
            fb = OffscreenContext_RenderToBuffer(offscreen, apr3d, 2, canvasWidth, canvasHeight); // 2 = Orthogonal 3D (3 APR handles)
        }
        if (!fb || !fb->pixels) {
            OffscreenContext_Destroy(offscreen);
            Napi::Error::New(env, "Failed to render").ThrowAsJavaScriptException();
            return env.Null();
        }
        
        // 转换�?Napi::Buffer (RGBA 格式)
        size_t dataSize = static_cast<size_t>(fb->width) * fb->height * 4;
        Napi::Buffer<unsigned char> buffer = Napi::Buffer<unsigned char>::Copy(
            env, fb->pixels, dataSize
        );
        
        // 清理
        FrameBuffer_Destroy(fb);
        OffscreenContext_Destroy(offscreen);
        
        // 返回结果
        Napi::Object result = Napi::Object::New(env);
        result.Set("success", Napi::Boolean::New(env, true));
        result.Set("pixelData", buffer);
        result.Set("width", Napi::Number::New(env, fb->width));
        result.Set("height", Napi::Number::New(env, fb->height));
        
        return result;
        
    } catch (const std::exception& e) {
        Napi::Error::New(env, std::string("Render exception: ") + e.what()).ThrowAsJavaScriptException();
        return env.Null();
    }
}

// ==================== 更新 APR 中心�?====================

Napi::Value UpdateAPRCenter(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    // 参数：sessionId, x, y, z
    if (info.Length() < 4) {
        Napi::TypeError::New(env, "Expected sessionId, x, y, z").ThrowAsJavaScriptException();
        return env.Null();
    }
    
    std::string sessionId = info[0].As<Napi::String>().Utf8Value();
    float x = info[1].As<Napi::Number>().FloatValue();
    float y = info[2].As<Napi::Number>().FloatValue();
    float z = info[3].As<Napi::Number>().FloatValue();
    
    // 更新所有视图的中心点（因为已经 LinkCenter，只需更新一个即可）
    std::string key = sessionId + "_axial";
    auto it = g_AprHandles.find(key);
    if (it != g_AprHandles.end()) {
        APR_SetCenter(it->second, x, y, z);
        
        // 不主动刷新，由渲染线程统一处理�?0fps固定频率�?
    } else {
        printf("[UpdateAPRCenter] ERROR: APR handle not found for key=%s\n", key.c_str());
    }
    
    Napi::Object result = Napi::Object::New(env);
    result.Set("success", Napi::Boolean::New(env, true));
    return result;
}

// ==================== 更新 APR 旋转 ====================

Napi::Value UpdateAPRRotation(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    // 参数：sessionId, angleX, angleY, angleZ
    if (info.Length() < 4) {
        Napi::TypeError::New(env, "Expected sessionId, angleX, angleY, angleZ").ThrowAsJavaScriptException();
        return env.Null();
    }
    
    std::string sessionId = info[0].As<Napi::String>().Utf8Value();
    float angleX = info[1].As<Napi::Number>().FloatValue();
    float angleY = info[2].As<Napi::Number>().FloatValue();
    float angleZ = info[3].As<Napi::Number>().FloatValue();
    
    // 更新所有视图的旋转�?D正交渲染依赖三视图一致的姿态）
    const std::vector<std::string> keys = {
        sessionId + "_axial",
        sessionId + "_coronal",
        sessionId + "_sagittal",
        sessionId + "_3d",
    };
    for (const auto& key : keys) {
        auto it = g_AprHandles.find(key);
        if (it != g_AprHandles.end() && it->second) {
            APR_SetRotation(it->second, angleX, angleY, angleZ);
        }
    }
    
    Napi::Object result = Napi::Object::New(env);
    result.Set("success", Napi::Boolean::New(env, true));
    return result;
}

// ==================== 清理 APR 资源 ====================

Napi::Value DestroyAPRViews(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1 || !info[0].IsString()) {
        Napi::TypeError::New(env, "Expected sessionId").ThrowAsJavaScriptException();
        return env.Null();
    }
    
    std::string sessionId = info[0].As<Napi::String>().Utf8Value();

#ifdef _WIN32
    // If this session was created from a .his4d file, close the mapping and free its volume.
    auto hisIt = g_His4dSessions.find(sessionId);
    if (hisIt != g_His4dSessions.end()) {
        if (hisIt->second.volume) {
            // Remove from generic volume map to avoid dangling pointers.
            auto vIt = g_VolumeHandles.find(sessionId);
            if (vIt != g_VolumeHandles.end() && vIt->second == hisIt->second.volume) {
                g_VolumeHandles.erase(vIt);
            }
            Dicom_Volume_Destroy(hisIt->second.volume);
            hisIt->second.volume = nullptr;
        }
        hisIt->second.mapped.Close();
        g_His4dSessions.erase(hisIt);
    }
#endif
    
    // 销毁窗�?
    std::vector<std::string> views = { "_axial", "_coronal", "_sagittal", "_3d" };
    for (const auto& view : views) {
        std::string key = sessionId + view;
        auto winIt = g_WindowHandles.find(key);
        if (winIt != g_WindowHandles.end()) {
            Window_Destroy(winIt->second);
            g_WindowHandles.erase(winIt);
        }
    }
    
    // 销�?APR 句柄
    for (const auto& view : views) {
        std::string key = sessionId + view;
        auto it = g_AprHandles.find(key);
        if (it != g_AprHandles.end()) {
            APR_Destroy(it->second);
            g_AprHandles.erase(it);
        }
    }
    
    // 释放 Volume 引用（由 cache 管理生命周期�?
    auto keyIt = g_SessionToVolumeKey.find(sessionId);
    if (keyIt != g_SessionToVolumeKey.end()) {
        auto cacheIt = g_VolumeCache.find(keyIt->second);
        if (cacheIt != g_VolumeCache.end()) {
            cacheIt->second.refCount = std::max(0, cacheIt->second.refCount - 1);
            cacheIt->second.lastUsedMs = NowMs();
        }
        g_SessionToVolumeKey.erase(keyIt);
    }
    g_VolumeHandles.erase(sessionId);
    EvictUnusedVolumesIfNeeded();
    
    Napi::Object result = Napi::Object::New(env);
    result.Set("success", Napi::Boolean::New(env, true));
    return result;
}

Napi::Value DestroyMPRViews(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsString()) {
        Napi::TypeError::New(env, "Expected (sessionId: string)").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    std::string sessionId = info[0].As<Napi::String>().Utf8Value();

    // 销毁窗�?
    std::vector<std::string> views = { "_axial", "_coronal", "_sagittal", "_3d" };
    for (const auto& view : views) {
        std::string key = sessionId + view;
        auto winIt = g_WindowHandles.find(key);
        if (winIt != g_WindowHandles.end()) {
            Window_Destroy(winIt->second);
            g_WindowHandles.erase(winIt);
        }
    }

    // 销�?MPR 句柄
    for (const auto& view : std::vector<std::string>{ "_axial", "_coronal", "_sagittal" }) {
        std::string key = sessionId + view;
        auto it = g_MprHandles.find(key);
        if (it != g_MprHandles.end()) {
            MPR_Destroy(it->second);
            g_MprHandles.erase(it);
        }
    }

    // 销�?ROI 3D 使用�?APR
    // 注意：CreateMPRViews 会把同一�?APR 句柄注册到两�?key�?
    //  - sessionId + "_roi3d_*"（原�?key�?
    //  - sessionId + "_axial/_coronal/_sagittal"（标�?key，便于渲染循环查找）
    // 这里必须避免对同一�?APR_Destroy 两次�?

    APRHandle roiAx = nullptr;
    APRHandle roiCo = nullptr;
    APRHandle roiSa = nullptr;

    {
        auto it = g_AprHandles.find(sessionId + "_roi3d_axial");
        if (it != g_AprHandles.end()) roiAx = it->second;
    }
    {
        auto it = g_AprHandles.find(sessionId + "_roi3d_coronal");
        if (it != g_AprHandles.end()) roiCo = it->second;
    }
    {
        auto it = g_AprHandles.find(sessionId + "_roi3d_sagittal");
        if (it != g_AprHandles.end()) roiSa = it->second;
    }

    // 先移除标�?key 的别名（不销毁，只擦除映射）
    for (const auto& key : std::vector<std::string>{
             sessionId + "_axial",
             sessionId + "_coronal",
             sessionId + "_sagittal",
         }) {
        auto it = g_AprHandles.find(key);
        if (it == g_AprHandles.end()) continue;
        if (it->second == roiAx || it->second == roiCo || it->second == roiSa) {
            g_AprHandles.erase(it);
        }
    }

    // 再销毁并移除 roi3d key
    for (const auto& key : std::vector<std::string>{
             sessionId + "_roi3d_axial",
             sessionId + "_roi3d_coronal",
             sessionId + "_roi3d_sagittal",
         }) {
        auto it = g_AprHandles.find(key);
        if (it != g_AprHandles.end()) {
            APR_Destroy(it->second);
            g_AprHandles.erase(it);
        }
    }

    // 释放 Volume 引用（由 cache 管理生命周期�?
    auto keyIt = g_SessionToVolumeKey.find(sessionId);
    if (keyIt != g_SessionToVolumeKey.end()) {
        auto cacheIt = g_VolumeCache.find(keyIt->second);
        if (cacheIt != g_VolumeCache.end()) {
            cacheIt->second.refCount = std::max(0, cacheIt->second.refCount - 1);
            cacheIt->second.lastUsedMs = NowMs();
        }
        g_SessionToVolumeKey.erase(keyIt);
    }
    g_VolumeHandles.erase(sessionId);
    EvictUnusedVolumesIfNeeded();

    Napi::Object result = Napi::Object::New(env);
    result.Set("success", Napi::Boolean::New(env, true));
    return result;
}

// ==================== Crosshair 显示/隐藏 ====================

Napi::Value SetCrosshairVisible(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (info.Length() < 2 || !info[0].IsString() || !info[1].IsBoolean()) {
        Napi::TypeError::New(env, "Expected (sessionId: string, visible: boolean)").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    std::string sessionId = info[0].As<Napi::String>().Utf8Value();
    bool visible = info[1].As<Napi::Boolean>().Value();

    const std::vector<std::string> keys = {
        sessionId + "_axial",
        sessionId + "_coronal",
        sessionId + "_sagittal",
    };

    for (const auto& key : keys) {
        auto it = g_AprHandles.find(key);
        if (it != g_AprHandles.end() && it->second) {
            APR_SetShowCrossHair(it->second, visible);
        }

        auto mit = g_MprHandles.find(key);
        if (mit != g_MprHandles.end() && mit->second) {
            MPR_SetShowCrossHair(mit->second, visible);
        }
    }

    return env.Undefined();
}

// ==================== 嵌入原生窗口�?Electron ====================

Napi::Value EmbedWindow(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    // 参数: windowIdOrHwnd (string|number), electronHwnd (Buffer), x, y, width, height
    if (info.Length() < 6) {
        Napi::TypeError::New(env, "Expected 6 arguments").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    // Preferred: windowId string (e.g. sessionId + "_axial").
    // Legacy fallback: native HWND value.
    std::string windowId;
    bool hasWindowId = false;
    uint64_t hwndValue = 0;
    if (info[0].IsString()) {
        windowId = info[0].As<Napi::String>().Utf8Value();
        hasWindowId = !windowId.empty();
    } else if (info[0].IsNumber()) {
        hwndValue = static_cast<uint64_t>(info[0].As<Napi::Number>().Int64Value());
    } else {
        Napi::TypeError::New(env, "Expected first argument to be windowId (string) or hwnd (number)").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    
    // Electron 主窗口句�?
    Napi::Buffer<void> electronBuffer = info[1].As<Napi::Buffer<void>>();
    HWND electronHwnd = *(HWND*)electronBuffer.Data();
    
    // 位置和大�?
    int x = info[2].As<Napi::Number>().Int32Value();
    int y = info[3].As<Napi::Number>().Int32Value();
    int width = info[4].As<Napi::Number>().Int32Value();
    int height = info[5].As<Napi::Number>().Int32Value();
    
    // 查找对应�?WindowHandle
    WindowHandle windowHandle = nullptr;
    if (hasWindowId) {
        auto it = g_WindowHandles.find(windowId);
        if (it != g_WindowHandles.end()) {
            windowHandle = it->second;
        }
    } else {
        for (const auto& pair : g_WindowHandles) {
            void* nativeHandle = Window_GetNativeHandle(pair.second);
            if (reinterpret_cast<uint64_t>(nativeHandle) == hwndValue) {
                windowHandle = pair.second;
                break;
            }
        }
    }
    
    if (!windowHandle) {
        if (hasWindowId) {
            printf("[EmbedWindow] ERROR: Could not find WindowHandle for id=%s\n", windowId.c_str());
        } else {
            printf("[EmbedWindow] ERROR: Could not find WindowHandle for HWND 0x%llx\n", hwndValue);
        }
        Napi::Error::New(env, "Window handle not found").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    
    // 使用 DLL 的新函数来设置父窗口
    NativeResult result = Window_SetParentWindow(windowHandle, electronHwnd, x, y, width, height);
    
    if (result != NATIVE_OK) {
        const char* error = Visualization_GetLastError();
        printf("[EmbedWindow] ERROR: Window_SetParentWindow failed: %s\n", error ? error : "unknown");
        Napi::Error::New(env, error ? error : "Failed to set parent window").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    
    return env.Undefined();
}

// ==================== 工具管理和裁切框 API ====================

// 设置窗口的工具类型（0=定位�? 1=Line, 2=Angle, 3=Rect, 4=Circle, 5=Bezier�?
Napi::Value SetWindowToolType(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 2 || !info[0].IsString() || !info[1].IsNumber()) {
        Napi::TypeError::New(env, "Expected (windowId: string, toolType: number)").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    
    std::string windowId = info[0].As<Napi::String>().Utf8Value();
    int toolType = info[1].As<Napi::Number>().Int32Value();
    
    auto it = g_WindowHandles.find(windowId);
    if (it != g_WindowHandles.end()) {
        Window_SetToolType(it->second, toolType);
        return Napi::Boolean::New(env, true);
    }
    
    return Napi::Boolean::New(env, false);
}

// ==================== Window/Level API ====================

// Set window/level for all views in a session (APR + MPR, if present).
// Args: (sessionId: string, windowWidth: number, windowLevel: number)
Napi::Value SetSessionWindowLevel(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 3 || !info[0].IsString() || !info[1].IsNumber() || !info[2].IsNumber()) {
        Napi::TypeError::New(env, "Expected (sessionId: string, windowWidth: number, windowLevel: number)").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    const std::string sessionId = info[0].As<Napi::String>().Utf8Value();
    const float ww = info[1].As<Napi::Number>().FloatValue();
    const float wl = info[2].As<Napi::Number>().FloatValue();

    // Apply to APR handles (including ROI 3D orthogonal APRs if registered).
    for (auto& pair : g_AprHandles) {
        const auto& key = pair.first;
        if (key.rfind(sessionId + "_", 0) != 0) continue;
        if (pair.second) {
            APR_SetWindowLevel(pair.second, ww, wl);
        }
    }

    // Apply to MPR handles.
    for (auto& pair : g_MprHandles) {
        const auto& key = pair.first;
        if (key.rfind(sessionId + "_", 0) != 0) continue;
        if (pair.second) {
            MPR_SetWindowLevel(pair.second, ww, wl);
        }
    }

    return env.Undefined();
}

// Get current window/level for a session.
// Returns: { windowWidth, windowLevel }
Napi::Value GetSessionWindowLevel(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsString()) {
        Napi::TypeError::New(env, "Expected (sessionId: string)").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    const std::string sessionId = info[0].As<Napi::String>().Utf8Value();
    float ww = 0.0f;
    float wl = 0.0f;
    bool found = false;

    // Prefer APR 2D view if available.
    {
        auto it = g_AprHandles.find(sessionId + "_axial");
        if (it != g_AprHandles.end() && it->second) {
            APR_GetWindowLevel(it->second, &ww, &wl);
            found = true;
        }
    }

    // Fallback to MPR axial.
    {
        auto it = g_MprHandles.find(sessionId + "_axial");
        if (it != g_MprHandles.end() && it->second) {
            MPR_GetWindowLevel(it->second, &ww, &wl);
            found = true;
        }
    }

    Napi::Object out = Napi::Object::New(env);
    out.Set("success", Napi::Boolean::New(env, found));
    out.Set("windowWidth", Napi::Number::New(env, ww));
    out.Set("windowLevel", Napi::Number::New(env, wl));
    return out;
}

// Set MIP/MinIP projection mode for APR views
// Args: (sessionId: string, mode: number, thickness: number)
// mode: 0=Normal (single slice), 1=MIP (max intensity), 2=MinIP (min intensity)
Napi::Value SetSessionProjectionMode(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 3 || !info[0].IsString() || !info[1].IsNumber() || !info[2].IsNumber()) {
        Napi::TypeError::New(env, "Expected (sessionId: string, mode: number, thickness: number)").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    const std::string sessionId = info[0].As<Napi::String>().Utf8Value();
    const int mode = info[1].As<Napi::Number>().Int32Value();
    const float thickness = info[2].As<Napi::Number>().FloatValue();

    // Apply to APR handles
    for (auto& pair : g_AprHandles) {
        const auto& key = pair.first;
        if (key.rfind(sessionId + "_", 0) != 0) continue;
        if (pair.second) {
            APR_SetProjectionMode(pair.second, mode, thickness);
        }
    }

    return env.Undefined();
}

// Get current projection mode for a session
// Returns: { mode, thickness }
Napi::Value GetSessionProjectionMode(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsString()) {
        Napi::TypeError::New(env, "Expected (sessionId: string)").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    const std::string sessionId = info[0].As<Napi::String>().Utf8Value();
    int mode = 0;
    float thickness = 10.0f;

    auto it = g_AprHandles.find(sessionId + "_axial");
    if (it != g_AprHandles.end() && it->second) {
        APR_GetProjectionMode(it->second, &mode, &thickness);
    }

    Napi::Object out = Napi::Object::New(env);
    out.Set("mode", Napi::Number::New(env, mode));
    out.Set("thickness", Napi::Number::New(env, thickness));
    return out;
}

// 启用/禁用 APR 裁切�?
Napi::Value EnableAPRCropBox(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1 || !info[0].IsBoolean()) {
        Napi::TypeError::New(env, "Expected (enable: boolean)").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    
    bool enable = info[0].As<Napi::Boolean>().Value();
    APR_EnableCropBox(enable);
    
    return env.Undefined();
}

// 初始�?APR 裁切框（以体数据中心为初始位置）
Napi::Value SetAPRCropBox(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 3 || !info[0].IsNumber() || !info[1].IsNumber() || !info[2].IsNumber()) {
        Napi::TypeError::New(env, "Expected (width: number, height: number, depth: number)").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    
    int width = info[0].As<Napi::Number>().Int32Value();
    int height = info[1].As<Napi::Number>().Int32Value();
    int depth = info[2].As<Napi::Number>().Int32Value();
    
    APR_SetCropBox(width, height, depth);
    
    return env.Undefined();
}

// 设置 APR 裁切框范围（volume space coords�?
Napi::Value SetAPRCropBoxRange(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 6 ||
        !info[0].IsNumber() || !info[1].IsNumber() ||
        !info[2].IsNumber() || !info[3].IsNumber() ||
        !info[4].IsNumber() || !info[5].IsNumber()) {
        Napi::TypeError::New(env, "Expected (xStart, xEnd, yStart, yEnd, zStart, zEnd)").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    float xStart = info[0].As<Napi::Number>().FloatValue();
    float xEnd = info[1].As<Napi::Number>().FloatValue();
    float yStart = info[2].As<Napi::Number>().FloatValue();
    float yEnd = info[3].As<Napi::Number>().FloatValue();
    float zStart = info[4].As<Napi::Number>().FloatValue();
    float zEnd = info[5].As<Napi::Number>().FloatValue();

    APR_SetCropBoxRange(xStart, xEnd, yStart, yEnd, zStart, zEnd);
    return env.Undefined();
}

// 获取 APR 裁切框范�?
Napi::Value GetAPRCropBox(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    float xStart = 0, xEnd = 0, yStart = 0, yEnd = 0, zStart = 0, zEnd = 0;
    APR_GetCropBox(&xStart, &xEnd, &yStart, &yEnd, &zStart, &zEnd);

    Napi::Object out = Napi::Object::New(env);
    out.Set("xStart", Napi::Number::New(env, xStart));
    out.Set("xEnd", Napi::Number::New(env, xEnd));
    out.Set("yStart", Napi::Number::New(env, yStart));
    out.Set("yEnd", Napi::Number::New(env, yEnd));
    out.Set("zStart", Napi::Number::New(env, zStart));
    out.Set("zEnd", Napi::Number::New(env, zEnd));
    return out;
}

// 检查裁切框是否启用
Napi::Value IsAPRCropBoxEnabled(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    bool enabled = APR_IsCropBoxEnabled();
    return Napi::Boolean::New(env, enabled);
}

// 设置裁切形状
// Args: (shape: number)  0=立方�? 1=球体, 2=圆柱�?
Napi::Value SetAPRCropShape(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsNumber()) {
        Napi::TypeError::New(env, "Expected (shape: number)").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    int shape = info[0].As<Napi::Number>().Int32Value();
    APR_SetCropShape(shape);
    return env.Undefined();
}

// 获取裁切形状
Napi::Value GetAPRCropShape(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    int shape = APR_GetCropShape();
    return Napi::Number::New(env, shape);
}

// 设置圆柱体方�?
// Args: (direction: number)  0=轴向, 1=冠状, 2=矢状
Napi::Value SetAPRCropCylinderDirection(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsNumber()) {
        Napi::TypeError::New(env, "Expected (direction: number)").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    int direction = info[0].As<Napi::Number>().Int32Value();
    APR_SetCropCylinderDirection(direction);
    return env.Undefined();
}

// 获取圆柱体方�?
Napi::Value GetAPRCropCylinderDirection(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    int direction = APR_GetCropCylinderDirection();
    return Napi::Number::New(env, direction);
}

// 按尺寸设置裁切框
// Args: (sizeX: number, sizeY: number, sizeZ: number, volumeWidth: number, volumeHeight: number, volumeDepth: number)
Napi::Value SetAPRCropBoxSize(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 6) {
        Napi::TypeError::New(env, "Expected (sizeX, sizeY, sizeZ, volumeWidth, volumeHeight, volumeDepth)").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    int sizeX = info[0].As<Napi::Number>().Int32Value();
    int sizeY = info[1].As<Napi::Number>().Int32Value();
    int sizeZ = info[2].As<Napi::Number>().Int32Value();
    int volumeWidth = info[3].As<Napi::Number>().Int32Value();
    int volumeHeight = info[4].As<Napi::Number>().Int32Value();
    int volumeDepth = info[5].As<Napi::Number>().Int32Value();
    APR_SetCropBoxSize(sizeX, sizeY, sizeZ, volumeWidth, volumeHeight, volumeDepth);
    return env.Undefined();
}

// 获取裁切设置的完整状态（用于对话框）
// Returns: { shape, cylinderDirection, cropBox: {xStart, xEnd, ...}, enabled }
Napi::Value GetAPRCropSettings(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    Napi::Object result = Napi::Object::New(env);
    result.Set("shape", Napi::Number::New(env, APR_GetCropShape()));
    result.Set("cylinderDirection", Napi::Number::New(env, APR_GetCropCylinderDirection()));
    result.Set("enabled", Napi::Boolean::New(env, APR_IsCropBoxEnabled()));
    
    // 裁切框范�?
    float xStart = 0, xEnd = 0, yStart = 0, yEnd = 0, zStart = 0, zEnd = 0;
    APR_GetCropBox(&xStart, &xEnd, &yStart, &yEnd, &zStart, &zEnd);
    
    Napi::Object cropBox = Napi::Object::New(env);
    cropBox.Set("xStart", Napi::Number::New(env, xStart));
    cropBox.Set("xEnd", Napi::Number::New(env, xEnd));
    cropBox.Set("yStart", Napi::Number::New(env, yStart));
    cropBox.Set("yEnd", Napi::Number::New(env, yEnd));
    cropBox.Set("zStart", Napi::Number::New(env, zStart));
    cropBox.Set("zEnd", Napi::Number::New(env, zEnd));
    result.Set("cropBox", cropBox);
    
    return result;
}

// 执行裁切操作
// Args: (sessionId: string)
// Returns: { success, width, height, depth, spacingX, spacingY, spacingZ }
Napi::Value CropVolume(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1 || !info[0].IsString()) {
        Napi::TypeError::New(env, "Expected (sessionId: string)").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    
    std::string sessionId = info[0].As<Napi::String>().Utf8Value();
    
    Napi::Object result = Napi::Object::New(env);
    
    // 获取 axial APR handle
    auto it = g_AprHandles.find(sessionId + "_axial");
    if (it == g_AprHandles.end() || !it->second) {
        result.Set("success", Napi::Boolean::New(env, false));
        result.Set("error", Napi::String::New(env, "Session not found or axial APR not available"));
        return result;
    }
    
    // 执行裁切
    APRHandle croppedHandle = APR_CropVolume(it->second);
    if (!croppedHandle) {
        result.Set("success", Napi::Boolean::New(env, false));
        result.Set("error", Napi::String::New(env, "Crop operation failed"));
        return result;
    }
    
    // 使用公开API获取裁切后的尺寸和spacing
    int newWidth = 0, newHeight = 0, newDepth = 0;
    APR_GetCroppedVolumeDimensions(&newWidth, &newHeight, &newDepth);
    
    float spacingX = 1.0f, spacingY = 1.0f, spacingZ = 1.0f;
    APR_GetCroppedVolumeSpacing(&spacingX, &spacingY, &spacingZ);
    
    result.Set("success", Napi::Boolean::New(env, true));
    result.Set("width", Napi::Number::New(env, newWidth));
    result.Set("height", Napi::Number::New(env, newHeight));
    result.Set("depth", Napi::Number::New(env, newDepth));
    result.Set("spacingX", Napi::Number::New(env, spacingX));
    result.Set("spacingY", Napi::Number::New(env, spacingY));
    result.Set("spacingZ", Napi::Number::New(env, spacingZ));
    
    return result;
}

// 将裁切结果应用到当前 session (替换所有 APR 的 volume)
// Args: (sessionId: string)
Napi::Value ApplyCroppedVolumeToSession(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1 || !info[0].IsString()) {
        Napi::TypeError::New(env, "Expected (sessionId: string)").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    
    std::string sessionId = info[0].As<Napi::String>().Utf8Value();
    Napi::Object result = Napi::Object::New(env);
    
    // 调用DLL API执行应用操作 - 使用 session-aware 版本
    int success = APR_ApplyCroppedVolumeForSession(sessionId.c_str());
    if (!success) {
        result.Set("success", Napi::Boolean::New(env, false));
        result.Set("error", Napi::String::New(env, "No cropped volume available for this session or apply failed"));
        return result;
    }
    
    // 获取应用后的尺寸
    int width = 0, height = 0, depth = 0;
    APR_GetCroppedVolumeDimensions(&width, &height, &depth);
    
    result.Set("success", Napi::Boolean::New(env, true));
    result.Set("width", Napi::Number::New(env, width));
    result.Set("height", Napi::Number::New(env, height));
    result.Set("depth", Napi::Number::New(env, depth));
    
    return result;
}

// ==================== MPR Mask 编辑和管�?API ====================

// 获取体数�?spacing (mm)
Napi::Value GetVolumeSpacing(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsString()) {
        Napi::TypeError::New(env, "Expected (sessionId: string)").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    std::string sessionId = info[0].As<Napi::String>().Utf8Value();
    float sx = 0.0f, sy = 0.0f, sz = 0.0f;

    NativeResult result = MPR_GetVolumeSpacing(sessionId.c_str(), &sx, &sy, &sz);
    if (result != NATIVE_OK) {
        const char* error = Visualization_GetLastError();
        Napi::Error::New(env, error ? error : "Failed to get volume spacing").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    Napi::Object out = Napi::Object::New(env);
    out.Set("spacingX", Napi::Number::New(env, sx));
    out.Set("spacingY", Napi::Number::New(env, sy));
    out.Set("spacingZ", Napi::Number::New(env, sz));
    return out;
}

// 获取体数据直方图
Napi::Value GetVolumeHistogram(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1 || !info[0].IsString()) {
        Napi::TypeError::New(env, "Expected (sessionId: string)").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    
    std::string sessionId = info[0].As<Napi::String>().Utf8Value();
    
    int histogram[256];
    int minValue = 0, maxValue = 0;
    
    NativeResult result = MPR_GetVolumeHistogram(
        sessionId.c_str(),
        histogram,
        &minValue,
        &maxValue
    );
    
    if (result != NATIVE_OK) {
        Napi::Error::New(env, "Failed to get volume histogram").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    
    // 构造返回对�?
    Napi::Object resultObj = Napi::Object::New(env);
    Napi::Array dataArray = Napi::Array::New(env, 256);
    for (int i = 0; i < 256; i++) {
        dataArray[i] = Napi::Number::New(env, histogram[i]);
    }
    
    resultObj.Set("data", dataArray);
    resultObj.Set("minValue", Napi::Number::New(env, minValue));
    resultObj.Set("maxValue", Napi::Number::New(env, maxValue));
    
    return resultObj;
}

// 获取指定 mask 的统计信息（HU min/max/hist/mean/std/count/volume�?
Napi::Value GetMaskStatistics(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (info.Length() < 2 || !info[0].IsString() || !info[1].IsNumber()) {
        Napi::TypeError::New(env, "Expected (sessionId: string, maskId: number)").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    std::string sessionId = info[0].As<Napi::String>().Utf8Value();
    int maskId = info[1].As<Napi::Number>().Int32Value();

    int histogram[256];
    int minValue = 0, maxValue = 0;
    double mean = 0.0, stddev = 0.0;
    unsigned long long count = 0;
    double volumeMm3 = 0.0;

    NativeResult result = MPR_GetMaskStatistics(
        sessionId.c_str(),
        maskId,
        histogram,
        &minValue,
        &maxValue,
        &mean,
        &stddev,
        &count,
        &volumeMm3
    );

    if (result != NATIVE_OK) {
        const char* error = Visualization_GetLastError();
        Napi::Error::New(env, error ? error : "Failed to get mask statistics").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    Napi::Object resultObj = Napi::Object::New(env);
    Napi::Array dataArray = Napi::Array::New(env, 256);
    for (int i = 0; i < 256; i++) {
        dataArray[i] = Napi::Number::New(env, histogram[i]);
    }

    resultObj.Set("histogram", dataArray);
    resultObj.Set("minValue", Napi::Number::New(env, minValue));
    resultObj.Set("maxValue", Napi::Number::New(env, maxValue));
    resultObj.Set("mean", Napi::Number::New(env, mean));
    resultObj.Set("stdDev", Napi::Number::New(env, stddev));
    resultObj.Set("count", Napi::Number::New(env, static_cast<double>(count)));
    resultObj.Set("volumeMm3", Napi::Number::New(env, volumeMm3));
    return resultObj;
}

// 计算骨分析指�?
Napi::Value CalculateBoneMetrics(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (info.Length() < 2 || !info[0].IsString() || !info[1].IsNumber() || (info.Length() >= 3 && !info[2].IsNumber())) {
        Napi::TypeError::New(env, "Expected (sessionId: string, maskId: number, roiMaskId?: number)").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    std::string sessionId = info[0].As<Napi::String>().Utf8Value();
    int maskId = info[1].As<Napi::Number>().Int32Value();
    int roiMaskId = 0;
    if (info.Length() >= 3) {
        roiMaskId = info[2].As<Napi::Number>().Int32Value();
    }

    BoneMetrics m{};
    NativeResult result = (roiMaskId > 0)
        ? MPR_CalculateBoneMetricsEx(sessionId.c_str(), maskId, roiMaskId, &m)
        : MPR_CalculateBoneMetrics(sessionId.c_str(), maskId, &m);
    if (result != NATIVE_OK) {
        const char* error = Visualization_GetLastError();
        Napi::Error::New(env, error ? error : "Failed to calculate bone metrics").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    Napi::Object out = Napi::Object::New(env);
    out.Set("maskId", Napi::Number::New(env, m.maskId));
    out.Set("roiMaskId", Napi::Number::New(env, m.roiMaskId));
    out.Set("voxelCount", Napi::Number::New(env, m.voxelCount));
    out.Set("roiVoxelCount", Napi::Number::New(env, m.roiVoxelCount));
    out.Set("tvRoiMm3", Napi::Number::New(env, m.tvRoiMm3));
    out.Set("mvRoiMm3", Napi::Number::New(env, m.mvRoiMm3));
    out.Set("bv_tv_roi", Napi::Number::New(env, m.bv_tv_roi));
    out.Set("volumeMm3", Napi::Number::New(env, m.volumeMm3));
    out.Set("volumeCm3", Napi::Number::New(env, m.volumeCm3));
    out.Set("surfaceAreaMm2", Napi::Number::New(env, m.surfaceAreaMm2));
    out.Set("surfaceAreaCm2", Napi::Number::New(env, m.surfaceAreaCm2));
    out.Set("bs_bv_1_per_mm", Napi::Number::New(env, m.bs_bv_1_per_mm));

    out.Set("bboxMinX", Napi::Number::New(env, m.bboxMinX));
    out.Set("bboxMinY", Napi::Number::New(env, m.bboxMinY));
    out.Set("bboxMinZ", Napi::Number::New(env, m.bboxMinZ));
    out.Set("bboxMaxX", Napi::Number::New(env, m.bboxMaxX));
    out.Set("bboxMaxY", Napi::Number::New(env, m.bboxMaxY));
    out.Set("bboxMaxZ", Napi::Number::New(env, m.bboxMaxZ));

    out.Set("centroidXmm", Napi::Number::New(env, m.centroidXmm));
    out.Set("centroidYmm", Napi::Number::New(env, m.centroidYmm));
    out.Set("centroidZmm", Napi::Number::New(env, m.centroidZmm));

    out.Set("tvBoxMm3", Napi::Number::New(env, m.tvBoxMm3));
    out.Set("bv_tv", Napi::Number::New(env, m.bv_tv));
    out.Set("tbThMm", Napi::Number::New(env, m.tbThMm));
    out.Set("tbSpMm", Napi::Number::New(env, m.tbSpMm));
    out.Set("tbNm_1_per_mm", Napi::Number::New(env, m.tbNm_1_per_mm));

    out.Set("smi", Napi::Number::New(env, m.smi));

    out.Set("da", Napi::Number::New(env, m.da));
    out.Set("daEigen1", Napi::Number::New(env, m.daEigen1));
    out.Set("daEigen2", Napi::Number::New(env, m.daEigen2));
    out.Set("daEigen3", Napi::Number::New(env, m.daEigen3));

    return out;
}

// 更新预览mask
Napi::Value UpdatePreviewMask(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 4 || !info[0].IsString() || !info[1].IsNumber() || 
        !info[2].IsNumber() || !info[3].IsString()) {
        Napi::TypeError::New(env, "Expected (sessionId: string, minThreshold: number, maxThreshold: number, hexColor: string)").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    
    std::string sessionId = info[0].As<Napi::String>().Utf8Value();
    float minThreshold = info[1].As<Napi::Number>().FloatValue();
    float maxThreshold = info[2].As<Napi::Number>().FloatValue();
    std::string hexColor = info[3].As<Napi::String>().Utf8Value();
    
    NativeResult result = MPR_UpdatePreviewMask(
        sessionId.c_str(),
        minThreshold,
        maxThreshold,
        hexColor.c_str()
    );
    
    if (result != NATIVE_OK) {
        Napi::Error::New(env, "Failed to update preview mask").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    
    return env.Undefined();
}

// 清除预览mask
Napi::Value ClearPreviewMask(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1 || !info[0].IsString()) {
        Napi::TypeError::New(env, "Expected (sessionId: string)").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    
    std::string sessionId = info[0].As<Napi::String>().Utf8Value();
    
    NativeResult result = MPR_ClearPreviewMask(sessionId.c_str());
    
    if (result != NATIVE_OK) {
        Napi::Error::New(env, "Failed to clear preview mask").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    
    return env.Undefined();
}

// 创建permanent mask
Napi::Value CreateMaskFromThreshold(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 5 || !info[0].IsString() || !info[1].IsNumber() || 
        !info[2].IsNumber() || !info[3].IsString() || !info[4].IsString()) {
        Napi::TypeError::New(env, "Expected (sessionId: string, minThreshold: number, maxThreshold: number, hexColor: string, maskName: string)").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    
    std::string sessionId = info[0].As<Napi::String>().Utf8Value();
    float minThreshold = info[1].As<Napi::Number>().FloatValue();
    float maxThreshold = info[2].As<Napi::Number>().FloatValue();
    std::string hexColor = info[3].As<Napi::String>().Utf8Value();
    std::string maskName = info[4].As<Napi::String>().Utf8Value();
    
    int maskId = -1;
    NativeResult result = MPR_CreateMaskFromThreshold(
        sessionId.c_str(),
        minThreshold,
        maxThreshold,
        hexColor.c_str(),
        maskName.c_str(),
        &maskId
    );
    
    Napi::Object resultObj = Napi::Object::New(env);
    resultObj.Set("success", Napi::Boolean::New(env, result == NATIVE_OK));
    
    if (result == NATIVE_OK) {
        resultObj.Set("maskId", Napi::Number::New(env, maskId));
    } else {
        resultObj.Set("error", Napi::String::New(env, "Failed to create mask"));
    }
    
    return resultObj;
}

// 创建一个全0的permanent mask（用于ROI手工绘制�?
Napi::Value CreateEmptyMask(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (info.Length() < 3 || !info[0].IsString() || !info[1].IsString() || !info[2].IsString()) {
        Napi::TypeError::New(env, "Expected (sessionId: string, hexColor: string, maskName: string)").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    std::string sessionId = info[0].As<Napi::String>().Utf8Value();
    std::string hexColor = info[1].As<Napi::String>().Utf8Value();
    std::string maskName = info[2].As<Napi::String>().Utf8Value();

    int maskId = -1;
    NativeResult result = MPR_CreateEmptyMask(sessionId.c_str(), hexColor.c_str(), maskName.c_str(), &maskId);

    Napi::Object resultObj = Napi::Object::New(env);
    resultObj.Set("success", Napi::Boolean::New(env, result == NATIVE_OK));
    if (result == NATIVE_OK) {
        resultObj.Set("maskId", Napi::Number::New(env, maskId));
    } else {
        const char* error = Visualization_GetLastError();
        resultObj.Set("error", Napi::String::New(env, error ? error : "Failed to create empty mask"));
    }
    return resultObj;
}

// 选中一个session内部mask，用于MaskEdit模式编辑
Napi::Value SelectMaskForEditing(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (info.Length() < 2 || !info[0].IsString() || !info[1].IsNumber()) {
        Napi::TypeError::New(env, "Expected (sessionId: string, maskId: number)").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    std::string sessionId = info[0].As<Napi::String>().Utf8Value();
    int maskId = info[1].As<Napi::Number>().Int32Value();

    NativeResult result = MPR_SelectMaskForEditing(sessionId.c_str(), maskId);
    if (result != NATIVE_OK) {
        const char* error = Visualization_GetLastError();
        Napi::Error::New(env, error ? error : "Failed to select mask for editing").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    return Napi::Boolean::New(env, true);
}

// 设置MaskEdit子工具：1=Brush,2=Eraser,...
Napi::Value SetMaskTool(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsNumber()) {
        Napi::TypeError::New(env, "Expected (maskTool: number)").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    int tool = info[0].As<Napi::Number>().Int32Value();
    Mask_SetTool(tool);
    return env.Undefined();
}

Napi::Value GetMaskTool(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    return Napi::Number::New(env, Mask_GetTool());
}

Napi::Value SetMaskBrushRadius(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsNumber()) {
        Napi::TypeError::New(env, "Expected (radius: number)").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    float radius = info[0].As<Napi::Number>().FloatValue();
    Mask_SetBrushRadius(radius);
    return env.Undefined();
}

Napi::Value GetMaskBrushRadius(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    return Napi::Number::New(env, Mask_GetBrushRadius());
}

// 删除mask
Napi::Value DeleteMask(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 2 || !info[0].IsString() || !info[1].IsNumber()) {
        Napi::TypeError::New(env, "Expected (sessionId: string, maskId: number)").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    
    std::string sessionId = info[0].As<Napi::String>().Utf8Value();
    int maskId = info[1].As<Napi::Number>().Int32Value();
    
    NativeResult result = MPR_DeleteMask(sessionId.c_str(), maskId);
    
    if (result != NATIVE_OK) {
        Napi::Error::New(env, "Failed to delete mask").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    
    return env.Undefined();
}

// 保存masks
Napi::Value SaveMasks(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 3 || !info[0].IsString() || !info[1].IsString() || !info[2].IsString()) {
        Napi::TypeError::New(env, "Expected (sessionId: string, folderPath: string, maskName: string)").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    
    std::string sessionId = info[0].As<Napi::String>().Utf8Value();
    std::string folderPath = info[1].As<Napi::String>().Utf8Value();
    std::string maskName = info[2].As<Napi::String>().Utf8Value();
    
    char filePath[1024] = {0};
    NativeResult result = MPR_SaveMasks(
        sessionId.c_str(),
        folderPath.c_str(),
        maskName.c_str(),
        filePath,
        sizeof(filePath)
    );
    
    Napi::Object resultObj = Napi::Object::New(env);
    resultObj.Set("success", Napi::Boolean::New(env, result == NATIVE_OK));
    
    if (result == NATIVE_OK) {
        resultObj.Set("filePath", Napi::String::New(env, filePath));
    } else {
        resultObj.Set("error", Napi::String::New(env, "Failed to save masks"));
    }
    
    return resultObj;
}

// 导出 mask 网格（STL�?
Napi::Value ExportMaskToStl(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 4 || !info[0].IsString() || !info[1].IsNumber() || !info[2].IsString() || !info[3].IsNumber()) {
        Napi::TypeError::New(env, "Expected (sessionId: string, maskId: number, filepath: string, step: number)").ThrowAsJavaScriptException();
        return env.Null();
    }

    const std::string sessionId = info[0].As<Napi::String>().Utf8Value();
    const int maskId = info[1].As<Napi::Number>().Int32Value();
    const std::string filepath = info[2].As<Napi::String>().Utf8Value();
    const int step = info[3].As<Napi::Number>().Int32Value();

    Napi::Object resultObj = Napi::Object::New(env);
    NativeResult r = MPR_ExportMaskToSTL(sessionId.c_str(), maskId, filepath.c_str(), step);
    if (r == NATIVE_OK) {
        resultObj.Set("success", Napi::Boolean::New(env, true));
        return resultObj;
    }

    resultObj.Set("success", Napi::Boolean::New(env, false));
    const char* error = Visualization_GetLastError();
    resultObj.Set(
        "error",
        Napi::String::New(env, error ? error : "Failed to export STL")
    );
    return resultObj;
}

// 加载masks
Napi::Value LoadMasks(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 2 || !info[0].IsString() || !info[1].IsString()) {
        Napi::TypeError::New(env, "Expected (sessionId: string, folderPath: string)").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    
    std::string sessionId = info[0].As<Napi::String>().Utf8Value();
    std::string folderPath = info[1].As<Napi::String>().Utf8Value();
    
    int maskCount = 0;
    MaskInfo* maskInfos = nullptr;
    
    NativeResult result = MPR_LoadMasks(
        sessionId.c_str(),
        folderPath.c_str(),
        &maskCount,
        &maskInfos
    );
    
    Napi::Object resultObj = Napi::Object::New(env);
    
    if (result == NATIVE_USER_CANCELLED) {
        resultObj.Set("success", Napi::Boolean::New(env, false));
        resultObj.Set("cancelled", Napi::Boolean::New(env, true));
        return resultObj;
    }
    
    if (result != NATIVE_OK) {
        resultObj.Set("success", Napi::Boolean::New(env, false));
        resultObj.Set("error", Napi::String::New(env, "Failed to load masks"));
        return resultObj;
    }
    
    // 转换为JavaScript数组
    Napi::Array masksArray = Napi::Array::New(env, maskCount);
    for (int i = 0; i < maskCount; i++) {
        Napi::Object maskObj = Napi::Object::New(env);
        maskObj.Set("maskId", Napi::Number::New(env, maskInfos[i].maskId));
        maskObj.Set("name", Napi::String::New(env, maskInfos[i].name));
        maskObj.Set("color", Napi::String::New(env, maskInfos[i].color));
        maskObj.Set("visible", Napi::Boolean::New(env, maskInfos[i].visible));
        maskObj.Set("minThreshold", Napi::Number::New(env, maskInfos[i].minThreshold));
        maskObj.Set("maxThreshold", Napi::Number::New(env, maskInfos[i].maxThreshold));
        masksArray[i] = maskObj;
    }
    
    // 释放C++内存
    delete[] maskInfos;
    
    resultObj.Set("success", Napi::Boolean::New(env, true));
    resultObj.Set("masks", masksArray);
    
    return resultObj;
}

// ==================== Morphology and Boolean Operations ====================

// 2D形态学操作（逐层处理�?
Napi::Value MaskMorphology2D(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 4 || !info[0].IsString() || !info[1].IsNumber() || 
        !info[2].IsNumber() || !info[3].IsNumber()) {
        Napi::TypeError::New(env, "Expected (sessionId: string, maskId: number, operation: number, kernelSize: number)").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    
    std::string sessionId = info[0].As<Napi::String>().Utf8Value();
    int maskId = info[1].As<Napi::Number>().Int32Value();
    int operation = info[2].As<Napi::Number>().Int32Value();
    int kernelSize = info[3].As<Napi::Number>().Int32Value();
    
    NativeResult result = MPR_MaskMorphology2D(
        sessionId.c_str(),
        maskId,
        static_cast<MorphologyOperation>(operation),
        kernelSize,
        1
    );
    
    Napi::Object resultObj = Napi::Object::New(env);
    if (result == NATIVE_OK) {
        resultObj.Set("success", Napi::Boolean::New(env, true));
    } else {
        resultObj.Set("success", Napi::Boolean::New(env, false));
        const char* error = Visualization_GetLastError();
        resultObj.Set("error", Napi::String::New(env, error ? error : "Morphology operation failed"));
    }
    
    return resultObj;
}

// 3D形态学操作�?6邻域连通）
Napi::Value MaskMorphology3D(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 4 || !info[0].IsString() || !info[1].IsNumber() || 
        !info[2].IsNumber() || !info[3].IsNumber()) {
        Napi::TypeError::New(env, "Expected (sessionId: string, maskId: number, operation: number, kernelSize: number)").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    
    std::string sessionId = info[0].As<Napi::String>().Utf8Value();
    int maskId = info[1].As<Napi::Number>().Int32Value();
    int operation = info[2].As<Napi::Number>().Int32Value();
    int kernelSize = info[3].As<Napi::Number>().Int32Value();
    
    NativeResult result = MPR_MaskMorphology3D(
        sessionId.c_str(),
        maskId,
        static_cast<MorphologyOperation>(operation),
        kernelSize,
        1
    );
    
    Napi::Object resultObj = Napi::Object::New(env);
    if (result == NATIVE_OK) {
        resultObj.Set("success", Napi::Boolean::New(env, true));
    } else {
        resultObj.Set("success", Napi::Boolean::New(env, false));
        const char* error = Visualization_GetLastError();
        resultObj.Set("error", Napi::String::New(env, error ? error : "Morphology operation failed"));
    }
    
    return resultObj;
}

// 布尔运算（并集、交集、差集）
Napi::Value MaskBoolean(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 5 || !info[0].IsString() || !info[1].IsNumber() || 
        !info[2].IsNumber() || !info[3].IsNumber()) {
        Napi::TypeError::New(env, "Expected (sessionId: string, maskIdA: number, maskIdB: number, operation: number, name?: string, color?: string)").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    
    std::string sessionId = info[0].As<Napi::String>().Utf8Value();
    int maskIdA = info[1].As<Napi::Number>().Int32Value();
    int maskIdB = info[2].As<Napi::Number>().Int32Value();
    int operation = info[3].As<Napi::Number>().Int32Value();
    
    std::string name = "Boolean Result";
    std::string color = "#FFFF00";
    if (info.Length() >= 5 && info[4].IsString()) {
        name = info[4].As<Napi::String>().Utf8Value();
    }
    if (info.Length() >= 6 && info[5].IsString()) {
        color = info[5].As<Napi::String>().Utf8Value();
    }
    
    int newMaskId = 0;
    NativeResult result = MPR_MaskBoolean(
        sessionId.c_str(),
        maskIdA,
        maskIdB,
        static_cast<BooleanOperation>(operation),
        color.c_str(),
        name.c_str(),
        &newMaskId
    );
    
    Napi::Object resultObj = Napi::Object::New(env);
    if (result == NATIVE_OK) {
        resultObj.Set("success", Napi::Boolean::New(env, true));
        resultObj.Set("newMaskId", Napi::Number::New(env, newMaskId));
    } else {
        resultObj.Set("success", Napi::Boolean::New(env, false));
        const char* error = Visualization_GetLastError();
        resultObj.Set("error", Napi::String::New(env, error ? error : "Boolean operation failed"));
    }
    
    return resultObj;
}

// Mask反转
Napi::Value MaskInverse(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 2 || !info[0].IsString() || !info[1].IsNumber()) {
        Napi::TypeError::New(env, "Expected (sessionId: string, maskId: number)").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    
    std::string sessionId = info[0].As<Napi::String>().Utf8Value();
    int maskId = info[1].As<Napi::Number>().Int32Value();
    
    NativeResult result = MPR_MaskInverse(sessionId.c_str(), maskId);
    
    Napi::Object resultObj = Napi::Object::New(env);
    if (result == NATIVE_OK) {
        resultObj.Set("success", Napi::Boolean::New(env, true));
    } else {
        resultObj.Set("success", Napi::Boolean::New(env, false));
        const char* error = Visualization_GetLastError();
        resultObj.Set("error", Napi::String::New(env, error ? error : "Inverse operation failed"));
    }
    
    return resultObj;
}

// ==================== 3D Primitives (Window-based) ====================

Napi::Value Add3DPrimitive(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (info.Length() < 2 || !info[0].IsString() || !info[1].IsNumber()) {
        Napi::TypeError::New(env, "Expected (windowId: string, type: number, ...params)").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    std::string windowId = info[0].As<Napi::String>().Utf8Value();
    int type = info[1].As<Napi::Number>().Int32Value();

    auto it = g_WindowHandles.find(windowId);
    if (it == g_WindowHandles.end() || !it->second) {
        Napi::Error::New(env, "Window not found").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    WindowHandle wh = it->second;
    int primId = 0;

    if (type == (int)PRIM3D_CUBE) {
        if (info.Length() < 5 || !info[2].IsNumber() || !info[3].IsNumber() || !info[4].IsNumber()) {
            Napi::TypeError::New(env, "Expected cube params (sizeX,sizeY,sizeZ)").ThrowAsJavaScriptException();
            return env.Undefined();
        }
        float sx = info[2].As<Napi::Number>().FloatValue();
        float sy = info[3].As<Napi::Number>().FloatValue();
        float sz = info[4].As<Napi::Number>().FloatValue();
        primId = Window3D_AddCube(wh, sx, sy, sz);
    } else if (type == (int)PRIM3D_SPHERE) {
        if (info.Length() < 3 || !info[2].IsNumber()) {
            Napi::TypeError::New(env, "Expected sphere params (radius)").ThrowAsJavaScriptException();
            return env.Undefined();
        }
        float r = info[2].As<Napi::Number>().FloatValue();
        primId = Window3D_AddSphere(wh, r);
    } else if (type == (int)PRIM3D_CYLINDER) {
        if (info.Length() < 4 || !info[2].IsNumber() || !info[3].IsNumber()) {
            Napi::TypeError::New(env, "Expected cylinder params (radius,height)").ThrowAsJavaScriptException();
            return env.Undefined();
        }
        float r = info[2].As<Napi::Number>().FloatValue();
        float h = info[3].As<Napi::Number>().FloatValue();
        primId = Window3D_AddCylinder(wh, r, h);
    } else {
        Napi::TypeError::New(env, "Unknown primitive type").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    return Napi::Number::New(env, primId);
}

Napi::Value Remove3DPrimitive(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 2 || !info[0].IsString() || !info[1].IsNumber()) {
        Napi::TypeError::New(env, "Expected (windowId: string, primitiveId: number)").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    std::string windowId = info[0].As<Napi::String>().Utf8Value();
    int primId = info[1].As<Napi::Number>().Int32Value();

    auto it = g_WindowHandles.find(windowId);
    if (it == g_WindowHandles.end() || !it->second) return Napi::Boolean::New(env, false);
    return Napi::Boolean::New(env, Window3D_RemovePrimitive(it->second, primId) == NATIVE_OK);
}

Napi::Value Clear3DPrimitives(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsString()) {
        Napi::TypeError::New(env, "Expected (windowId: string)").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    std::string windowId = info[0].As<Napi::String>().Utf8Value();
    auto it = g_WindowHandles.find(windowId);
    if (it != g_WindowHandles.end() && it->second) {
        Window3D_ClearPrimitives(it->second);
        return Napi::Boolean::New(env, true);
    }
    return Napi::Boolean::New(env, false);
}

Napi::Value Set3DPrimitiveTransform(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 11 || !info[0].IsString() || !info[1].IsNumber()) {
        Napi::TypeError::New(env, "Expected (windowId, primitiveId, tx,ty,tz, rx,ry,rz, sx,sy,sz)").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    std::string windowId = info[0].As<Napi::String>().Utf8Value();
    int primId = info[1].As<Napi::Number>().Int32Value();
    float tx = info[2].As<Napi::Number>().FloatValue();
    float ty = info[3].As<Napi::Number>().FloatValue();
    float tz = info[4].As<Napi::Number>().FloatValue();
    float rx = info[5].As<Napi::Number>().FloatValue();
    float ry = info[6].As<Napi::Number>().FloatValue();
    float rz = info[7].As<Napi::Number>().FloatValue();
    float sx = info[8].As<Napi::Number>().FloatValue();
    float sy = info[9].As<Napi::Number>().FloatValue();
    float sz = info[10].As<Napi::Number>().FloatValue();

    auto it = g_WindowHandles.find(windowId);
    if (it == g_WindowHandles.end() || !it->second) return Napi::Boolean::New(env, false);

    return Napi::Boolean::New(env, Window3D_SetPrimitiveTransform(it->second, primId, tx,ty,tz, rx,ry,rz, sx,sy,sz) == NATIVE_OK);
}

Napi::Value Set3DSceneTransform(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 10 || !info[0].IsString()) {
        Napi::TypeError::New(env, "Expected (windowId, tx,ty,tz, rx,ry,rz, sx,sy,sz)").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    std::string windowId = info[0].As<Napi::String>().Utf8Value();
    float tx = info[1].As<Napi::Number>().FloatValue();
    float ty = info[2].As<Napi::Number>().FloatValue();
    float tz = info[3].As<Napi::Number>().FloatValue();
    float rx = info[4].As<Napi::Number>().FloatValue();
    float ry = info[5].As<Napi::Number>().FloatValue();
    float rz = info[6].As<Napi::Number>().FloatValue();
    float sx = info[7].As<Napi::Number>().FloatValue();
    float sy = info[8].As<Napi::Number>().FloatValue();
    float sz = info[9].As<Napi::Number>().FloatValue();

    auto it = g_WindowHandles.find(windowId);
    if (it == g_WindowHandles.end() || !it->second) return Napi::Boolean::New(env, false);
    return Napi::Boolean::New(env, Window3D_SetSceneTransform(it->second, tx,ty,tz, rx,ry,rz, sx,sy,sz) == NATIVE_OK);
}

Napi::Value Set3DPrimitiveColor(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 6 || !info[0].IsString() || !info[1].IsNumber()) {
        Napi::TypeError::New(env, "Expected (windowId, primitiveId, r,g,b,a)").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    std::string windowId = info[0].As<Napi::String>().Utf8Value();
    int primId = info[1].As<Napi::Number>().Int32Value();
    float r = info[2].As<Napi::Number>().FloatValue();
    float g = info[3].As<Napi::Number>().FloatValue();
    float b = info[4].As<Napi::Number>().FloatValue();
    float a = info[5].As<Napi::Number>().FloatValue();
    auto it = g_WindowHandles.find(windowId);
    if (it == g_WindowHandles.end() || !it->second) return Napi::Boolean::New(env, false);
    return Napi::Boolean::New(env, Window3D_SetPrimitiveColor(it->second, primId, r,g,b,a) == NATIVE_OK);
}

// ==================== Fat / Vascular Analysis Functions ====================

Napi::Value FatAnalyzeSeparateFat(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 4) {
        Napi::TypeError::New(env, "Expected (sessionId, maskId, minFat, maxFat)").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    std::string sessionId = info[0].As<Napi::String>().Utf8Value();
    int maskId = info[1].As<Napi::Number>().Int32Value();
    int minFat = info[2].As<Napi::Number>().Int32Value();
    int maxFat = info[3].As<Napi::Number>().Int32Value();

    auto itKey = g_SessionToVolumeKey.find(sessionId);
    if (itKey == g_SessionToVolumeKey.end()) return env.Undefined();
    auto volIt = g_VolumeHandles.find(itKey->second);
    if (volIt == g_VolumeHandles.end()) return env.Undefined();
    auto* pVolume = (VolumeData*)volIt->second;

    auto result = FatAnalysis::SeprateFat(
        reinterpret_cast<short*>(pVolume->data.data()), pVolume->width, pVolume->height, pVolume->depth,
        maskId, minFat, maxFat
    );

    size_t size = (size_t)pVolume->width * pVolume->height * pVolume->depth;
    Napi::Object out = Napi::Object::New(env);
    
    auto finalizer = [](Napi::Env, uint8_t* val) { delete[] val; };
    out.Set("visceral", Napi::Buffer<uint8_t>::New(env, result.first, size, finalizer));
    out.Set("subcutaneous", Napi::Buffer<uint8_t>::New(env, result.second, size, finalizer));
    
    return out;
}

Napi::Value FatAnalyzeSeparateLung(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    // Simplified args, assume defaults for lung/bone or pass 0
    std::string sessionId = info[0].As<Napi::String>().Utf8Value();
    int maskId = info[1].As<Napi::Number>().Int32Value();
    
    auto itKey = g_SessionToVolumeKey.find(sessionId);
    if (itKey == g_SessionToVolumeKey.end()) return env.Undefined();
    auto volIt = g_VolumeHandles.find(itKey->second);
    if (volIt == g_VolumeHandles.end()) return env.Undefined();
    auto* pVolume = (VolumeData*)volIt->second;

    // Hardcoded thresholds for demo
    auto results = FatAnalysis::SeprateLung(
        reinterpret_cast<short*>(pVolume->data.data()), pVolume->width, pVolume->height, pVolume->depth,
        -190, -30, -500, -190, 200, 3000
    );
    
    auto finalizer = [](Napi::Env, uint8_t* val) { delete[] val; };
    size_t size = (size_t)pVolume->width * pVolume->height * pVolume->depth;

    Napi::Object out = Napi::Object::New(env);
    if(results.size() >= 4) {
        out.Set("visceral", Napi::Buffer<uint8_t>::New(env, results[0], size, finalizer));
        out.Set("subcutaneous", Napi::Buffer<uint8_t>::New(env, results[1], size, finalizer));
        out.Set("lung", Napi::Buffer<uint8_t>::New(env, results[2], size, finalizer));
        out.Set("bone", Napi::Buffer<uint8_t>::New(env, results[3], size, finalizer));
    }
    return out;
}

Napi::Value VascularFilterKeepLargest(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    // args: (maskBuffer, width, height, depth)
    if (info.Length() < 4 || !info[0].IsTypedArray()) return env.Undefined();
    
    Napi::Uint8Array maskArr = info[0].As<Napi::Uint8Array>();
    int w = info[1].As<Napi::Number>().Int32Value();
    int h = info[2].As<Napi::Number>().Int32Value();
    int d = info[3].As<Napi::Number>().Int32Value();
    
if (maskArr.ByteLength() != (size_t)w*h*d) return Napi::Boolean::New(env, false);

    unsigned char* data = maskArr.Data();
    // In-place modification
    VascularAnalysis::FilterKeepLargest(data, w, h, d);

    return Napi::Boolean::New(env, true);
}

Napi::Value VascularAnalyzeCompute(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    VascularAnalysis::AnalysisResult result;

    // Option 1: Compute from Buffer (passed from JS)
    // args: (sessionId, maskBuffer)
    if (info.Length() >= 2 && info[1].IsTypedArray()) {
        std::string sessionId = info[0].As<Napi::String>().Utf8Value();
        Napi::Uint8Array maskArr = info[1].As<Napi::Uint8Array>();
        
        auto itKey = g_SessionToVolumeKey.find(sessionId);
        if (itKey == g_SessionToVolumeKey.end()) return env.Undefined();
        auto volIt = g_VolumeHandles.find(itKey->second);
        auto* pVolume = (VolumeData*)volIt->second;

        // Make copy to be safe? Or reuse if strict input
        // Using vector copy for safety
        std::vector<unsigned char> mask(maskArr.Data(), maskArr.Data() + maskArr.ByteLength());
        
        // Auto-filter noise (Keep Largest) for calculation stability
        VascularAnalysis::FilterKeepLargest(mask.data(), pVolume->width, pVolume->height, pVolume->depth);
        
        result = VascularAnalysis::ComputeDiameter(
            mask.data(), pVolume->width, pVolume->height, pVolume->depth, pVolume->spacingX
        );
    }
    // Option 2: Compute from Threshold (sessionId, min, max)
    else if (info.Length() >= 3 && info[1].IsNumber() && info[2].IsNumber()) {
        std::string sessionId = info[0].As<Napi::String>().Utf8Value();
        int minTh = info[1].As<Napi::Number>().Int32Value();
        int maxTh = info[2].As<Napi::Number>().Int32Value();
        
        auto itKey = g_SessionToVolumeKey.find(sessionId);
        if (itKey == g_SessionToVolumeKey.end()) return env.Undefined();
        auto volIt = g_VolumeHandles.find(itKey->second);
        auto* pVolume = (VolumeData*)volIt->second;
        
        size_t size = (size_t)pVolume->width * pVolume->height * pVolume->depth;
        std::vector<unsigned char> mask(size);
        
        #pragma omp parallel for
        for(int i=0; i<(int)size; ++i) {
            short v = pVolume->data[i];
            mask[i] = (v >= minTh && v <= maxTh) ? 255 : 0;
        }
        
        VascularAnalysis::FilterKeepLargest(mask.data(), pVolume->width, pVolume->height, pVolume->depth);
        
        result = VascularAnalysis::ComputeDiameter(
            mask.data(), pVolume->width, pVolume->height, pVolume->depth, pVolume->spacingX
        );
    }
    // Option 3: Compute from Mask ID (sessionId, maskId)
    else if (info.Length() >= 2 && info[1].IsNumber()) {
        std::string sessionId = info[0].As<Napi::String>().Utf8Value();
        int maskId = info[1].As<Napi::Number>().Int32Value();

        auto itKey = g_SessionToVolumeKey.find(sessionId);
        if (itKey == g_SessionToVolumeKey.end()) return env.Undefined();
        auto volIt = g_VolumeHandles.find(itKey->second);
        auto* pVolume = (VolumeData*)volIt->second;

        size_t size = (size_t)pVolume->width * pVolume->height * pVolume->depth;
        std::vector<unsigned char> mask(size);
        
        if (MPR_GetMaskData(sessionId.c_str(), maskId, mask.data(), size) != NATIVE_OK) {
             Napi::Error::New(env, "Failed to get mask data").ThrowAsJavaScriptException();
             return env.Undefined();
        }

        VascularAnalysis::FilterKeepLargest(mask.data(), pVolume->width, pVolume->height, pVolume->depth);
        
        result = VascularAnalysis::ComputeDiameter(
            mask.data(), pVolume->width, pVolume->height, pVolume->depth, pVolume->spacingX
        );
    }
    
    Napi::Object out = Napi::Object::New(env);
    out.Set("meanDiameter", Napi::Number::New(env, result.meanDiameter));
    out.Set("maxDiameter", Napi::Number::New(env, result.maxDiameter));
    out.Set("stdDiameter", Napi::Number::New(env, result.stdDiameter));
    // Skip histograms/maps for brevity if not needed continuously, or add them back if needed
    return out;
}

Napi::Value Set3DPrimitiveVisible(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 3 || !info[0].IsString() || !info[1].IsNumber() || !info[2].IsBoolean()) {
        Napi::TypeError::New(env, "Expected (windowId, primitiveId, visible)").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    std::string windowId = info[0].As<Napi::String>().Utf8Value();
    int primId = info[1].As<Napi::Number>().Int32Value();
    bool visible = info[2].As<Napi::Boolean>().Value();
    auto it = g_WindowHandles.find(windowId);
    if (it == g_WindowHandles.end() || !it->second) return Napi::Boolean::New(env, false);
    return Napi::Boolean::New(env, Window3D_SetPrimitiveVisible(it->second, primId, visible) == NATIVE_OK);
}
// ==================== 模块初始�?====================

void InitVisualizationModule(Napi::Env env, Napi::Object& exports) {
    printf("====================================================================\n");
    printf("[InitVisualizationModule] DLL LOADED - BUILD: %s %s\n", __DATE__, __TIME__);
    printf("[InitVisualizationModule] 3D Renderer Colors:\n");
    printf("  PURPLE = ImageBrowserOrthogonal3DRenderer (WM_PAINT kind=0/1)\n");
    printf("  CYAN   = RoiOrthogonal3DRenderer (WM_PAINT kind=2)\n");
    printf("  ORANGE = ReconstructionRaycast3DRenderer (WM_PAINT kind=3)\n");
    printf("  YELLOW = APR_RenderOrthogonal3D (RenderAllViews path)\n");
    printf("====================================================================\n");
    fflush(stdout);
    
    // Fat / Vascular
    exports.Set("fatAnalyzeSeparateFat", Napi::Function::New(env, FatAnalyzeSeparateFat));
    exports.Set("fatAnalyzeSeparateLung", Napi::Function::New(env, FatAnalyzeSeparateLung));
    exports.Set("vascularFilterKeepLargest", Napi::Function::New(env, VascularFilterKeepLargest));
    exports.Set("vascularAnalyzeCompute", Napi::Function::New(env, VascularAnalyzeCompute));

    exports.Set("createAPRViews", Napi::Function::New(env, CreateAPRViews));
    exports.Set("createMPRViews", Napi::Function::New(env, CreateMPRViews));
    exports.Set("renderAPRSlice", Napi::Function::New(env, RenderAPRSlice));
    exports.Set("updateAPRCenter", Napi::Function::New(env, UpdateAPRCenter));
    exports.Set("updateAPRRotation", Napi::Function::New(env, UpdateAPRRotation));
    exports.Set("setCrosshairVisible", Napi::Function::New(env, SetCrosshairVisible));
    exports.Set("getAPRState", Napi::Function::New(env, GetAPRState));
    exports.Set("getCompletedMeasurements", Napi::Function::New(env, GetCompletedMeasurements));
    exports.Set("getMeasurementProfile", Napi::Function::New(env, GetMeasurementProfile));
    exports.Set("deleteMeasurement", Napi::Function::New(env, DeleteMeasurement));
    exports.Set("getMeasurementRegionHistogram", Napi::Function::New(env, GetMeasurementRegionHistogram));
    exports.Set("updateMPRCenter", Napi::Function::New(env, UpdateMPRCenter));
    exports.Set("destroyAPRViews", Napi::Function::New(env, DestroyAPRViews));
    exports.Set("destroyMPRViews", Napi::Function::New(env, DestroyMPRViews));
    exports.Set("embedWindow", Napi::Function::New(env, EmbedWindow));
    exports.Set("renderAllViews", Napi::Function::New(env, RenderAllViews));  // 保留，用于向后兼�?
    exports.Set("invalidateAllWindows", Napi::Function::New(env, InvalidateAllWindows));  // 新API
    exports.Set("invalidateWindow", Napi::Function::New(env, InvalidateWindow));  // 新API
    exports.Set("resetView", Napi::Function::New(env, ResetView));
    exports.Set("set3DOrthogonalMode", Napi::Function::New(env, Set3DOrthogonalMode));
    exports.Set("set3DMaskIsoSurface", Napi::Function::New(env, Set3DMaskIsoSurface));
    exports.Set("set3DVramOptimized", Napi::Function::New(env, Set3DVramOptimized));
    exports.Set("set3DLightParameters", Napi::Function::New(env, Set3DLightParameters));
    exports.Set("set3DTransferFunction", Napi::Function::New(env, Set3DTransferFunction));
    exports.Set("getGpuInfo", Napi::Function::New(env, GetGpuInfo));
    exports.Set("resizeWindow", Napi::Function::New(env, ResizeWindow));  // 新API
    exports.Set("hideAllWindows", Napi::Function::New(env, HideAllWindows));  // 隐藏窗口（Tab切换�?
    exports.Set("showAllWindows", Napi::Function::New(env, ShowAllWindows));  // 显示窗口（回到Tab�?
    exports.Set("destroyAll3DWindows", Napi::Function::New(env, DestroyAll3DWindows));  // Tab切换专用清理
    exports.Set("destroyAllWindows", Napi::Function::New(env, DestroyAllWindows));  // 全局清理
    exports.Set("startRenderLoop", Napi::Function::New(env, StartRenderLoop));  // 新API - 固定帧率渲染
    exports.Set("stopRenderLoop", Napi::Function::New(env, StopRenderLoop));  // 新API
    exports.Set("processWindowEvents", Napi::Function::New(env, ProcessWindowEvents));  // 处理窗口事件
    exports.Set("refreshAllWindowsZOrder", Napi::Function::New(env, RefreshAllWindowsZOrder));
    exports.Set("refreshWindowZOrder", Napi::Function::New(env, RefreshWindowZOrder));
    exports.Set("raiseAllWindows", Napi::Function::New(env, RaiseAllWindows));
    exports.Set("raiseWindow", Napi::Function::New(env, RaiseWindow));

#ifdef _WIN32
    // HIS4D (4D cine volume)
    exports.Set("packHis4dFromFolders", Napi::Function::New(env, PackHis4dFromFolders));
    exports.Set("createAPRViewsFromHis4d", Napi::Function::New(env, CreateAPRViewsFromHis4d));
    exports.Set("his4dSetFrame", Napi::Function::New(env, His4dSetFrame));
    exports.Set("his4dGetSessionInfo", Napi::Function::New(env, His4dGetSessionInfo));
#endif
    
    // 工具管理和裁切框 API
    exports.Set("setWindowToolType", Napi::Function::New(env, SetWindowToolType));  // 设置窗口工具类型

    // Per-window crop box visibility
    exports.Set("setWindowCropBoxVisible", Napi::Function::New(env, SetWindowCropBoxVisible));

    exports.Set("setSessionWindowLevel", Napi::Function::New(env, SetSessionWindowLevel));  // 设置窗宽窗位
    exports.Set("getSessionWindowLevel", Napi::Function::New(env, GetSessionWindowLevel));  // 获取窗宽窗位
    exports.Set("setSessionProjectionMode", Napi::Function::New(env, SetSessionProjectionMode));  // 设置MIP/MinIP模式
    exports.Set("getSessionProjectionMode", Napi::Function::New(env, GetSessionProjectionMode));  // 获取MIP/MinIP模式
    exports.Set("enableAPRCropBox", Napi::Function::New(env, EnableAPRCropBox));  // 启用/禁用裁切�?
    exports.Set("setAPRCropBox", Napi::Function::New(env, SetAPRCropBox));  // 初始化裁切框
    exports.Set("setAPRCropBoxRange", Napi::Function::New(env, SetAPRCropBoxRange));  // 设置裁切框范�?
    exports.Set("getAPRCropBox", Napi::Function::New(env, GetAPRCropBox));  // 获取裁切框范�?
    exports.Set("isAPRCropBoxEnabled", Napi::Function::New(env, IsAPRCropBoxEnabled));  // 检查裁切框状�?
    exports.Set("setAPRCropShape", Napi::Function::New(env, SetAPRCropShape));  // 设置裁切形状
    exports.Set("getAPRCropShape", Napi::Function::New(env, GetAPRCropShape));  // 获取裁切形状
    exports.Set("setAPRCropCylinderDirection", Napi::Function::New(env, SetAPRCropCylinderDirection));  // 设置圆柱体方�?
    exports.Set("getAPRCropCylinderDirection", Napi::Function::New(env, GetAPRCropCylinderDirection));  // 获取圆柱体方�?
    exports.Set("setAPRCropBoxSize", Napi::Function::New(env, SetAPRCropBoxSize));  // 按尺寸设置裁切框
    exports.Set("getAPRCropSettings", Napi::Function::New(env, GetAPRCropSettings));  // 获取裁切设置
    exports.Set("cropVolume", Napi::Function::New(env, CropVolume));  // 执行裁切
    exports.Set("applyCroppedVolumeToSession", Napi::Function::New(env, ApplyCroppedVolumeToSession));  // 应用裁切到当前session
    
    // MPR Mask 编辑和管�?API
    exports.Set("getVolumeSpacing", Napi::Function::New(env, GetVolumeSpacing));  // 获取spacing
    exports.Set("getVolumeHistogram", Napi::Function::New(env, GetVolumeHistogram));  // 获取直方�?
    exports.Set("getMaskStatistics", Napi::Function::New(env, GetMaskStatistics));  // 获取mask统计
    exports.Set("calculateBoneMetrics", Napi::Function::New(env, CalculateBoneMetrics));  // 计算骨分析指�?
    exports.Set("updatePreviewMask", Napi::Function::New(env, UpdatePreviewMask));  // 更新预览mask
    exports.Set("clearPreviewMask", Napi::Function::New(env, ClearPreviewMask));  // 清除预览mask
    exports.Set("createMaskFromThreshold", Napi::Function::New(env, CreateMaskFromThreshold));  // 创建permanent mask
    exports.Set("createEmptyMask", Napi::Function::New(env, CreateEmptyMask));  // 创建空mask(ROI绘制)
    exports.Set("selectMaskForEditing", Napi::Function::New(env, SelectMaskForEditing));  // 选择mask用于编辑
    exports.Set("setMaskTool", Napi::Function::New(env, SetMaskTool));  // 设置MaskEdit子工�?
    exports.Set("getMaskTool", Napi::Function::New(env, GetMaskTool));  // 获取MaskEdit子工�?
    exports.Set("setMaskBrushRadius", Napi::Function::New(env, SetMaskBrushRadius));  // 设置笔刷半径
    exports.Set("getMaskBrushRadius", Napi::Function::New(env, GetMaskBrushRadius));  // 获取笔刷半径
    exports.Set("deleteMask", Napi::Function::New(env, DeleteMask));  // 删除mask
    exports.Set("saveMasks", Napi::Function::New(env, SaveMasks));  // 保存masks
    exports.Set("loadMasks", Napi::Function::New(env, LoadMasks));  // 加载masks
    exports.Set("exportMaskToStl", Napi::Function::New(env, ExportMaskToStl));  // 导出mask网格(STL)
    
    // Morphology and Boolean Operations
    exports.Set("maskMorphology2D", Napi::Function::New(env, MaskMorphology2D));  // 2D形态学操作
    exports.Set("maskMorphology3D", Napi::Function::New(env, MaskMorphology3D));  // 3D形态学操作
    exports.Set("maskBoolean", Napi::Function::New(env, MaskBoolean));  // 布尔运算
    exports.Set("maskInverse", Napi::Function::New(env, MaskInverse));  // Mask反转

    // 3D primitives
    exports.Set("add3DPrimitive", Napi::Function::New(env, Add3DPrimitive));
    exports.Set("remove3DPrimitive", Napi::Function::New(env, Remove3DPrimitive));
    exports.Set("clear3DPrimitives", Napi::Function::New(env, Clear3DPrimitives));
    exports.Set("set3DPrimitiveTransform", Napi::Function::New(env, Set3DPrimitiveTransform));
    exports.Set("set3DSceneTransform", Napi::Function::New(env, Set3DSceneTransform));
    exports.Set("set3DPrimitiveColor", Napi::Function::New(env, Set3DPrimitiveColor));
    exports.Set("set3DPrimitiveVisible", Napi::Function::New(env, Set3DPrimitiveVisible));
}

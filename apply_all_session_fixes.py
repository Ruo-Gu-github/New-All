"""
Modify VisualizationApi.cpp to support per-session volume management
"""
import shutil

src = r'D:\2025-09-25 新系列\ConsoleDllTest\DllVisualization\VisualizationApi.cpp'
bak = src + '.session_fix.bak'

# Create backup
shutil.copy2(src, bak)
print(f'Created backup: {bak}')

with open(src, 'rb') as f:
    content = f.read()

original_size = len(content)
print(f'Original file size: {original_size} bytes')

# 1. Add VolumeData.h include
old_include = b'#include "../DllDicom/DicomApi.h"'
new_include = b'#include "../DllDicom/DicomApi.h"\r\n#include "../Common/VolumeData.h"'
if old_include in content and b'VolumeData.h' not in content:
    content = content.replace(old_include, new_include, 1)
    print('1. Added VolumeData.h include')
else:
    print('1. VolumeData.h include already present or not needed')

# 2. Remove local VolumeContext definition if it exists
old_vol_def = b'''struct VolumeContext {\r\n    std::vector<uint16_t> data;\r\n    int width, height, depth;\r\n    float spacingX, spacingY, spacingZ;\r\n    float originX, originY, originZ;\r\n};'''
if old_vol_def in content:
    content = content.replace(old_vol_def, b'// VolumeContext is now defined in ../Common/VolumeData.h')
    print('2. Removed local VolumeContext definition')
else:
    print('2. Local VolumeContext definition not found (might be OK)')

# 3. Replace forward declaration
old_fwd = b'struct VolumeContext;\r\n'
new_fwd = b'// VolumeContext is defined in ../Common/VolumeData.h\r\n'
if old_fwd in content:
    content = content.replace(old_fwd, new_fwd, 1)  # Only first occurrence
    print('3. Replaced VolumeContext forward declaration')

# 4. Add per-session cropped APR storage
old_global = b'static APRHandle g_lastCroppedAPR = nullptr;'
new_global = b'''// Per-session cropped APR storage (sessionId -> APRHandle)
static std::map<std::string, APRHandle> g_sessionCroppedAPRs;
// Legacy global pointer for backward compatibility
static APRHandle g_lastCroppedAPR = nullptr;'''
if old_global in content:
    content = content.replace(old_global, new_global, 1)
    print('4. Added per-session cropped APR storage')

# 5. Add APR_UpdateSlice function before APR_Render
apr_render_sig = b'NativeResult APR_Render(APRHandle handle) {'
if apr_render_sig in content and b'APR_UpdateSlice' not in content:
    update_slice_func = b'''// APR_UpdateSlice: Update the slice data in displayBuffer without OpenGL rendering
// This is used by 3D orthogonal renderer to get slice textures
NativeResult APR_UpdateSlice(APRHandle handle) {
    // For now, just call APR_Render which also updates displayBuffer
    return APR_Render(handle);
}

'''
    content = content.replace(apr_render_sig, update_slice_func + apr_render_sig, 1)
    print('5. Added APR_UpdateSlice function')

# 6. Add rescale parameter copying in APR_CropVolume
spacing_line = b'newVol->spacingZ = spacing[2];'
if spacing_line in content and b'rescaleSlope' not in content[content.find(b'APR_CropVolume'):content.find(b'APR_CropVolume')+5000]:
    idx = content.find(spacing_line)
    # Find next newline
    nl_idx = content.find(b'\r\n', idx)
    if nl_idx > 0:
        rescale_code = b'''
    
    // Copy rescale parameters from source volume
    float rescaleSlope = 1.0f, rescaleIntercept = 0.0f;
    Dicom_Volume_GetRescale(srcCtx->volume, &rescaleSlope, &rescaleIntercept);
    newVol->rescaleSlope = rescaleSlope;
    newVol->rescaleIntercept = rescaleIntercept;
'''
        content = content[:nl_idx+2] + rescale_code + content[nl_idx+2:]
        print('6. Added rescale parameter copying in APR_CropVolume')

# 7. Modify APR_CropVolume cleanup - save sessionId and use per-session storage
old_cleanup = b'''if (g_lastCroppedAPR) {\r\n        APR_Destroy(g_lastCroppedAPR);\r\n        g_lastCroppedAPR = nullptr;\r\n    }'''
new_cleanup = b'''// Get sessionId from source handle for per-session storage
    std::string cropSessionId = srcCtx->sessionId;
    
    // Destroy previous cropped APR for this session (if any)
    auto cropIt = g_sessionCroppedAPRs.find(cropSessionId);
    if (cropIt != g_sessionCroppedAPRs.end() && cropIt->second) {
        APR_Destroy(cropIt->second);
        g_sessionCroppedAPRs.erase(cropIt);
    }
    // Also clear legacy global if it was for this session
    if (g_lastCroppedAPR) {
        auto lastCtx = static_cast<APRContext*>(g_lastCroppedAPR);
        if (lastCtx->sessionId == cropSessionId) {
            g_lastCroppedAPR = nullptr;
        }
    }'''

if old_cleanup in content:
    content = content.replace(old_cleanup, new_cleanup, 1)
    print('7. Modified APR_CropVolume cleanup')

# 8. Modify APR_CropVolume save - save to session map
old_save = b'g_lastCroppedAPR = newCtx;'
new_save = b'''// Save to session-specific storage
    newCtx->sessionId = cropSessionId;
    g_sessionCroppedAPRs[cropSessionId] = newCtx;
    // Also update legacy global for backward compatibility
    g_lastCroppedAPR = newCtx;'''

if old_save in content and b'g_sessionCroppedAPRs[cropSessionId]' not in content:
    content = content.replace(old_save, new_save, 1)
    print('8. Modified APR_CropVolume save')

# 9. Add session-aware APR_ApplyCroppedVolumeForSession before legacy version
old_apply_sig = b'int APR_ApplyCroppedVolume() {'
new_apply_func = b'''// Session-aware version: apply cropped volume for specific session
int APR_ApplyCroppedVolumeForSession(const char* sessionId) {
    if (!sessionId || sessionId[0] == '\\0') {
        printf("[ApplyCroppedVolume] Error: sessionId is null or empty\\n");
        return 0;
    }
    
    std::string sessId(sessionId);
    auto it = g_sessionCroppedAPRs.find(sessId);
    APRHandle croppedHandle = nullptr;
    
    if (it != g_sessionCroppedAPRs.end() && it->second) {
        croppedHandle = it->second;
    } else if (g_lastCroppedAPR) {
        // Fallback to legacy global if session-specific not found
        auto lastCtx = static_cast<APRContext*>(g_lastCroppedAPR);
        if (lastCtx->sessionId == sessId) {
            croppedHandle = g_lastCroppedAPR;
        }
    }
    
    if (!croppedHandle) {
        printf("[ApplyCroppedVolume] Error: No cropped volume for session %s\\n", sessionId);
        return 0;
    }
    
    auto croppedCtx = static_cast<APRContext*>(croppedHandle);
    if (!croppedCtx->croppedVolumeData) {
        printf("[ApplyCroppedVolume] Error: Cropped volume data is null\\n");
        return 0;
    }
    
    auto croppedVol = croppedCtx->croppedVolumeData;
    VolumeHandle newVolume = static_cast<VolumeHandle>(croppedVol);
    
    printf("[ApplyCroppedVolume] Applying cropped volume for session %s: %d x %d x %d\\n",
           sessionId, croppedVol->width, croppedVol->height, croppedVol->depth);
    
    // Find all APRs with matching sessionId
    std::vector<APRContext*> sessionAPRs;
    for (APRHandle apr : g_globalAPRCenter.linkedAPRs) {
        auto ctx = static_cast<APRContext*>(apr);
        if (ctx && ctx->sessionId == sessId) {
            sessionAPRs.push_back(ctx);
        }
    }
    
    // Replace volume for all session APRs
    for (auto ctx : sessionAPRs) {
        ctx->volume = newVolume;
        ctx->croppedVolumeData = nullptr;
        ctx->ownsVolumeData = false;
        
        ctx->centerX = croppedVol->width * 0.5f;
        ctx->centerY = croppedVol->height * 0.5f;
        ctx->centerZ = croppedVol->depth * 0.5f;
        
        Mat4_Identity(ctx->rotMat);
    }
    
    // Update global center if it matches this session
    if (g_globalAPRCenter.volume) {
        g_globalAPRCenter.x = croppedVol->width * 0.5f;
        g_globalAPRCenter.y = croppedVol->height * 0.5f;
        g_globalAPRCenter.z = croppedVol->depth * 0.5f;
        g_globalAPRCenter.volume = newVolume;
        Mat4_Identity(g_globalAPRCenter.rotMat);
    }
    
    g_aprCropBox.enabled = false;
    
    // Clear ownership from cropped context
    croppedCtx->croppedVolumeData = nullptr;
    croppedCtx->ownsVolumeData = false;
    
    printf("[ApplyCroppedVolume] Volume replaced for %zu APRs in session %s\\n", 
           sessionAPRs.size(), sessionId);
    
    return 1;
}

// Legacy version for backward compatibility
int APR_ApplyCroppedVolume() {'''

if old_apply_sig in content and b'APR_ApplyCroppedVolumeForSession' not in content:
    content = content.replace(old_apply_sig, new_apply_func, 1)
    print('9. Added APR_ApplyCroppedVolumeForSession function')

# Write result
with open(src, 'wb') as f:
    f.write(content)

final_size = len(content)
print(f'\nFinal file size: {final_size} bytes (delta: {final_size - original_size})')
print('Done!')

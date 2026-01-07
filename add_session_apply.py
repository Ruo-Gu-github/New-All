src = r'D:\2025-09-25 新系列\ConsoleDllTest\DllVisualization\VisualizationApi.cpp'

with open(src, 'rb') as f:
    content = f.read()

# Find the start of APR_ApplyCroppedVolume
old_sig = b'int APR_ApplyCroppedVolume() {'

new_impl = b'''// Session-aware version: apply cropped volume for specific session
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

if old_sig in content:
    content = content.replace(old_sig, new_impl)
    with open(src, 'wb') as f:
        f.write(content)
    print('Added APR_ApplyCroppedVolumeForSession function')
else:
    print('APR_ApplyCroppedVolume signature not found')

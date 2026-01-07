$filePath = "D:\2025-09-25 新系列\ConsoleDllTest\DllVisualization\VisualizationApi.cpp"
$bytes = [System.IO.File]::ReadAllBytes($filePath)
$encoding = [System.Text.Encoding]::GetEncoding("gb2312")
$content = $encoding.GetString($bytes)

# Pattern to find and replace
$oldPattern = @'
NativeResult APR_Render(APRHandle handle) {
    if (!handle) return NATIVE_E_INVALID_ARGUMENT;
    auto ctx = static_cast<APRContext*>(handle);
    if (!ctx->volume) return NATIVE_E_INVALID_ARGUMENT;

    float rescaleSlope = 1.0f;
    float rescaleIntercept = 0.0f;
    (void)Dicom_Volume_GetRescale(ctx->volume, &rescaleSlope, &rescaleIntercept);
'@

$newCode = @'
NativeResult APR_Render(APRHandle handle) {
    if (!handle) return NATIVE_E_INVALID_ARGUMENT;
    auto ctx = static_cast<APRContext*>(handle);
    
    // 优先使用全局活跃 volume（裁切后的 volume），否则使用 APR 绑定的 volume
    bool useActiveVolume = (g_activeVolume != nullptr && !g_activeVolume->data.empty());
    
    if (!useActiveVolume && !ctx->volume) return NATIVE_E_INVALID_ARGUMENT;

    float rescaleSlope = 1.0f;
    float rescaleIntercept = 0.0f;
'@

if ($content.Contains($oldPattern)) {
    $content = $content.Replace($oldPattern, $newCode)
    Write-Host "Replaced first part"
} else {
    Write-Host "First pattern NOT found"
    exit 1
}

# Replace the volume data acquisition section
$oldPattern2 = @'
    // ===== 线程安全：查找绑定的 WindowContext =====
    WindowContext* windowCtx = nullptr;
    for (auto winHandle : g_AllWindows) {
        auto win = static_cast<WindowContext*>(winHandle);
        if (win && win->boundRenderer == handle) {
            windowCtx = win;
            break;
        }
    }
    
    // 如果找到窗口上下文，加锁保护
    std::unique_ptr<std::lock_guard<std::mutex>> lock;
    if (windowCtx) {
        lock = std::make_unique<std::lock_guard<std::mutex>>(windowCtx->renderMutex);
    }
    
    // 获取 Volume 尺寸、数据和间距
    int width, height, depth;
    float spacingX, spacingY, spacingZ;
    
    if (Dicom_Volume_GetDimensions(ctx->volume, &width, &height, &depth) != NATIVE_OK) {
        SetLastError("Failed to get volume dimensions");
        return NATIVE_E_INTERNAL_ERROR;
    }
    
    if (Dicom_Volume_GetSpacing(ctx->volume, &spacingX, &spacingY, &spacingZ) != NATIVE_OK) {
        SetLastError("Failed to get volume spacing");
        return NATIVE_E_INTERNAL_ERROR;
    }
    
    uint16_t* volumeData = static_cast<uint16_t*>(Dicom_Volume_GetData(ctx->volume));
    if (!volumeData) {
        SetLastError("Volume data is null");
        return NATIVE_E_INTERNAL_ERROR;
    }
    
    // 旋转中心（体数据中心，固定不变）
'@

$newCode2 = @'
    // 获取 Volume 尺寸、数据和间距
    int width, height, depth;
    float spacingX, spacingY, spacingZ;
    uint16_t* volumeData = nullptr;
    
    if (useActiveVolume) {
        // 使用裁切后的 volume
        width = g_activeVolume->width;
        height = g_activeVolume->height;
        depth = g_activeVolume->depth;
        spacingX = g_activeVolume->spacingX;
        spacingY = g_activeVolume->spacingY;
        spacingZ = g_activeVolume->spacingZ;
        rescaleSlope = g_activeVolume->rescaleSlope;
        rescaleIntercept = g_activeVolume->rescaleIntercept;
        volumeData = const_cast<uint16_t*>(g_activeVolume->data.data());
    } else {
        // 使用原始 DICOM volume
        (void)Dicom_Volume_GetRescale(ctx->volume, &rescaleSlope, &rescaleIntercept);
        
        if (Dicom_Volume_GetDimensions(ctx->volume, &width, &height, &depth) != NATIVE_OK) {
            SetLastError("Failed to get volume dimensions");
            return NATIVE_E_INTERNAL_ERROR;
        }
        
        if (Dicom_Volume_GetSpacing(ctx->volume, &spacingX, &spacingY, &spacingZ) != NATIVE_OK) {
            SetLastError("Failed to get volume spacing");
            return NATIVE_E_INTERNAL_ERROR;
        }
        
        volumeData = static_cast<uint16_t*>(Dicom_Volume_GetData(ctx->volume));
    }
    
    if (!volumeData) {
        SetLastError("Volume data is null");
        return NATIVE_E_INTERNAL_ERROR;
    }

    // 线程安全：查找绑定的 WindowContext
    WindowContext* windowCtx = nullptr;
    for (auto winHandle : g_AllWindows) {
        auto win = static_cast<WindowContext*>(winHandle);
        if (win && win->boundRenderer == handle) {
            windowCtx = win;
            break;
        }
    }
    
    // 如果找到窗口上下文，加锁保护
    std::unique_ptr<std::lock_guard<std::mutex>> lock;
    if (windowCtx) {
        lock = std::make_unique<std::lock_guard<std::mutex>>(windowCtx->renderMutex);
    }
    
    // 旋转中心（体数据中心，固定不变）
'@

if ($content.Contains($oldPattern2)) {
    $content = $content.Replace($oldPattern2, $newCode2)
    Write-Host "Replaced second part"
} else {
    Write-Host "Second pattern NOT found - trying without exact match..."
}

$newBytes = $encoding.GetBytes($content)
[System.IO.File]::WriteAllBytes($filePath, $newBytes)

Write-Host "File updated successfully!"

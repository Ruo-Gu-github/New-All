# -*- coding: utf-8 -*-
import re
import codecs

file_path = r"D:\2025-09-25 新系列\ConsoleDllTest\DllVisualization\VisualizationApi.cpp"

# Read file with GBK encoding
with codecs.open(file_path, 'r', 'gbk', errors='replace') as f:
    content = f.read()

# Pattern to find - just the first few lines to be safe
old_pattern = '''NativeResult APR_Render(APRHandle handle) {
    if (!handle) return NATIVE_E_INVALID_ARGUMENT;
    auto ctx = static_cast<APRContext*>(handle);
    if (!ctx->volume) return NATIVE_E_INVALID_ARGUMENT;

    float rescaleSlope = 1.0f;
    float rescaleIntercept = 0.0f;
    (void)Dicom_Volume_GetRescale(ctx->volume, &rescaleSlope, &rescaleIntercept);'''

new_code = '''NativeResult APR_Render(APRHandle handle) {
    if (!handle) return NATIVE_E_INVALID_ARGUMENT;
    auto ctx = static_cast<APRContext*>(handle);
    
    // 优先使用全局活跃 volume（裁切后的 volume），否则使用 APR 绑定的 volume
    bool useActiveVolume = (g_activeVolume != nullptr && !g_activeVolume->data.empty());
    
    if (!useActiveVolume && !ctx->volume) return NATIVE_E_INVALID_ARGUMENT;

    float rescaleSlope = 1.0f;
    float rescaleIntercept = 0.0f;'''

if old_pattern in content:
    content = content.replace(old_pattern, new_code)
    print("Replaced first part successfully")
else:
    print("First pattern NOT found")
    exit(1)

# Now find and replace the volume acquisition section
# We need to find the section that starts with the WindowContext search
# and ends before the rotation center

old_pattern2 = '''    WindowContext* windowCtx = nullptr;
    for (auto winHandle : g_AllWindows) {
        auto win = static_cast<WindowContext*>(winHandle);
        if (win && win->boundRenderer == handle) {
            windowCtx = win;
            break;
        }
    }'''

# After first pattern replaced, find the position after new code
# and insert the volume acquisition logic

# Actually, let me try a simpler approach - just find where we need to add the if/else block
# The structure after replacement should be:
# 1. useActiveVolume check
# 2. rescaleSlope/Intercept initialization
# 3. Volume data acquisition (need to add if/else here)
# 4. WindowContext search
# 5. lock
# 6. rest of function

# Let's find and replace the section from WindowContext to rotationCenterZ
import_section = '''    WindowContext* windowCtx = nullptr;
    for (auto winHandle : g_AllWindows) {
        auto win = static_cast<WindowContext*>(winHandle);
        if (win && win->boundRenderer == handle) {
            windowCtx = win;
            break;
        }
    }'''

# Find end pattern - we need to find the section to replace
# This is tricky due to encoding issues in comments. Let's use a regex pattern

# Pattern: from WindowContext search to just before rotationCenterX
pattern = r'(    WindowContext\* windowCtx = nullptr;.*?)(    float rotationCenterX = width / 2\.0f;)'

match = re.search(pattern, content, re.DOTALL)
if match:
    print(f"Found section to replace: {len(match.group(1))} chars")
    
    # New volume acquisition code
    new_volume_code = '''    // 获取 Volume 尺寸、数据和间距
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
    '''
    
    content = content[:match.start()] + new_volume_code + match.group(2) + content[match.end():]
    print("Replaced second part successfully")
else:
    print("Second pattern NOT found")

# Write back
with codecs.open(file_path, 'w', 'gbk') as f:
    f.write(content)

print("File updated successfully!")

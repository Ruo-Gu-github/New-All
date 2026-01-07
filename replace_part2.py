import shutil

src = r'D:\2025-09-25 新系列\ConsoleDllTest\DllVisualization\VisualizationApi.cpp'

with open(src, 'rb') as f:
    content = f.read()

# Find the section to replace
idx = content.find(b'bool useActiveVolume = (g_activeVolume')
if idx < 0:
    print('Cannot find useActiveVolume marker')
    exit(1)

start_marker = b'float rescaleIntercept = 0.0f;'
start_idx = content.find(start_marker, idx)
if start_idx < 0:
    print('Cannot find rescaleIntercept marker')
    exit(1)

end_marker = b'float rotationCenterX = width / 2.0f;'
section_start = start_idx + len(start_marker)
section_end = content.find(end_marker, section_start)
if section_end < 0:
    print('Cannot find rotationCenterX marker')
    exit(1)

# New code to insert
new_section = b'''
    
    // Volume dimensions and data (using g_activeVolume if available)
    int width, height, depth;
    float spacingX, spacingY, spacingZ;
    uint16_t* volumeData = nullptr;
    
    if (useActiveVolume) {
        // Use g_activeVolume (cropped volume or externally set volume)
        width = g_activeVolume->width;
        height = g_activeVolume->height;
        depth = g_activeVolume->depth;
        spacingX = g_activeVolume->spacingX;
        spacingY = g_activeVolume->spacingY;
        spacingZ = g_activeVolume->spacingZ;
        volumeData = static_cast<uint16_t*>(g_activeVolume->data);
        rescaleSlope = g_activeVolume->rescaleSlope;
        rescaleIntercept = g_activeVolume->rescaleIntercept;
    } else {
        // Use original volume from ctx->volume via Dicom API
        Dicom_Volume_GetDimensions(ctx->volume, &width, &height, &depth);
        Dicom_Volume_GetSpacing(ctx->volume, &spacingX, &spacingY, &spacingZ);
        volumeData = static_cast<uint16_t*>(Dicom_Volume_GetData(ctx->volume));
        
        // Get rescale parameters from window context
        WindowContext* windowCtx = nullptr;
        for (auto winHandle : g_AllWindows) {
            WindowContext* wctx = static_cast<WindowContext*>(winHandle);
            if (wctx && wctx->volumeHandle == ctx->volume) {
                windowCtx = wctx;
                break;
            }
        }
        if (windowCtx) {
            std::lock_guard<std::mutex> lock(windowCtx->mutex);
            rescaleSlope = windowCtx->rescaleSlope;
            rescaleIntercept = windowCtx->rescaleIntercept;
        }
    }
    
    if (!volumeData) {
        LOG_ERROR("APR_Render: Volume data is null");
        return NATIVE_E_INTERNAL_ERROR;
    }
    
    '''

# Convert LF to CRLF for Windows
new_section = new_section.replace(b'\n', b'\r\n')

# Replace the old section with new section
new_content = content[:section_start] + new_section + content[section_end:]

# Write back
with open(src, 'wb') as f:
    f.write(new_content)

print(f'Successfully replaced second part!')
print(f'Old section: {section_end - section_start} bytes')
print(f'New section: {len(new_section)} bytes')

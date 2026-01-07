import shutil

src = r'D:\2025-09-25 新系列\ConsoleDllTest\DllVisualization\VisualizationApi.cpp'

with open(src, 'rb') as f:
    content = f.read()

# Find where spacing is set and add rescale after it
old_pattern = b'''newVol->spacingZ = spacing[2];
    newVol->originX = 0.0f;'''

new_code = b'''newVol->spacingZ = spacing[2];
    
    // Copy rescale parameters from source volume
    float rescaleSlope = 1.0f, rescaleIntercept = 0.0f;
    Dicom_Volume_GetRescale(srcCtx->volume, &rescaleSlope, &rescaleIntercept);
    newVol->rescaleSlope = rescaleSlope;
    newVol->rescaleIntercept = rescaleIntercept;
    
    newVol->originX = 0.0f;'''

if old_pattern in content:
    content = content.replace(old_pattern, new_code)
    with open(src, 'wb') as f:
        f.write(content)
    print('Added rescale parameter copying to APR_CropVolume')
else:
    print('Pattern not found - checking with different formatting')
    # Try to find the pattern
    idx = content.find(b'newVol->spacingZ = spacing[2]')
    if idx >= 0:
        ctx = content[idx:idx+100].decode('utf-8', errors='replace')
        print(f'Found at {idx}: {ctx}')

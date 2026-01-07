src = r'D:\2025-09-25 新系列\ConsoleDllTest\DllVisualization\VisualizationApi.cpp'

with open(src, 'rb') as f:
    content = f.read()

# Find the includes section and add VolumeData.h
old_include = b'#include "../DllDicom/DicomApi.h"'
new_include = b'#include "../DllDicom/DicomApi.h"\r\n#include "../Common/VolumeData.h"'

if old_include in content:
    content = content.replace(old_include, new_include)
    with open(src, 'wb') as f:
        f.write(content)
    print('Added VolumeData.h include')
else:
    print('Could not find DicomApi.h include')

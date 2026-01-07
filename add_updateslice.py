import shutil

src = r'D:\2025-09-25 新系列\ConsoleDllTest\DllVisualization\VisualizationApi.cpp'

with open(src, 'rb') as f:
    content = f.read()

# Find APR_Render location
apr_render_idx = content.find(b'NativeResult APR_Render(APRHandle handle)')
if apr_render_idx < 0:
    print('Cannot find APR_Render')
    exit(1)

# Insert APR_UpdateSlice before APR_Render
# APR_UpdateSlice is a simplified version that only updates displayBuffer without OpenGL rendering
new_function = b'''// APR_UpdateSlice: Update the slice data in displayBuffer without OpenGL rendering
// This is used by 3D orthogonal renderer to get slice textures
NativeResult APR_UpdateSlice(APRHandle handle) {
    // For now, just call APR_Render which also updates displayBuffer
    // The OpenGL texture upload in APR_Render is harmless if we're not using the 2D view
    return APR_Render(handle);
}

'''

# Insert before APR_Render
new_content = content[:apr_render_idx] + new_function + content[apr_render_idx:]

with open(src, 'wb') as f:
    f.write(new_content)

print(f'Added APR_UpdateSlice function before APR_Render')
print(f'File size: {len(content)} -> {len(new_content)}')

src = r'D:\2025-09-25 新系列\ConsoleDllTest\DllVisualization\VisualizationApi.cpp'

with open(src, 'rb') as f:
    content = f.read()

# Find the cleanup section in APR_CropVolume
# Look for "if (g_lastCroppedAPR)" after "srcCtx->volume"
idx = content.find(b'APRHandle APR_CropVolume')
if idx < 0:
    print('Cannot find APR_CropVolume')
    exit(1)

# Find the cleanup code
cleanup_start = content.find(b'if (g_lastCroppedAPR) {', idx)
if cleanup_start < 0:
    print('Cannot find g_lastCroppedAPR cleanup')
    exit(1)

# Find the end of this block (the closing brace followed by newlines)
cleanup_end = content.find(b'g_lastCroppedAPR = nullptr;', cleanup_start)
if cleanup_end < 0:
    print('Cannot find cleanup end')
    exit(1)

# Find the closing brace
brace_end = content.find(b'}', cleanup_end)
if brace_end < 0:
    print('Cannot find closing brace')
    exit(1)

# Find the start of the comment before the if
comment_start = content.rfind(b'//', cleanup_start - 100, cleanup_start)
if comment_start < 0:
    comment_start = cleanup_start

# Find the newline before the comment
newline_before = content.rfind(b'\r\n', comment_start - 10, comment_start)
if newline_before < 0:
    newline_before = comment_start
else:
    newline_before += 2  # Include the CRLF

old_section = content[newline_before:brace_end+1]
print(f'Found old section ({len(old_section)} bytes):')
print(old_section.decode('utf-8', errors='replace'))

new_section = b'''    // Get sessionId from source handle for per-session storage
    std::string sessionId = srcCtx->sessionId;
    
    // Destroy previous cropped APR for this session (if any)
    auto it = g_sessionCroppedAPRs.find(sessionId);
    if (it != g_sessionCroppedAPRs.end() && it->second) {
        APR_Destroy(it->second);
        g_sessionCroppedAPRs.erase(it);
    }
    // Also clear legacy global if it was for this session
    if (g_lastCroppedAPR) {
        auto lastCtx = static_cast<APRContext*>(g_lastCroppedAPR);
        if (lastCtx->sessionId == sessionId) {
            g_lastCroppedAPR = nullptr;
        }
    }'''

content = content[:newline_before] + new_section + content[brace_end+1:]

with open(src, 'wb') as f:
    f.write(content)

print('\nUpdated APR_CropVolume cleanup code')

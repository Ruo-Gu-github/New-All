# -*- coding: utf-8 -*-
import re

file_path = r"D:\2025-09-25 新系列\ConsoleDllTest\DllVisualization\ImageBrowserOrthogonal3DRenderer.cpp"

with open(file_path, 'r', encoding='utf-8') as f:
    content = f.read()

print(f"Original file length: {len(content)}")

# 1. Remove debug helper functions (TryReadIntFromFile through DrawTexturedQuad_ScreenSpace)
# Find the start of TryReadIntFromFile and end at } // namespace
start_marker = "bool TryReadIntFromFile"
end_marker = "} // namespace"
start_idx = content.find(start_marker)
end_idx = content.find(end_marker)

if start_idx > 0 and end_idx > start_idx:
    content = content[:start_idx] + content[end_idx:]
    print(f"Removed debug helper functions from {start_idx} to {end_idx}")

# 2. Remove the debug stage block
start_marker2 = "// ====== 分阶段矩阵排查"
end_marker2 = "// ========== 3D正交渲染"
start_idx2 = content.find(start_marker2)
end_idx2 = content.find(end_marker2)

if start_idx2 > 0 and end_idx2 > start_idx2:
    content = content[:start_idx2] + content[end_idx2:]
    print(f"Removed debug stage block from {start_idx2} to {end_idx2}")

# 3. Remove all fopen_s debug logging blocks
pattern = r'fopen_s\(&logFile, "D:\\\\3d_render_debug\.log", "a"\);\s*\r?\n\s*if \(logFile\) \{[\s\S]*?fclose\(logFile\);\s*\r?\n\s*\}'
content, count = re.subn(pattern, '', content)
print(f"Removed {count} fopen_s debug blocks")

# 4. Remove FILE* logFile declarations with init
pattern2 = r'\s*FILE\* logFile = nullptr;\s*\r?\n'
content, count = re.subn(pattern2, '\n', content)
print(f"Removed {count} FILE* logFile declarations")

# 5. Remove printf("[3D]...") statements
pattern3 = r'printf\("\[3D\][^"]*"[^;]*;\s*\r?\n?'
content, count = re.subn(pattern3, '', content)
print(f"Removed {count} printf statements")

# 6. Clean up the render function entry (remove initial debug log block comment)
old_entry = """) {
    // 写日志到文件，确保能看到"""
new_entry = ") {"
content = content.replace(old_entry, new_entry)

# 7. Remove the #if 0 block
if0_start = "// ========== 以下是原来的复杂渲染代码，暂时跳过 =========="
if0_end = "#endif  // #if 0 关闭复杂渲染"
if0_start_idx = content.find(if0_start)
if0_end_idx = content.find(if0_end)

if if0_start_idx > 0 and if0_end_idx > if0_start_idx:
    # Find end of line after #endif
    newline_idx = content.find('\n', if0_end_idx)
    if newline_idx > 0:
        content = content[:if0_start_idx] + content[newline_idx + 1:]
        print(f"Removed #if 0 block")

# 8. Remove final debug log
final_debug = '''    fopen_s(&logFile, "D:\\\\3d_render_debug.log", "a");
    if (logFile) {
        fprintf(logFile, "========== [3D Render End - SUCCESS] ==========\\n\\n");
        fclose(logFile);
    }'''
content = content.replace(final_debug, '')

# 9. Clean up extra blank lines
content = re.sub(r'\n\n\n+', '\n\n', content)

print(f"New file length: {len(content)}")

with open(file_path, 'w', encoding='utf-8') as f:
    f.write(content)

print("Cleanup complete!")

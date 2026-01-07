# -*- coding: utf-8 -*-
"""
修复旋转后 MIP/MinIP 只在 Axial 方向生效的问题
使用精确行号匹配
"""

filepath = r"d:\2025-09-25 新系列\ConsoleDllTest\DllVisualization\VisualizationApi.cpp"

# 读取文件
with open(filepath, 'r', encoding='utf-8-sig') as f:
    lines = f.readlines()

print(f"总行数: {len(lines)}")

# 备份
backup_path = filepath + '.bak_mip4'
with open(backup_path, 'w', encoding='utf-8-sig') as f:
    f.writelines(lines)
print(f"备份保存到: {backup_path}")

# 找到两个关键行
line_9155_text = lines[9154]  # 0-based index
line_9218_text = lines[9217]

print(f"Line 9155: {line_9155_text.strip()}")
print(f"Line 9218: {line_9218_text.strip()}")

# 确认是我们要修改的行
if 'uint16_t value = SampleVolume' not in line_9155_text:
    print("错误: line 9155 不是预期的代码")
    exit(1)
if 'uint16_t value = SampleVolume' not in line_9218_text:
    print("错误: line 9218 不是预期的代码")
    exit(1)

#修复函数
def create_mip_code_block(direction_name, varying_axis, fixed_axis1, fixed_axis2, center_name):
    """
    direction_name: "Y" or "X" - 采样方向
    varying_axis: "X","Y" - 变化的轴
    fixed_axis1, fixed_axis2: 固定的轴名
    center_name: 固定中心的变量名
    """
    return f'''                                float virtual{varying_axis} = static_cast<float>(out{varying_axis});
                                float virtual{fixed_axis2} = static_cast<float>(out{fixed_axis2});
                                
                                uint16_t finalValue;
                                if (projMode == 0) {{
                                    // Normal mode: single slice at {center_name}
                                    float virtual{fixed_axis1} = {center_name};
                                    float dx = virtualX - rotationCenterX;
                                    float dy = virtualY - rotationCenterY;
                                    float dz = virtualZ - rotationCenterZ;
                                    float realX = rotationCenterX + (r00 * dx + r01 * dy + r02 * dz);
                                    float realY = rotationCenterY + (r10 * dx + r11 * dy + r12 * dz);
                                    float realZ = rotationCenterZ + (r20 * dx + r21 * dy + r22 * dz);
                                    finalValue = SampleVolume(volumeData, width, height, depth, realX, realY, realZ);
                                }} else {{
                                    // MIP/MinIP mode: sample along {direction_name} direction
                                    finalValue = (projMode == 1) ? 0 : 65535;
                                    for (int k = -halfThickness; k <= halfThickness; ++k) {{
                                        float virtual{fixed_axis1} = {center_name} + static_cast<float>(k);
                                        float dx = virtualX - rotationCenterX;
                                        float dy = virtualY - rotationCenterY;
                                        float dz = virtualZ - rotationCenterZ;
                                        float realX = rotationCenterX + (r00 * dx + r01 * dy + r02 * dz);
                                        float realY = rotationCenterY + (r10 * dx + r11 * dy + r12 * dz);
                                        float realZ = rotationCenterZ + (r20 * dx + r21 * dy + r22 * dz);
                                        uint16_t sample = SampleVolume(volumeData, width, height, depth, realX, realY, realZ);
                                        if (projMode == 1) {{ finalValue = std::max(finalValue, sample); }}
                                        else {{ finalValue = std::min(finalValue, sample); }}
                                    }}
                                }}

                                // 锟斤拷转Z锟斤拷锟斤拷MPR一锟斤拷
                                size_t outIdx = static_cast<size_t>(outHeight - 1 - out{fixed_axis2}) * outWidth + out{varying_axis};
                                sliceData[outIdx] = finalValue;'''

# Coronal: 从 line 9139开始替换 (Y方向采样, X和Z变化)
# 删除 9139-9160 (22行), 替换为新代码
coronal_start = 9138  # 0-based, 这是 "for (int outX"行
coronal_end = 9160    # 包含 "sliceData[outIdx] = value" 的行

coronal_new = create_mip_code_block("Y", "X", "Y", "Z", "virtualCenterY")

# Sagittal: 从 line 9202 开始替换 (X方向采样, Y和Z变化)
sagittal_start = 9201  # 0-based
sagittal_end = 9223

sagittal_new = create_mip_code_block("X", "Y", "X", "Z", "virtualCenterX")

# 检查要替换的行范围
print(f"\nCoronal 修复范围: lines {coronal_start+1}-{coronal_end+1}")
print(f"原代码首行: {lines[coronal_start].strip()}")
print(f"原代码末行: {lines[coronal_end].strip()}")

print(f"\nSagittal 修复范围: lines {sagittal_start+1}-{sagittal_end+1}")
print(f"原代码首行: {lines[sagittal_start].strip()}")
print(f"原代码末行: {lines[sagittal_end].strip()}")

# 执行替换
# Coronal
lines[coronal_start:coronal_end+1] = [coronal_new + '\n']
print("\n✓ Coronal 代码已修复 (沿 Y 方向采样)")

# 需要重新计算 Sagittal 的位置（因为 Coronal 改变了行数）
line_diff = 1 - (coronal_end - coronal_start + 1)
sagittal_start += line_diff
sagittal_end += line_diff

lines[sagittal_start:sagittal_end+1] = [sagittal_new + '\n']
print("✓ Sagittal 代码已修复 (沿 X 方向采样)")

# 写回文件
with open(filepath, 'w', encoding='utf-8-sig') as f:
    f.writelines(lines)

print(f"\n文件已更新: {filepath}")
print("总行数变化:", len(lines) - 17124)
print("\n修复完成! 现在旋转后 MIP/MinIP 在所有三个方向都应该正常工作。")

# -*- coding: utf-8 -*-
import codecs

filepath = r"d:\2025-09-25 新系列\ConsoleDllTest\DllVisualization\VisualizationApi.cpp"

# 读取文件
with codecs.open(filepath, 'r', encoding='utf-8-sig') as f:
    lines = f.readlines()

print(f"总行数: {len(lines)}")
print(f"Line 9154: {lines[9153][:80]}")
print(f"Line 9217: {lines[9216][:80]}")

# Coronal fix: 从 line 9137开始
coronal_start = 9137 - 1  # 转换为0-based索引
coronal_end = 9161 - 1

# Sagittal fix: 从 line 9200开始  
sagittal_start = 9200 - 1  # 转换为0-based索引
sagittal_end = 9224 - 1

# 备份
backup_path = filepath + '.bak_mip2'
with codecs.open(backup_path, 'w', encoding='utf-8-sig') as f:
    f.writelines(lines)
print(f"备份保存到: {backup_path}")

# Coronal 替换 (沿 Y 方向)
new_coronal = """                        for (int outZ = startZ; outZ < endZ; ++outZ) {
                            for (int outX = 0; outX < outWidth; ++outX) {
                                float virtualX = static_cast<float>(outX);
                                float virtualZ = static_cast<float>(outZ);
                                
                                uint16_t finalValue;
                                if (projMode == 0) {
                                    // Normal mode: single slice at virtualCenterY
                                    float virtualY = virtualCenterY;
                                    float dx = virtualX - rotationCenterX;
                                    float dy = virtualY - rotationCenterY;
                                    float dz = virtualZ - rotationCenterZ;
                                    float realX = rotationCenterX + (r00 * dx + r01 * dy + r02 * dz);
                                    float realY = rotationCenterY + (r10 * dx + r11 * dy + r12 * dz);
                                    float realZ = rotationCenterZ + (r20 * dx + r21 * dy + r22 * dz);
                                    finalValue = SampleVolume(volumeData, width, height, depth, realX, realY, realZ);
                                } else {
                                    // MIP/MinIP mode: sample along Y direction
                                    finalValue = (projMode == 1) ? 0 : 65535;
                                    for (int k = -halfThickness; k <= halfThickness; ++k) {
                                        float virtualY = virtualCenterY + static_cast<float>(k);
                                        float dx = virtualX - rotationCenterX;
                                        float dy = virtualY - rotationCenterY;
                                        float dz = virtualZ - rotationCenterZ;
                                        float realX = rotationCenterX + (r00 * dx + r01 * dy + r02 * dz);
                                        float realY = rotationCenterY + (r10 * dx + r11 * dy + r12 * dz);
                                        float realZ = rotationCenterZ + (r20 * dx + r21 * dy + r22 * dz);
                                        uint16_t sample = SampleVolume(volumeData, width, height, depth, realX, realY, realZ);
                                        if (projMode == 1) { finalValue = std::max(finalValue, sample); }
                                        else { finalValue = std::min(finalValue, sample); }
                                    }
                                }

                                // 锟斤拷转Z锟斤拷锟斤拷MPR一锟斤拷
                                size_t outIdx = static_cast<size_t>(outHeight - 1 - outZ) * outWidth + outX;
                                sliceData[outIdx] = finalValue;
                            }
                        }
"""

# Sagittal 替换 (沿 X 方向)
new_sagittal = """                        for (int outZ = startZ; outZ < endZ; ++outZ) {
                            for (int outY = 0; outY < outWidth; ++outY) {
                                float virtualY = static_cast<float>(outY);
                                float virtualZ = static_cast<float>(outZ);
                                
                                uint16_t finalValue;
                                if (projMode == 0) {
                                    // Normal mode: single slice at virtualCenterX
                                    float virtualX = virtualCenterX;
                                    float dx = virtualX - rotationCenterX;
                                    float dy = virtualY - rotationCenterY;
                                    float dz = virtualZ - rotationCenterZ;
                                    float realX = rotationCenterX + (r00 * dx + r01 * dy + r02 * dz);
                                    float realY = rotationCenterY + (r10 * dx + r11 * dy + r12 * dz);
                                    float realZ = rotationCenterZ + (r20 * dx + r21 * dy + r22 * dz);
                                    finalValue = SampleVolume(volumeData, width, height, depth, realX, realY, realZ);
                                } else {
                                    // MIP/MinIP mode: sample along X direction
                                    finalValue = (projMode == 1) ? 0 : 65535;
                                    for (int k = -halfThickness; k <= halfThickness; ++k) {
                                        float virtualX = virtualCenterX + static_cast<float>(k);
                                        float dx = virtualX - rotationCenterX;
                                        float dy = virtualY - rotationCenterY;
                                        float dz = virtualZ - rotationCenterZ;
                                        float realX = rotationCenterX + (r00 * dx + r01 * dy + r02 * dz);
                                        float realY = rotationCenterY + (r10 * dx + r11 * dy + r12 * dz);
                                        float realZ = rotationCenterZ + (r20 * dx + r21 * dy + r22 * dz);
                                        uint16_t sample = SampleVolume(volumeData, width, height, depth, realX, realY, realZ);
                                        if (projMode == 1) { finalValue = std::max(finalValue, sample); }
                                        else { finalValue = std::min(finalValue, sample); }
                                    }
                                }

                                // 锟斤拷转Z锟斤拷锟斤拷MPR一锟斤拷
                                size_t outIdx = static_cast<size_t>(outHeight - 1 - outZ) * outWidth + outY;
                                sliceData[outIdx] = finalValue;
                            }
                        }
"""

# 替换 Coronal
lines[coronal_start:coronal_end] = new_coronal.split('\n')[:-1]  # 不包括最后的空行
for i in range(len(lines[coronal_start:coronal_start+len(new_coronal.split('\n')[:-1])])):
    lines[coronal_start + i] = lines[coronal_start + i] + '\n'

print("✓ Coronal 代码已修复")

# 重新计算 Sagittal 的位置（因为行数可能变化了）
sagittal_start = 9200 - 1 + (len(new_coronal.split('\n')[:-1]) - (coronal_end - coronal_start))
sagittal_end = sagittal_start + 24

lines[sagittal_start:sagittal_end] = new_sagittal.split('\n')[:-1]
for i in range(len(lines[sagittal_start:sagittal_start+len(new_sagittal.split('\n')[:-1])])):
    lines[sagittal_start + i] = lines[sagittal_start + i] + '\n'

print("✓ Sagittal 代码已修复")

# 写回文件
with codecs.open(filepath, 'w', encoding='utf-8-sig') as f:
    f.writelines(lines)

print(f"\n文件已更新: {filepath}")
print("修复完成！")

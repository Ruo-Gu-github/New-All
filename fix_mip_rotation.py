#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
修复旋转后 MIP/MinIP 只在 Axial 方向生效的问题
为 Coronal 和 Sagittal 方向添加 MIP/MinIP 支持
"""

import os
import re

def fix_mip_rotation():
    filepath = r"d:\2025-09-25 新系列\ConsoleDllTest\DllVisualization\VisualizationApi.cpp"
    
    # 读取文件
    with open(filepath, 'r', encoding='utf-8-sig') as f:
        content = f.read()
    
    # 保存备份
    backup_path = filepath + '.bak_mip'
    with open(backup_path, 'w', encoding='utf-8-sig') as f:
        f.write(content)
    print(f"备份已保存到: {backup_path}")
    
    # 修复 Coronal (case 1) - 在 Y 方向采样
    coronal_old = r'''                            for (int outX = 0; outX < outWidth; \+\+outX\) \{
                                // 锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟叫碉拷锟斤拷锟疥（锟斤拷锟斤拷XZ平锟斤拷锟较的点）
                                float virtualX = static_cast<float>\(outX\);
                                float virtualY = virtualCenterY;  // 锟斤拷锟斤拷Y平锟斤拷
                                float virtualZ = static_cast<float>\(outZ\);

                                // 锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟阶拷锟斤拷牡锟狡拷锟\?
                                float dx = virtualX - rotationCenterX;
                                float dy = virtualY - rotationCenterY;
                                float dz = virtualZ - rotationCenterZ;

                                // 应锟斤拷锟斤拷转锟斤拷锟斤拷R锟斤拷锟斤拷锟斤拷 锟斤拷 实锟斤拷
                                float realX = rotationCenterX \+ \(r00 \* dx \+ r01 \* dy \+ r02 \* dz\);
                                float realY = rotationCenterY \+ \(r10 \* dx \+ r11 \* dy \+ r12 \* dz\);
                                float realZ = rotationCenterZ \+ \(r20 \* dx \+ r21 \* dy \+ r22 \* dz\);

                                // 锟斤拷实锟斤拷锟斤拷锟斤拷锟捷诧拷锟斤拷
                                uint16_t value = SampleVolume\(volumeData, width, height, depth, realX, realY, realZ\);

                                // 锟斤拷转Z锟斤拷锟斤拷MPR一锟斤拷
                                size_t outIdx = static_cast<size_t>\(outHeight - 1 - outZ\) \* outWidth \+ outX;
                                sliceData\[outIdx\] = value;'''
    
    coronal_new = '''                            for (int outX = 0; outX < outWidth; ++outX) {
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
                                sliceData[outIdx] = finalValue;'''
    
    # 修复 Sagittal (case 2) - 在 X 方向采样
    sagittal_old = r'''                            for \(int outY = 0; outY < outWidth; \+\+outY\) \{
                                // 锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟叫碉拷锟斤拷锟疥（锟斤拷锟斤拷YZ平锟斤拷锟较的点）
                                float virtualX = virtualCenterX;  // 锟斤拷锟斤拷X平锟斤拷
                                float virtualY = static_cast<float>\(outY\);
                                float virtualZ = static_cast<float>\(outZ\);

                                // 锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟阶拷锟斤拷牡锟狡拷锟\?
                                float dx = virtualX - rotationCenterX;
                                float dy = virtualY - rotationCenterY;
                                float dz = virtualZ - rotationCenterZ;

                                // 应锟斤拷锟斤拷转锟斤拷锟斤拷R锟斤拷锟斤拷锟斤拷 锟斤拷 实锟斤拷
                                float realX = rotationCenterX \+ \(r00 \* dx \+ r01 \* dy \+ r02 \* dz\);
                                float realY = rotationCenterY \+ \(r10 \* dx \+ r11 \* dy \+ r12 \* dz\);
                                float realZ = rotationCenterZ \+ \(r20 \* dx \+ r21 \* dy \+ r22 \* dz\);

                                // 锟斤拷实锟斤拷锟斤拷锟斤拷锟捷诧拷锟斤拷
                                uint16_t value = SampleVolume\(volumeData, width, height, depth, realX, realY, realZ\);

                                // 锟斤拷转Z锟斤拷锟斤拷MPR一锟斤拷
                                size_t outIdx = static_cast<size_t>\(outHeight - 1 - outZ\) \* outWidth \+ outY;
                                sliceData\[outIdx\] = value;'''
    
    sagittal_new = '''                            for (int outY = 0; outY < outWidth; ++outY) {
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
                                sliceData[outIdx] = finalValue;'''
    
    # 执行替换
    content_new = re.sub(coronal_old, coronal_new, content, flags=re.MULTILINE)
    if content_new == content:
        print("警告: Coronal 代码未找到或未替换")
    else:
        print("✓ Coronal 代码已修复")
        content = content_new
    
    content_new = re.sub(sagittal_old, sagittal_new, content, flags=re.MULTILINE)
    if content_new == content:
        print("警告: Sagittal 代码未找到或未替换")
    else:
        print("✓ Sagittal 代码已修复")
        content = content_new
    
    # 写回文件
    with open(filepath, 'w', encoding='utf-8-sig') as f:
        f.write(content)
    
    print(f"\n文件已更新: {filepath}")
    print("修复完成！")

if __name__ == '__main__':
    fix_mip_rotation()

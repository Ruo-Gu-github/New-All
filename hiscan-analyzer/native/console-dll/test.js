// test.js - 测试 native addon
const path = require('path');

console.log('=== ConsoleDllTest Native Addon 测试 ===\n');

try {
  // 加载 addon
  const { DicomVolume, MaskManager, ROIManager } = require('./index.js');
  
  console.log('✓ Addon 加载成功');
  console.log('  - DicomVolume:', typeof DicomVolume);
  console.log('  - MaskManager:', typeof MaskManager);
  console.log('  - ROIManager:', typeof ROIManager);
  
  // 测试 DicomVolume 创建
  console.log('\n--- 测试 DicomVolume ---');
  const volume = new DicomVolume();
  console.log('✓ DicomVolume 对象创建成功');
  
  // 测试加载 DICOM（需要提供实际的 DICOM 数据路径）
  const testDataPath = 'D:\\DICOM_TEST_DATA'; // 修改为实际路径
  
  if (require('fs').existsSync(testDataPath)) {
    console.log(`\n尝试加载: ${testDataPath}`);
    
    try {
      volume.loadFromFolder(testDataPath);
      console.log('✓ DICOM 序列加载成功');
      
      // 获取尺寸
      const dims = volume.getDimensions();
      console.log(`  尺寸: ${dims.width} x ${dims.height} x ${dims.depth}`);
      
      // 获取间距
      const spacing = volume.getSpacing();
      console.log(`  间距: ${spacing.x.toFixed(3)} x ${spacing.y.toFixed(3)} x ${spacing.z.toFixed(3)} mm`);
      
      // 生成缩略图
      const thumbnail = volume.generateThumbnail(256);
      console.log(`  缩略图: ${thumbnail.length} bytes (${256}x${256} RGBA)`);
      
      // 保存缩略图为 PNG（需要额外的库）
      // const fs = require('fs');
      // const { PNG } = require('pngjs');
      // const png = new PNG({ width: 256, height: 256 });
      // png.data = thumbnail;
      // png.pack().pipe(fs.createWriteStream('thumbnail.png'));
      
    } catch (error) {
      console.error('✗ 加载失败:', error.message);
    }
  } else {
    console.log(`\n⚠ 测试数据路径不存在: ${testDataPath}`);
    console.log('  请修改 test.js 中的 testDataPath 为实际的 DICOM 数据路径');
  }
  
  console.log('\n=== 测试完成 ===');
  
} catch (error) {
  console.error('✗ 错误:', error.message);
  console.error('\n可能的原因:');
  console.error('  1. Addon 未编译，请运行: npm run build');
  console.error('  2. DLL 文件缺失，请确保 ConsoleDllTest DLL 已编译');
  console.error('  3. 路径配置错误，请检查 binding.gyp 中的路径');
  process.exit(1);
}

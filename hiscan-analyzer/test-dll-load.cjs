/**
 * 测试脚本：验证 DLL 加载和基本调用
 * 运行: node test-dll-load.cjs
 */

const path = require('path');
const ffi = require('ffi-napi');
const ref = require('ref-napi');

// DLL 路径
const dllPath = path.resolve(__dirname, '../ConsoleDllTest/Dlls/debug/bin');
console.log('DLL 路径:', dllPath);

try {
  // 定义类型
  const voidPtr = ref.refType(ref.types.void);
  const intPtr = ref.refType(ref.types.int);
  const floatPtr = ref.refType(ref.types.float);
  const charPtr = ref.types.CString;

  // 加载 DllDicom
  const dicomDllPath = path.join(dllPath, 'DllDicom.dll');
  console.log('\n正在加载:', dicomDllPath);

  const DllDicom = ffi.Library(dicomDllPath, {
    'Dicom_GetLastError': [charPtr, []],
    'Dicom_Volume_Create': [voidPtr, []],
    'Dicom_Volume_Destroy': ['void', [voidPtr]],
    'Dicom_Volume_LoadFromDicomSeries': ['int', [voidPtr, charPtr]],
    'Dicom_Volume_GetDimensions': ['int', [voidPtr, intPtr, intPtr, intPtr]],
    'Dicom_Volume_GetSpacing': ['int', [voidPtr, floatPtr, floatPtr, floatPtr]],
    'Dicom_Volume_GetData': [voidPtr, [voidPtr]],
  });

  console.log('✅ DLL 加载成功！\n');

  // 测试创建 Volume
  console.log('测试 1: 创建 Volume Handle');
  const volumeHandle = DllDicom.Dicom_Volume_Create();
  console.log('Volume Handle:', volumeHandle);

  if (!volumeHandle.isNull()) {
    console.log('✅ Volume 创建成功\n');

    // 测试加载 DICOM 序列（使用测试数据路径）
    const testDicomPath = 'D:\\DICOM_TEST_DATA'; // 修改为你的测试数据路径
    console.log(`测试 2: 加载 DICOM 序列: ${testDicomPath}`);
    
    const result = DllDicom.Dicom_Volume_LoadFromDicomSeries(volumeHandle, testDicomPath);
    
    if (result === 0) {
      console.log('✅ DICOM 加载成功\n');

      // 获取尺寸
      const width = ref.alloc('int');
      const height = ref.alloc('int');
      const depth = ref.alloc('int');

      DllDicom.Dicom_Volume_GetDimensions(volumeHandle, width, height, depth);
      
      console.log('图像尺寸:');
      console.log('  宽度:', width.deref());
      console.log('  高度:', height.deref());
      console.log('  深度:', depth.deref());

      // 获取间距
      const spacingX = ref.alloc('float');
      const spacingY = ref.alloc('float');
      const spacingZ = ref.alloc('float');

      DllDicom.Dicom_Volume_GetSpacing(volumeHandle, spacingX, spacingY, spacingZ);
      
      console.log('\n像素间距:');
      console.log('  X:', spacingX.deref(), 'mm');
      console.log('  Y:', spacingY.deref(), 'mm');
      console.log('  Z:', spacingZ.deref(), 'mm');

      // 获取数据指针
      const dataPtr = DllDicom.Dicom_Volume_GetData(volumeHandle);
      console.log('\n数据指针:', dataPtr.isNull() ? 'NULL' : 'Valid');

    } else {
      const error = DllDicom.Dicom_GetLastError();
      console.log('❌ DICOM 加载失败:', error || `错误代码: ${result}`);
      console.log('   提示: 请确保测试数据路径正确');
    }

    // 清理
    console.log('\n清理资源...');
    DllDicom.Dicom_Volume_Destroy(volumeHandle);
    console.log('✅ 资源已清理');

  } else {
    console.log('❌ Volume 创建失败');
  }

} catch (error) {
  console.error('\n❌ 错误:', error.message);
  console.error('详细信息:', error);
  
  if (error.message.includes('找不到指定的模块')) {
    console.error('\n可能的原因:');
    console.error('1. DLL 路径不正确');
    console.error('2. 缺少依赖的 DLL (GDCM, OpenCV 等)');
    console.error('3. 需要将依赖 DLL 放到同一目录或系统 PATH 中');
  }
}

console.log('\n测试完成！');

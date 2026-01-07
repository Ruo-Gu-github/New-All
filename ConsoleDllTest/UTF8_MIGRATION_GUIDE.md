# UTF-8迁移指南

## 快速开始

### 方法一：一键迁移（推荐）

直接运行主脚本，自动完成所有操作：

```powershell
.\migrate_to_utf8.ps1
```

这个脚本会：
1. ✓ 转换所有C/C++源文件为UTF-8 with BOM
2. ✓ 更新所有项目文件，添加/utf-8编译选项
3. ✓ 显示详细的处理进度和结果

### 方法二：分步执行

如果你想分步控制，可以依次运行：

```powershell
# 1. 转换源文件编码
.\convert_to_utf8_with_bom.ps1

# 2. 更新项目配置
.\update_vcxproj_utf8_precise.ps1
```

---

## 为什么需要UTF-8迁移？

### 当前问题
- ❌ 源文件使用GBK/ANSI编码，包含中文注释
- ❌ 不同开发者环境下编码不一致
- ❌ 需要手动进行编码转换（EncodingUtils）
- ❌ Git提交时编码冲突

### 迁移后优势
- ✅ 统一使用UTF-8编码，国际标准
- ✅ 中文注释正常显示，不再乱码
- ✅ 跨平台兼容性好
- ✅ Git管理更方便
- ✅ 编译器原生支持中文

---

## 技术细节

### UTF-8 with BOM vs UTF-8 without BOM

**本项目使用UTF-8 with BOM**，原因：
- Visual Studio 2022推荐使用BOM
- MSVC编译器更好地识别UTF-8 with BOM
- 避免编码检测错误

### 编译器选项

脚本会在所有项目的`<ClCompile>`节点中添加：

```xml
<AdditionalOptions>/utf-8 %(AdditionalOptions)</AdditionalOptions>
```

这个选项告诉MSVC编译器：
- 源文件和执行字符集都使用UTF-8
- 正确处理中文字符串字面量
- 等同于：`/source-charset:utf-8 /execution-charset:utf-8`

---

## 影响的文件

### 源文件（自动转换）
```
*.cpp  - 所有C++源文件
*.h    - 所有头文件
*.hpp  - C++头文件
*.c    - C源文件
```

### 项目文件（自动更新）
```
*.vcxproj  - 所有Visual Studio项目文件
```

### 排除目录
以下目录会被自动跳过：
- `x64/`, `Debug/`, `Release/` - 编译输出
- `.vs/` - Visual Studio缓存
- `packages/`, `vcpkg_installed/` - 依赖包
- `Dlls/bin/`, `Dlls/debug/`, `Dlls/lib/` - DLL输出

---

## 验证步骤

### 1. 检查文件编码

在PowerShell中验证文件是否为UTF-8 with BOM：

```powershell
$file = Get-Content "DllCore\CoreApi.cpp" -Raw -Encoding Byte
if ($file[0] -eq 0xEF -and $file[1] -eq 0xBB -and $file[2] -eq 0xBF) {
    Write-Host "✓ UTF-8 with BOM" -ForegroundColor Green
} else {
    Write-Host "✗ 不是UTF-8 with BOM" -ForegroundColor Red
}
```

### 2. 检查项目配置

打开任意`.vcxproj`文件，查找：

```xml
<ItemDefinitionGroup>
  <ClCompile>
    <AdditionalOptions>/utf-8 %(AdditionalOptions)</AdditionalOptions>
```

### 3. 编译测试

```batch
.\build_all_dlls.bat
```

如果编译成功且无编码相关警告，说明迁移成功。

---

## 常见问题

### Q1: 运行脚本时提示"无法运行脚本"

**原因**: PowerShell执行策略限制

**解决**:
```powershell
Set-ExecutionPolicy -Scope CurrentUser -ExecutionPolicy RemoteSigned
```

### Q2: 转换后VS中文注释显示乱码

**原因**: VS没有重新加载文件

**解决**:
1. 关闭Visual Studio
2. 重新打开解决方案
3. 或右键文件 → "高级保存选项" → 选择"Unicode (UTF-8 with BOM)"

### Q3: 编译时出现"非法字符"错误

**原因**: 某些文件可能没有正确转换

**解决**:
1. 找到报错的文件
2. 在VS中打开
3. 文件 → 高级保存选项 → UTF-8 with BOM
4. 保存并重新编译

### Q4: Git显示所有文件都被修改

**原因**: 编码变化会被Git检测为修改

**解决**: 这是正常的，一次性提交即可：
```bash
git add .
git commit -m "Migrate all files to UTF-8 with BOM encoding"
```

### Q5: 需要恢复到原来的编码吗？

**答**: 不需要！UTF-8是现代开发的标准，所有主流编译器和工具都完美支持。

---

## 手动在VS2022中操作（备选方案）

如果你不想使用脚本，可以手动操作：

### 转换单个文件
1. 在VS2022中打开文件
2. 文件 → 高级保存选项
3. 选择"Unicode (UTF-8 with signature) - Codepage 65001"
4. 保存

### 设置项目UTF-8编译
1. 右键项目 → 属性
2. C/C++ → 命令行
3. 添加选项：`/utf-8`
4. 应用到所有配置（Debug/Release）

### 批量转换（VS2022扩展）
推荐安装扩展：
- **Force UTF-8 (with BOM)** 
- **Encoding Tools**

---

## 与现有代码的兼容性

### EncodingUtils.h/cpp
迁移后，`EncodingUtils`仍然有用：
- 处理外部GBK文件（如DICOM文件路径）
- 与GDCM库交互（GDCM使用GBK）
- 系统API调用（Windows API很多用ANSI）

示例：
```cpp
// 仍然需要用于GDCM路径转换
std::string utf8Path = EncodingUtils::GetUtf8Path(gbkPath);

// 源代码中的中文字符串现在可以直接使用
std::string message = "加载完成"; // 直接UTF-8，无需转换
```

### 现有代码无需修改
- 所有现有的C++代码继续工作
- 字符串字面量自动使用UTF-8
- 只需重新编译即可

---

## 迁移检查清单

迁移完成后，请检查：

- [ ] 所有`.cpp`和`.h`文件已转换为UTF-8 with BOM
- [ ] 所有`.vcxproj`文件包含`/utf-8`编译选项
- [ ] Visual Studio能正确显示中文注释
- [ ] 所有DLL项目成功编译
- [ ] 运行时测试通过（加载DICOM、渲染等）
- [ ] Git提交编码迁移的变更

---

## 性能影响

**无性能影响**：
- UTF-8编码不影响运行时性能
- 编译时间基本无变化
- 生成的二进制文件大小相同

---

## 技术支持

如果遇到问题：

1. **查看脚本输出**：脚本会显示详细的处理过程和错误信息
2. **检查VS输出窗口**：编译时的详细错误信息
3. **逐个项目测试**：隔离有问题的项目单独处理

---

## 总结

使用`migrate_to_utf8.ps1`脚本，**5分钟内**完成整个项目的UTF-8迁移，享受现代编码标准带来的便利！

**迁移后的好处**：
- ✅ 统一的编码标准
- ✅ 更好的跨平台兼容性
- ✅ 更简单的Git管理
- ✅ 国际化支持更完善
- ✅ 开发体验更流畅

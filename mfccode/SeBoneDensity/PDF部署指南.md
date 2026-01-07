# PDF报告生成 - 快速部署指南

## 方案说明

我已经为您实现了一个**不需要编译任何库**的简化方案！

### 工作原理
1. 程序先生成漂亮的HTML报告（包含3D截图和参数表格）
2. 使用 `wkhtmltopdf.exe` 将HTML转为PDF
3. 如果没有 wkhtmltopdf，用户可以直接用浏览器打开HTML并打印为PDF

---

## 快速部署步骤

### 步骤1: 下载 wkhtmltopdf（免费、开源、预编译）

**下载地址：** https://wkhtmltopdf.org/downloads.html

**推荐版本：** 
- Windows 64位：`wkhtmltox-0.12.6-1.msvc2015-win64.exe`
- Windows 32位：`wkhtmltox-0.12.6-1.msvc2015-win32.exe`

**文件大小：** 约 30MB

### 步骤2: 安装或提取 wkhtmltopdf.exe

**方式A（推荐）：** 安装到系统
- 运行下载的安装程序
- 默认安装到 `C:\Program Files\wkhtmltopdf`
- wkhtmltopdf.exe 会自动加入系统PATH

**方式B：** 放到程序目录
- 使用 7-Zip 或 WinRAR 打开安装包
- 提取 `bin\wkhtmltopdf.exe` 文件
- 将 `wkhtmltopdf.exe` 复制到您的程序目录（与 .exe 同目录）

### 步骤3: 编译并运行程序

就这样！不需要任何编译配置，直接编译运行即可。

---

## 使用效果

### 有 wkhtmltopdf 的情况：
1. 点击"导出PDF"按钮
2. 程序自动生成 HTML → 转换为 PDF
3. 提示成功，询问是否打开PDF
4. 用户可直接查看专业的PDF报告

### 没有 wkhtmltopdf 的情况：
1. 点击"导出PDF"按钮
2. 程序生成 HTML 报告
3. 提示"未找到PDF转换工具"
4. 询问是否打开HTML
5. 用户可在浏览器中打印为PDF（Ctrl+P → 另存为PDF）

---

## 生成的报告示例

报告包含：
- ✅ 标题：骨参数分析报告
- ✅ 3D截图（自动从ScreenCapture文件夹获取最新图片）
- ✅ 参数表格（所有计算的骨参数）
- ✅ 生成时间戳

报告样式：
- 现代化的HTML5设计
- 专业的表格样式（带斑马纹）
- 自适应页面布局
- 高质量的PDF输出

---

## 优势对比

### 方案对比表

| 方案 | 需要编译 | 文件大小 | 部署难度 | 输出质量 |
|------|---------|---------|---------|---------|
| libharu | ✗ 需要 | 小 | 困难 | 一般 |
| PDFlib | ✗ 需要 | 中 | 困难 | 好 |
| wkhtmltopdf | ✓ 不需要 | 30MB | **极易** | **优秀** |
| GDI+ | ✓ 不需要 | 0 | 中等 | 一般 |

**当前方案：** wkhtmltopdf + HTML（最佳选择）

---

## wkhtmltopdf 下载链接（国内镜像）

如果官网下载慢，可以使用以下镜像：

**GitHub Release:**
https://github.com/wkhtmltopdf/packaging/releases/tag/0.12.6-1

**直接下载链接（Windows 64位）:**
```
https://github.com/wkhtmltopdf/packaging/releases/download/0.12.6-1/wkhtmltox-0.12.6-1.msvc2015-win64.exe
```

**直接下载链接（Windows 32位）:**
```
https://github.com/wkhtmltopdf/packaging/releases/download/0.12.6-1/wkhtmltox-0.12.6-1.msvc2015-win32.exe
```

---

## 常见问题

### Q1: 必须安装 wkhtmltopdf 吗？
**A:** 不是必须的。如果没有，程序会生成HTML报告，用户可以用浏览器打印为PDF。

### Q2: wkhtmltopdf 是免费的吗？
**A:** 完全免费且开源（LGPLv3许可证），可商用。

### Q3: 如何检查 wkhtmltopdf 是否安装成功？
**A:** 打开命令提示符，输入 `wkhtmltopdf --version`，如果显示版本号就成功了。

### Q4: 能否把 wkhtmltopdf 打包进安装程序？
**A:** 可以。将 wkhtmltopdf.exe 包含在安装包中，安装时复制到程序目录即可。

### Q5: PDF文件大小多大？
**A:** 取决于截图大小，通常 500KB - 2MB。

### Q6: 支持中文吗？
**A:** 完全支持中文，使用 UTF-8 编码 + 微软雅黑字体。

---

## 技术细节

### HTML 模板特点
- 响应式设计，适配 A4 纸张
- CSS3 样式，现代化外观
- 表格自动斑马纹
- 图片自动缩放
- UTF-8 编码支持中文

### wkhtmltopdf 命令参数
```
wkhtmltopdf --page-size A4 --encoding UTF-8 --enable-local-file-access input.html output.pdf
```

参数说明：
- `--page-size A4`: 使用A4纸张大小
- `--encoding UTF-8`: 使用UTF-8编码（支持中文）
- `--enable-local-file-access`: 允许访问本地图片文件

---

## 总结

✅ **无需编译** - wkhtmltopdf 是预编译的独立程序  
✅ **部署简单** - 只需下载一个 exe 文件  
✅ **输出专业** - 生成标准的PDF文件  
✅ **容错性强** - 没有工具也能生成HTML  
✅ **完全免费** - 开源软件，可商用  

这是目前最简单、最可靠的方案！

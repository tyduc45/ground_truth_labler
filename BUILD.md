# GT_labler 编译与运行指南

## 环境要求

| 工具 | 版本要求 | 说明 |
|------|----------|------|
| Visual Studio 2022 | 17.x | 需勾选"使用 C++ 的桌面开发"工作负载 |
| CMake | ≥ 3.16 | 推荐 3.31（本项目验证版本） |
| vcpkg | 任意新版 | 依赖管理器，路径示例：`E:/vcpkg/vcpkg` |

---

## 一、安装依赖（vcpkg）

所有外部依赖均通过 vcpkg 管理，首次使用需安装以下包（三元组 `x64-windows`）：

```bash
vcpkg install qtbase:x64-windows
vcpkg install opencv4[contrib]:x64-windows
```

> 已安装版本：Qt 6.10.0、OpenCV 4.12.0（含 contrib/tracking 模块）。
> 若 vcpkg 已集成到 Visual Studio（`vcpkg integrate install`），上述包会自动对 VS 可见，但 CMake 配置仍需显式传入 toolchain 文件。

---

## 二、CMake 配置

在项目根目录执行（将 `<vcpkg_root>` 替换为实际路径，如 `E:/vcpkg/vcpkg`）：

```bash
cmake -DCMAKE_TOOLCHAIN_FILE=<vcpkg_root>/scripts/buildsystems/vcpkg.cmake -B build
```

**首次配置**或**依赖发生变化后**，加 `--fresh` 清除旧缓存，避免 `NOTFOUND` 被错误复用：

```bash
cmake -DCMAKE_TOOLCHAIN_FILE=<vcpkg_root>/scripts/buildsystems/vcpkg.cmake -B build --fresh
```

配置成功的输出末尾应包含：

```
-- Found OpenCV: ... (found version "4.12.0") found components: core imgproc videoio tracking
-- Configuring done
-- Generating done
-- Build files have been written to: .../build
```

---

## 三、编译

### 方式 A：命令行（推荐，与 IDE 无关）

```bash
# Debug 版本（默认）
cmake --build build

# Release 版本
cmake --build build --config Release

# 并行加速（使用所有逻辑核心）
cmake --build build --config Release -- /maxcpucount
```

### 方式 B：Visual Studio IDE

1. 用 Visual Studio 2022 打开 `build/GT_labler.sln`
2. 工具栏选择配置（`Debug` 或 `Release`）
3. 菜单 **生成 → 生成解决方案**（或 `Ctrl+Shift+B`）

可执行文件输出位置：

```
build/src/Debug/GT_labler.exe
build/src/Release/GT_labler.exe
```

---

## 四、运行

### Qt 平台插件（自动处理）

`src/CMakeLists.txt` 已通过 `POST_BUILD` 命令在每次编译后自动将 Qt 平台插件复制到可执行文件同级的 `platforms/` 目录，无需手动操作：

```
build/src/Debug/platforms/qwindowsd.dll    ← Debug 编译后自动生成
build/src/Release/platforms/qwindows.dll   ← Release 编译后自动生成
```

### 配置运行时 DLL 路径

程序还依赖 vcpkg 安装的 Qt 和 OpenCV 主体动态库，运行前需确保系统能找到这些 DLL。

**临时方案（当前终端会话有效）：**

```bash
set PATH=<vcpkg_root>\installed\x64-windows\bin;%PATH%
```

**永久方案：** 将以下路径追加到系统/用户环境变量 `PATH`：

```
<vcpkg_root>\installed\x64-windows\bin
```

> 涉及的关键 DLL：`Qt6Core.dll`、`Qt6Gui.dll`、`Qt6Widgets.dll`、`opencv_core4.dll`、`opencv_imgproc4.dll`、`opencv_videoio4.dll`、`opencv_tracking4.dll`

### 启动程序

```bash
build\src\Debug\GT_labler.exe
# 或
build\src\Release\GT_labler.exe
```

---

## 五、常见问题

### CMake 报 `Qt6_DIR-NOTFOUND`

旧缓存残留导致，使用 `--fresh` 重新配置：

```bash
cmake -DCMAKE_TOOLCHAIN_FILE=<vcpkg_root>/scripts/buildsystems/vcpkg.cmake -B build --fresh
```

### 程序启动报 `Could not find the Qt platform plugin "windows"`

编译后 `platforms/` 目录应由 `POST_BUILD` 自动生成。若缺失，重新编译一次即可：

```bash
cmake --build build --config Debug
```

### 程序启动报"找不到 DLL"

检查 `PATH` 是否包含 `<vcpkg_root>\installed\x64-windows\bin`，或将该目录下所需 DLL 复制到 `.exe` 同级目录。

### OpenCV tracking 模块缺失

确认 vcpkg 安装时包含 `contrib` 特性：

```bash
vcpkg install opencv4[contrib]:x64-windows
```

### 更换机器/重新克隆后配置失败

`build/` 目录不应提交到版本控制，克隆后须从第二节重新配置。

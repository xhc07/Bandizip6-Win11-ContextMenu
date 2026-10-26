# BandiZip11

![Platform](https://img.shields.io/badge/platform-Windows%2011-0078D4?style=flat-square)
![Bandizip](https://img.shields.io/badge/Bandizip-6.x-0066CC?style=flat-square)
![License](https://img.shields.io/badge/license-MIT-green?style=flat-square)

**把 Bandizip 6.x 的右键菜单带进 Windows 11 新版右键菜单。**

BandiZip11 是一个开源的小工具。它不修改 Bandizip 的任何二进制文件，而是通过一个带包身份的 MSIX 包注册原生 `IExplorerCommand`，在 Windows 11 的一级右键菜单里增加一个 **Bandizip** 子菜单，再调用 Bandizip 自带的命令行完成解压、压缩、测试等操作。

> 适用环境：Windows 11 21H2（Build 22000）及以上 + Bandizip 6.x（已在 **6.29.0.1** 上验证）。
>
> Windows 11 的“显示更多选项”（Shift+F10）里仍然保留 Bandizip 原来的旧菜单，两者互不冲突。

---

## 为什么需要它

Windows 11 的新版右键菜单不会加载传统的 `IContextMenu` Shell 扩展。Bandizip 6.x 注册的是旧式 `AABdzCtx`（`bdzshl64.dll`），所以只能出现在“显示更多选项”里。

Windows 11 只接受通过 **带包身份（package identity）的 `windows.fileExplorerContextMenus` 清单扩展** 注册的 `IExplorerCommand`。BandiZip11 就是按这个机制重新实现了一套菜单，而不是去改 Bandizip 的 DLL。

---

## 菜单预览

在文件、文件夹或文件夹空白处右键，会出现 **Bandizip** 子菜单：

```text
Bandizip
├─ 解压到当前文件夹
├─ 自动解压
├─ 解压到 "sample\"
├─ 选择解压路径...
├─ 用 Bandizip 打开...
├─ 测试压缩文件            ← 默认关闭
├─ 添加到 "sample.zip"
├─ 添加到单独的压缩文件     ← 多选时显示
└─ 添加到压缩文件 (Bandizip)...
```

实际显示的项目会根据当前选择和 Bandizip 设置变化。

---

## 功能特性

| 菜单项 | 说明 | 实际调用 |
| --- | --- | --- |
| 解压到当前文件夹 | 把压缩包直接解压到所在目录 | `x -o:<目录> -y <压缩包>` |
| 自动解压 | Bandizip 的“智能解压” | `x -target:auto -o:<目录> -y <压缩包>` |
| 解压到 `<压缩包名>\` | 解压到以压缩包名命名的子文件夹 | `x -target:name -o:<目录> -y <压缩包>` |
| 选择解压路径... | 弹出 Bandizip 的目标文件夹选择框 | `x -target:dlg -o:<目录> <压缩包>` |
| 用 Bandizip 打开... | 用 Bandizip 打开压缩包 | `<压缩包>` |
| 测试压缩文件 | 测试压缩包完整性 | `t -y <压缩包>` |
| 添加到 `<名称>.zip` | 把选中的文件/文件夹压缩成一个 ZIP | `c -fmt:zip -o:<目录> -y <唯一文件名>.zip <选中项>` |
| 添加到单独的压缩文件 | 每个选中项分别压缩成自己的 ZIP | `bc -fmt:zip -o:<目录> -y <选中项>` |
| 添加到压缩文件 (Bandizip)... | 打开 Bandizip 的“新建压缩文件”对话框，预填文件名和选中项 | `cd -o:<目录> <唯一文件名>.zip <选中项>` |

> 表中的 `-y` 仅在 `AutoClose=1`（默认）时添加。

其他特性：

- **原生外观**：真正的 `IExplorerCommand`，出现在 Windows 11 一级右键菜单。
- **多语言**：直接读取 Bandizip 的 `langs\*.ini`，跟随 Bandizip 语言设置或系统语言。
- **跟随 Bandizip 设置**：读取 `HKCU\Software\Bandizip` 里的 `shell_*` 开关。
- **不修改 Bandizip**：只调用 `Bandizip.exe` 的公开命令行参数。
- **原生 x64 DLL**：C++ 编写，不依赖 .NET 运行时。
- **无网络、无遥测**。

---

## 系统要求

| 项目 | 要求 |
| --- | --- |
| 操作系统 | Windows 11 Build **22000** 或更高，x64 |
| Bandizip | **6.x**，安装目录中必须有 `bc.exe`（6.29 自带） |
| PowerShell | Windows PowerShell 5.1 或更高 |
| 权限 | 安装/卸载通常不需要管理员；开启开发者模式需要 UAC |
| 开发者模式 | 安装未签名本地 MSIX 时需要 |

> Bandizip 7.x 的命令行工具和菜单行为与 6.x 可能不同，本项目未针对 7.x 适配。

---

## 快速开始

### 一键安装 / 卸载（推荐）

1. 下载并解压本仓库，或克隆到本地。
2. 双击 **`BandiZip11-Setup.bat`**。
3. 在菜单中选择：

```text
[1] Install Bandizip 11 context menu
[2] Uninstall Bandizip 11 context menu
[0] Exit
```

4. 安装完成后，脚本会询问是否重启资源管理器：

```text
Restart File Explorer now?
  [1] Yes
  [2] No
```

选择 **1** 立即生效，选择 **2** 稍后手动重启或注销。

也可以在命令行中直接指定动作：

```bat
.\BandiZip11-Setup.bat install
.\BandiZip11-Setup.bat uninstall
.\BandiZip11-Setup.bat install --no-restart
```

### 命令行安装（可选）

如果不想用 bat，也可以直接运行 PowerShell 脚本：

```powershell
powershell -ExecutionPolicy Bypass -File .\install.ps1
```

开发者模式未开启时：

```powershell
powershell -ExecutionPolicy Bypass -File .\install.ps1 -EnableDeveloperMode
```

卸载：

```powershell
powershell -ExecutionPolicy Bypass -File .\uninstall.ps1
```

---

## 权限说明

| 操作 | 是否需要管理员 | 说明 |
| --- | --- | --- |
| 安装（开发者模式已开启） | **否** | 只写 `HKCU` 和 `%LOCALAPPDATA%`，注册当前用户包 |
| 安装（开发者模式未开启） | **仅开启开发者模式时** | bat 只提升 `reg.exe` 写 HKLM，包仍按当前用户注册 |
| 卸载（当前用户包） | **通常否** | `Remove-AppxPackage` 删除当前用户包 |
| 卸载失败后重试 | 可选 | bat 会询问是否用管理员权限重试 |
| 重启资源管理器 | 否 | 普通权限即可；管理员窗口中会跳过自动重启 |

为什么安装不整体提权：

- 包是 **per-user** 的，安装到 `%LOCALAPPDATA%`；
- 整体以管理员运行时，容易把包注册到错误的安全上下文，甚至启动一个管理员权限的 Explorer；
- 所以只把“开启开发者模式”这一步单独提权，其余保持普通用户权限。

---

## 安装 / 卸载行为

### 再次运行安装脚本会怎样？

**不会装出两个，也不会出现两份菜单。**

`install.ps1` 每次都会先移除同身份的旧包，再复制文件、重新注册：

- 包身份固定为 `BandiZip11.ContextMenu`，同一用户只会有一个；
- `%LOCALAPPDATA%\BandiZip11\Package` 会重新生成；
- `%LOCALAPPDATA%\BandiZip11\config.ini` 在 `Package` 目录外，**不会被删除**；
- Bandizip 自己的设置不会被修改。

如果旧包正在被资源管理器占用，卸载旧包可能失败，安装会中止；此时旧包仍然存在，不会产生第二份。重启资源管理器后重新运行即可。

### 卸载会删除什么？

- 当前用户的 `BandiZip11.ContextMenu` MSIX 包；
- `%LOCALAPPDATA%\BandiZip11` 目录（包括 `config.ini`）；
- 可能残留的 HKCU COM 注册。

不会修改 Bandizip 的安装目录和 Bandizip 设置。

### 卸载失败怎么办？

如果提示包正在使用：

1. 关闭所有资源管理器窗口；
2. 重启资源管理器或注销；
3. 重新运行卸载；
4. 或在 bat 提示时选择“以管理员权限重试”。

---

## 使用说明

- 在 **文件**、**文件夹**、**文件夹内部空白处** 右键，才会出现 Bandizip 子菜单；
- 桌面壁纸空白处没有注册（Windows 11 的 `DesktopBackground` 与 `Directory\Background` 是不同位置）；
- 这是 Windows 11 的**一级新菜单**，不是“显示更多选项”里的旧菜单；
- “测试压缩文件”默认可能不显示，因为 Bandizip 6.29 的 `shell_TestArchive` 默认是 `0`；
- 旧菜单仍然保留在“显示更多选项”中，这是正常现象。

---

## 配置

### 自定义菜单开关

在 `%LOCALAPPDATA%\BandiZip11\config.ini` 新建（安装后目录已存在）以下内容：

```ini
[General]
ShowExtractHere=1
ShowExtractSmart=1
ShowExtractToFolder=1
ShowExtractDialog=1
ShowOpen=1
ShowTest=0
ShowCompressZip=1
ShowCompressEach=1
ShowCompressDialog=1
AutoClose=1
```

| 键 | 默认 | 说明 |
| --- | --- | --- |
| `ShowExtractHere` | 1 | 解压到当前文件夹 |
| `ShowExtractSmart` | 1 | 自动解压 |
| `ShowExtractToFolder` | 1 | 解压到 `<压缩包名>\` |
| `ShowExtractDialog` | 1 | 选择解压路径... |
| `ShowOpen` | 1 | 用 Bandizip 打开 |
| `ShowTest` | 0 | 测试压缩文件（Bandizip 6.29 默认关闭） |
| `ShowCompressZip` | 1 | 添加到 `<名称>.zip` |
| `ShowCompressEach` | 1 | 添加到单独的压缩文件（多选） |
| `ShowCompressDialog` | 1 | 添加到压缩文件 (Bandizip)... |
| `AutoClose` | 1 | 给 Bandizip 传 `-y`：自动确认提示并在完成后关闭进度窗口 |

> **关于 `AutoClose`**
>
> `-y` 是 Bandizip 的“Assume Yes on all queries”。在解压遇到同名文件时，它可能自动确认覆盖。
>
> 如果希望保留 Bandizip 的覆盖确认，请设置 `AutoClose=0`；此时进度窗口可能停在 100%，需要手动关闭。

修改 `config.ini` 后需要重启资源管理器（DLL 会缓存设置）。

### 跟随 Bandizip 设置

以下 `HKCU\Software\Bandizip` 值会被读取（DWORD，`0` 隐藏，非 `0` 显示）：

| Bandizip 值 | 对应菜单 |
| --- | --- |
| `shell_ExtractHere` | 解压到当前文件夹 |
| `shell_ExtractAuto` | 自动解压 |
| `shell_ExtractDlg` | 选择解压路径... |
| `shell_OpenArchive` | 用 Bandizip 打开... |
| `shell_TestArchive` | 测试压缩文件 |
| `shell_ArchivingAsFileName` | 添加到 `<名称>.zip` |
| `shell_ArchivingEach` | 添加到单独的压缩文件 |
| `shell_ArchivingPopup` | 添加到压缩文件 (Bandizip)... |

`config.ini` 的优先级高于 Bandizip 注册表设置。没有对应注册表值时使用上表默认值。

---

## 工作原理

```text
File Explorer
  └─ Windows 11 新版右键菜单
       └─ MSIX 包：windows.fileExplorerContextMenus
            └─ COM Surrogate（dllhost.exe）
                 └─ BandiZip11Menu.dll
                      ├─ IExplorerCommand
                      ├─ IEnumExplorerCommand
                      └─ 读取选择 / 设置 / 语言
                           └─ Bandizip.exe 解压 / 压缩 / 测试
```

具体流程：

1. MSIX 清单声明 `windows.comServer`（`com:SurrogateServer`）和 `windows.fileExplorerContextMenus`；
2. `BandiZip11Menu.dll` 实现 `IExplorerCommand` 和 `IEnumExplorerCommand`，由 COM Surrogate 加载，而不是注入 `explorer.exe`；
3. `EnumSubCommands` 返回 9 个候选子命令，每个子命令通过 `GetState` 根据当前选择和 Bandizip 设置决定显示/隐藏；
4. 用户点击时，`Invoke` 读取 `IShellItemArray` 中的文件路径，拼接 Bandizip 命令行，用 `CreateProcessW` 启动 `Bandizip.exe`。

这种结构对 Explorer 更友好：插件崩溃只会影响 Surrogate 进程，不会拖垮整个资源管理器。

---

## 项目结构

```text
BandiZip11/
├─ BandiZip11-Setup.bat      # 安装/卸载入口（1 安装，2 卸载）
├─ install.ps1               # 安装逻辑
├─ uninstall.ps1             # 卸载逻辑
├─ build.ps1                 # 使用 Zig 构建
├─ dist/                     # 预编译包 + 清单 + 图标（安装脚本使用）
├─ package/
│  ├─ AppxManifest.xml       # MSIX 清单模板
│  └─ Assets/                # 应用图标
├─ src/
│  ├─ common.h / .cpp        # 字符串、路径、进程启动
│  ├─ bandizip.h / .cpp      # Bandizip 检测、设置、本地化、选择分析
│  ├─ command.h / .cpp       # IExplorerCommand / IEnumExplorerCommand / 类工厂
│  └─ host.c                 # 包的应用入口（仅显示说明，不参与菜单）
├─ LICENSE
└─ README.md
```

---

## 构建

`dist/` 已经包含可用的二进制。只有修改源码后才需要重新构建。

### 使用 Zig（推荐）

需要 **Zig 0.13 或更高版本**。`build.ps1` 会按以下顺序查找：

1. `PATH` 中的 `zig`；
2. 仓库上一级的 `tools\zig\ziglang\zig.exe`；
3. 仓库内的 `tools\zig\ziglang\zig.exe`。

```powershell
powershell -ExecutionPolicy Bypass -File .\build.ps1 -Clean
```

脚本会编译出：

```text
dist\BandiZip11Menu.dll
dist\BandiZip11Host.exe
```

并复制清单与图标。

### 使用 Visual Studio / MSVC

也可以把 `src/*.cpp` 作为 C++20 DLL 编译，链接：

```text
ole32.lib shlwapi.lib uuid.lib advapi32.lib shell32.lib user32.lib
```

并导出：

```text
DllGetClassObject
DllCanUnloadNow
```

宿主 EXE 使用 `/SUBSYSTEM:WINDOWS`，入口点 `HostEntry`（源码已经避免依赖 CRT）。如果使用 MSVC 默认 CRT 启动，请把 `host.c` 改成普通的 `WinMain` 入口。

### 发布 Release

```powershell
powershell -ExecutionPolicy Bypass -File .\build.ps1 -Clean
Compress-Archive -Path .\BandiZip11 -DestinationPath .\BandiZip11-v1.0.0.zip -Force
```

然后把 zip 上传到 GitHub Releases。

`dist/` 可以选择提交到仓库（开箱即用），也可以不提交、只在 Release 中提供 zip；后者需要用户自己构建。

---

## 常见问题

### 安装后右键没有 Bandizip？

1. 确认系统是 Windows 11 Build 22000+；
2. 确认 `Get-AppxPackage BandiZip11.ContextMenu` 返回 `Status: Ok`；
3. 重启资源管理器或注销再登录；
4. 右键**文件/文件夹/文件夹空白处**，不是桌面壁纸；
5. 看的是 Windows 11 一级菜单，不是“显示更多选项”；
6. 确认开发者模式已开启，包已成功注册。

### 提示“无法使用指定的命名参数解析参数集”？

这是旧版脚本把 `-AllowUnsigned` 和 `-Register` 一起使用导致的。

`-AllowUnsigned` 只属于 `Add-AppxPackage -Path` 的参数集，不能和 `-Register` 一起用。新版脚本已经移除。手动注册时也不要加：

```powershell
Add-AppxPackage -Register "$env:LOCALAPPDATA\BandiZip11\Package\AppxManifest.xml"
```

### 再次运行安装会装出两份吗？

不会。包身份固定，安装脚本会先移除旧包再注册；同一用户始终只有一个 `BandiZip11.ContextMenu`。

### 为什么没有“测试压缩文件”？

Bandizip 6.29 的 `shell_TestArchive` 默认是 `0`。在 Bandizip 设置里开启，或在 `config.ini` 中设置 `ShowTest=1`。

### 卸载失败，提示包正在使用？

关闭所有资源管理器窗口，重启资源管理器或注销，然后重新卸载；也可以在 bat 提示时选择以管理员权限重试。

### 支持 Bandizip 7.x 吗？

未适配。7.x 的命令行工具和菜单行为可能不同，本项目依赖 6.x 的 `bc.exe`。

### 为什么桌面右键没有？

目前只注册了 `*`、`Directory`、`Directory\Background`。桌面壁纸属于 `DesktopBackground`，没有注册。

### 为什么只能压缩成 ZIP？

当前版本固定使用 `-fmt:zip`。7z、tar、zipx 等格式还没有逐项映射。

### 安装需要管理员权限吗？

- 开发者模式已开启：**不需要**；
- 开发者模式未开启：只有开启开发者模式那一步需要 UAC；
- 卸载：通常**不需要**，失败时可选择管理员重试。

### 会修改 Bandizip 吗？

不会。只读取 Bandizip 的语言文件和设置，并调用 `Bandizip.exe`。

---

## 已知限制

- **仅 Windows 11**：Windows 10 的新菜单机制不同，本项目不适用。
- **未签名包需要开发者模式**：发布给普通用户时，建议用代码签名证书签名 MSIX，并让清单中的 `Publisher` 与证书主题一致。
- **仅 Bandizip 6.x**：依赖 6.x 的 `bc.exe` 和命令行。
- **只输出 ZIP**：压缩格式固定为 ZIP。
- **未实现的功能**：解压每个压缩包到单独文件夹（多压缩包）、解压后删除原文件、管理员身份运行、压缩并发送邮件、分卷压缩等。
- **`AutoClose=1` 的风险**：会传 `-y`，可能自动确认覆盖提示。
- **修改设置后需要重启 Explorer**：DLL 会缓存 Bandizip 路径、语言和菜单开关。
- **旧菜单仍存在**：Bandizip 原来的右键菜单仍然在“显示更多选项”里。

---

## 验证记录

在 **Windows 11 23H2（Build 22631）+ Bandizip 6.29.0.1** 上验证了：

- MSIX 清单注册成功，`Get-AppxPackage` 状态为 `Ok`；
- 通过包身份激活 COM 类，`CoCreateInstance` 返回 `S_OK`；
- `GetTitle` 正确读取 Bandizip `SimpChinese.ini`，显示中文菜单标题；
- `GetState` 根据压缩包/普通文件正确显示或隐藏项目；
- `EnumSubCommands` 返回 9 个候选命令，子命令状态正确；
- `Invoke` 实际启动 Bandizip 并成功生成 `a.zip`；
- 安装/卸载脚本可重复执行；
- 连续安装两次后只有一个 `BandiZip11.ContextMenu` 包，`config.ini` 保留。

由于资源管理器的菜单外观属于 UI 层，建议安装后右键实际确认一次；若未出现，重启 Explorer 或注销再登录。

---

## 安全说明

- 不修改 Bandizip 安装目录中的任何文件。
- 不联网、不收集数据。
- 只读取 Bandizip 的 `langs\*.ini`、`HKCU\Software\Bandizip` 和文件路径。
- 只启动 Bandizip 自带的 `Bandizip.exe`，参数由本工具拼接。
- 源码完全公开，欢迎审计。

---

## 致谢

- 思路参考：[Wings-Fantasy/7-Zip_Context_menu_plugin](https://github.com/Wings-Fantasy/7-Zip_Context_menu_plugin)（Apache-2.0）。本项目没有复用其代码，只借鉴了“MSIX + IExplorerCommand + 扁平化二级菜单”的产品思路。
- 机制参考：[Microsoft：Add a File Explorer context menu command to a packaged desktop app](https://learn.microsoft.com/en-us/windows/apps/desktop/modernize/integrate-packaged-app-with-file-explorer)。
- Bandizip 是 Bandisoft 的商标；本项目与 Bandisoft 无关联。

---

## 许可证

[MIT](LICENSE)

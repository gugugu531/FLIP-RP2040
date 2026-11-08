# VS Code · Raspberry Pi Pico 扩展

配置依据为 [官方 Pico 扩展](https://github.com/raspberrypi/pico-vscode) 及本机安装的 0.23.0 版本。
本工程使用扩展原生配置流程（`useCmakeTools=false`），不要求同时启用 CMake Tools 集成。

## 打开、编译、烧录

1. 安装 `raspberry-pi.raspberry-pi-pico`，并安装仓库推荐的 C/C++、Cortex-Debug、Serial Monitor。
2. 用 **File → Open Folder** 打开仓库根目录；不要只打开 `src/`，也不要加入多根工作区。
3. 等待扩展准备 SDK 2.2.0、ARM 工具链 14_2_Rel1、picotool 2.2.0-a4，并完成自动配置。
4. 点击 **Compile Project** 或按 `Ctrl+Shift+B`，生成 `build/flip_rp2040.elf`、`.uf2`、`.bin`。
5. 接上开发板，点击 **Run Project**；该任务先构建，再通过 picotool USB 加载并运行。
   初次烧录、固件不能响应 USB 重启或设备未找到时，按住 BOOT、短按 RESET、松开 BOOT 后重试。
   也可以把 UF2 拖入 `RPI-RP2`。

默认构建类型是 **Release**，用于正常佩戴与性能测量。使用串口监视器查看 `STAT`；
串口被其他程序占用时，先关闭监视器再烧录。Linux 若提示 USB 权限不足，应使用 Pico 扩展/系统的设备权限配置。

## 调试与任务

| 入口 | 行为 / 要求 |
| --- | --- |
| Configure Project | 用扩展安装的 CMake + Ninja 配置 `build/` |
| Compile Project | 配置后编译；默认构建任务 |
| Run Project | USB 烧录并运行；普通 Type-C 数据线即可 |
| Flash | 通过 CMSIS-DAP / SWD 写入；需要 Debug Probe |
| F5 / Pico Debug (Cortex-Debug) | 构建后经 OpenOCD/GDB 调试，停在 `main`；需要 SWD 连接 |

断点调试前通过 Pico 扩展的 **Switch Build Type** 选择 **Debug**。
工程 Debug 使用 `-Og`，其余配置使用 `-O3`；Debug 不用于验证 60 FPS。
调试器需要连接板卡的 SWDIO、SWCLK、GND，不能仅凭 USB 串口进行 SWD 调试。
本轮验证了 Debug 构建与配置文件，未连接探针对断点和烧录进行实机验证。

## 工程约定

- 根 CMake 中保留扩展识别标记与 `sdkVersion`、`toolchainVersion`、`picotoolVersion`，位于 SDK 导入前。
- `project(flip_rp2040)` 与可执行目标同名，符合扩展根据项目名定位 ELF 的方式。
- 扩展固定使用 `build/`；命令行脚本使用 `build-pendant/`，桌面程序使用 `build-desktop/`，互不共享 CMake 缓存。
- 编辑器从 `build/compile_commands.json` 获取编译参数；首次编译完成后代码补全才有完整 SDK 信息。
- GDB、picotool、OpenOCD 路径由扩展命令解析；SVD 在配置时从所选 SDK 复制到 `build/RP2040.svd`。
- CMake/Ninja 路径采用扩展默认的用户安装目录，分别为 `v3.31.5` / `v1.12.1`，Windows 任务另带 `.exe`。
  更换这两个工具版本或使用非默认安装目录时，同步更新 `.vscode/settings.json` 与 `tasks.json`。
- `PICO_BOARD=pico` 是 SDK 基底，实际触摸板引脚由驱动定义，Flash 为 4 MiB。不要用 **Switch Board** 换成不带触摸的 Waveshare 板定义。

源码已经包含识别配置，一般不需要再次 Import Project。若更新扩展后确需重新导入，
保留触摸板引脚、Release 默认值及本工程任务；导入器可能重写 `.vscode/`。

## 常见问题

- 看不到 Pico 项目菜单：确认打开的是仓库根目录、工作区受信任、扩展启用，再执行 **Developer: Reload Window**。
- 提示构建生成器不匹配：清除 `build/` 内旧缓存，再点击 Configure Project；不要把 Makefiles 构建目录挪作 Ninja 目录。
- 找不到 Ninja/CMake：检查设置指向的扩展工具版本是否已安装；让扩展完成依赖安装后重试。
- 修改配置后没生效：重新 Configure Project；自定义外部 SDK 的命令行构建使用 `PENDANT_USE_PICO_VSCODE=OFF`，见 [开发指南](development.md)。

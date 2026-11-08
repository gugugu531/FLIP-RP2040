# 开发与验证

所有命令从仓库根目录执行。VS Code 用户优先阅读 [Pico 扩展指南](vscode.md)；本页说明独立命令行构建、串口与可选回归检查。

## 固件与使用

交付固件：`artifacts/firmware/flip_rp2040.uf2`。按住 BOOT，短按 RESET，松开 BOOT，将 UF2 复制到 `RPI-RP2`。
程序会独立运行，无需电脑连接：倾斜/晃动改变重力，触摸液体产生向外扰动。

屏幕坐标 X 向右、Y 向下。根据当前实板反馈，默认映射为 `screen_gx = accel_y`、`screen_gy = -accel_x`。
加速度在屏幕平面上的投影直接参与仿真；屏幕朝上平放时，平面内重力应接近零，不会强制吸向屏幕下方。

USB CDC 串口命令（波特率任意，示例用 115200）：

| 命令 | 动作 |
| --- | --- |
| `1` | 流畅：32×32、834 粒子、8 次投影 |
| `2` | 均衡：40×40、1388 粒子、12 次投影 |
| `3` | 精细：48×48、2076 粒子、16 次投影 |
| `r` | 按当前档位重新注水 |
| `+` / `-` | 压力迭代数增减 2，范围 2–40 |
| `x` / `y` / `s` | 翻转 X / 翻转 Y / 交换 IMU 轴；重启恢复编译默认值 |
| `p` | 输出五个求解阶段最近一次耗时（微秒） |
| `d` | 输出 IMU 寄存器及时间戳 |
| `b` | 自动测试 9 组参数，完成后回到默认流畅档 |
| `u` | 进入 BOOTSEL，方便重新烧录 |

```bash
python3 tools/serial_monitor.py --seconds 20
python3 tools/serial_monitor.py --send b --seconds 55 --output /tmp/my-benchmark.log
```

`STAT` 每两秒报告一次；`fps_x10=600` 表示 60.0 FPS，`sim_hz_x10` 为物理更新频率。
`sim_us`、`peak_us` 是求解均值/峰值，`raster_us` 是着色计算，`lcd_us` 含着色及重叠的 DMA。
`late` 表示丢弃过期调度的次数，`clip` 是自重置起 G2P 速度限幅的分量数，`i2c_err` 是启动探测后累计通信错误。

## 构建

需要 Pico SDK、ARM GCC、CMake、picotool。当前已用 SDK 2.2.0 和 ARM GCC 14.2 构建验证。

```bash
./tools/build.sh
```

脚本关闭扩展路径接管（`PENDANT_USE_PICO_VSCODE=OFF`），独立使用 `build-pendant/`，避免与扩展的 Ninja 缓存冲突。
脚本优先使用 `PICO_SDK_PATH` / `PICO_TOOLCHAIN_PATH` / `picotool_DIR` 环境变量，默认探测当前用户 `.pico-sdk` 中的上述版本。
输出在 `build-pendant/flip_rp2040.uf2`。其他安装路径可直接使用标准 CMake：

```bash
cmake -S . -B build-pendant \
  -DPENDANT_USE_PICO_VSCODE=OFF \
  -DPICO_SDK_PATH=/path/to/pico-sdk \
  -DPICO_TOOLCHAIN_PATH=/path/to/arm-toolchain \
  -Dpicotool_DIR=/path/to/picotool/cmake \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build-pendant -j
```

可调构建参数：

| 参数 | 默认 | 用途 |
| --- | --- | --- |
| `PENDANT_SYS_KHZ` | 133000 | CPU 主频；交付数据没有使用 CPU 超频 |
| `PENDANT_SPI_HZ` | 66500000 | LCD 请求时钟；串口报告实际分频值 |
| `FLUID_RENDER_SCALE` | 4 | 默认 60×60 像素风；2 为 120×120；1 为原生 240×240 着色 |
| `PENDANT_IMU_SWAP` | 1 | 交换加速度计 X/Y |
| `PENDANT_IMU_X_SIGN` / `PENDANT_IMU_Y_SIGN` | 1 / -1 | 屏幕坐标轴正负号 |

若不同板卡在高速 SPI 下显示异常，可以用 `./tools/build.sh build-slow -DPENDANT_SPI_HZ=40000000` 生成回退版；133 MHz 外设时钟下实际为 33.25 MHz。
这里使用触摸版的 GPIO13 复位、4 MB Flash；SDK 中同名但不带触摸的 `waveshare_rp2040_lcd_1.28` 板定义不适合直接替换。

## 桌面预览与可选回归检查

日常固件构建不依赖主机编译器、SDL2 或测试工具。桌面入口为 `desktop/CMakeLists.txt`，默认只构建预览和动画帧导出：

```bash
cmake -S desktop -B build-desktop -DCMAKE_BUILD_TYPE=Release
cmake --build build-desktop -j
./build-desktop/flip_rp2040_preview
```

SDL2 开发包可选；缺少它时只跳过 `flip_rp2040_preview`。左右键旋转重力，鼠标左键扰动，`1/2/3` 切档，`R` 重置，Esc 退出。

只保留两项有独立作用的回归检查，需要修改算法或渲染时显式启用：

```bash
cmake -S desktop -B build-checks -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_TESTING=ON -DSANITIZE=ON
cmake --build build-checks -j
ctest --test-dir build-checks --output-on-failure
```

- `fluid_stability`：3 档各 2400 步，检查质量、圆形边界、速度界限、重力响应、投影散度和零重力耗散。
- `render_depth_and_bezel`：旋转后水深色阶、原生圆框细节、整帧与条带输出一致性。

`SANITIZE=ON` 需要支持 ASan/UBSan 的主机编译器（本机 Linux GCC 已验证）。
受跟踪环境无法运行 LeakSanitizer 时，可对 CTest 命令加 `ASAN_OPTIONS=detect_leaks=0`；地址和未定义行为检查仍启用。
已移除数值测试中的重复条带检查、PPM 图片导出、主机耗时统计和独立开方冒烟断言；图片输出由 `flip_rp2040_capture` 负责。

导出演示动画（需要 ffmpeg）：

```bash
./build-desktop/flip_rp2040_capture /tmp/flip_rp2040.rgb
ffmpeg -f rawvideo -pixel_format rgb24 -video_size 240x240 -framerate 30 \
  -i /tmp/flip_rp2040.rgb -vf 'split[a][b];[a]palettegen[p];[b][p]paletteuse' \
  -y /tmp/flip_rp2040.gif
```

输出 240 帧 RGB24，来自 480 个固定步，以 30 FPS 编码得到 8 秒动画；桌面速度不代表 RP2040 性能。

## 修改后的检查

- 修改求解器：运行 `fluid_stability`，再在实机测量对应档位的 `STAT` / `BENCH`。
- 修改着色、缓存或字节序：运行 `render_depth_and_bezel`，用桌面预览观察，再检查实机颜色及 SPI 输出。
- 修改调度、I²C、DMA 或时钟：主机测试不能覆盖这些行为，必须补实机串口采样及屏幕观察。
- 修改文档、注释或路径：干净构建、现有测试及引用检查即可；不能把历史实机数据写成新测量结果。

## 交付文件与证据

`artifacts/firmware/` 保存当前交付 UF2 及相对该目录校验的 SHA-256；`artifacts/preview/` 保存演示。
`artifacts/measurements/` 保存历史原始计时、测试日志与构建信息，详细来源见 [产物说明](../artifacts/README.md)。
本地实验输出放在 `build-*` 或 `/tmp`，不要混入交付目录。

```bash
(cd artifacts/firmware && sha256sum -c firmware.sha256)
```

发布新功能时，先构建和测试，再烧录实测，更新交付 UF2、校验和、性能报告及相应日志。
保留测量的配置、日期和固件身份，避免将旧版本结果归到新版本名下。

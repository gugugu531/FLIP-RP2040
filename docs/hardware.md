# 硬件与坐标约定

适用目标为 Waveshare **RP2040-Touch-LCD-1.28 触摸版**。引脚以
`lib/Config/DEV_Config.h` 为准，固件使用通用 `pico` SDK 定义并显式设定 4 MiB Flash。
不要直接换成不带触摸的 `waveshare_rp2040_lcd_1.28` 定义：本工程 LCD 复位使用 GPIO13。

## 当前引脚

| 外设 | 信号 | GPIO / 接口 |
| --- | --- | --- |
| GC9A01A LCD | DC / CS / CLK / MOSI | 8 / 9 / 10 / 11，SPI1 |
| LCD | RESET / 背光 PWM | 13 / 25 |
| QMI8658、CST816 系列触摸 | SDA / SCL | 6 / 7，I²C1 |
| 触摸 | INT / RESET | 21 / 22 |
| 电池 ADC | 输入 | 29（驱动初始化，应用尚未显示电量） |

LCD_MISO 定义为 GPIO12，但当前显示路径只写屏。IMU/触摸均在主循环轮询，未使用中断驱动采样。

## 传感器初始化

IMU 与触摸驱动分别位于 `lib/qmi8658/`、`lib/cst816/`，由 `src/input.c` 组合。
它们共用 `lib/sensor_i2c/` 的事务接口，每次 SDK I²C 操作有 2 ms 超时并校验返回长度，启动初始化结束后清零各自错误计数。
`STAT i2c_err` 汇总两者的运行期错误数。
若没有识别到 IMU，仿真保留默认向下重力；触摸未识别时禁用触摸扰动。

| 器件 | 当前初始化 / 读取约定 |
| --- | --- |
| QMI8658 | 探测 0x6A / 0x6B，WHO_AM_I=0x05；0x60 写 0xB0 软复位 |
| CTRL1 / CTRL2 / CTRL7 | 0x02=0x40 自动递增、小端；0x03=0x25 ±8g、250 Hz；0x08=0x01 仅启用加速度计 |
| 加速度 | 从 0x35 完整读取 12 字节，前 6 字节为小端有符号 XYZ；4096 LSB/g |
| 触摸 | 地址 0x15，0xA7 接受 0xB4/0xB5/0xB6；0xFE=1、0xFA=0x41；从 0x02 读取 5 字节触点数与坐标 |

驱动的 `qmi8658_read_accel()` 返回传感器原始 XYZ，读取失败时不修改输出；主循环因此保留上次重力值。
`input.c` 根据编译默认值与串口设置进行屏幕轴映射，再换算为 milli-g 并使用 1/4 的一阶平滑。未启用陀螺仪，也没有姿态融合；
设备运动时线性加速度会与重力一同影响液体。平放朝上时屏幕平面内重力接近零。

## 坐标映射与排错

屏幕 X 向右、Y 向下；当前实板反馈校正后的默认值为 `gx=ay, gy=-ax`。
串口 `x/y/s` 分别翻转 X、翻转 Y、交换输入轴，重启后恢复 CMake 默认值。
确认方向后通过 `PENDANT_IMU_SWAP`、`PENDANT_IMU_X_SIGN`、`PENDANT_IMU_Y_SIGN` 固化配置。
触摸驱动提供面板像素坐标，输入服务保留该单位；`main.c` 在调用求解器前按 `pixel*n/240` 映射到当前网格。

`d` 经驱动诊断接口输出 IMU 寄存器与时间戳；设备缺失时不访问 I²C 地址 0，读取失败时明确提示。
`STAT raw` 是原始 XYZ，`i2c_err` 是启动探测后累计通信错误，
`touch` 表示触摸初始化成功，不表示当前手指按下。

LCD 默认请求 SPI 66.5 MHz，实际时钟由固件日志报告。如果其他板卡出现花屏，
用 `./tools/build.sh build-slow -DPENDANT_SPI_HZ=40000000` 降速验证（133 MHz 外设时钟下实际 33.25 MHz）。
带宽、计时与未验证事项见 [性能报告](performance.md)；官方资料链接见 [参考索引](../reference/README.md)。

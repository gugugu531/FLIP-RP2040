# 第三方来源与许可

## FLIP 参考算法

`reference/flip.html` 来自 Matthias Müller / Ten Minute Physics，文件头声明
Copyright 2022 Matthias Müller 和 MIT License，完整声明保留在该文件。
`src/fluid.c` 基于其粒子—网格流程进行定点化、圆形容器和嵌入式性能适配。

## Waveshare 驱动

`lib/Config/`、`lib/LCD/` 保留 Waveshare 示例驱动的文件头许可声明，
本工程修改了触摸板引脚配置和异步 SPI/DMA 显示路径。
删除未使用的字体、GUI、触摸和 IMU 示例模块不影响保留文件中的许可声明。

## Raspberry Pi Pico SDK

`pico_sdk_import.cmake` 保留 Raspberry Pi 的 BSD-3-Clause 文件头声明。
Pico SDK、工具链和 SDL2 由开发环境提供，未将其源码复制进本仓库；遵循各自许可证。

项目新增代码采用 [MIT License](LICENSE)。第三方文件保留各自原始版权和许可证；Pico SDK 的 BSD-3-Clause 等声明不被项目许可证替换。

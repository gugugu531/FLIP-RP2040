# 交付与测量产物

| 目录 | 内容 |
| --- | --- |
| `firmware/` | 唯一当前 `flip_rp2040.uf2`，默认像素水体、自由液面距离色阶、原生抗锯齿圆框；`firmware.sha256` 在该目录内校验，`manifest.json` 记录该交付版本构建时的源码哈希与验证来源 |
| `preview/` | `flip_rp2040.gif`，共享核心生成的桌面演示，不是实机录像 |
| `measurements/` | 2026-11-08 的原始实机计时、主机数值检查、链接大小与参数矩阵 |

`device-depth.log` 对应当前画风；`device-pixel.log` 和 `device-final-*` 等记录之前的画风/优化阶段。
完整解释见 [性能报告](../docs/performance.md)。日志保留原文，内部的旧路径与构建目录不代表当前目录结构。

`measured-build.json` 是整理前、已在实机运行的版本清单，其中源码路径/哈希属于当时布局，
不作为整理后源码的哈希清单。`measured-build.sha256` 同样保留当时的原文路径。
后续 Pico 扩展接入与测试精简见 [变更记录](../CHANGELOG.md)。
交付 UF2 已按 `flip_rp2040` 工程名重新构建，该次发布的源码哈希记录在 `firmware/manifest.json`，不随之后的工作区源码变更自动更新。
本轮通过 Release / Debug 构建和主机回归检查，未重新采集实机计时；历史日志属于重命名前的固件，
其 UF2 哈希保存在清单的 `previous_measured_uf2_sha256` 中。求解器、渲染器及默认物理参数未修改。

无需提交本地每次构建日志、原始 RGB 帧、PPM、重复固件或源码副本；新测量应明确配置、日期和对应版本。

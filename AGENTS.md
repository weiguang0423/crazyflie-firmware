# Crazyflie 固件 — AI 代理指南

## 构建与测试命令

### 环境准备
- ARM GCC 工具链 (`gcc-arm-none-eabi` ≥ 10.3)
- 克隆时需带子模块：`git clone --recursive https://github.com/bitcraze/crazyflie-firmware.git`
- Docker 替代方案：使用 `bitcraze/builder` 镜像

### 配置（基于 Kbuild，类似 Linux 内核）
```bash
make cf2_defconfig      # 默认 Crazyflie 2.x 配置
make bolt_defconfig     # Bolt 配置
make tag_defconfig      # Roadrunner 配置
make flapper_defconfig  # Flapper 配置
make cf21bl_defconfig   # 无刷电机配置
make menuconfig         # 交互式配置界面
```

### 构建
```bash
make                    # 构建当前平台的 .hex、.bin、.elf
make clean
```

### 烧录
```bash
make cload              # 无线 bootloader 烧录（需要 cfloader）
make flash              # 有线 STLink/V2 烧录（需要 openocd）
make flash_dfu          # USB DFU 烧录
```

### 测试
```bash
make unit                          # 运行所有单元测试（需要 rake、ruby、libasan）
make unit FILES=test/utils/src/test_num.c  # 运行单个测试
tb make unit                       # 通过 toolbelt（Docker）运行
```
**CI 使用**: `docker run --rm -v ${PWD}:/module bitcraze/builder bash -c "./tools/build/build UNIT_TEST_STYLE=min"`

### 构建示例/树外应用
```bash
./tools/build/make_app examples/app_hello_world
```

## 架构概览

这是 **嵌入式 C 固件**，用于 Bitcraze Crazyflie 无人机系列，运行在 **STM32F405**（Cortex-M4F）上，使用 **FreeRTOS** 实时操作系统。

### 核心控制回路（1 kHz）
稳定器是整个系统的核心。数据流如下：
```
传感器 → 状态估计器 → 控制器 → 功率分配 → 电机
```
参见 `src/modules/src/stabilizer.c` — `stabilizerTask()` 函数。  
详细文档：[docs/functional-areas/sensor-to-control/](docs/functional-areas/sensor-to-control/index.md)

### 关键子系统

| 子系统 | 位置 | 功能 |
|-----------|----------|---------|
| **稳定器** | `src/modules/src/stabilizer.c` | 1kHz 主循环：传感器 → 估计器 → 控制器 → 电机 |
| **状态估计器** | `src/modules/src/estimator/` | 互补滤波器 & 扩展卡尔曼滤波器 |
| **控制器** | `src/modules/src/controller/` | PID、Mellinger、INDI、Brescianini、Lee |
| **传感器** | `src/drivers/` + `src/hal/` | IMU（BMI088/MPU9250）、气压计、磁力计 |
| **指令器** | `src/modules/src/crtp_commander*.c` | 通过无线电/CRTP 处理设定值 |
| **功率分配** | `src/modules/src/power_distribution_*.c` | 电机推力 → PWM 转换，含电池补偿 |
| **扩展板系统** | `src/deck/` | 扩展板驱动（参见 [howto](docs/development/howto.md)） |
| **日志/参数** | `src/modules/src/log.c`、`param.c` | 基于 CRTP 的实时日志与参数系统 |
| **监控器** | `src/modules/src/supervisor.c` | 安全状态机，电机解锁控制 |

### 源码目录结构
```
src/
├── modules/src/       # 主要飞行逻辑（稳定器、指令器、日志、参数等）
├── modules/interface/ # 模块公开头文件
├── deck/drivers/src/  # 扩展板驱动
├── drivers/src/       # 硬件驱动（IMU、气压计等）
├── hal/src/           # 硬件抽象层
├── platform/          # 平台特定代码（cf2、bolt、tag、flapper）
├── init/              # 系统初始化
├── utils/src/         # 工具函数
└── lib/               # STM32F4xx 标准外设库、FatFS、USB 等
vendor/                # 第三方库：FreeRTOS、CMSIS、libdw1000、Unity、CMock
configs/               # 各平台/功能的 Kconfig 默认配置文件
test/                  # 单元测试（镜像 src/ 的结构）
docs/                  # Markdown 文档
examples/              # 树外应用示例
tools/                 # 构建脚本、调试工具
```

## 项目约定

### 构建系统：Kbuild
- 每个源码目录有一个 `Kbuild` 文件，列出 `obj-y += file.o` 或 `obj-$(CONFIG_FEATURE) += file.o`
- 通过 `Kconfig` 文件使用 Linux 内核 Kconfig 语法进行配置
- `menuconfig` 生成 `.config`，进而转换为 C 宏定义 (`CONFIG_*`)
- C 代码中使用 `#ifdef CONFIG_FEATURE` 进行条件编译

### 模块模式
每个功能模块遵循以下模式：
- `src/modules/src/foo.c` — 实现
- `src/modules/interface/foo.h` — 公开 API
- 初始化函数：`fooInit()`，测试函数：`fooTest()`
- 模块在 `stabilizerInit()` 中组合在一起

### 扩展板驱动模式
扩展板驱动使用注册宏。示例：
```c
static const DeckDriver myDriver = {
  .name = "myDeck",
  .init = myInit,
  .test = myTest,
};
DECK_DRIVER(myDriver);
```
添加到 `src/deck/drivers/src/Kbuild`：`obj-$(CONFIG_DECK_MY) += my.o`

### 日志与调试
- `DEBUG_PRINT("格式", ...)` — printf 风格的无线调试输出（也可配置为 UART1 输出）
- `#define DEBUG_MODULE "NAME"` — 在文件顶部定义，用于标识日志来源
- `LOG_GROUP_START(name)` / `LOG_ADD(...)` / `LOG_GROUP_STOP(name)` — 实时变量日志
- `PARAM_GROUP_START(name)` / `PARAM_ADD(...)` / `PARAM_GROUP_STOP(name)` — 运行时参数

### 安全关键代码段
在 `stabilizerTask()` 中，标记为以下注释的代码段：
```c
// Critical for safety, be careful if you modify this code!
```
这些包括监控器检查、指令器阻塞、设定值覆盖和电机控制部分。对此处的任何修改都会影响飞行安全。

### 测试
- 单元测试使用 **Unity** 框架 + **CMock** 进行 mock
- 测试文件位于 `test/`，结构镜像 `src/` 的目录结构
- 测试文件可根据 Kconfig 排除：`// @IGNORE_IF_NOT CONFIG_DECK_LIGHTHOUSE`
- `test_python/` 中有 Python 测试，用于验证控制器/估计器的数学计算

### 代码风格
- C99/C11，缩进：2 个空格
- 变量命名：局部变量用 camelCase，结构体成员用 snake_case
- 文件内局部函数和变量使用 `static`
- 所有源文件需包含 GPL-3.0 许可证头
- 头文件保护：`#ifndef __FILENAME_H__` / `#define __FILENAME_H__`

## 平台变体
同一代码库支持 5 种平台，通过 Kconfig 区分：
- **cf2** — Crazyflie 2.0/2.1（有刷电机）
- **cf21bl** — Crazyflie 2.1 无刷版
- **bolt** — Crazyflie Bolt
- **tag** — Roadrunner（不飞行）
- **flapper** — Flapper Nimble+

## 重要约束

- **飞行路径中禁止动态内存分配** — 使用静态内存 (`STATIC_MEM_TASK_ALLOC`)
- **栈空间有限** — F405 的 RAM 为 128 KB，CCM 为 64 KB
- **1 kHz 稳定器循环** — 函数必须在约 1ms 内完成
- **硬件浮点单元** — Cortex-M4F 带 FPU，但避免使用 `double`（使用 `float`）
- **构建路径不能包含空格**
- **单元测试需要 `-DUNITY_INCLUDE_DOUBLE`** 以支持 double 类型比较

## 有用的文档链接

- [构建与烧录](docs/building-and-flashing/build.md)
- [Kbuild 配置](docs/development/kbuild.md)
- [单元测试](docs/development/unit_testing.md)
- [传感器到控制流水线](docs/functional-areas/sensor-to-control/index.md)
- [创建扩展板驱动](docs/development/howto.md)
- [树外构建](docs/development/oot.md)
- [官方文档](https://www.bitcraze.io/documentation/repository/crazyflie-firmware/master/)

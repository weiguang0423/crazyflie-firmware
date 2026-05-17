---
title: 指令器框架
page_id: commanders_setpoints
---

本节将深入介绍指令器（commander）框架，该框架负责处理期望状态的设定值，控制器将尝试引导估计状态向这些设定值靠拢。

 * [指令器模块](#指令器模块)
 * [设定值结构](#设定值结构)
 * [高层指令器](#高层指令器)


## 指令器模块

![commander framework](/docs/images/commander_framework.png){:width="700"}

指令器模块处理来自多个来源的输入设定值（固件仓库中的 `src/modules/src/commander.c`）。设定值可以直接设置，方式包括：通过 Python 脚本使用 [cflib](https://github.com/bitcraze/crazyflie-lib-python)/[cfclient](https://github.com/bitcraze/crazyflie-clients-python)，或通过[应用层](/docs/userguides/app_layer.md)（图中蓝色通路），也可以由高层指令器模块生成（紫色通路）。而高层指令器本身既可以通过 Python 库远程控制，也可以在 Crazyflie 内部运行。

需要特别注意的是，指令器模块还会检查最近一次收到设定值距今已过了多长时间。如果超过一小段时间（由 `commander.c` 中的阈值 `COMMANDER_WDT_TIMEOUT_STABILIZE` 定义），它会将姿态角置为 0，以保持 Crazyflie 的稳定。如果时间进一步超过 `COMMANDER_WDT_TIMEOUT_SHUTDOWN`，则会给出空设定值，导致 Crazyflie 关闭电机并从空中坠落。如果你使用高层指令器，则不会发生这种情况。

## 设定值结构

要理解指令器模块，你必须先理解设定值结构。具体实现在 Crazyflie 固件的 `src/modules/interface/stabilizer_types.h` 中，以 `setpoint_t` 结构体定义。

有两个控制层级，分别是：

    位置（X, Y, Z）
    姿态（俯仰角 pitch、滚转角 roll、偏航角 yaw，或用四元数表示）

这些可以按不同模式进行控制，即：

    绝对模式（modeAbs）
    速度模式（modeVelocity）
    禁用（modeDisable）

![commander framework](/docs/images/setpoint_structure.png){:width="700"}

因此，如果希望进行绝对位置控制（例如飞到 x, y, z 坐标 (1, 0, 1) 处），当 `setpoint.mode.xyz` 设为 `modeAbs` 时，控制器将遵循 `setpoint.position.xyz` 中给出的值。如果你更希望控制速度（例如沿 x 方向以 0.5 m/s 的速度飞行），当 `setpoint.mode.xyz` 设为 `modeVel` 时，控制器将监听 `setpoint.velocity.xyz` 中给出的值。此时所有姿态设定值模式将被设为禁用（`modeDisabled`）。如果只需要控制姿态，那么所有位置模式都将设为 `modeDisabled`。例如，当你通过 [cfclient](https://www.bitcraze.io/documentation/repository/crazyflie-clients-python/master/) 以姿态模式操控 Crazyflie 时，就是这种情况。


## 高层指令器

![high level commander](/docs/images/high_level_commander.png){:width="700"}

如前所述：高层指令器在固件内部基于预定义轨迹生成设定值。这是作为 [USC ACT 实验室](https://act.usc.edu/)的 [Crazyswarm](https://crazyswarm.readthedocs.io/en/latest/) 项目的一部分合并进来的。高层指令器使用规划器（planner），基于 "起飞"、"前往" 或 "降落" 等动作，通过 7 阶多项式生成平滑轨迹。规划器生成一组设定值，由高层指令器逐一发送给指令器框架。


### 设定值优先级

指令器框架可能同时收到来自高层指令器和低层设定值的输入，例如来自地面用户的控制。低层设定值始终具有更高优先级，因为它们可能来自希望接管高层指令器控制权并处理紧急情况的用户。一旦高层指令器因收到低层设定值而被禁用，它将不再生成任何设定值，直到通过调用 `commanderRelaxPriority()` 函数（或通过 Python 库的 `cf.commander.send_notify_setpoint_stop()`）显式地重新启用。

请注意，只要飞行器处于飞行状态，[监控器（supervisor）](/docs/functional-areas/supervisor/)就会检查指令器框架是否持续收到设定值流；如果没有，它将采取行动以保护飞行器和周围人员的安全，最终可能导致进入锁定状态。

### 在高层指令器与低层设定值之间切换

有时需要在高层指令器和低层设定值之间来回切换。从高层指令器切换到低层设定值很简单，只需开始发送低层设定值即可。而要重新切回高层指令器，则需要调用 `commanderRelaxPriority()` 函数（或通过 Python 库的 `cf.commander.send_notify_setpoint_stop()`）来重新启用高层指令器。

请注意，降落后飞行器需要几秒钟才能意识到自己已不再处于飞行状态。如果你在降落阶段使用脚本或应用程序向 Crazyflie 发送低层设定值，你需要继续发送零设定值一段时间，以避免监控器锁定飞行器。另一种选择是重新启用高层指令器，因为它会持续向指令器框架发送零设定值——即使在不执行轨迹飞行时也是如此。


## Python 库（CFLib）中的支持

通过 [Python 库](https://github.com/bitcraze/crazyflie-lib-python/)与指令器框架交互主要有四种方式。

* **autonomousSequence.py**：使用 Crazyflie 对象的 Commander 类直接发送设定值。
* **motion_commander_demo.py**：MotionCommander 类提供了简化的 API，根据所调用的方法持续发送速度设定值。
* **autonomous_sequence_high_level.py**：使用 Crazyflie 对象的 HighLevelCommander 类直接与高层指令器交互。
* **position_commander_demo.py**：使用 PositionHlCommander 类提供简化的 API，向高层指令器发送命令。

---
title: Crazyflie 中的控制器
page_id: controllers
---


当[状态估计器](/docs/functional-areas/sensor-to-control/state_estimators.md)输出 Crazyflie 当前（估计）的位置、速度和姿态状态后，控制器便需要维持该状态，或根据设定值将 Crazyflie 移动到新的位置。这是 Crazyflie 稳定系统中至关重要的一部分。

- [控制概览](#控制概览)
- [级联 PID 控制器](#级联-pid-控制器)
- [Mellinger 控制器](#mellinger-控制器)
- [INDI 控制器](#indi-控制器)
- [Brescianini 控制器](#brescianini-控制器)
- [Lee 控制器](#lee-控制器)

## 控制概览
Crazyflie 中有四个控制层级：
* 姿态角速率
* 姿态绝对角度
* 速度
* 位置

以下是各层级对应的控制器类型概览：

![controller overview](/docs/images/controller_overview.png){:width="500"}

接下来将逐一解释各个控制器在 [crazyflie-firmware](https://github.com/bitcraze/crazyflie-firmware/) 中的具体实现方式。

## 级联 PID 控制器

默认情况下，Crazyflie 固件使用[比例-积分-微分（PID）](https://en.wikipedia.org/wiki/PID_controller)控制器来管理无人机状态。固件为每个控制层级——位置、速度、姿态和姿态角速率——分别使用不同的 PID 控制器。每个控制器的输出会馈送到下一个较低层级控制器的输入，形成级联 PID 结构。根据[控制模式](/docs/functional-areas/sensor-to-control/commanders_setpoints/#setpoint-structure)的不同，可以向系统输入不同的设定值，从而影响哪些 PID 控制器被激活。例如：使用姿态角速率设定值时，只有姿态角速率 PID 控制器处于活动状态；使用姿态设定值时，姿态和姿态角速率 PID 控制器都会被使用；速度设定值和位置设定值也以此类推。最终，无论处于哪种控制模式，角速率控制器都会将期望的角速率转换为发送给电机的 PWM 指令。

为了增强控制稳定性并防止大幅设定值变化时出现问题，我们实施了一种解决方案来避免"微分突变"（derivative kick）——即由设定值变化引起的控制输出突然尖峰。该方案使用测量的过程变量的变化率来计算微分项，而不是使用误差的变化率。通过在大幅设定值变化时防止微分突变，这种方法确保了更平滑、更可靠的控制。

以下是 PID 控制器实现的框图。

![cascaded pid controller](/docs/images/cascaded_pid_controller.png){:width="700"}

下面对级联 PID 的各个回路进行更详细的说明。

### 姿态角速率 PID 控制器

姿态角速率 PID 控制器直接控制姿态角速率。它几乎直接接收陀螺仪测量的角速率（经过少量滤波处理），以期望角速率与当前角速率之间的误差作为输入，输出直接发送到功率分配模块 `power_distribution_quadrotor.c`。该控制回路以 500 Hz 的频率运行。

实现细节请参见 `attitude_pid_controller.c` 中的 `attitudeControllerCorrectRatePID()` 函数。

### 姿态 PID 控制器

绝对姿态 PID 控制器是姿态控制的外环。它以状态估计器输出的估计姿态作为输入，将期望姿态设定值与实际姿态之间的误差用于控制 Crazyflie 的姿态。其输出为期望角速率，发送给姿态角速率控制器。该控制回路以 500 Hz 的频率运行。

实现细节请参见 `attitude_pid_controller.c` 中的 `attitudeControllerCorrectAttitudePID()` 函数。

### 位置与速度控制器

级联 PID 控制器的最外环是位置和速度控制器。它接收来自指令器的位置或速度输入进行处理，因为可以在 `setpoint_t` 结构体中设置使用哪种稳定模式 `stab_mode_t`（位置模式：`modeAbs` 或速度模式：`modeVelocity`），这些定义可以在 `stabilizer_types.h` 中找到。该控制回路以 100 Hz 的频率运行。

实现细节请参见 `position_controller_pid.c` 中的 `positionController()` 和 `velocityController()` 函数。

## Mellinger 控制器

_**注意：** 此控制器在计算时依赖于平台质量。每当配置发生变化时，请确保在[固件的平台默认设置](https://github.com/bitcraze/crazyflie-firmware/tree/master/src/platform/interface)中更新平台质量。_

从概念上讲，Mellinger 控制器与级联 PID 控制器类似，即包含一个姿态控制器（以 250 Hz 运行）和一个位置控制器（以 100 Hz 运行）。与级联 PID 控制器的主要区别在于误差的定义方式，以及位置误差如何转换为期望的姿态设定值。与级联 PID 一样，这是一种反应式几何控制器，利用了微分平坦性这一数学特性。详细内容见以下科学论文：

```
Daniel Mellinger, and Vijay Kumar
Minimum snap trajectory generation and control for quadrotors
IEEE International Conference on Robotics and Automation (ICRA), 2011
https://doi.org/10.1109/ICRA.2011.5980409
```

固件实现遵循论文描述，包括变量命名。主要区别在于增加了积分增益（I 增益）和角速度的微分项（D 项）。

## INDI 控制器

该控制算法是增量非线性动态逆（Incremental Nonlinear Dynamic Inversion，INDI）控制器。详细内容见以下科学论文：

```
Ewoud J. J. Smeur, Qiping Chu, and Guido C. H. E. de Croon
Adaptive Incremental Nonlinear Dynamic Inversion for Attitude Control of Micro Air Vehicles
JCGD 2015
https://doi.org/10.2514/1.G001490
```

## Brescianini 控制器

_**注意：** 此控制器在计算时依赖于平台质量。每当配置发生变化时，请确保在[固件的平台默认设置](https://github.com/bitcraze/crazyflie-firmware/tree/master/src/platform/interface)中更新平台质量。_

该控制器的详细信息见以下科学论文：

```
Dario Brescianini, Markus Hehn, and Raffaello D'Andrea
Nonlinear quadrocopter attitude control
Technical Report ETHZ, 2013
https://doi.org/10.3929/ethz-a-009970340
```

## Lee 控制器

_**注意：** 此控制器在计算时依赖于平台质量。每当配置发生变化时，请确保在[固件的平台默认设置](https://github.com/bitcraze/crazyflie-firmware/tree/master/src/platform/interface)中更新平台质量。_

从概念上讲，Lee 控制器与级联 PID 控制器类似，即包含一个姿态控制器（以 250 Hz 运行）和一个位置控制器（以 100 Hz 运行）。与级联 PID 控制器的主要区别在于误差的定义方式，以及位置误差如何转换为期望的姿态设定值。与级联 PID 一样，这是一种反应式几何控制器，利用了微分平坦性这一数学特性。与 Mellinger 控制器相比，Lee 控制器使用了不同的角速度误差定义和姿态控制器中的高阶项。包括稳定性证明在内的详细内容见以下科学论文：

```
Taeyoung Lee, Melvin Leok, and N. Harris McClamroch
Geometric Tracking Control of a Quadrotor UAV on SE(3)
CDC 2010
https://doi.org/10.1109/CDC.2010.5717652
```

固件实现遵循论文描述，包括变量命名。主要区别在于增加了积分增益（I 增益），虽然在理论证明中不需要，但在实际系统中非常有用。

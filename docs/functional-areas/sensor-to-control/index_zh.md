---
title: 稳定器模块
page_id: stabilizer_index
---

本页面旨在介绍和概述从传感器采集到电机控制的整个通路，也称为稳定器模块。本文不会深入细节，而是大致勾勒出传感器测量数据如何进入状态估计器、再传给控制器，最终由功率分配模块分配给电机的整体流程。当然，电机会影响 Crazyflie 的飞行状态，而这又间接影响传感器在下一时间步所检测到的数据。

 * [传感器](#传感器)
 * [状态估计](state_estimators.md)
 * [状态控制器](controllers.md)
 * [指令器框架](commanders_setpoints.md)
 * [功率分配](#功率分配)
 * [配置估计器与控制器](configure_estimator_controller.md)



### 概览

![sensor](/docs/images/sensors_to_motors.png)

## 模块


### 传感器

传感器是 Crazyflie 飞行的基础。以下是 Crazyflie 最终用于状态估计的主要传感器选型：


* [板载传感器](https://store.bitcraze.io/products/crazyflie-2-1)
  * 加速度计：机体坐标系下的加速度，单位为 m/s²
  * 陀螺仪：滚转、俯仰和偏航方向的角速率（rad/s）
  * 气压传感器：大气压力，单位为 mBar
* [Flowdeck v2](https://store.bitcraze.io/products/flow-deck-v2)
  * ToF 传感器*：到表面的距离，单位为毫米
  * 光流传感器：检测像素在每个时间采样内的移动量（px）
* [Loco 定位扩展板](https://store.bitcraze.io//products/loco-positioning-deck)：
  * 超宽带模块：两个 UWB 模块之间的距离，或 TDOA*** 值，单位为米
* [Lighthouse 扩展板](https://store.bitcraze.io/products/lighthouse-positioning-deck)：
  * 红外接收器：HTC Vive 基站的扫描角度，单位为弧度
* 动作捕捉（MoCap）：[主动标记扩展板](https://www.bitcraze.io/products/active-marker-deck/) 或 [动作捕捉标记扩展板](https://www.bitcraze.io/products/motion-capture-marker-deck/)
  * 外部计算的位置和姿态数据，通常通过 Crazyradio 广播给 Crazyflie

<sub><sup>_*Time-of-Flight（飞行时间测距）_</sup></sub>

<sub><sup>_**[Zranger v2](https://store.bitcraze.io/collections/decks/products/z-ranger-deck-v2) 也包含一个激光测距器_</sup></sub>

<sub><sup>_***Time-difference of Arrival（到达时间差）_</sup></sub>


### 状态估计

Crazyflie 中有 2 种状态估计器：
* 互补滤波器
* 扩展卡尔曼滤波器

更多详细信息请参见[状态估计页面](state_estimators.md)。


### 状态控制器
Crazyflie 中有 3 种控制器：
* PID 控制器
* INDI 控制器
* Mellinger 控制器

更多详细信息请参见[控制器页面](controllers.md)。


### 指令器框架
期望状态可以通过设定值结构以位置或姿态模式进行处理，可以通过 [cflib](https://www.bitcraze.io/documentation/repository/crazyflie-lib-python/master/) 或高层指令器来设置。

更多详细信息请参见[指令器页面](commanders_setpoints.md)。

### 功率分配

状态控制器发出指令后，流程尚未结束。控制器发出的是与偏航、滚转和俯仰角相关的指令。
电机如何响应以执行这些基于姿态的指令取决于以下几个因素：
  * 电机：
    * 有刷电机：支持有刷电机的 Crazyflie 2.X 平台支持电池补偿，可调整电机输出以在整个电池电压范围内保持一致的推力。实现细节请参见 `motors.c`，以及关于这些电机的 [PWM 到推力研究](/docs/functional-areas/pwm-to-thrust.md) 和[电池补偿文档](/docs/functional-areas/battery_compensation.md)。
    * 无刷电机：Crazyflie 2.1 无刷版和 Crazyflie Bolt 支持无刷电机控制。Crazyflie 2.1 无刷版同样提供电池补偿，但 Crazyflie Bolt 不提供，因为其设置取决于用户的电机配置。更多信息请查看 [Bolt 产品页面](https://www.bitcraze.io/products/crazyflie-bolt-1-1/)。

## 配置控制器与估计器
如果你想配置不同的控制器和估计器，请参见[配置页面](configure_estimator_controller.md)。

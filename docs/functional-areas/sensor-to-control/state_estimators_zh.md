---
title: 状态估计
page_id: state_estimators
---



状态估计器将传感器信号转换为对 Crazyflie 当前所处状态的估计。正如[概览页面](/docs/functional-areas/sensor-to-control/index.md)所述，这是 Crazyflie 稳定系统中至关重要的组成部分。状态估计在四旋翼飞行器（以及整个机器人领域）中极其重要。Crazyflie 首先需要知道自己处于什么角度（滚转 roll、俯仰 pitch、偏航 yaw）。如果它以几度的滚转角倾斜飞行，Crazyflie 就会朝那个方向加速。因此，控制器需要获得当前角度状态的良好估计并加以补偿。在更高的自主层级上，良好的位置估计也变得重要，因为你希望它能可靠地从 A 点移动到 B 点。

* [互补滤波器](#互补滤波器)
* [扩展卡尔曼滤波器](#扩展卡尔曼滤波器)
- [无迹卡尔曼滤波器](#无迹卡尔曼滤波器)（实验性）
* [参考文献](#参考文献)

## 互补滤波器

![complementary filter](/docs/images/complementary_filter.png){:width="500"}

互补滤波器是一种非常轻量且高效的滤波器，通常只使用 IMU 输出的陀螺仪（角速率）和加速度计数据。该估计器已扩展为同时支持 [Zranger 扩展板](https://store.bitcraze.io/collections/decks/products/z-ranger-deck-v2)的 ToF 距离测量输出。估计输出为 Crazyflie 的姿态（滚转、俯仰、偏航）和高度（Z 方向）。这些值可供控制器使用，主要用于手动控制模式。

实现细节请参见固件中的 `estimator_complementary.c` 和 `sensfusion6.c`。互补滤波器是 Crazyflie 固件上的默认状态估计器，除非安装了需要扩展卡尔曼滤波器的扩展板。

## 扩展卡尔曼滤波器

![extended kalman filter](/docs/images/extended_kalman_filter.png){:width="500"}

与互补滤波器相比，（扩展）卡尔曼滤波器（EKF）在复杂度上更进了一步，因为它接受来自内部和外部传感器的更多传感器输出。它是一种递归滤波器，基于传入的测量值（结合预测的噪声标准差）、测量模型以及系统自身的模型来估计 Crazyflie 的当前状态。

我们不在此详细展开，但鼓励大家通过阅读[相关资料](https://idsc.ethz.ch/education/lectures/recursive-estimation.html)来进一步学习 EKF。

由于具有更多的状态估计能力，对于某些能够提供**全位姿估计**（位置/速度 + 姿态）信息的扩展板，我们更倾向于使用 EKF。这些扩展板包括：[Flowdeck v2](https://store.bitcraze.io/collections/decks/products/flow-deck-v2)、[Loco 定位扩展板](https://store.bitcraze.io/collections/positioning/products/loco-positioning-deck)、[Lighthouse 扩展板](https://store.bitcraze.io/products/lighthouse-positioning-deck)、[被动式](https://store.bitcraze.io/products/motion-capture-marker-deck) / [主动式](https://store.bitcraze.io/products/active-marker-deck) 动作捕捉扩展板。这些扩展板的估计器偏好设置在其[扩展板 API](/docs/userguides/deck.md) 中定义。

下面将解释实现中的几个重要关键元素，同时也鼓励大家查看代码 `estimator_kalman.c` 和 `kalman_core.c` 的实现，以及阅读论文 [1] 和 [2] 了解实现细节。

### 卡尔曼监控器

卡尔曼滤波器有一个监控器（supervisor），如果状态估计值超出范围，它会重置状态估计。实现代码参见 `kalman_supervisor.c`。

### 测量模型

本节将解释传感器信号如何转换为状态估计。这些方程是 EKF 测量模型的基础。

* [Flowdeck 测量模型](#flowdeck-测量模型)
* [Locodeck 测量模型](/docs/functional-areas/loco-positioning-system/index.md)
* [Lighthouse 测量模型](/docs/functional-areas/lighthouse/kalman_measurement_model.md)


#### Flowdeck 测量模型

下图解释了来自 VL53L1x 传感器的高度数据和来自 PMW3901 传感器的光流数据如何结合起来计算速度。这是基于文献 [3] 的工作实现的。速度计算在 `kalman_core.c` 的 `kalmanCoreUpdateWithFlow()` 函数中，高度计算在 `mm_tof.c` 中。

![flowdeck velocity](/docs/images/flowdeck_velocity.png){:width="600"}

## 无迹卡尔曼滤波器
**注意**
*该算法仍处于实验阶段，需要在 [kbuild](/docs/development/kbuild.md) 中的 "Controllers and Estimators" 下启用 "Enable error-state UKF estimator"。*

误差状态无迹卡尔曼滤波器（error-state UKF）是 Crazyflie 的另一种导航算法，旨在提高依赖 Loco 定位系统时的导航质量。它将捷联导航算法 [4]（对加速度计和陀螺仪测量值进行时间积分）与无迹卡尔曼滤波器相结合，通过扩展板测量值来估计捷联导航解与真实位置之间的误差状态。与上述 EKF 一样，UKF 也是一种递归滤波器。然而，与 EKF 方法不同，UKF 不依赖对非线性测量方程进行线性化，而是使用确定性选择的 sigma 点来近似误差状态的均值和协方差。这提高了近似精度，在使用 Loco 定位系统进行绝对定位时尤为有利。该算法的更多细节见参考文献 [5]。关于推导误差状态预测模型以及 UKF 方法的更详细推导，可参见 [6] 和 [7]。

在当前实现中，该算法能够融合以下测量值：

* Loco 定位到达时间差（TdoA）
* Flow-Deck v2
 

Flow-deck 的飞行时间测量值还经过一种简单的异常值剔除方案处理，使 Crazyflie 能够在飞越地面障碍物时不会引起高度跳变。默认参数配置针对的是融合 Loco 定位和/或 Flow-Deck 测量值的 Crazyflie 2.1 四旋翼飞行器。当仅融合 Flow-Deck 测量值时，飞行时间测量值的质量门限（参数 "ukf.qualityGateTof"）需要提高。



## 参考文献
[1] Mueller, Mark W., Michael Hamer, and Raffaello D'Andrea. "Fusing ultra-wideband range measurements with accelerometers and rate gyroscopes for quadrocopter state estimation." 2015 IEEE International Conference on Robotics and Automation (ICRA). IEEE, 2015.

[2] Mueller, Mark W., Markus Hehn, and Raffaello D'Andrea. "Covariance correction step for kalman filtering with an attitude." Journal of Guidance, Control, and Dynamics 40.9 (2017): 2301-2306.

[3] M. Greiff, Modelling and Control of the Crazyflie Quadrotor for Aggressive and Autonomous Flight by Optical Flow Driven State Estimation, Master's thesis, Lund University, 2017

[4] D.H. Titterton, J.L. Weston, "Strapdown Inertial Navigation Technology - Second Edition", Institution of Electrical Engineers, 2004

[5] Kefferpütz, Klaus, McGuire, Kimberly. "Error-State Unscented Kalman-Filter for UAV Indoor Navigation", 25th International Conference on Information Fusion (FUSION), Linköping, Schweden, 2022

[6] J. Sola, "Quaternion kinematics for the error-state Kalman filter", arXiv:1711.02508, November 2017

[7] S.J. Julier, J.K. Uhlmann, "A New Extension of the Kalman Filter to Nonlinear Systems", Signal Processing, Sensor Fusion, and Target Recognition VI, Aerosense 97, Orlando, USA, 1997

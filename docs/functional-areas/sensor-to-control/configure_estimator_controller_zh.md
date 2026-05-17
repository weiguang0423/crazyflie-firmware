---
title: 配置估计器与控制器
page_id: config_est_ctrl
---

所有估计器和控制器都是标准固件构建的一部分，活跃的估计器和控制器可以在运行时或编译时进行配置。

## 估计器

可用的估计器在 `src/modules/interface/estimator.h` 中的 `StateEstimatorType` 枚举中定义。

### 运行时设置

要激活特定的估计器，请根据 `StateEstimatorType` 将 `stabilizer.estimator` 参数设置为相应的值。

该参数可以通过 [Python 客户端](https://www.bitcraze.io/documentation/repository/crazyflie-clients-python/master/)、[Python 库](https://www.bitcraze.io/documentation/repository/crazyflie-lib-python/master/) 或 [Crazyflie 板载应用](/docs/userguides/app_layer.md)来设置。

### 默认估计器

互补滤波器是默认的估计器。

某些扩展板需要使用卡尔曼估计器，当检测到这些扩展板时，系统会自动激活相应的估计器。激活的估计器基于 [扩展板驱动 API](/docs/userguides/deck.md) 中的 `.requiredEstimator` 成员。

### 编译时设置默认估计器

可以通过编译时设置 `ESTIMATOR` 来强制使用特定的估计器，参见 [在 kbuild 中配置构建](/docs/development/kbuild.md)。

示例：

`ESTIMATOR=kalman`

## 控制器

可用的控制器在 `src/modules/interface/controller.h` 中的 `ControllerType` 枚举中定义。

### 运行时设置

要激活特定的控制器，请根据 `ControllerType` 将 `stabilizer.controller` 参数设置为相应的值。

该参数可以通过 Python 客户端、Python 库或 Crazyflie 板载应用来设置。

### 默认控制器

PID 控制器是默认的控制器。

### 编译时设置

可以通过编译时设置 `CONTROLLER` 来强制使用特定的控制器，参见 [在 kbuild 中配置构建](/docs/development/kbuild.md)。

示例：

`CONTROLLER=Mellinger`

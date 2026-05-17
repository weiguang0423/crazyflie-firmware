/**
 *    ||          ____  _ __                           
 * +------+      / __ )(_) /_______________ _____  ___ 
 * | 0xBC |     / __  / / __/ ___/ ___/ __ `/_  / / _ \
 * +------+    / /_/ / / /_/ /__/ /  / /_/ / / /_/  __/
 *  ||  ||    /_____/_/\__/\___/_/   \__,_/ /___/\___/
 *
 * Crazyflie control firmware
 *
 * Copyright (C) 2011-2012 Bitcraze AB
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, in version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * main.c - Containing the main function.
 */

/* Personal configs */
#include "FreeRTOSConfig.h"

/* FreeRtos includes */
#include "FreeRTOS.h"
#include "task.h"

/* Project includes */
#include "config.h"
#include "platform.h"
#include "system.h"
#include "usec_time.h"

#include "led.h"

/* ST includes */
#include "stm32fxxx.h"

#include "bootloader.h"

int main() 
{
  check_enter_bootloader();

  //Initialize the platform.
  int err = platformInit();
  if (err != 0) {
    // The firmware is running on the wrong hardware. Halt
    while(1);
  }

  /* 启动用于初始化并开启所有程序的系统任务 */
  systemLaunch();

  /* 启动 FreeRTOS 调度器 */
  vTaskStartScheduler();

  //TODO: Move to platform launch failed
  ledInit();
  ledSet(0, 1);
  ledSet(1, 1);

  //Should never reach this point!
  while(1);

  return 0;
}


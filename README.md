# STM32F407 探索者开发板 BSP 说明

## 简介

本文档为 RT-Thread 开发团队为 STM32F407 探索者开发板提供的 BSP (板级支持包) 说明。

主要内容如下：

- 开发板资源介绍
- BSP 快速上手
- 进阶使用方法

通过阅读快速上手章节开发者可以快速地上手该 BSP，将 RT-Thread 运行在开发板上。在进阶使用指南章节，将会介绍更多高级功能，帮助开发者利用 RT-Thread 驱动更多板载资源。

## 开发板介绍

探索者 STM32F407 是正点原子推出的一款基于 ARM Cortex-M4 内核的开发板，最高主频为 168Mhz，该开发板具有丰富的板载资源，可以充分发挥 STM32F407 的芯片性能。

开发板外观如下图所示：

![board](figures/board.png)

该开发板常用 **板载资源** 如下：

- MCU：STM32F407ZGT6，主频 168MHz，1024KB FLASH ，192KB RAM
- 外部 RAM：IS62WV51216（1MB）
- 外部 FLASH：W25Q128（SPI，16MB）
- 常用外设
  - LED：2个，DS0（红色，PB1），DS1（绿色，PB0）
  - 按键，4个，KEY_UP（兼具唤醒功能，PIN：0），K0（PIN：68），K1（PIN：67），K2（PIN：66）
- 常用接口：USB 转串口、SD 卡接口、以太网接口、LCD 接口、USB SLAVE、USB HOST
- 调试接口，标准 JTAG/SWD

开发板更多详细信息请参考正点原子 [STM32 探索者开发板介绍](https://eboard.taobao.com/index.htm)。

## 外设支持

本 BSP 目前对外设的支持情况如下：

| **板载外设** | **支持情况** |               **备注**                |
| :------------ | :----------: | :-----------------------------------: |
| USB 转串口(COM1) |     支持     |                                    |
| COM2         |   支持  | 和以太网、PWM 冲突，如需使用该外设，请使用 CubeMX 重新配置 UART2 管脚 |
| COM3         |  支持   |                                           |
| MPU6050      |     支持     |                                       |
| Flash        |   支持   |                               |
| SRAM         |   支持   |  LVGL 会使用到，但此时不能启用 RT_USING_MEMHEAP_AS_HEAP 内存算法 |
| TFTLCD       |   支持   | F407 不带 LTDC 外设，所以需要使用 MCU LCD，而不能直接驱动 RGB 屏幕 |
| LCD-TOUCH    |   支持   | 仅测试过 GT9147, 其他触摸驱动需要添加适配 |
| SD卡         |   支持   | 支持FATFS文件系统 |
| W25Q128 | 支持 | 支持LittleFS文件系统 |
| 以太网       |   支持   |                               |
| WM8978       |   暂不支持   |                                       |
| **片上外设** | **支持情况** |               **备注**                |
| GPIO         |     支持     | PA0, PA1... PH1 ---> PIN: 0, 1...144 |
| UART         |     支持     |              UART1/2/3              |
| SPI          |     支持     |               SPI1/2/3                |
| I2C          |     支持     |      软件 I2C1, I2C2[仅触摸屏使用]     |
| ADC          |     支持     |                                       |
| RTC          |     支持     | 支持外部晶振和内部低速时钟 |
| WDT          |     支持     |                                       |
| FLASH | 支持 | 已适配 [FAL](https://github.com/RT-Thread-packages/fal) |
| PWM          |   支持   |                               |
| USB Device   |   支持   |                               |
| USB Host     |   支持   |                               |
| **扩展模块** | **支持情况** |               **备注**                |
| ATK-ESP8266 模块 | 支持 | COM3 |


## 使用说明

使用说明分为如下两个章节：

- 快速上手

    本章节是为刚接触 RT-Thread 的新手准备的使用说明，遵循简单的步骤即可将 RT-Thread 操作系统运行在该开发板上，看到实验效果 。

- 进阶使用

    本章节是为需要在 RT-Thread 操作系统上使用更多开发板资源的开发者准备的。通过使用 ENV 工具对 BSP 进行配置，可以开启更多板载资源，实现更多高级功能。


### 快速上手

本 BSP 为开发者提供 MDK4、MDK5 和 IAR 工程，并且支持 GCC 开发环境。下面以 MDK5 开发环境为例，介绍如何将系统运行起来。

#### 硬件连接

使用数据线连接开发板到 PC，打开电源开关。

#### 编译下载

双击 project.uvprojx 文件，打开 MDK5 工程，编译并下载程序到开发板。

> 工程默认配置使用 JLink 下载程序，在通过 JLink 连接开发板的基础上，点击下载按钮即可下载程序到开发板

#### 运行结果

下载程序成功之后，系统会自动运行，观察开发板上 LED 的运行效果，红色 LED 常亮、绿色 LED 会周期性闪烁。

连接开发板对应串口到 PC , 在终端工具里打开相应的串口（115200-8-1-N），复位设备后，可以看到 RT-Thread 的输出信息:

> 注：正点原子开发板 在使用终端工具如：PuTTy、XShell 时，会出现系统不能启动的问题，推荐使用串口调试助手如：sscom

```bash
 \ | /
- RT -     Thread Operating System
 / | \     3.1.1 build Nov 19 2018
 2006 - 2018 Copyright by rt-thread team
msh >
```
### 进阶使用

此 BSP 默认只开启了 GPIO 和 串口1 的功能，如果需使用 SD 卡、Flash 等更多高级功能，需要利用 ENV 工具对BSP 进行配置，步骤如下：

1. 在 bsp 下打开 env 工具。

2. 输入`menuconfig`命令配置工程，配置好之后保存退出。

3. 输入`pkgs --update`命令更新软件包。

4. 输入`scons --target=mdk4/mdk5/iar` 命令重新生成工程。

本章节更多详细的介绍请参考 [STM32 系列 BSP 外设驱动使用教程](../docs/STM32系列BSP外设驱动使用教程.md)。

## 注意事项

1. 编译报错找不到“select.h”的问题

   打开env进入**RT-Thread Components-->POSIX layer and C standard library**取消勾选`Enable POSIX layer for compatibility with UNIX APIs, poll/select etc`选项。

2. 如何打开littlefs并使用spiflash设备

   打开env进入**Hardware Drivers Config-->Onboard Peripheral Drivers-->Enable File System**勾选`Enable SPI-FLASH (LittleFS)`选项。

   格式化命令：`mkfs -t lfs spiflash`

3. CmBacktrace使用注意事项

   注意链接脚本中的**_system_stack_size**不能为0,否则将导致发生错误时CmBacktrace无法工作，此时可以修改此值使之不为0,也可修改**cm_backtrace.c**文件：屏蔽掉第235行的如下代码：

   ```c
   /* if (main_stack_size == 0) { */
   /*     cmb_println(print_info[PRINT_MAIN_STACK_CFG_ERROR]); */
   /*     return; */
   /* } */
   ```

4. 如何修改程序的运行其起始地址：需要修改board/linker_scripts/link.lds下的`CODE (rx) : ORIGIN = 0x08020000, LENGTH = 1024k /* 1024KB flash */`字段。并且修改board/CubeMX_Config/Src/system_stm32f4xx.c下的`VECT_TAB_OFFSET`宏修改中断向量表偏移值。
   
5. 按照rtconfig.h生成.config文件命令：`scons --genconfig`

6. RT-thread 4.1版本中自带了fal组件，故原有依赖fal的软件包在下载完fal组件后，删除掉即可。

7. 使用Qboot组件制作bootloader时，需要将线程栈的大小调大，否则可能导致越界。

## 联系人信息

维护人:

- [guozhanxin](https://github.com/Guozhanxin)
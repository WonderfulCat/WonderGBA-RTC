# WonderGBA-RTC

> **GBA Cartridge RTC Editor / GBA卡带实时时钟调节工具**
>
> *Developed by WonderCat* | [WonderGBA-RTC]([http://furlocks-forest.net](https://github.com/WonderfulCat/WonderGBA-RTC))

![WonderGBA-RTC Logo](WonderGBA-RTC.png)

---

## 📝 项目简介 / Introduction

### 简介 / Short Description
* **中文：** 一款专为 GBA 自制卡带打造的 RTC（实时时钟）读写与调节工具，支持 GBA 和 NDS 双端运行。
* **English:** A GBA and NDS homebrew tool designed to read, write, and calibrate Real-Time Clocks (RTC) on custom GBA cartridges.

### 详细描述 / Detailed Description
#### 中文：
**WonderGBA-RTC** 是一款专门用于调整 GBA 卡带实时时钟（RTC）的开源工具，目前提供 **GBA** 和 **NDS** 两个运行版本。

本工具最初是为了解决一些自制 RTC 烧录卡的时序同步问题而开发，并对底层通信时序进行了针对性的微调与优化。在开发过程中，本项目主要参考了开源项目 [GBA_RTCRead](https://github.com/megaboyexe/GBA_RTCRead) 的思路，并且该项目的大部分核心代码与结构是在 AI 的辅助下协作完成。

#### English:
**WonderGBA-RTC** is an open-source utility dedicated to configuring and calibrating the Real-Time Clock (RTC) on GBA cartridges, available in both **GBA** and **NDS** native versions.

This tool was specifically developed to resolve synchronization issues found in certain custom-built RTC flashcarts, featuring tailored micro-adjustments to the low-level hardware communication timings. The development of this project was heavily inspired by the [GBA_RTCRead](https://github.com/megaboyexe/GBA_RTCRead) repository, and a significant portion of the source code and logic was co-authored and implemented with the assistance of AI.

---

## ⚠️ 免责声明 / Disclaimer

* **中文：** 本程序目前**仅在作者个人的定制卡带上进行了完整的硬件测试**。由于市面上各类自制卡、改版卡及烧录卡的硬件设计、总线驱动能力和电平特性各不相同，**本工具对其他卡带的兼容性完全未知**。因使用本工具导致卡带数据丢失、硬件损坏或芯片死锁等任何不良后果，作者概不承担任何责任。请在测试前自行备份存档。
* **English:** This software has **only been verified and tested on the author's own custom cartridges**. Due to variances in hardware design, bus drive capabilities, and voltage level characteristics across different custom carts or flashcarts on the market, **compatibility with other third-party hardware is completely unknown**. The author assumes no responsibility for any data loss, hardware damage, or chip bricking caused by using this tool. Use it at your own risk.

---

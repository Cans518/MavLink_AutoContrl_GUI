# 电创（1）结项项目 —— ArduPilot自动控制GUI程序
> 项目名称：ArduPilot自动控制GUI程序
> 上海大学中欧工程技术学院2023~2024学年冬季学期《电子创新技术（1）》结项项目
> 作者： Mzee
> 说明：使用C语言通过MavLink协议和Udp协议实现与飞控的通信，实现自动控制功能。

# 项目简介
 在Linux下使用`GTK+-3.0`库实现一个图形界面，实现自动降落功能。
 通信软件协议使用`MavLink`协议，通信方式使用`Udp`协议。
 GUI上展示飞行器的`飞行高度`和`飞行模式`
 GUI同时提供`自动降落`，`自动起飞`，`油门解锁`，`油门上锁`按钮。

# 项目构建
 1. 安装`gtk+-3.0`库：
    ```shell
    sudo ./envirment_install.sh
    ```
    或者可以使用`make`进行安装
    ```shell
    make install_env
    ```
 2. 安装`MavLink`库：
   在仓库`Lib`目录下已经提供 `MavLink-v2.0`的c语言构建本地库，可以直接使用。

 3. 编译源码：
   本项目已经提供`Makefile`，直接在项目根目录下执行`make`即可。
    ```shell
    make
    ```

# 项目功能
 GUI实现`Ardupliot`项目下的无人机控制，实现`一键降落`，`一键起飞`，`解锁油门`，`上锁油门`

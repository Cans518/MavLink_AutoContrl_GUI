// 图形化头文件引入，GTK+-3.0
#include <gtk/gtk.h>

// 基本库，输入输出库引入
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#include <termios.h>
#include <stdbool.h>
#include <sys/ioctl.h>

// 网络通信库引入
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

// 通信协议mavlink引入
#include <common/mavlink.h>

// 定义常量
#define MAVLINK_SYSTEM_ID_SELF 1    // 自身系统ID
#define MAVLINK_COMPONENT_ID_SELF 0 // 自身组件ID
#define TARGET_IP "127.0.0.1"       // 本地ip
#define UDP_PORT 14550              // 本地UDP端口（接收）
#define ARDUPILOT_IP TARGET_IP      // Ardupilot的IP地址
#define ARDUPILOT_PORT UDP_PORT     // 默认的MAVLink端口


const char *flightModeStrings[] = {
    "STABILIZE", // 0 稳定模式
    "ACRO",      // 1 俯仰
    "ALT_HOLD",  // 2 定高模式
    "AUTO",      // 3 自动模式
    "GUIDED",    // 4 导航模式
    "LOITER",    // 5 悬停模式
    "RTL",       // 6 返航模式
    "CIRCLE",    // 7 环绕模式
    "POSITION",  // 8 定点模式
    "LAND",      // 9 降落模式
    "OF_LOITER", // 10 光流悬停模式
    "DRIFT",     // 11 机漂移模式
    "SPORT",     // 12 速度模式
    "FLIP",      // 13 翻滚模式
    "AUTOTUNE",  // 14 自动调整模式
    "POSHOLD",   // 15 位置保持模式
    "BRAKE",     // 16 刹车模式
    "ALT_FLIP",  // 17 高度翻滚模式
};

// 信息储存的额外结构体创建

// 定义结构体储存更新信息
typedef struct
{
    float altitude;
    int modes;
    int sysid;
    int compid;
} Update_date;
// 窗口结构体定义
typedef struct
{
    GtkWidget *window1;
    GtkWidget *window2;
    GtkWidget *window3;
} S_Windows;

// 定义结构体储存IP和端口
typedef struct
{
    char ip[20];
    int port;
} Ip_Info;

// 创建全局变量
Ip_Info ipinfo;
S_Windows windows_s;
Update_date Info;

int tx_port = -1; // 默认输出端口为空，等待读取到
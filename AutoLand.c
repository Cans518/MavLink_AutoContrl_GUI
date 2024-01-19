// 图形化头文件引入，GTK+-3.0
#include <gtk/gtk.h>


// 基本库，输入输出库引入
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <unistd.h>
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
#define MAVLINK_SYSTEM_ID 1
#define MAVLINK_COMPONENT_ID 1
#define MAVLINK_UDP_PORT 5501   // 发送Mavlink消息端口
#define TARGET_IP "127.0.0.1"   // 本地ip
#define UDP_PORT 14550          // 本地UDP端口（接收）
#define ARDUPILOT_IP TARGET_IP  // Ardupilot的IP地址
#define ARDUPILOT_PORT UDP_PORT // 默认的MAVLink端口

const char* flightModeStrings[] = {
    "STABILIZE",
    "ACRO",
    "ALT_HOLD",
    "AUTO",
    "GUIDED",
    "NONE",
    "RTL",
    "",
    "",
    "LAND"
    // 添加其他模式
};

// 初始化高度
float currentAltitude = 0.0;
int mode = 0;

typedef struct{
    float altitude;
    int modes;
}Update_date;

Update_date Info;

/*
* @brief 发送命令
* @param MAV_CMD_NAV_LAND
* @param 1
* @param 1
* @param 1
* @param 0 0 0 0 0 0 0
*/
void mav_Send()
{
    // 创建UDP套接字
    int udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_socket == -1)
    {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    // 设置目标地址
    struct sockaddr_in target_addr;
    memset(&target_addr, 0, sizeof(target_addr));
    target_addr.sin_family = AF_INET;
    target_addr.sin_port = htons(MAVLINK_UDP_PORT);
    if (inet_pton(AF_INET, TARGET_IP, &target_addr.sin_addr) <= 0)
    {
        perror("Error setting up target address");
        close(udp_socket);
        exit(EXIT_FAILURE);
    }

    // 创建MAVLink消息
    mavlink_message_t msg; // 消息结构体
    mavlink_msg_command_long_pack(
        MAVLINK_SYSTEM_ID, MAVLINK_COMPONENT_ID, &msg,
        MAV_CMD_NAV_LAND,   // Land命令
        1,                  // 确认
        1,                  // 目标系统
        1,                  // 目标组件
        0, 0, 0, 0, 0, 0, 0 // 参数（在Land模式中未使用）
    );

    // 序列化MAVLink消息
    uint8_t buf[MAVLINK_MAX_PACKET_LEN];
    uint16_t len = mavlink_msg_to_send_buffer(buf, &msg);

    // 通过UDP发送序列化的消息
    if (sendto(udp_socket, buf, len, 0, (struct sockaddr *)&target_addr, sizeof(target_addr)) == -1)
        perror("Error sending MAVLink message");

    // 关闭UDP
    close(udp_socket);
}

/*
* @brief 获取高度
* @return 高度
*/
Update_date get_high()
{

    Update_date date;
    bool get_h, get_m;
    get_h = true;
    get_m = true;

    // 创建UDP socket
    int udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udpSocket < 0)
    {
        perror("Error creating UDP socket");
    }

    // 设置本地地址
    struct sockaddr_in localAddr;
    memset(&localAddr, 0, sizeof(localAddr));
    localAddr.sin_family = AF_INET;
    localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    localAddr.sin_port = htons(UDP_PORT);

    // 绑定本地地址到UDP socket
    if (bind(udpSocket, (struct sockaddr *)&localAddr, sizeof(localAddr)) < 0)
    {
        perror("Error binding UDP socket");
        close(udpSocket);
    }

    // 设置Ardupilot地址
    struct sockaddr_in ardupilotAddr;
    memset(&ardupilotAddr, 0, sizeof(ardupilotAddr));
    ardupilotAddr.sin_family = AF_INET;
    ardupilotAddr.sin_addr.s_addr = inet_addr(ARDUPILOT_IP);
    ardupilotAddr.sin_port = htons(ARDUPILOT_PORT);

    // Mavlink变量
    mavlink_message_t msg;
    uint8_t buf[MAVLINK_MAX_PACKET_LEN];



    while (get_h || get_m)
    {
        // 接收Mavlink消息
        mavlink_message_t msg;
        mavlink_status_t status;
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];

        ssize_t len = recv(udpSocket, buffer, sizeof(buffer), 0);
        for (ssize_t i = 0; i < len; ++i)
        {
            if (mavlink_parse_char(MAVLINK_COMM_0, buffer[i], &msg, &status))
            {
                // Mavlink消息已解析
                if (msg.msgid == MAVLINK_MSG_ID_GLOBAL_POSITION_INT)
                {
                    // 如果是全局位置消息
                    mavlink_global_position_int_t positionData;
                    mavlink_msg_global_position_int_decode(&msg, &positionData); // 解码消息
                    date.altitude = (positionData.alt / 1000.0) - 584;
                    get_h = false;
                }
                    
                if (msg.msgid == MAVLINK_MSG_ID_HEARTBEAT) {
                    // 检查消息类型是否为心跳消息
                    mavlink_heartbeat_t heartbeat;
                    mavlink_msg_heartbeat_decode(&msg, &heartbeat);
                    date.modes = heartbeat.custom_mode;
                    get_m = false;
                }
                if (!(get_h && get_m))
                    break;  
            }
        }
    }
    // 关闭UDP socket
    close(udpSocket);
    return date;
}

/*
* @brief 更新高度
* @param data 高度标签
* @return G_SOURCE_CONTINUE
*/
gboolean updateAltitude(gpointer data)
{
    Update_date info;
    info = get_high();
    currentAltitude = info.altitude;
    if (currentAltitude <= 0.1)
        currentAltitude = 0;
    // 更新高度标签中的值
    GtkWidget *label = GTK_WIDGET(data);
    char buff[100];
    sprintf(buff, "The mode : %s \n 当前高度: %.2fm", flightModeStrings[info.modes], currentAltitude);
    gtk_label_set_text(GTK_LABEL(label), buff);

    return G_SOURCE_CONTINUE;
}

/*
* @brief 按钮点击事件响应
*/
void onButtonClicked(GtkWidget *widget, gpointer data)
{
    //mav_Send();
    system("./Script/b \"mode land\"");
}

void onButtonClicked2(GtkWidget *widget, gpointer data)
{
    system("./Script/a");
}

int main(int argc, char *argv[])
{
    // 初始化 GTK+
    gtk_init(&argc, &argv);

    // 创建窗口
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL); // 顶层窗口
    gtk_window_set_title(GTK_WINDOW(window), "AutoControl APP"); // 设置窗口标题
    gtk_container_set_border_width(GTK_CONTAINER(window), 10); 

    // 创建一个垂直布局
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5); // 垂直布局
    gtk_container_add(GTK_CONTAINER(window), vbox); // 将布局添加到窗口

    // 创建一个标签来展示高度
    char buffer[50];
    sprintf(buffer, "当前高度: %.2fm", currentAltitude);
    GtkWidget *altitudeLabel = gtk_label_new(buffer); // 标签
    gtk_widget_set_size_request(altitudeLabel,280,10);
    gtk_box_pack_start(GTK_BOX(vbox), altitudeLabel, TRUE, TRUE, 0); // 将标签添加到布局

    // 创建一个按键
    GtkWidget *button = gtk_button_new_with_label("Auto Land"); // 按键
    gtk_widget_set_size_request(button, 50, 10);
    gtk_box_pack_start(GTK_BOX(vbox), button, false, false, 0); // 将按键添加到布局

    // 链接点击事件与回调函数
    g_signal_connect(button, "clicked", G_CALLBACK(onButtonClicked), NULL); // 链接点击事件

    // 创建一个按键
    GtkWidget *button2 = gtk_button_new_with_label("Auto Fly"); // 按键
    gtk_widget_set_size_request(button2,10,10);
    gtk_box_pack_start(GTK_BOX(vbox), button2, false, false, 0); // 将按键添加到布局

    // 链接点击事件与回调函数
    g_signal_connect(button2, "clicked", G_CALLBACK(onButtonClicked2), NULL); // 链接点击事件

    // 设置更新高度的定时器
    g_timeout_add_seconds(1, updateAltitude, altitudeLabel); // 每隔1秒更新一次高度

    // 显示窗口
    gtk_widget_show_all(window); 

    // 运行主循环
    gtk_main();

    return 0;
}
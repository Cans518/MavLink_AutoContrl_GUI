#include "AutoLand.h"


/*
* @brief 发送消息到ArduPilot
* @param cmd 写好的MavLink命令
*/
void send_to_ardupilot(mavlink_command_long_t cmd){
    // Encode
    mavlink_message_t message;
    mavlink_msg_command_long_encode(1, 0, &message, &cmd);

    // 将消息序列化为字节数组
    uint8_t buf[MAVLINK_MAX_PACKET_LEN];
    uint16_t len = mavlink_msg_to_send_buffer(buf, &message);

    // 设置ArduPilot SITL的IP地址和端口
    const char *target_ip = "127.0.0.1"; // ArduPilot SITL的IP地址
    const int target_port = tx_port;     // ArduPilot SITL监听的端口

    // 创建UDP套接字
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    // 设置目标地址
    struct sockaddr_in target_addr;
    memset(&target_addr, 0, sizeof(target_addr));
    target_addr.sin_family = AF_INET;
    target_addr.sin_port = htons(target_port);
    inet_pton(AF_INET, target_ip, &target_addr.sin_addr);

    // 发送数据
    sendto(sockfd, buf, len, 0, (const struct sockaddr *)&target_addr, sizeof(target_addr));

    // 关闭套接字
    close(sockfd);
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

                if (msg.msgid == MAVLINK_MSG_ID_HEARTBEAT)
                {
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
    Info = get_high();
    if (Info.altitude <= 0.1)
        Info.altitude = 0;
    // 更新高度标签中的值
    GtkWidget *label = GTK_WIDGET(data);
    char buff[50];
    sprintf(buff, "The mode : %s \n 当前高度: %.2fm", flightModeStrings[Info.modes], Info.altitude);
    gtk_label_set_text(GTK_LABEL(label), buff);

    return G_SOURCE_CONTINUE;
}

/*
 * @brief 更新IP信息标签
 * @param data IP信息标签
 */
gboolean updateipinfo(gpointer data)
{
    // 更新高度标签中的值
    GtkWidget *label = GTK_WIDGET(data);
    char buff[50];
    sprintf(buff, "Listening to %s:%d", ipinfo.ip, ipinfo.port);
    gtk_label_set_text(GTK_LABEL(label), buff);

    return G_SOURCE_CONTINUE;
}

/*
 * @brief 自动降落，AutoLand按钮回调函数
 * @param widget 自动降落按钮
 * @param data NULL
 */
void Auto_Land(GtkWidget *widget, gpointer data)
{
    // 切换到Land模式的Mavlink命令
    mavlink_command_long_t cmd = {0};
    cmd.target_system = 1; // 目标系统ID
    cmd.target_component = Info.compid;
    cmd.command = MAV_CMD_NAV_LAND;
    cmd.param1 = 0;

    send_to_ardupilot(cmd);
}

/*
 * @brief 切换到Guided模式
 * @param NULL
 */
void Mode_Guided()
{
    // 切换到Guided模式的Mavlink命令
    mavlink_command_long_t cmd = {0};
    cmd.target_system = 1; // 目标系统ID（根据实际情况修改）
    cmd.target_component = Info.compid;
    cmd.command = MAV_CMD_DO_SET_MODE;
    cmd.confirmation = true;
    cmd.param1 = 81;
    cmd.param2 = 4;

    send_to_ardupilot(cmd);
}

/*
 * @brief 起飞
 * @param high 起飞需要达到的高度
 */
void Take_Off(float high)
{
    // Takeoff
    mavlink_command_long_t cmd = {0};
    cmd.target_system = 1; // 目标系统ID（根据实际情况修改）
    cmd.target_component = Info.compid;
    cmd.command = MAV_CMD_NAV_TAKEOFF;
    cmd.param7 = high;

    send_to_ardupilot(cmd);
}

/*
 * @brief 解锁油门
 * @param NULL
 */
void Arm()
{
    bool flag = true;

    // Prepare command for off-board mode
    mavlink_command_long_t cmd = {0};
    cmd.target_system = 1;
    cmd.target_component = Info.compid;
    cmd.command = MAV_CMD_COMPONENT_ARM_DISARM;
    cmd.confirmation = true;
    cmd.param1 = (float)(flag);

    send_to_ardupilot(cmd);
}

/*
 * @brief 锁油门
 * @param NULL
 */
void DIS_Arm()
{
    bool flag = true;

    mavlink_command_long_t cmd = {0};
    cmd.target_system = 1;
    cmd.target_component = Info.compid;
    cmd.command = MAV_CMD_COMPONENT_ARM_DISARM;
    cmd.confirmation = true;
    cmd.param1 = (float)(!flag);

    send_to_ardupilot(cmd);
}

/*
 * @brief 自动起飞
 * @param widget
 * @param data
 */
void Auto_Fly(GtkWidget *widget, gpointer data)
{
    gtk_widget_show_all(windows_s.window3);
    gtk_widget_hide(windows_s.window2);
}

/*
 * @brief 判断是否是本机IP
 * @param addr
 */
void checkAndRetrievePort(struct sockaddr_in *addr)
{
    if (strcmp(inet_ntoa(addr->sin_addr), "127.0.0.1") == 0)
    {
        tx_port = ntohs(addr->sin_port);
        printf("IP地址匹配，端口号为：%d\n", tx_port);
    }
    else
    {
        printf("IP地址不匹配\n");
    }
}

/*
 * @brief 获取写入的tx_port
 * @param NULL
 */
void get_tx_port()
{
    int socket_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    // 创建UDP套接字
    socket_fd = socket(AF_INET, SOCK_DGRAM, 0);

    // 设置服务器地址结构体
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(14550);

    // 绑定套接字
    bind(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));

    printf("等待数据...\n");

    // 读取数据
    char buffer[1024];
    mavlink_message_t msg;
    mavlink_status_t status;
    bool get_id = false;
    ssize_t len;
    len = recvfrom(socket_fd, buffer, sizeof(buffer), 0, (struct sockaddr *)&client_addr, &client_addr_len);
    checkAndRetrievePort(&client_addr);

get_id_loop:
    for (ssize_t i = 0; i < len; ++i)
    {
        if (mavlink_parse_char(MAVLINK_COMM_0, buffer[i], &msg, &status))
        {
            // Message decoded, check message ID
            switch (msg.msgid)
            {
            case MAVLINK_MSG_ID_HEARTBEAT:
            {
                mavlink_heartbeat_t heartbeat;
                mavlink_msg_heartbeat_decode(&msg, &heartbeat);

                Info.sysid = msg.sysid;
                Info.compid = msg.compid;
                Info.modes = heartbeat.custom_mode;

                get_id = true;
                printf("SysID: %d, CompID: %d, Custom Mode: %u\n", Info.sysid, Info.compid, Info.modes);
                break;
            }
            }
        }
    }

    if (get_id == false)
    {
        len = recvfrom(socket_fd, buffer, sizeof(buffer), 0, (struct sockaddr *)&client_addr, &client_addr_len);
        goto get_id_loop;
    }
    // 关闭套接字
    close(socket_fd);
    return;
}

/*
 * @brief 确定高度，确定起飞的高度
 * @param widget
 * @param data
*/
void ok_button_cb(GtkWidget *widget,gpointer data){
    const gchar *value =  gtk_entry_get_text(GTK_ENTRY((GtkWidget *)data));
    if (g_strcmp0(value, "") != 0 ){
        Info.high = atoi(value);
        gtk_widget_show_all(windows_s.window2);
        gtk_widget_hide(windows_s.window3);
        Mode_Guided();
        sleep(1);
        Arm();
        sleep(1);
        printf("%f",Info.high);
        Take_Off(Info.high);
    }

}

/*
 * @brief 读取窗口输入
 * @param widget
 * @param data
 */
void read_input(GtkWidget *widget, gpointer data)
{
    // 获取IP输入框中的文本
    const gchar *ip_text = gtk_entry_get_text(GTK_ENTRY((GtkWidget *)data));

    // 获取端口输入框中的文本
    GtkWidget *port_entry = GTK_WIDGET(g_object_get_data(G_OBJECT(widget), "port_entry"));
    const gchar *port_text = gtk_entry_get_text(GTK_ENTRY(port_entry));

    // 检查是否有输入
    if (g_strcmp0(ip_text, "") != 0 && g_strcmp0(port_text, "") != 0)
    {
        // 如果有输入，跳转到下一个界面
        strcpy(ipinfo.ip,ip_text);
        ipinfo.port = atoi(port_text);
        gtk_widget_hide(windows_s.window1);
        gtk_widget_show_all(windows_s.window2);
    }
    else
    {
        // 如果没有输入，弹窗提示请输入
        GtkWidget *dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR,
                                                   GTK_BUTTONS_OK, "Please enter both IP and Port");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
    }
}

int main(int argc, char *argv[])
{
    get_tx_port(); // 获取tx_port

    // 初始化Info
    Info.sysid = 1;
    Info.compid = 1;

    // 初始化 GTK+
    gtk_init(&argc, &argv);

    windows_s.window1 = gtk_window_new(GTK_WINDOW_TOPLEVEL);                          // 创建窗口
    gtk_window_set_title(GTK_WINDOW(windows_s.window1), "AutoControl APP IP Config"); // 设置窗口标题

    // 创建横向布局
    GtkWidget *grid = gtk_grid_new();
    gtk_container_add(GTK_CONTAINER(windows_s.window1), grid);

    // 创建IP输入框
    GtkWidget *ip_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(ip_entry), "Enter IP"); // 设置占位文本
    gtk_grid_attach(GTK_GRID(grid), ip_entry, 0, 0, 1, 1);           // 将输入框附加到网格中

    // 创建端口输入框
    GtkWidget *port_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(port_entry), "Enter Port"); // 设置占位文本
    gtk_grid_attach(GTK_GRID(grid), port_entry, 1, 0, 1, 1);             // 将输入框附加到网格中

    // 创建确认按钮
    GtkWidget *confirm_button = gtk_button_new_with_label("Confirm"); // 确认按钮
    gtk_grid_attach(GTK_GRID(grid), confirm_button, 0, 1, 2, 1);      // 将按钮附加到网格中

    // 将端口输入框作为数据附加到确认按钮
    g_object_set_data(G_OBJECT(confirm_button), "port_entry", port_entry);

    // 连接确认按钮的点击事件到回调函数
    g_signal_connect(confirm_button, "clicked", G_CALLBACK(read_input), (gpointer)ip_entry);

    // 创建窗口
    windows_s.window2 = gtk_window_new(GTK_WINDOW_TOPLEVEL);                // 顶层窗口
    gtk_window_set_title(GTK_WINDOW(windows_s.window2), "AutoControl APP"); // 设置窗口标题
    gtk_container_set_border_width(GTK_CONTAINER(windows_s.window2), 10);

    // 创建一个垂直布局
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5); // 垂直布局
    gtk_container_add(GTK_CONTAINER(windows_s.window2), vbox);  // 将布局添加到窗口

    char ip_info_buffer[50];
    sprintf(ip_info_buffer, "Listening to %s:%d", ipinfo.ip, ipinfo.port);
    GtkWidget *ip_info_Label = gtk_label_new(ip_info_buffer); // 标签
    gtk_widget_set_size_request(ip_info_Label, 280, 10);
    gtk_box_pack_start(GTK_BOX(vbox), ip_info_Label, TRUE, TRUE, 0); // 将标签添加到布局

    // 创建一个标签来展示高度
    char buffer[50];
    sprintf(buffer, "The mode : %s \n 当前高度: %.2fm", flightModeStrings[Info.modes], Info.altitude);
    GtkWidget *altitudeLabel = gtk_label_new(buffer); // 标签
    gtk_widget_set_size_request(altitudeLabel, 280, 10);
    gtk_box_pack_start(GTK_BOX(vbox), altitudeLabel, TRUE, TRUE, 0); // 将标签添加到布局

    // 创建land按键
    GtkWidget *Auto_Land_button = gtk_button_new_with_label("Auto Land"); // 按键
    gtk_widget_set_size_request(Auto_Land_button, 50, 10);
    gtk_box_pack_start(GTK_BOX(vbox), Auto_Land_button, false, false, 0); // 将按键添加到布局

    // 链接点击事件与回调函数
    g_signal_connect(Auto_Land_button, "clicked", G_CALLBACK(Auto_Land), NULL); // 链接点击事件

    // 创建一个按键
    GtkWidget *Auto_Fly_button = gtk_button_new_with_label("Auto Fly"); // 按键
    gtk_widget_set_size_request(Auto_Fly_button, 10, 10);
    gtk_box_pack_start(GTK_BOX(vbox), Auto_Fly_button, false, false, 0); // 将按键添加到布局

    // 链接点击事件与回调函数
    g_signal_connect(Auto_Fly_button, "clicked", G_CALLBACK(Auto_Fly), NULL); // 链接点击事件

    // 创建横向布局
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, false, false, 0);

    // 创建一个按键
    GtkWidget *arm_button = gtk_button_new_with_label("    arm    "); // 按键
    gtk_widget_set_size_request(arm_button, 5, 10);
    gtk_box_pack_start(GTK_BOX(hbox), arm_button, false, false, 0); // 将按键添加到布局

    // 链接点击事件与回调函数
    g_signal_connect(arm_button, "clicked", G_CALLBACK(Arm), NULL); // 链接点击事件

    // 创建一个按键
    GtkWidget *disarm_button = gtk_button_new_with_label("  disarm  "); // 按键
    gtk_widget_set_size_request(disarm_button, 5, 10);
    gtk_box_pack_start(GTK_BOX(hbox), disarm_button, false, false, 0); // 将按键添加到布局
    // 链接点击事件与回调函数
    g_signal_connect(disarm_button, "clicked", G_CALLBACK(DIS_Arm), NULL); // 链接点击事件

    windows_s.window3 = gtk_window_new(GTK_WINDOW_TOPLEVEL); 
    gtk_window_set_title(GTK_WINDOW(windows_s.window3), "Input");

    GtkWidget *sbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5); 
    gtk_container_add(GTK_CONTAINER(windows_s.window3), sbox);

    GtkWidget *entry_h = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_h), "Enter the high"); // 设置占位文本
    gtk_box_pack_start(GTK_BOX(sbox), entry_h, false, false, 0);           // 将输入框附加到网格中

    GtkWidget* ok_button = gtk_button_new_with_label("OK"); // 创建一个按钮
    gtk_box_pack_start(GTK_BOX(sbox), ok_button, false, false, 0); // 将按钮附加到网格中
    g_signal_connect(ok_button, "clicked", G_CALLBACK(ok_button_cb), entry_h); // 链接点击事件与回调函数

    // 设置更新高度的定时器
    g_timeout_add_seconds(1, updateAltitude, altitudeLabel); // 每隔1秒更新一次高度
    g_timeout_add_seconds(1, updateipinfo, ip_info_Label);   // 每隔1秒更新一次IP信息

    // 设置窗口位置为默认居中
    gtk_window_set_position(GTK_WINDOW(windows_s.window1), GTK_WIN_POS_CENTER_ALWAYS);
    gtk_window_set_position(GTK_WINDOW(windows_s.window2), GTK_WIN_POS_CENTER_ALWAYS);
    gtk_window_set_position(GTK_WINDOW(windows_s.window3), GTK_WIN_POS_CENTER_ALWAYS);

    // 链接信号与回调函数, 用于退出窗口并且退出程序
    g_signal_connect(windows_s.window1, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(windows_s.window2, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(windows_s.window3, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    // 显示窗口
    gtk_widget_show_all(windows_s.window1);

    // 运行主循环
    gtk_main();

    return 0;
}
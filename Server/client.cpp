#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "Winsock.h"
#include "windows.h"
#include "time.h"
#include "stdio.h"
#include <iostream>
using namespace std;

#pragma comment(lib, "wsock32.lib")	

char fileName[20];
char readBuffer[1024];		//接收缓冲区
char sendBuffer[1024];		//发送缓冲区
SOCKET client_socket;		//客户端对象
sockaddr_in server_addr;	//服务器地址

//初始化WSA，使得程序可以调用windows socket
int initSocket() {
	WORD sockVersion = MAKEWORD(2, 2);
	WSADATA wsaData;
	if (WSAStartup(sockVersion, &wsaData) != 0) {
		cout << "socket初始化失败" << endl;
		return -1;
	}
	//设置地址结构
	//AF_INET使用 TCP/IPv4 进行通信
	//服务器IP，转化成二进制IPV4地址
	//设置端口号，htons用于将主机字节序改为网络字节序
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	server_addr.sin_port = htons(8080);
	return 1;
}
/*创建监听用套接字，server_socket
	并检测是否创建成功*/
int createSocket() { //创建socket
	//要使用套接字，首先必须调用socket()函数创建一个套接字描述符，就如同操作文件时，首先得调用fopen()函数打开一个文件。

	client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (client_socket == INVALID_SOCKET) {
		cout << "创建socket失败" << endl;
		WSACleanup();//终止使用
		return -1;
	}
	return 1;
}
//发送连接请求
int reqConnect() {
	createSocket();
	if (connect(client_socket, (SOCKADDR*)&server_addr, sizeof(SOCKADDR)) == SOCKET_ERROR) {//connect()创建与指定外部端口的连接
		cout << "连接失败" << endl;
		return -1;
	}
	return 1;
}
//帮助菜单
void help() {
	cout << "get 下载  (get 文件名/文件路径 )" << endl
		<< "pwd 显示当前服务器文件夹的绝对路径" << endl
		<< "dir 显示服务器当前目录的文件" << endl
		<< "cd  改变服务器当前路径" << endl
		<< "      进入下级目录: cd 路径名" << endl
		<< "     进入上级目录: cd .." << endl
		<< "help 进入帮助菜单" << endl
		<< "quit 退出" << endl;
}
//发送内容
int send_to_server(char buffer[]) { //发送要执行的命令至服务端
	if (send(client_socket, buffer, strlen(buffer), 0) <= 0) {
		cout << "发送命令至服务端失败" << endl;
		closesocket(client_socket);
		WSACleanup();
		return -1;
	}
	return 1;
}
//dir展示服务器端目录
void showDirec(SOCKET soc) {
	int nRead;
	memset(sendBuffer, '\0', sizeof(sendBuffer));
	while (1) {
		nRead = recv(client_socket, readBuffer, sizeof(readBuffer), 0);
		//recv通过sockClient套接口接受数据存入rbuff缓冲区，返回接收到的字节数
		if (nRead == SOCKET_ERROR) {
			cout << "读取时发生错误" << endl;
			exit(1);
		}
		if (nRead == 0) { //数据读取结束
			break;
		}
		//显示数据
		readBuffer[nRead] = '\0';
		cout << readBuffer << endl;
	}
}


int main() {
	while (1) {
		char oper[20],	    //操作
			file_name[20],	//输入的文件名
			path[20];		//本地的路径
		char cmd[30];		//命令
		char buff[80];						//用来存储经过字符串格式化的命令
		FILE* fp1, * fp2;					//fp指针用于指向要打开的文件

		//初始化数据
		memset(oper, 0, sizeof(oper));
		memset(file_name, 0, sizeof(file_name));
		memset(path, 0, sizeof(path));
		memset(cmd, 0, sizeof(cmd));
		memset(buff, 0, sizeof(buff));
		memset(readBuffer, 0, sizeof(readBuffer));
		memset(sendBuffer, 0, sizeof(sendBuffer));

		//启动winsock并初始化
		initSocket();
		//循环检查连接情况
		if (reqConnect() == -1) {
			continue;
		}
		cout << "请输入命令（输入help调出命令菜单）:";
		cin >> oper;
		//输入文件
		if (strncmp(oper, "get", 3) == 0 || strncmp(oper, "cd", 2) == 0) {
			cin >> file_name;
		}
		//退出功能
		else if (strncmp(oper, "quit", 4) == 0) {
			cout << "感谢您的使用" << endl;
			closesocket(client_socket);
			WSACleanup();
			break;
		}
		//帮助菜单功能
		else if (strncmp(oper, "help", 4) == 0) {
			help();
		}

		strcat(cmd, oper);
		strcat(cmd, " ");
		strcat(cmd, file_name);
		//把指令和文件名放入buff
		sprintf(buff, cmd);
		send_to_server(buff);									//发送
		recv(client_socket, readBuffer, sizeof(readBuffer), 0);		//接收
		//pwd
		cout << readBuffer << endl;
		//下载功能
		if (strncmp(readBuffer, "get", 3) == 0) {
			cout << "输入你想保存的路径(包括要下载的文件名）：" << endl;
			cin >> path;
			//打开文件
			errno_t F_ERR = fopen_s(&fp1, path, "wb");
			if (F_ERR != 0) {
				cout << "打开or新建 " << file_name << "文件失败" << endl;
				continue;
			}
			memset(readBuffer, 0, sizeof(readBuffer));
			int length = 0;
			while ((length = recv(client_socket, readBuffer, sizeof(readBuffer), 0)) > 0) {
				fwrite(readBuffer, sizeof(char), length, fp1);
				memset(readBuffer, 0, sizeof(readBuffer));
			}
			fclose(fp1);								//关闭文件
		}
		//显示目录
		else if (strncmp(readBuffer, "dir", 3) == 0) {
			showDirec(client_socket);
		}
		closesocket(client_socket);	//关闭连接
		WSACleanup();				//释放Winsock
	}
	return 0;
}

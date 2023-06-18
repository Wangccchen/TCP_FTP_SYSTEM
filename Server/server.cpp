#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "Winsock.h"
#include "windows.h"
#include <iostream>
#include <string>
using namespace std;

#pragma comment(lib, "wsock32.lib")

int send_file(SOCKET soc, FILE* fp);
int send_list(SOCKET soc);
int send_record(SOCKET soc, WIN32_FIND_DATA* pfd);
int addr_length;		//地址长度
char file_name[200];	//文件名
char cmd[20];
char readBuffer[1024];
char sendBuffer[1024];
SOCKET server_socket, client_socket;
sockaddr_in server_addr;//服务器地址
sockaddr_in client_addr;//客户端地址 

//初始化WSA，使得程序可以调用windows socket
int initSocket() {
	WORD sockVersion = MAKEWORD(2, 2);
	WSADATA wsaData;
	if (WSAStartup(sockVersion, &wsaData) != 0) {
		cout << "socket初始化失败" << endl;
		return -1;
	}
}
int createSocket() {
	server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (server_socket == INVALID_SOCKET) {
		cout << "创建失败" << endl;
		WSACleanup();
		return -1;
	}
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	server_addr.sin_port = htons(8080);
	//bind将socket和地址结构绑定
	if (bind(server_socket, (LPSOCKADDR)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
		cout << "绑定失败" << endl;
		return -1;
	}
	return 1;
}
int connectTCP() {
	addr_length = sizeof(client_addr);
	//server_socket开始监听
	//被动监听状态，20为请求队列的最大长度
	if (listen(server_socket, 20) < 0) {
		cout << "监听失败" << endl;
		return -1;
	}
	cout << "服务器正在监听中…" << endl;
	while (1) {
		//accept取出队列头部的连接请求
		// 接受连接请求，返回一个新的socket(描述符)，这个新socket用于同连接的客户端通信 
		// accept函数会把连接到的客户端信息写到client_addr中 
		client_socket = accept(server_socket, (struct sockaddr FAR*) & client_addr, &addr_length);
		while (1) {
			memset(readBuffer, 0, sizeof(readBuffer));
			memset(sendBuffer, 0, sizeof(sendBuffer));
			//接收客户端请求的的文件名
			//recv函数接收数据到缓冲区readBuffer中 
			if (recv(client_socket, readBuffer, sizeof(readBuffer), 0) <= 0) {
				break;
			}
			cout << endl << "收到命令：" << readBuffer << endl;
			//处理get
			if (strncmp(readBuffer, "get", 3) == 0) {
				// 然后从buffer(缓冲区)拷贝到file_name中
				strcpy(file_name, readBuffer + 4);
				FILE* fp;
				//定义一个文件访问指针
				fp = fopen(file_name, "rb");//二进制打开文件，只允许读
				if (fp) {
					sprintf(sendBuffer, "get %s", file_name);
					if (!send(client_socket, sendBuffer, sizeof(sendBuffer), 0)) {
						fclose(fp);
						return 0;
					}
					else {//创建额外数据连接传送数据
						if (!send_file(client_socket, fp)) {
							return 0;
						}
						fclose(fp);
					}
				}
				else {
					strcpy(sendBuffer, "无法打开文件\n");
					if (send(client_socket, sendBuffer, sizeof(sendBuffer), 0)) {
						return 0;
					}
				}
			}
			//处理pwd
			else if (strncmp(readBuffer, "pwd", 3) == 0) {
				char path[1000];
				//找到当前进程的当前目录
				GetCurrentDirectory(sizeof(path), path);
				strcpy(sendBuffer, path);
				send(client_socket, sendBuffer, sizeof(sendBuffer), 0);
			}
			//处理dir
			else if (strncmp(readBuffer, "dir", 3) == 0) {
				strcpy(sendBuffer, readBuffer);
				send(client_socket, sendBuffer, sizeof(sendBuffer), 0);
				send_list(client_socket);
			}
			//处理cd
			else if (strncmp(readBuffer, "cd", 2) == 0) {
				strcpy(file_name, readBuffer + 3);
				strcpy(sendBuffer, readBuffer);
				send(client_socket, sendBuffer, sizeof(sendBuffer), 0);
				//设置当前目录 
				SetCurrentDirectory(file_name);
			}
			closesocket(client_socket);
		}
	}
}
int send_file(SOCKET soc, FILE* fp) {
	cout << "正在发送…" << endl;
	memset(sendBuffer, 0, sizeof(sendBuffer));
	//每读取一段数据，便将其发送给客户端，循环直到文件读完为止 
	while (1) {
		int length = fread(sendBuffer, sizeof(char), sizeof(sendBuffer), fp);
		if (send(soc, sendBuffer, length, 0) == SOCKET_ERROR) {
			cout << "连接失败" << endl;
			closesocket(soc);
			return 0;
		}
		//文件传送结束
		if (length < sizeof(sendBuffer)) {
			break;
		}
	}
	closesocket(soc);
	cout << "发送成功" << endl;
	return 1;
}
int send_list(SOCKET soc) {
	//建立一个线程
	HANDLE thff;		
	//搜索文件
	WIN32_FIND_DATA fd;		
	//查找文件把待操作文件属性读取
	thff = FindFirstFile("*", &fd);		
	//发生错误
	if (thff == INVALID_HANDLE_VALUE) {		
		const char* errStr = "列出文件列表时发生错误\n";
		cout << *errStr << endl;
		if (send(soc, errStr, strlen(errStr), 0) == SOCKET_ERROR) {
			cout << "发送失败" << endl;
		}
		closesocket(soc);
		return 0;
	}
	BOOL flag = TRUE;
	while (flag) {//发送文件信息
		if (!send_record(soc, &fd)) {
			closesocket(soc);
			return 0;
		}
		flag = FindNextFile(thff, &fd);//查找下一个文件
	}
	closesocket(soc);
	return 1;
}
int send_record(SOCKET soc, WIN32_FIND_DATA* pfd) {
	//用于存储文件记录
	char file_record[MAX_PATH + 32];
	//文件建立时间
	FILETIME ft;			
	//将文件的最后写入时间转换为本地时间
	FileTimeToLocalFileTime(&pfd->ftLastWriteTime, &ft);
	//用于存储系统时间
	SYSTEMTIME lastWriteTime;
	//本地时间 ft 转换为系统时间
	FileTimeToSystemTime(&ft, &lastWriteTime);
	//根据文件的属性判断是否为目录
	//如果是目录则将 dir 设置为 "<DIR>"，否则设置为空格
	const char* dir = pfd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ? "<DIR>" : " ";
	//使用格式化字符串将文件记录的信息填充
	sprintf(file_record, "%04d-%02d-%02d %02d:%02d %5s %10d   %-20s\n",
		lastWriteTime.wYear,
		lastWriteTime.wMonth,
		lastWriteTime.wDay,
		lastWriteTime.wHour,
		lastWriteTime.wMinute,
		dir,
		pfd->nFileSizeLow,
		pfd->cFileName
	);
	//通过soc接口发送记录数据，成功返回发送的字节数  
	if (send(soc, file_record, strlen(file_record), 0) == SOCKET_ERROR) {
		cout << "发送失败" << endl;
		return 0;
	}
	return 1;
}
int main() {
	//分别调用初始化的函数即可运行
	if (initSocket() == -1 || createSocket() == -1 || connectTCP() == -1) {
		return -1;
	}
	return 1;
}

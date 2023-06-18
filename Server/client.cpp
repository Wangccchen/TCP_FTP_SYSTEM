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
char readBuffer[1024];		//���ջ�����
char sendBuffer[1024];		//���ͻ�����
SOCKET client_socket;		//�ͻ��˶���
sockaddr_in server_addr;	//��������ַ

//��ʼ��WSA��ʹ�ó�����Ե���windows socket
int initSocket() {
	WORD sockVersion = MAKEWORD(2, 2);
	WSADATA wsaData;
	if (WSAStartup(sockVersion, &wsaData) != 0) {
		cout << "socket��ʼ��ʧ��" << endl;
		return -1;
	}
	//���õ�ַ�ṹ
	//AF_INETʹ�� TCP/IPv4 ����ͨ��
	//������IP��ת���ɶ�����IPV4��ַ
	//���ö˿ںţ�htons���ڽ������ֽ����Ϊ�����ֽ���
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	server_addr.sin_port = htons(8080);
	return 1;
}
/*�����������׽��֣�server_socket
	������Ƿ񴴽��ɹ�*/
int createSocket() { //����socket
	//Ҫʹ���׽��֣����ȱ������socket()��������һ���׽���������������ͬ�����ļ�ʱ�����ȵõ���fopen()������һ���ļ���

	client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (client_socket == INVALID_SOCKET) {
		cout << "����socketʧ��" << endl;
		WSACleanup();//��ֹʹ��
		return -1;
	}
	return 1;
}
//������������
int reqConnect() {
	createSocket();
	if (connect(client_socket, (SOCKADDR*)&server_addr, sizeof(SOCKADDR)) == SOCKET_ERROR) {//connect()������ָ���ⲿ�˿ڵ�����
		cout << "����ʧ��" << endl;
		return -1;
	}
	return 1;
}
//�����˵�
void help() {
	cout << "get ����  (get �ļ���/�ļ�·�� )" << endl
		<< "pwd ��ʾ��ǰ�������ļ��еľ���·��" << endl
		<< "dir ��ʾ��������ǰĿ¼���ļ�" << endl
		<< "cd  �ı��������ǰ·��" << endl
		<< "      �����¼�Ŀ¼: cd ·����" << endl
		<< "     �����ϼ�Ŀ¼: cd .." << endl
		<< "help ��������˵�" << endl
		<< "quit �˳�" << endl;
}
//��������
int send_to_server(char buffer[]) { //����Ҫִ�е������������
	if (send(client_socket, buffer, strlen(buffer), 0) <= 0) {
		cout << "���������������ʧ��" << endl;
		closesocket(client_socket);
		WSACleanup();
		return -1;
	}
	return 1;
}
//dirչʾ��������Ŀ¼
void showDirec(SOCKET soc) {
	int nRead;
	memset(sendBuffer, '\0', sizeof(sendBuffer));
	while (1) {
		nRead = recv(client_socket, readBuffer, sizeof(readBuffer), 0);
		//recvͨ��sockClient�׽ӿڽ������ݴ���rbuff�����������ؽ��յ����ֽ���
		if (nRead == SOCKET_ERROR) {
			cout << "��ȡʱ��������" << endl;
			exit(1);
		}
		if (nRead == 0) { //���ݶ�ȡ����
			break;
		}
		//��ʾ����
		readBuffer[nRead] = '\0';
		cout << readBuffer << endl;
	}
}


int main() {
	while (1) {
		char oper[20],	    //����
			file_name[20],	//������ļ���
			path[20];		//���ص�·��
		char cmd[30];		//����
		char buff[80];						//�����洢�����ַ�����ʽ��������
		FILE* fp1, * fp2;					//fpָ������ָ��Ҫ�򿪵��ļ�

		//��ʼ������
		memset(oper, 0, sizeof(oper));
		memset(file_name, 0, sizeof(file_name));
		memset(path, 0, sizeof(path));
		memset(cmd, 0, sizeof(cmd));
		memset(buff, 0, sizeof(buff));
		memset(readBuffer, 0, sizeof(readBuffer));
		memset(sendBuffer, 0, sizeof(sendBuffer));

		//����winsock����ʼ��
		initSocket();
		//ѭ������������
		if (reqConnect() == -1) {
			continue;
		}
		cout << "�������������help��������˵���:";
		cin >> oper;
		//�����ļ�
		if (strncmp(oper, "get", 3) == 0 || strncmp(oper, "cd", 2) == 0) {
			cin >> file_name;
		}
		//�˳�����
		else if (strncmp(oper, "quit", 4) == 0) {
			cout << "��л����ʹ��" << endl;
			closesocket(client_socket);
			WSACleanup();
			break;
		}
		//�����˵�����
		else if (strncmp(oper, "help", 4) == 0) {
			help();
		}

		strcat(cmd, oper);
		strcat(cmd, " ");
		strcat(cmd, file_name);
		//��ָ����ļ�������buff
		sprintf(buff, cmd);
		send_to_server(buff);									//����
		recv(client_socket, readBuffer, sizeof(readBuffer), 0);		//����
		//pwd
		cout << readBuffer << endl;
		//���ع���
		if (strncmp(readBuffer, "get", 3) == 0) {
			cout << "�������뱣���·��(����Ҫ���ص��ļ�������" << endl;
			cin >> path;
			//���ļ�
			errno_t F_ERR = fopen_s(&fp1, path, "wb");
			if (F_ERR != 0) {
				cout << "��or�½� " << file_name << "�ļ�ʧ��" << endl;
				continue;
			}
			memset(readBuffer, 0, sizeof(readBuffer));
			int length = 0;
			while ((length = recv(client_socket, readBuffer, sizeof(readBuffer), 0)) > 0) {
				fwrite(readBuffer, sizeof(char), length, fp1);
				memset(readBuffer, 0, sizeof(readBuffer));
			}
			fclose(fp1);								//�ر��ļ�
		}
		//��ʾĿ¼
		else if (strncmp(readBuffer, "dir", 3) == 0) {
			showDirec(client_socket);
		}
		closesocket(client_socket);	//�ر�����
		WSACleanup();				//�ͷ�Winsock
	}
	return 0;
}

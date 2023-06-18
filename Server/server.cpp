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
int addr_length;		//��ַ����
char file_name[200];	//�ļ���
char cmd[20];
char readBuffer[1024];
char sendBuffer[1024];
SOCKET server_socket, client_socket;
sockaddr_in server_addr;//��������ַ
sockaddr_in client_addr;//�ͻ��˵�ַ 

//��ʼ��WSA��ʹ�ó�����Ե���windows socket
int initSocket() {
	WORD sockVersion = MAKEWORD(2, 2);
	WSADATA wsaData;
	if (WSAStartup(sockVersion, &wsaData) != 0) {
		cout << "socket��ʼ��ʧ��" << endl;
		return -1;
	}
}
int createSocket() {
	server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (server_socket == INVALID_SOCKET) {
		cout << "����ʧ��" << endl;
		WSACleanup();
		return -1;
	}
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	server_addr.sin_port = htons(8080);
	//bind��socket�͵�ַ�ṹ��
	if (bind(server_socket, (LPSOCKADDR)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
		cout << "��ʧ��" << endl;
		return -1;
	}
	return 1;
}
int connectTCP() {
	addr_length = sizeof(client_addr);
	//server_socket��ʼ����
	//��������״̬��20Ϊ������е���󳤶�
	if (listen(server_socket, 20) < 0) {
		cout << "����ʧ��" << endl;
		return -1;
	}
	cout << "���������ڼ����С�" << endl;
	while (1) {
		//acceptȡ������ͷ������������
		// �����������󣬷���һ���µ�socket(������)�������socket����ͬ���ӵĿͻ���ͨ�� 
		// accept����������ӵ��Ŀͻ�����Ϣд��client_addr�� 
		client_socket = accept(server_socket, (struct sockaddr FAR*) & client_addr, &addr_length);
		while (1) {
			memset(readBuffer, 0, sizeof(readBuffer));
			memset(sendBuffer, 0, sizeof(sendBuffer));
			//���տͻ�������ĵ��ļ���
			//recv�����������ݵ�������readBuffer�� 
			if (recv(client_socket, readBuffer, sizeof(readBuffer), 0) <= 0) {
				break;
			}
			cout << endl << "�յ����" << readBuffer << endl;
			//����get
			if (strncmp(readBuffer, "get", 3) == 0) {
				// Ȼ���buffer(������)������file_name��
				strcpy(file_name, readBuffer + 4);
				FILE* fp;
				//����һ���ļ�����ָ��
				fp = fopen(file_name, "rb");//�����ƴ��ļ���ֻ�����
				if (fp) {
					sprintf(sendBuffer, "get %s", file_name);
					if (!send(client_socket, sendBuffer, sizeof(sendBuffer), 0)) {
						fclose(fp);
						return 0;
					}
					else {//���������������Ӵ�������
						if (!send_file(client_socket, fp)) {
							return 0;
						}
						fclose(fp);
					}
				}
				else {
					strcpy(sendBuffer, "�޷����ļ�\n");
					if (send(client_socket, sendBuffer, sizeof(sendBuffer), 0)) {
						return 0;
					}
				}
			}
			//����pwd
			else if (strncmp(readBuffer, "pwd", 3) == 0) {
				char path[1000];
				//�ҵ���ǰ���̵ĵ�ǰĿ¼
				GetCurrentDirectory(sizeof(path), path);
				strcpy(sendBuffer, path);
				send(client_socket, sendBuffer, sizeof(sendBuffer), 0);
			}
			//����dir
			else if (strncmp(readBuffer, "dir", 3) == 0) {
				strcpy(sendBuffer, readBuffer);
				send(client_socket, sendBuffer, sizeof(sendBuffer), 0);
				send_list(client_socket);
			}
			//����cd
			else if (strncmp(readBuffer, "cd", 2) == 0) {
				strcpy(file_name, readBuffer + 3);
				strcpy(sendBuffer, readBuffer);
				send(client_socket, sendBuffer, sizeof(sendBuffer), 0);
				//���õ�ǰĿ¼ 
				SetCurrentDirectory(file_name);
			}
			closesocket(client_socket);
		}
	}
}
int send_file(SOCKET soc, FILE* fp) {
	cout << "���ڷ��͡�" << endl;
	memset(sendBuffer, 0, sizeof(sendBuffer));
	//ÿ��ȡһ�����ݣ��㽫�䷢�͸��ͻ��ˣ�ѭ��ֱ���ļ�����Ϊֹ 
	while (1) {
		int length = fread(sendBuffer, sizeof(char), sizeof(sendBuffer), fp);
		if (send(soc, sendBuffer, length, 0) == SOCKET_ERROR) {
			cout << "����ʧ��" << endl;
			closesocket(soc);
			return 0;
		}
		//�ļ����ͽ���
		if (length < sizeof(sendBuffer)) {
			break;
		}
	}
	closesocket(soc);
	cout << "���ͳɹ�" << endl;
	return 1;
}
int send_list(SOCKET soc) {
	//����һ���߳�
	HANDLE thff;		
	//�����ļ�
	WIN32_FIND_DATA fd;		
	//�����ļ��Ѵ������ļ����Զ�ȡ
	thff = FindFirstFile("*", &fd);		
	//��������
	if (thff == INVALID_HANDLE_VALUE) {		
		const char* errStr = "�г��ļ��б�ʱ��������\n";
		cout << *errStr << endl;
		if (send(soc, errStr, strlen(errStr), 0) == SOCKET_ERROR) {
			cout << "����ʧ��" << endl;
		}
		closesocket(soc);
		return 0;
	}
	BOOL flag = TRUE;
	while (flag) {//�����ļ���Ϣ
		if (!send_record(soc, &fd)) {
			closesocket(soc);
			return 0;
		}
		flag = FindNextFile(thff, &fd);//������һ���ļ�
	}
	closesocket(soc);
	return 1;
}
int send_record(SOCKET soc, WIN32_FIND_DATA* pfd) {
	//���ڴ洢�ļ���¼
	char file_record[MAX_PATH + 32];
	//�ļ�����ʱ��
	FILETIME ft;			
	//���ļ������д��ʱ��ת��Ϊ����ʱ��
	FileTimeToLocalFileTime(&pfd->ftLastWriteTime, &ft);
	//���ڴ洢ϵͳʱ��
	SYSTEMTIME lastWriteTime;
	//����ʱ�� ft ת��Ϊϵͳʱ��
	FileTimeToSystemTime(&ft, &lastWriteTime);
	//�����ļ��������ж��Ƿ�ΪĿ¼
	//�����Ŀ¼�� dir ����Ϊ "<DIR>"����������Ϊ�ո�
	const char* dir = pfd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ? "<DIR>" : " ";
	//ʹ�ø�ʽ���ַ������ļ���¼����Ϣ���
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
	//ͨ��soc�ӿڷ��ͼ�¼���ݣ��ɹ����ط��͵��ֽ���  
	if (send(soc, file_record, strlen(file_record), 0) == SOCKET_ERROR) {
		cout << "����ʧ��" << endl;
		return 0;
	}
	return 1;
}
int main() {
	//�ֱ���ó�ʼ���ĺ�����������
	if (initSocket() == -1 || createSocket() == -1 || connectTCP() == -1) {
		return -1;
	}
	return 1;
}

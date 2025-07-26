#pragma once
#include "pch.h"
#include "framework.h"

class CServerSocket
{
public:
	static CServerSocket* GetInstance() {
		if (m_instance == NULL) {
			m_instance = new CServerSocket();
		}
		return m_instance;
	}
	bool InitServerSocket() {
		//0表示使用默认协议
		//todo: 返回值处理
		if (m_socket == -1) return false; // 创建套接字失败
		sockaddr_in server_addr;
		memset(&server_addr, 0, sizeof(server_addr));
		server_addr.sin_family = AF_INET; // IPv4 协议族
		server_addr.sin_addr.s_addr = INADDR_ANY; // 绑定到所有可用的接口
		server_addr.sin_port = htons(9528);
		if (bind(m_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
			return false; // 绑定失败
		}
		if (listen(m_socket, 1) == -1) {
			return false; // 监听失败
		} // 监听连接请求
		return true; // 初始化成功
	}
	bool AcceptClient(SOCKET& client_sock, sockaddr_in& client_addr) {
		sockaddr_in client_addr;
		char szBuffer[1024] = { 0 };
		int cli_size = sizeof(client_addr);
		m_client = accept(m_socket, (sockaddr*)&client_addr, &cli_size); // 阻塞等待客户端连接
		if (m_client == INVALID_SOCKET) {
			return false; // 接受连接失败
		}
		//recv(m_socket, szBuffer, sizeof(szBuffer), 0); // 接收数据
		//send(m_socket, szBuffer, sizeof(szBuffer), 0);
		return true; // 接受连接成功
	}
	int DealCommand() {
		char szBuffer[1024] = { 0 };
		while (true)
		{
			int len = recv(m_client, szBuffer, sizeof(szBuffer), 0); // 接收数据
			if (len <= 0) {
				return -1; // 连接断开或出错
			}
		}
	}
	bool SendData(const char* data, int len) {
		if (m_client == -1) return false; // 客户端未连接
		send(m_client, data, len, 0);
	}
private:
	SOCKET m_socket;
	SOCKET m_client;
	CServerSocket& operator= (const CServerSocket& ss) {

	}
	CServerSocket(const CServerSocket& ss) {
		m_socket = ss.m_socket;
		m_client = ss.m_client;
	}
	CServerSocket() {
		m_client = INVALID_SOCKET; // 初始化客户端套接字为无效值
		if (InitSocketEnv() == FALSE) {
			MessageBox(NULL, _T("Socket环境初始化失败"), _T("错误"), MB_OK | MB_ICONERROR);
			exit(0);
		}
		m_socket = socket(AF_INET, SOCK_STREAM, 0);
	}

	~CServerSocket() {
		WSACleanup(); // 清理套接字资源
		closesocket(m_socket);
	}
	BOOL InitSocketEnv() {
		WSADATA wsaData;
		//todo: 返回值处理
		if (WSAStartup(MAKEWORD(1, 1), &wsaData) != 0)
		{
			return FALSE;
		}
		return true;
	}
	static CServerSocket* m_instance;
	static void releaseInstance() {
		if (m_instance) {
			CServerSocket* temp = m_instance;
			m_instance = NULL;
			delete temp;
		}
	}
	class CHelper
	{
	public:
		CHelper() {
			CServerSocket::GetInstance(); // 确保单例被创建
		}
		~CHelper() {
			if (m_instance) {
				delete m_instance;
			}
		}
	};
	static CHelper m_helper; // 确保在程序结束时释放单例
};

extern CServerSocket server;
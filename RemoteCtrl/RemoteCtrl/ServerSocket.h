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
		//0��ʾʹ��Ĭ��Э��
		//todo: ����ֵ����
		if (m_socket == -1) return false; // �����׽���ʧ��
		sockaddr_in server_addr;
		memset(&server_addr, 0, sizeof(server_addr));
		server_addr.sin_family = AF_INET; // IPv4 Э����
		server_addr.sin_addr.s_addr = INADDR_ANY; // �󶨵����п��õĽӿ�
		server_addr.sin_port = htons(9528);
		if (bind(m_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
			return false; // ��ʧ��
		}
		if (listen(m_socket, 1) == -1) {
			return false; // ����ʧ��
		} // ������������
		return true; // ��ʼ���ɹ�
	}
	bool AcceptClient(SOCKET& client_sock, sockaddr_in& client_addr) {
		sockaddr_in client_addr;
		char szBuffer[1024] = { 0 };
		int cli_size = sizeof(client_addr);
		m_client = accept(m_socket, (sockaddr*)&client_addr, &cli_size); // �����ȴ��ͻ�������
		if (m_client == INVALID_SOCKET) {
			return false; // ��������ʧ��
		}
		//recv(m_socket, szBuffer, sizeof(szBuffer), 0); // ��������
		//send(m_socket, szBuffer, sizeof(szBuffer), 0);
		return true; // �������ӳɹ�
	}
	int DealCommand() {
		char szBuffer[1024] = { 0 };
		while (true)
		{
			int len = recv(m_client, szBuffer, sizeof(szBuffer), 0); // ��������
			if (len <= 0) {
				return -1; // ���ӶϿ������
			}
		}
	}
	bool SendData(const char* data, int len) {
		if (m_client == -1) return false; // �ͻ���δ����
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
		m_client = INVALID_SOCKET; // ��ʼ���ͻ����׽���Ϊ��Чֵ
		if (InitSocketEnv() == FALSE) {
			MessageBox(NULL, _T("Socket������ʼ��ʧ��"), _T("����"), MB_OK | MB_ICONERROR);
			exit(0);
		}
		m_socket = socket(AF_INET, SOCK_STREAM, 0);
	}

	~CServerSocket() {
		WSACleanup(); // �����׽�����Դ
		closesocket(m_socket);
	}
	BOOL InitSocketEnv() {
		WSADATA wsaData;
		//todo: ����ֵ����
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
			CServerSocket::GetInstance(); // ȷ������������
		}
		~CHelper() {
			if (m_instance) {
				delete m_instance;
			}
		}
	};
	static CHelper m_helper; // ȷ���ڳ������ʱ�ͷŵ���
};

extern CServerSocket server;
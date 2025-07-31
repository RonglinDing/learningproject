#pragma once
#include "pch.h"
#include "framework.h"

class CPacket {
public:
	CPacket() : sHead(0), nLength(0),sCmd(0), sSum(0) {}
	CPacket(const CPacket& pack) {
		sHead = pack.sHead;
		nLength = pack.nLength;
		sCmd = pack.sCmd;
		strData = pack.strData;
		sSum = pack.sSum;
	}

	CPacket(const BYTE* pData, size_t& nSize) {
		size_t i = 0;
		for (;i < nSize; i++) {
			if (*(WORD*)(pData + i) == 0xFEFF) {
				sHead = *(WORD*)(pData + i);
				i += sizeof(WORD); // 跳过包头
				break;
			}
		}
		if (i+4+2+2 >= nSize) {
			nSize = 0;
			return; // 没有找到包头
		}
		nLength = *(DWORD*)(pData + i);
		i += sizeof(DWORD); // 跳过数据包长度
		if (nLength + i > nSize) {
			nSize = 0;
			return; // 包未完全接收，或数据包不全
		}
		sCmd = *(WORD*)(pData + i);
		i += sizeof(WORD); // 跳过命令字
		if (nLength > 4) {
			strData.resize(nLength - 4);
			memcpy(&strData[0], pData + i, nLength - 4); // 拷贝数据内容
			//memcpy((void*)strData.c_str(), pData + i, nLength - 4); // 拷贝数据内容
			i += nLength - 4; // 跳过数据内容
		}
		sSum = *(WORD*)(pData + i);
		i += sizeof(WORD); // 跳过校验和
		WORD sum = 0;
		for (size_t j = 0; j < strData.size(); j++) {
			sum += static_cast<unsigned char>(strData[j]); // 计算校验和
		}
		if (sum == sSum) {
			nSize = i; // 包完整
			return;
		}
		nSize = 0; // 校验和错误，包不完整
	}
	~CPacket() {}
	CPacket& operator=(const CPacket& pack) {
		if (this != &pack) {
			sHead = pack.sHead;
			nLength = pack.nLength;
			sCmd = pack.sCmd;
			strData = pack.strData;
			sSum = pack.sSum;
		}
		return *this;
	}
public:
	WORD sHead; // et包头(0xFEFF)
	DWORD nLength; // 数据包长度(从控制命令开始，到和校验结束)
	WORD sCmd;
	std::string strData; // 数据内容
	WORD sSum; // 校验和

};
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
	bool AcceptClient() {
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
#define BFFER_SIZE 4096
	int DealCommand() {
		if (m_client == -1) return -1; // 客户端未连接
		char* szBuffer = new char[BFFER_SIZE];
		memset(szBuffer, 0, BFFER_SIZE);
		size_t index = 0;
		while (true)
		{
			size_t len = recv(m_client, szBuffer, sizeof(szBuffer) - index, 0); // 接收数据
			index += len;
			if (len <= 0) {
				return -1; // 连接断开或出错
			}
			len = index;
			m_packet =CPacket((BYTE*)szBuffer, len);
			if (len > 0) {
				memmove(szBuffer, szBuffer + len, BFFER_SIZE - len); // 清除已处理的数据
				index -= len; // 更新索引
				return m_packet.sCmd;
			}

		}
		return -1; // 没有完整的包
	}
	bool SendData(const char* data, int len) {
		if (m_client == -1) return false; // 客户端未连接
		send(m_client, data, len, 0);
	}
private:
	SOCKET m_socket;
	SOCKET m_client;
	CPacket m_packet;
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
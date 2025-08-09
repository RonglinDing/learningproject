
#pragma once
#include "pch.h"
#include "framework.h"
#include "vector"
#include "string"
#include <ws2tcpip.h>

#pragma pack(push)
#pragma pack(1)
class CPacket {
public:
	CPacket() : sHead(0), nLength(0), sCmd(0), sSum(0) {}
	CPacket(WORD nCmd, const BYTE* pData, size_t nSize) {
		sHead = 0xFEFF; // 包头
		nLength = static_cast<DWORD>(nSize + 4); // 
		sCmd = nCmd; // 命令字
		if (nSize > 0) {
			strData.resize(nSize);
			memcpy(&strData[0], pData, nSize); // 拷贝数据内容
		}
		else {
			strData.clear(); // 如果没有数据内容，则清空
		}
		sSum = 0; // 初始化校验和
		for (size_t j = 0; j < strData.size(); j++) {
			sSum += static_cast<unsigned char>(strData[j]); // 计算校验和
		}
	}
	CPacket(const CPacket& pack) {
		sHead = pack.sHead;
		nLength = pack.nLength;
		sCmd = pack.sCmd;
		strData = pack.strData;
		sSum = pack.sSum;
	}

	CPacket(const BYTE* pData, size_t& nSize) {
		size_t i = 0;
		for (; i < nSize; i++) {
			if (*(WORD*)(pData + i) == 0xFEFF) {
				sHead = *(WORD*)(pData + i);
				i += sizeof(WORD); // 跳过包头
				break;
			}
		}
		if (i + 4 + 2 + 2 > nSize) {
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
	int Size() {
		return nLength + 6;
	}
	BYTE* Data() {
		strOut.resize(nLength + 6);
		BYTE* pData = strOut.data();
		*(WORD*)pData = sHead; pData += 2;
		*(DWORD*)(pData) = nLength; pData += 4;
		*(WORD*)(pData) = sCmd; pData += 2;
		memcpy(pData, strData.c_str(), strData.size()); pData += strData.size();
		*(WORD*)(pData) = sSum;
		return strOut.data();// 返回数据内容
	}
public:
	WORD sHead; // et包头(0xFEFF)
	DWORD nLength; // 数据包长度(从控制命令开始，到和校验结束)
	WORD sCmd;
	std::string strData; // 数据内容
	WORD sSum; // 校验和
	std::vector<BYTE> strOut;
};


typedef struct MouseEvent
{
	MouseEvent() : nAction(0), nButton(-1) {
		ptXY.x = 0;
		ptXY.y = 0;
	}
	WORD nAction; // 鼠标事件类型(0:移动, 1:左键按下, 2:左键弹起, 3:右键按下, 4:右键弹起)
	WORD nButton; // 鼠标按键(0:左键, 1:右键, 2:中键)
	POINT ptXY; // 鼠标位置

}MOUSEEV, * PMOUSEEV;


#pragma pop;

std::string GetErrInfo(int wsaErrCode);
//{
//	std::string ret;
//	LPVOID lpMsgBuf = NULL;
//	FormatMessage(
//		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
//		NULL,
//		wsaErrCode,
//		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // 默认语言
//		(LPTSTR)&lpMsgBuf, 0, NULL);
//	ret = (char*)lpMsgBuf; // 将错误信息转换为字符串
//	LocalFree(lpMsgBuf); // 释放缓冲区
//	return ret;
//
//}
class CClientSocket
{
public:
	static CClientSocket* GetInstance() {
		if (m_instance == NULL) {
			m_instance = new CClientSocket();
		}
		return m_instance;
	}
	bool InitSocket(int nIP, int nPort) {
		if (m_socket != -1) CloseSocket(); // 如果之前有连接，先关闭套接字

		m_socket = socket(AF_INET, SOCK_STREAM, 0); // 创建套接字
		//0表示使用默认协议
		//todo: 返回值处理
		if (m_socket == -1) return false; // 创建套接字失败
		sockaddr_in server_addr;
		memset(&server_addr, 0, sizeof(server_addr));
		server_addr.sin_family = AF_INET; // IPv4 协议族
		server_addr.sin_addr.s_addr = htonl(nIP); // 设置服务器IP地址
		server_addr.sin_port = htons(nPort);
		if (server_addr.sin_addr.s_addr == INADDR_NONE) {
			AfxMessageBox("IP地址无效");
			return false; // IP地址无效
		}
		int ret = connect(m_socket, (sockaddr*)&server_addr, sizeof(server_addr)); // 连接到服务器
		if (ret == SOCKET_ERROR) {
			AfxMessageBox("连接失败");
			TRACE("连接失败 %d %s\r\n",WSAGetLastError(),GetErrInfo(WSAGetLastError()).c_str());
			return false; // 连接失败
		}
		return true; // 初始化成功
	}


#define BFFER_SIZE 4096
	int DealCommand() {
		if (m_socket == -1) return -1; // 客户端未连接
		char* szBuffer = m_buffer.data();
		memset(szBuffer, 0, BFFER_SIZE);
		size_t index = 0;
		while (true)
		{
			size_t len = recv(m_socket, szBuffer+ index, BFFER_SIZE - index, 0); // 接收数据
			if (len <= 0) {
				return -1; // 连接断开或出错
			}
			index += len;
			len = index;
			m_packet = CPacket((BYTE*)szBuffer, len);
			if (len > 0) {
				memmove(szBuffer, szBuffer + len, BFFER_SIZE - len); // 清除已处理的数据
				index -= len; // 更新索引
				return m_packet.sCmd;
			}

		}
		return -1; // 没有完整的包
	}
	bool SendData(const char* data, int len) {
		if (m_socket == -1) return false; // 客户端未连接
		return send(m_socket, data, len, 0);
	}
	bool SendData(CPacket& pack) {
		TRACE("fason %d \r\n",m_socket);
		if (m_socket == -1) return false; // 客户端未连接
		return send(m_socket, (const char*)pack.Data(), pack.nLength + 6, 0) > 0;

	}
	bool GetFilePath(std::string& strPath) {
		if ((m_packet.sCmd >= 2) && (m_packet.sCmd <= 4)) {
			strPath = m_packet.strData;
			return true; // 成功获取文件路径
		}
		return false; // 命令不是获取文件路径
	}
	bool GetMouseEvent(MOUSEEV& mouse) {
		if (m_packet.sCmd == 5) {
			memcpy(static_cast<void*>(&mouse), m_packet.strData.c_str(), sizeof(MOUSEEV));
			return true; // 成功处理鼠标事件  
		}
		return false; // 不是鼠标事件  
	}
	CPacket& GetPacket() {
		return m_packet; // 返回当前处理的包
	}
	void CloseSocket() {
		if (m_socket != -1) {
			closesocket(m_socket); // 关闭套接字
			m_socket = -1; // 重置套接字状态
		}
	}
private:
	std::vector<char> m_buffer;
	SOCKET m_socket;
	CPacket m_packet;
	CClientSocket& operator= (const CClientSocket& ss) {

	}
	CClientSocket(const CClientSocket& ss) {
		m_socket = ss.m_socket;
	}
	CClientSocket() {
		if (InitSocketEnv() == FALSE) {
			MessageBox(NULL, _T("Socket环境初始化失败"), _T("错误"), MB_OK | MB_ICONERROR);
			exit(0);
		}
		m_buffer.resize(BFFER_SIZE);
	}

	~CClientSocket() {
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
	static CClientSocket* m_instance;
	static void releaseInstance() {
		if (m_instance) {
			CClientSocket* temp = m_instance;
			m_instance = NULL;
			delete temp;
		}
	}
	class CHelper
	{
	public:
		CHelper() {
			CClientSocket::GetInstance(); // 确保单例被创建
		}
		~CHelper() {
			if (m_instance) {
				delete m_instance;
			}
		}
	};
	static CHelper m_helper; // 确保在程序结束时释放单例
};

extern CClientSocket server;
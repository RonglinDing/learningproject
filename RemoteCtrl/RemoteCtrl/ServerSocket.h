#pragma once
#include "pch.h"
#include "framework.h"
#include "vector"

#pragma pack(push)
#pragma pack(1)
class CPacket {
public:
	CPacket() : sHead(0), nLength(0),sCmd(0), sSum(0) {}
	CPacket(WORD nCmd, const BYTE* pData, size_t nSize) {
		sHead = 0xFEFF; // 包头
		nLength = static_cast<DWORD>(nSize + 4); // 
		sCmd = nCmd; // 命令字
		if (nSize > 0){
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
	int Size() {
		return nLength + 6;
	}
	BYTE* Data(){
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
		return send(m_client, data, len, 0);
	}
	bool SendData(CPacket& pack) {
		if(m_client == -1) return false; // 客户端未连接
		return send(m_client, (const char*)pack.Data(), pack.nLength + 6, 0) > 0;

	}
	bool GetFilePath(std::string& strPath) {
		if ((m_packet.sCmd >=2) && (m_packet.sCmd <=4)) {
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
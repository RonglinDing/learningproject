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
		sHead = 0xFEFF; // ��ͷ
		nLength = static_cast<DWORD>(nSize + 4); // 
		sCmd = nCmd; // ������
		if (nSize > 0){
			strData.resize(nSize);
			memcpy(&strData[0], pData, nSize); // ������������
		}
		else {
			strData.clear(); // ���û���������ݣ������
		}
		sSum = 0; // ��ʼ��У���
		for (size_t j = 0; j < strData.size(); j++) {
			sSum += static_cast<unsigned char>(strData[j]); // ����У���
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
				i += sizeof(WORD); // ������ͷ
				break;
			}
		}
		if (i+4+2+2 >= nSize) {
			nSize = 0;
			return; // û���ҵ���ͷ
		}
		nLength = *(DWORD*)(pData + i);
		i += sizeof(DWORD); // �������ݰ�����
		if (nLength + i > nSize) {
			nSize = 0;
			return; // ��δ��ȫ���գ������ݰ���ȫ
		}
		sCmd = *(WORD*)(pData + i);
		i += sizeof(WORD); // ����������
		if (nLength > 4) {
			strData.resize(nLength - 4);
			memcpy(&strData[0], pData + i, nLength - 4); // ������������
			//memcpy((void*)strData.c_str(), pData + i, nLength - 4); // ������������
			i += nLength - 4; // ������������
		}
		sSum = *(WORD*)(pData + i);
		i += sizeof(WORD); // ����У���
		WORD sum = 0;
		for (size_t j = 0; j < strData.size(); j++) {
			sum += static_cast<unsigned char>(strData[j]); // ����У���
		}
		if (sum == sSum) {
			nSize = i; // ������
			return;
		}
		nSize = 0; // У��ʹ��󣬰�������
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
		return strOut.data();// ������������
	}
public:
	WORD sHead; // et��ͷ(0xFEFF)
	DWORD nLength; // ���ݰ�����(�ӿ������ʼ������У�����)
	WORD sCmd;
	std::string strData; // ��������
	WORD sSum; // У���
	std::vector<BYTE> strOut;
};


typedef struct MouseEvent
{
	MouseEvent() : nAction(0), nButton(-1) {
		ptXY.x = 0;
		ptXY.y = 0;
	}
	WORD nAction; // ����¼�����(0:�ƶ�, 1:�������, 2:�������, 3:�Ҽ�����, 4:�Ҽ�����)
	WORD nButton; // ��갴��(0:���, 1:�Ҽ�, 2:�м�)
	POINT ptXY; // ���λ��
	 
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
	bool AcceptClient() {
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


#define BFFER_SIZE 4096
	int DealCommand() {
		if (m_client == -1) return -1; // �ͻ���δ����
		char* szBuffer = new char[BFFER_SIZE];
		memset(szBuffer, 0, BFFER_SIZE);
		size_t index = 0;
		while (true)
		{
			size_t len = recv(m_client, szBuffer, sizeof(szBuffer) - index, 0); // ��������
			index += len;
			if (len <= 0) {
				return -1; // ���ӶϿ������
			}
			len = index;
			m_packet =CPacket((BYTE*)szBuffer, len);
			if (len > 0) {
				memmove(szBuffer, szBuffer + len, BFFER_SIZE - len); // ����Ѵ��������
				index -= len; // ��������
				return m_packet.sCmd;
			}

		}
		return -1; // û�������İ�
	}
	bool SendData(const char* data, int len) {
		if (m_client == -1) return false; // �ͻ���δ����
		return send(m_client, data, len, 0);
	}
	bool SendData(CPacket& pack) {
		if(m_client == -1) return false; // �ͻ���δ����
		return send(m_client, (const char*)pack.Data(), pack.nLength + 6, 0) > 0;

	}
	bool GetFilePath(std::string& strPath) {
		if ((m_packet.sCmd >=2) && (m_packet.sCmd <=4)) {
			strPath = m_packet.strData;
			return true; // �ɹ���ȡ�ļ�·��
		}
		return false; // ����ǻ�ȡ�ļ�·��
	}
    bool GetMouseEvent(MOUSEEV& mouse) {  
        if (m_packet.sCmd == 5) {  
            memcpy(static_cast<void*>(&mouse), m_packet.strData.c_str(), sizeof(MOUSEEV));  
            return true; // �ɹ���������¼�  
        }  
        return false; // ��������¼�  
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
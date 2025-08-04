// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "ServerSocket.h"
#include <direct.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 唯一的应用程序对象

CWinApp theApp;

using namespace std;
void Dump(BYTE* pData, size_t nSize) {
	string strOut;
	for (size_t i = 0; i < nSize; i++) {
		char szTemp[16] = { 0 };

		if (i > 0 && (i % 16 == 0)) strOut += "\n";
		sprintf_s(szTemp, "%02X ", pData[i] &0xFF);
		strOut += szTemp;
	}
	strOut += "\n";
	OutputDebugStringA(strOut.c_str());
}

int MakeDriverInfo() {
	string result;
	for (int i = 1; i < 26; i++) {
		if (_chdrive(i) == 0) {
			if(result.size()>0)
				result += ",";
			result += static_cast<char>('A' + i - 1);
		}
	}
	CPacket packet(1, (BYTE*)result.c_str(), result.size());
	Dump(packet.Data(), packet.Size());
	/*CServerSocket::GetInstance()->SendData(packet);*/
	return 0;
}
#include <io.h>
#include <list>
typedef struct file_info{
	file_info() {
		isInvalid = false;
		isDirectory = -1;
		hasNext = true;
		memset(fileName, 0, sizeof(fileName));
	}
	BOOL isInvalid;  // 是否为无效的文件信息
	BOOL isDirectory;  // 是否为目录
	BOOL hasNext;  // 是否有下一个文件信息
	char fileName[256];  // 文件名
} FileInfo, * PFileInfo;


int MakeDirectoryInfo() {
	string strPath;
	std::list<FileInfo> lstFileInfos;
	if (CServerSocket::GetInstance()->GetFilePath(strPath) == true) {
		OutputDebugString(_T("命令有错"));
		return -1;
	}
	if (_chdir(strPath.c_str()) != 0) {
		FileInfo fileInfo;
		fileInfo.isInvalid = TRUE;
		fileInfo.isDirectory = true;
		fileInfo.hasNext = false;
		memcpy(fileInfo.fileName, strPath.c_str(), strPath.size());
		CPacket packet(2, (BYTE*)&fileInfo, sizeof(fileInfo));
		CServerSocket::GetInstance()->SendData(packet);
		//lstFileInfos.push_back(fileInfo);
		OutputDebugString(_T("没有权限"));
		return -2;
	}
	_finddata_t fileData;
	int hfind = 0;
	if ((hfind =_findfirst("*", &fileData)) == -1) {
		OutputDebugString(_T("没有任何文件"));
		return -3;
	}
	do {
		FileInfo fileInfo;
		fileInfo.isDirectory = (fileData.attrib & _A_SUBDIR) != 0; // 判断是否为目录
		memcpy(fileInfo.fileName, fileData.name, strlen(fileInfo.fileName));
		//lstFileInfos.push_back(fileInfo);
		CPacket packet(2, (BYTE*)&fileInfo, sizeof(fileInfo));
		if(!CServerSocket::GetInstance()->SendData(packet)){
			OutputDebugString(_T("发送失败"));
			return -4;
		}
	} while (!_findnext(hfind, &fileData));
	//发送消息到控制端
	FileInfo fileInfo;
	fileInfo.hasNext = false; // 没有下一个文件信息
	CPacket packet(2, (BYTE*)&fileInfo, sizeof(fileInfo));
	CServerSocket::GetInstance()->SendData(packet);
	return 0;
}



int main()
{
	int nRetCode = 0;

	HMODULE hModule = ::GetModuleHandle(nullptr);

	if (hModule != nullptr)
	{
		// 初始化 MFC 并在失败时显示错误
		if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
		{
			// TODO: 在此处为应用程序的行为编写代码。
			wprintf(L"错误: MFC 初始化失败\n");
			nRetCode = 1;
		}
		else
		{

		/*	CServerSocket* serverSocket = CServerSocket::GetInstance();
			int count = 0;
			if (serverSocket->InitServerSocket() == false) {
				MessageBox(nullptr, L"网络初始化失败", L"错误", MB_OK | MB_ICONERROR);
				exit(0);
			}
			while (CServerSocket::GetInstance() != NULL) {
				if (serverSocket->AcceptClient() == false) {
					if (count >= 3) {
						MessageBox(nullptr, L"多次接入客户端失败，退出程序", L"错误", MB_OK | MB_ICONERROR);
						exit(0);
					}
					MessageBox(nullptr, L"接受客户端失败", L"错误", MB_OK | MB_ICONERROR);
					count++;
				}
				int ret = serverSocket->DealCommand();
			}*/
			int nCmd = 1;
			switch (nCmd)
			{case 1:
				MakeDriverInfo();
				break;
			case 2:
				MakeDirectoryInfo();
			}
		}
	}
	else
	{
		// TODO: 更改错误代码以符合需要
		wprintf(L"错误555: GetModuleHandle 失败\n");
		nRetCode = 1;
	}

	return nRetCode;
}

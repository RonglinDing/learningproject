// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "ServerSocket.h"
#include <direct.h>
#include <fstream>
#include <iostream>
#include <atlimage.h>


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


int RunFile() {
	std::string strPath;
	CServerSocket::GetInstance()->GetFilePath(strPath);
	ShellExecuteA(NULL, NULL, strPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
	CPacket packet(3, NULL, 0);
	CServerSocket::GetInstance()->SendData(packet);
	return 0;
}

int DownloadFile() {
	std::string strPath;
	CServerSocket::GetInstance()->GetFilePath(strPath);
	long long fileSize = 0;
	FILE* file = NULL;
	errno_t err = fopen_s(&file, strPath.c_str(), "rb");// 打开文件,b表示以二进制模式读取文件
	if (err != 0) {
		OutputDebugString(_T("打开文件失败"));
		CPacket packet(4, (BYTE*)&fileSize, sizeof(fileSize)); // 发送空数据包表示失败
		CServerSocket::GetInstance()->SendData(packet);
		return -1;
	}
	if (file != NULL) {
		fseek(file, 0, SEEK_END); // 移动到文件末尾

		fileSize = _ftelli64(file); // 获取文件大小
		CPacket headerPacket(4, (BYTE*)&fileSize, sizeof(fileSize)); // 创建数据包头
		CServerSocket::GetInstance()->SendData(headerPacket);
		fseek(file, 0, SEEK_SET); // 重置文件指针到开头
		char buffer[1024] = ""; // 缓冲区大小
		size_t rlen = 0;
		do {
			rlen = fread(buffer, 1, 1024, file); // 读取文件内容到缓冲区
			CPacket packet(4, (BYTE*)buffer, rlen); // 创建数据包
			CServerSocket::GetInstance()->SendData(packet);
		} while (rlen >= 1024);
		fclose(file); // 关闭文件
	}
	CPacket packet(4, NULL, 0); // 创建数据包
	CServerSocket::GetInstance()->SendData(packet);
	return 0;
}


//int DownloadFile() {
//	std::string strPath;
//	CServerSocket::GetInstance()->GetFilePath(strPath);
//
//	std::ifstream file(strPath, std::ios::binary); // 二进制打开
//	if (!file.is_open()) {
//		OutputDebugString(_T("打开文件失败"));
//		long long fileSize = 0;
//		CPacket packet(4, (BYTE*)&fileSize, sizeof(fileSize)); // 发送空数据包表示失败
//		CServerSocket::GetInstance()->SendData(packet);
//		return -1;
//	}
//
//	// 获取文件大小
//	file.seekg(0, std::ios::end);
//	long long fileSize = file.tellg();
//	file.seekg(0, std::ios::beg); // 重置文件指针到文件开头
//
//	// 发送文件大小作为头部包
//	CPacket headerPacket(4, (BYTE*)&fileSize, sizeof(fileSize));
//	CServerSocket::GetInstance()->SendData(headerPacket);
//
//	// 逐块读取并发送数据
//	const size_t bufferSize = 1024;
//	char buffer[bufferSize];
//	while (file.read(buffer, bufferSize) || file.gcount() > 0) {
//		size_t bytesRead = file.gcount(); // 实际读取字节数
//		CPacket packet(4, (BYTE*)buffer, bytesRead);
//		CServerSocket::GetInstance()->SendData(packet);
//	}
//
//	// 发送结束标志包
//	CPacket packet(4, nullptr, 0);
//	CServerSocket::GetInstance()->SendData(packet);
//
//	// file 自动析构关闭，也可以手动调用 file.close()
//	return 0;
//}

int MoouseEvent() {
	MOUSEEV mouse;
	if (CServerSocket::GetInstance()->GetMouseEvent(mouse)) {
		SetCursorPos(mouse.ptXY.x, mouse.ptXY.y); // 设置鼠标位置
		DWORD nFlags = 0;
		switch (mouse.nButton) {
			case 0: // 左键
				nFlags = 1;
			    break;
			case 1: // 中键
				nFlags = 2;
				break;
			case 2: // 右键
				nFlags = 4;
				break;
			case 3: // 右键
				nFlags = 8;
				break;
		}
		if (nFlags != 8) {
			SetCursorPos(mouse.ptXY.x, mouse.ptXY.y);
		}
		switch (mouse.nAction) {
			case 0: // 单机
				nFlags |= 0x10;
				break;
			case 1: // 双击
				nFlags |= 0x20;
				break;
			case 2: // 按下
				nFlags |= 0x40;
				break;
			case 3: // 放开
				nFlags |= 0x80;
				break;
			default:
				break;
		}
		switch (nFlags) {
		case 0x21:
			mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
		case 0x11:
			mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x41:
			mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x81:
			mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x22:
			mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
		case 0x12:
			mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());

			break;
		case 0x42:
			mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());

			break;
		case 0x82:
			mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());

			break;
		case 0x24:
			mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
		case 0x14:
			mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x44:
			mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());

			break;
		case 0x84:
			mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x08:
			mouse_event(MOUSEEVENTF_MOVE, mouse.ptXY.x, mouse.ptXY.y,0,GetMessageExtraInfo());
		}
		CPacket packet(4, NULL, 0);
		CServerSocket::GetInstance()->SendData(packet);
	}

	else {
		OutputDebugString(_T("获取鼠标事件失败"));
		return -1;
	}
}

int SendScreen() {
	CImage screen; //
	HDC hSCreen = GetDC(NULL);//
	int nBitPerPixel = GetDeviceCaps(hSCreen, BITSPIXEL);//得屏幕的颜色位数
	int nWidth = GetDeviceCaps(hSCreen, HORZRES);//得屏幕的宽度
	int nHeight = GetDeviceCaps(hSCreen, VERTRES);//得屏幕的高度
	screen.Create(nWidth, nHeight, nBitPerPixel); // 创建图像对象
	BitBlt(screen.GetDC(), 0, 0, 1920, 1020, hSCreen, 0, 0, SRCCOPY); // 从屏幕复制图像到图像对象
	ReleaseDC(NULL, hSCreen); // 释放屏幕设备上下文
	HGDIOBJ hBitmap = GlobalAlloc(GMEM_MOVEABLE,0); 
	if (hBitmap == NULL) return -1; // 分配全局内存对象
	IStream* pStream = NULL;
	HRESULT ret =  CreateStreamOnHGlobal(hBitmap, TRUE, &pStream); // 创建内存流
	if (ret == S_OK) {
		screen.Save(pStream, Gdiplus::ImageFormatJPEG); 
		LARGE_INTEGER bg = { 0 };
		pStream->Seek(bg, STREAM_SEEK_SET,NULL); // 重置流位置到开头
		PBYTE pData = (PBYTE)GlobalLock(hBitmap); // 锁定全局内存对象
		SIZE_T nSize = GlobalSize(hBitmap); // 获取全局内存对象的大小
		CPacket packet(6, pData, nSize); // 创建数据包
		CServerSocket::GetInstance()->SendData(packet);
		GlobalUnlock(hBitmap); // 解锁全局内存对象
	}
	pStream->Release(); // 释放内存流
	GlobalFree(hBitmap); // 释放全局内存对象
	screen.ReleaseDC(); // 释放图像对象的设备上下文
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
			int nCmd = 6;
			switch (nCmd)
			{
			case 1:
				MakeDriverInfo();
				break;
			case 2:
				MakeDirectoryInfo();
				break;
			case 3:
				RunFile();
				break;
			case 4:
				DownloadFile();
				break;
			case 5:
				MoouseEvent();
				break;
			case 6://发送屏幕
				SendScreen();
				break;
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

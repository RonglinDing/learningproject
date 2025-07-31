﻿// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "ServerSocket.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 唯一的应用程序对象

CWinApp theApp;

using namespace std;

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
			//先写难的部分，后写简单的部分
            //套接字初始
			CServerSocket* serverSocket = CServerSocket::GetInstance();
			int count = 0;
            while (CServerSocket::GetInstance()!= NULL) {
                if (serverSocket == nullptr)
                {
                    if (serverSocket->InitServerSocket() == false) {
                        MessageBox(nullptr, L"网络初始化失败", L"错误", MB_OK | MB_ICONERROR);
                        exit(0);
                    }
                    if (serverSocket->AcceptClient() == false) {
                        if (count >= 3) {
                            MessageBox(nullptr, L"多次接入客户端失败，退出程序", L"错误", MB_OK | MB_ICONERROR);
                        }
                        MessageBox(nullptr, L"接受客户端失败", L"错误", MB_OK | MB_ICONERROR);
                        count++;
                    }
                    int ret = serverSocket->DealCommand();
                }
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

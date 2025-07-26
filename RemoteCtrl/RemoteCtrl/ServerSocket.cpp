#include "pch.h"
#include "ServerSocket.h"

CServerSocket server;
CServerSocket* CServerSocket::m_instance = NULL;
CServerSocket* psever = CServerSocket::GetInstance();
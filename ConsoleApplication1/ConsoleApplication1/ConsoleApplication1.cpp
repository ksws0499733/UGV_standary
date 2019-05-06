// ConsoleApplication1.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"



#ifndef _WIN32_WINNT  
#define _WIN32_WINNT 0x0501  
#endif  

#include <WinSock2.h>
#include <windows.h>  
#include <winioctl.h>  
#include <Iphlpapi.h>
#include <iostream>

#pragma comment(lib, "Iphlpapi.lib") //需要添加Iphlpapi.lib库

//  
BOOL GetPhyDriveSerial(LPTSTR pModelNo, LPTSTR pSerialNo);
void ToLittleEndian(PUSHORT pWords, int nFirstIndex, int nLastIndex, LPTSTR pBuf);
void TrimStart(LPTSTR pBuf);

//  
// Model Number: 40 ASCII Chars  
// SerialNumber: 20 ASCII Chars  
//  
BOOL GetPhyDriveSerial(LPTSTR pModelNo, LPTSTR pSerialNo)
{
	//-1是因为 SENDCMDOUTPARAMS 的结尾是 BYTE bBuffer[1];  
	BYTE IdentifyResult[sizeof(SENDCMDOUTPARAMS) + IDENTIFY_BUFFER_SIZE - 1];
	DWORD dwBytesReturned;
	GETVERSIONINPARAMS get_version;
	SENDCMDINPARAMS send_cmd = { 0 };

	HANDLE hFile = CreateFile(_T("\\\\.\\PHYSICALDRIVE0"), GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return FALSE;

	//get version  
	DeviceIoControl(hFile, SMART_GET_VERSION, NULL, 0,
		&get_version, sizeof(get_version), &dwBytesReturned, NULL);

	//identify device  
	send_cmd.irDriveRegs.bCommandReg = (get_version.bIDEDeviceMap & 0x10) ? ATAPI_ID_CMD : ID_CMD;
	DeviceIoControl(hFile, SMART_RCV_DRIVE_DATA, &send_cmd, sizeof(SENDCMDINPARAMS) - 1,
		IdentifyResult, sizeof(IdentifyResult), &dwBytesReturned, NULL);
	CloseHandle(hFile);

	//adjust the byte order  
	PUSHORT pWords = (USHORT*)(((SENDCMDOUTPARAMS*)IdentifyResult)->bBuffer);
	ToLittleEndian(pWords, 27, 46, pModelNo);
	ToLittleEndian(pWords, 10, 19, pSerialNo);
	return TRUE;
}

//把WORD数组调整字节序为little-endian，并滤除字符串结尾的空格。  
void ToLittleEndian(PUSHORT pWords, int nFirstIndex, int nLastIndex, LPTSTR pBuf)
{
	int index;
	LPTSTR pDest = pBuf;
	for (index = nFirstIndex; index <= nLastIndex; ++index)
	{
		pDest[0] = pWords[index] >> 8;
		pDest[1] = pWords[index] & 0xFF;
		pDest += 2;
	}
	*pDest = 0;

	//trim space at the endof string; 0x20: _T(' ')  
	--pDest;
	while (*pDest == 0x20)
	{
		*pDest = 0;
		--pDest;
	}
}

//滤除字符串起始位置的空格  
void TrimStart(LPTSTR pBuf)
{
	if (*pBuf != 0x20)
		return;

	LPTSTR pDest = pBuf;
	LPTSTR pSrc = pBuf + 1;
	while (*pSrc == 0x20)
		++pSrc;

	while (*pSrc)
	{
		*pDest = *pSrc;
		++pDest;
		++pSrc;
	}
	*pDest = 0;
}
using namespace std;
int main_MAC(char maxInfo[128][128])
{
	//char maxInfo[128][128] = {""};
	DWORD netCardNum = 0;
	PIP_ADAPTER_INFO pIpAdapterInfo = new IP_ADAPTER_INFO();
	unsigned long stSize = sizeof(IP_ADAPTER_INFO);

	int nRel = GetAdaptersInfo(pIpAdapterInfo, &stSize);
	
	if (ERROR_BUFFER_OVERFLOW == nRel)	{
		delete pIpAdapterInfo;
		pIpAdapterInfo = (PIP_ADAPTER_INFO)new BYTE[stSize];
		nRel = GetAdaptersInfo(pIpAdapterInfo, &stSize);
	}

	if (ERROR_SUCCESS == nRel)	{
		while (pIpAdapterInfo)		{
			for (DWORD i = 0; i < pIpAdapterInfo->AddressLength; i++) {
				sprintf_s(maxInfo[netCardNum], "%s%02X", maxInfo[netCardNum], pIpAdapterInfo->Address[i]);
			}
			netCardNum++;
			pIpAdapterInfo = pIpAdapterInfo->Next;
		}
		for(int ii=0;ii<netCardNum;ii++)
			cout << maxInfo[ii] << endl;
	}
	cout << "---------------\n";
	if (pIpAdapterInfo)	{
		delete pIpAdapterInfo;
	}
	return netCardNum;
}

int _tmain(int argc, _TCHAR* argv[])
{
	TCHAR szModelNo[48], szSerialNo[24];
	if (GetPhyDriveSerial(szModelNo, szSerialNo))
	{
		_tprintf(_T("Model No: %s\n"), szModelNo);
		TrimStart(szSerialNo);
		_tprintf(_T("Serial No: %s\n"), szSerialNo);
	}
	else
	{
		_tprintf(_T("Failed.\n"));
	}
	char maxInfo[128][128] = { "" };
	int len = main_MAC(maxInfo);
	for (int ii = 0; ii < len; ii++)
		cout << maxInfo[ii] << endl;

	getchar();
	return 0;

	return 0;
}
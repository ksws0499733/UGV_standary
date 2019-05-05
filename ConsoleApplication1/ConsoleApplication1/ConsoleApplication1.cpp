// ConsoleApplication1.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
#include <iostream>

#include "stdafx.h"  

#ifndef _WIN32_WINNT  
#define _WIN32_WINNT 0x0501  
#endif  

#include <windows.h>  
#include <winioctl.h>  

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
	//-1����Ϊ SENDCMDOUTPARAMS �Ľ�β�� BYTE bBuffer[1];  
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

//��WORD��������ֽ���Ϊlittle-endian�����˳��ַ�����β�Ŀո�  
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

//�˳��ַ�����ʼλ�õĿո�  
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
	getchar();
	return 0;

	return 0;
}
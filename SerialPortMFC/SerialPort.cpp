// SerialPort.cpp
#include "pch.h" // 또는 "stdafx.h"
#include "SerialPort.h"

CSerialPort::CSerialPort()
{
    // 핸들을 유효하지 않은 값으로 초기화
    m_hComm = INVALID_HANDLE_VALUE;
}

CSerialPort::~CSerialPort()
{
   
}


std::vector<std::string> CSerialPort::GetAvailablePorts()
{
    std::vector<std::string> portList;
    HKEY hKey;
    LONG result;

    // 1. 레지스트리 키 열기
    result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, "HARDWARE\\DEVICEMAP\\SERIALCOMM", 0, KEY_READ, &hKey);
    if (result != ERROR_SUCCESS) {
        return portList;
    }

    // 2. 포트 개수 확인
    DWORD portCount = 0;
    result = RegQueryInfoKey(hKey, NULL, NULL, NULL, NULL, NULL, NULL, &portCount, NULL, NULL, NULL, NULL);
    if (result != ERROR_SUCCESS || portCount == 0) {
        RegCloseKey(hKey);
        return portList;
    }

    // 3. 포트 개수만큼 반복
    for (DWORD i = 0; i < portCount; i++) {
        char valueName[255];
        char portName[255];
        DWORD valueNameSize = 255;
        DWORD portNameSize = 255;
        DWORD type;

        // 4. i번째 포트 정보 읽기
        result = RegEnumValue(hKey, i, valueName, &valueNameSize, NULL, &type, (LPBYTE)portName, &portNameSize);

        if (result == ERROR_SUCCESS) {
            portList.push_back(portName);
        }
    }

    // 5. 키 닫기
    RegCloseKey(hKey);
    return portList;
}


bool CSerialPort::IsConnected()
{
    return m_bConnected;
}

void CSerialPort::Disconnect()
{
    if (m_bConnected && m_hComm != INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_hComm); // 포트 핸들 닫기
        m_hComm = INVALID_HANDLE_VALUE;
        m_bConnected = false;
    }
}

BOOL CSerialPort::Connect(CString portName, DCB& dcb, CWnd* pParent)
{
    // 1. 기존 연결이 있다면 완전히 해제
    Disconnect();

    // 2. 포트 이름 포맷팅 및 열기
    CString formattedPortName;
    formattedPortName.Format(_T("\\\\.\\%s"), portName);

    m_hComm = CreateFile(
        formattedPortName,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (m_hComm == INVALID_HANDLE_VALUE) {
        // 포트 열기 실패
        return FALSE;
    }

    // 3. 인자로 받은 DCB로 포트 설정 적용
    if (!SetCommState(m_hComm, &dcb)) {
        Disconnect();
        return FALSE;
    }

    // 4. 통신 타임아웃 설정
    COMMTIMEOUTS timeouts;
    timeouts.ReadIntervalTimeout = MAXDWORD;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.ReadTotalTimeoutConstant = 0; // 즉시 리턴하도록 설정
    timeouts.WriteTotalTimeoutMultiplier = 0;
    timeouts.WriteTotalTimeoutConstant = 0;

    if (!SetCommTimeouts(m_hComm, &timeouts)) {
        Disconnect();
        return FALSE;
    }

  

    m_bConnected = true;
    return TRUE;
}
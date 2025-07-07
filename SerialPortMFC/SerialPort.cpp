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

BOOL CSerialPort::Connect(CString portName, DWORD baudRate, BYTE parity, BYTE dataBits, BYTE stopBits, BYTE flowControl)
{
    // 1. 이미 연결되어 있다면 해제
    Disconnect();

    // 2. 포트 열기
    CString formattedPortName;
    formattedPortName.Format(_T("\\\\.\\%s"), portName);

    m_hComm = CreateFile(
        formattedPortName,                  // 포트 이름
        GENERIC_READ | GENERIC_WRITE,       // 읽기/쓰기 모드
        0,                                  // 공유 모드 (0 = 단독 사용)
        NULL,                               // 보안 속성
        OPEN_EXISTING,                      // 파일이 있을 때만 열기
        FILE_ATTRIBUTE_NORMAL,              // 일반 파일 속성
        NULL
    );

    if (m_hComm == INVALID_HANDLE_VALUE) {
        // 포트 열기 실패
        return FALSE;
    }

    // 3. 포트 설정 (DCB 구조체 사용)
    DCB dcb;
    memset(&dcb, 0, sizeof(DCB));
    dcb.DCBlength = sizeof(DCB);

    if (!GetCommState(m_hComm, &dcb)) {
        Disconnect();
        return FALSE; // 현재 상태 가져오기 실패
    }

    dcb.BaudRate = baudRate;    // 통신 속도
    dcb.ByteSize = dataBits;    // 데이터 비트
    dcb.Parity = parity;        // 패리티 (0:None, 1:Odd, 2:Even)
    dcb.StopBits = stopBits;    // 스톱 비트 (0:1, 1:1.5, 2:2)
    switch (flowControl)
    {
    case 0: // 0: None (흐름 제어 없음)
        dcb.fOutxCtsFlow = FALSE;
        dcb.fRtsControl = RTS_CONTROL_ENABLE;
        dcb.fOutX = FALSE;
        dcb.fInX = FALSE;
        break;
    case 1: // 1: Hardware (RTS/CTS)
        dcb.fOutxCtsFlow = TRUE;
        dcb.fRtsControl = RTS_CONTROL_HANDSHAKE;
        dcb.fOutX = FALSE;
        dcb.fInX = FALSE;
        break;
    case 2: // 2: Software (XON/XOFF)
        dcb.fOutxCtsFlow = FALSE;
        dcb.fRtsControl = RTS_CONTROL_ENABLE;
        dcb.fOutX = TRUE;
        dcb.fInX = TRUE;
        break;
    }
    if (!SetCommState(m_hComm, &dcb)) {
        Disconnect();
        return FALSE; // 새 상태 설정 실패
    }

    // 4. 타임아웃 설정 
    COMMTIMEOUTS timeouts;
    timeouts.ReadIntervalTimeout = MAXDWORD;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.ReadTotalTimeoutConstant = 0;
    timeouts.WriteTotalTimeoutMultiplier = 0;
    timeouts.WriteTotalTimeoutConstant = 0;

    if (!SetCommTimeouts(m_hComm, &timeouts)) {
        Disconnect();
        return FALSE; // 타임아웃 설정 실패
    }

    m_bConnected = true; // 모든 설정이 성공하면 연결 상태로 변경
    return TRUE;
}
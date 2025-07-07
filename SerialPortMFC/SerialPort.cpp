// SerialPort.cpp
#include "pch.h" // �Ǵ� "stdafx.h"
#include "SerialPort.h"

CSerialPort::CSerialPort()
{
    // �ڵ��� ��ȿ���� ���� ������ �ʱ�ȭ
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

    // 1. ������Ʈ�� Ű ����
    result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, "HARDWARE\\DEVICEMAP\\SERIALCOMM", 0, KEY_READ, &hKey);
    if (result != ERROR_SUCCESS) {
        return portList;
    }

    // 2. ��Ʈ ���� Ȯ��
    DWORD portCount = 0;
    result = RegQueryInfoKey(hKey, NULL, NULL, NULL, NULL, NULL, NULL, &portCount, NULL, NULL, NULL, NULL);
    if (result != ERROR_SUCCESS || portCount == 0) {
        RegCloseKey(hKey);
        return portList;
    }

    // 3. ��Ʈ ������ŭ �ݺ�
    for (DWORD i = 0; i < portCount; i++) {
        char valueName[255];
        char portName[255];
        DWORD valueNameSize = 255;
        DWORD portNameSize = 255;
        DWORD type;

        // 4. i��° ��Ʈ ���� �б�
        result = RegEnumValue(hKey, i, valueName, &valueNameSize, NULL, &type, (LPBYTE)portName, &portNameSize);

        if (result == ERROR_SUCCESS) {
            portList.push_back(portName);
        }
    }

    // 5. Ű �ݱ�
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
        CloseHandle(m_hComm); // ��Ʈ �ڵ� �ݱ�
        m_hComm = INVALID_HANDLE_VALUE;
        m_bConnected = false;
    }
}

BOOL CSerialPort::Connect(CString portName, DWORD baudRate, BYTE parity, BYTE dataBits, BYTE stopBits, BYTE flowControl)
{
    // 1. �̹� ����Ǿ� �ִٸ� ����
    Disconnect();

    // 2. ��Ʈ ����
    CString formattedPortName;
    formattedPortName.Format(_T("\\\\.\\%s"), portName);

    m_hComm = CreateFile(
        formattedPortName,                  // ��Ʈ �̸�
        GENERIC_READ | GENERIC_WRITE,       // �б�/���� ���
        0,                                  // ���� ��� (0 = �ܵ� ���)
        NULL,                               // ���� �Ӽ�
        OPEN_EXISTING,                      // ������ ���� ���� ����
        FILE_ATTRIBUTE_NORMAL,              // �Ϲ� ���� �Ӽ�
        NULL
    );

    if (m_hComm == INVALID_HANDLE_VALUE) {
        // ��Ʈ ���� ����
        return FALSE;
    }

    // 3. ��Ʈ ���� (DCB ����ü ���)
    DCB dcb;
    memset(&dcb, 0, sizeof(DCB));
    dcb.DCBlength = sizeof(DCB);

    if (!GetCommState(m_hComm, &dcb)) {
        Disconnect();
        return FALSE; // ���� ���� �������� ����
    }

    dcb.BaudRate = baudRate;    // ��� �ӵ�
    dcb.ByteSize = dataBits;    // ������ ��Ʈ
    dcb.Parity = parity;        // �и�Ƽ (0:None, 1:Odd, 2:Even)
    dcb.StopBits = stopBits;    // ���� ��Ʈ (0:1, 1:1.5, 2:2)
    switch (flowControl)
    {
    case 0: // 0: None (�帧 ���� ����)
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
        return FALSE; // �� ���� ���� ����
    }

    // 4. Ÿ�Ӿƿ� ���� 
    COMMTIMEOUTS timeouts;
    timeouts.ReadIntervalTimeout = MAXDWORD;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.ReadTotalTimeoutConstant = 0;
    timeouts.WriteTotalTimeoutMultiplier = 0;
    timeouts.WriteTotalTimeoutConstant = 0;

    if (!SetCommTimeouts(m_hComm, &timeouts)) {
        Disconnect();
        return FALSE; // Ÿ�Ӿƿ� ���� ����
    }

    m_bConnected = true; // ��� ������ �����ϸ� ���� ���·� ����
    return TRUE;
}
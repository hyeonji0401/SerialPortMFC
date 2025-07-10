// SerialPort.cpp
#include "pch.h" // �Ǵ� "stdafx.h"
#include "SerialPort.h"


CSerialPort::CSerialPort()
{
    // �ڵ��� ��ȿ���� ���� ������ �ʱ�ȭ
    m_hComm = INVALID_HANDLE_VALUE;
    m_bThreadRunning = FALSE;
    m_hTargetWnd = NULL;

    //���� �ʱ�ȭ
    int nowBufferPosition = 0;
    memset(&m_rxBuffer, 0, sizeof(MAX_BUFFER_SIZE));


}

CSerialPort::~CSerialPort()
{
   
}


//���� ������ ��Ʈ �̸� �޾ƿ���
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

//��Ʈ ���� ����
void CSerialPort::Disconnect()
{


    if (m_bThreadRunning)
    {
        m_bThreadRunning = FALSE;
        Sleep(100); // 0.1�� ���
    }

    if (m_hComm != INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_hComm); // ��Ʈ �ڵ� �ݱ�
        m_hComm = INVALID_HANDLE_VALUE;
        
    }


   
}

//��Ʈ ����
BOOL CSerialPort::Connect(CString portName, DCB& dcb, HWND hWnd)
{
    // ���� ������ �ִٸ� ������ ����
    Disconnect();

    // ��Ʈ �̸� ������ �� ����
    CString formattedPortName;
    formattedPortName.Format(_T("\\\\.\\%s"), portName); // CreateFile �Լ��� COM10 �̻��� ��Ʈ�� �ν��� �� �ֵ��� ��Ʈ �̸��� ������
    strPortName = portName;

    m_hComm = CreateFile( //�ø��� ��Ʈ ��ġ�� ����ó�� ���� �ڵ�(m_hDevSerial)�� ����
        formattedPortName,
        GENERIC_READ | GENERIC_WRITE, //��Ʈ ������ Ÿ�� ����
        0, //No Shared
        NULL, //��Ʈ�� ����Ʈ ���� �Ӽ� �Ҵ�
        OPEN_EXISTING, //���� ���Ͽ��� ȣ��
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, //�񵿱� ���
        NULL //���ø� ���Ͽ� ���� �ڵ� ���� -> ��Ʈ �� ���� �ʿ����
    );

    m_hTargetWnd = hWnd;      // �޽����� ���� ������ �ڵ� ����
    m_bThreadRunning = TRUE;  // ������ ���� ��� �ø���

    if (m_hComm == INVALID_HANDLE_VALUE) {
        // ��Ʈ ���� ����
        return FALSE;
    }

 
    return TRUE;
}

//��Ʈ ����
BOOL CSerialPort::SetupPort(DWORD baudrate, BYTE byteSize, BYTE parity, BYTE stopbits)
{
    //�ǽð� ��Ʈ ���� �޾ƿ���
    if (!GetCommState(m_hComm, &m_dcb))
    {
        strNotice.Format("GetCommState() Failed, Close Port(%s), Error#(%xh)", strPortName);
        MessageBox(strNotice, NULL, MB_OK | MB_ICONERROR);
        Disconnect();
        return FALSE;
    }

    m_dcb.BaudRate = baudrate;
    m_dcb.ByteSize = byteSize;
    m_dcb.Parity = parity;
    m_dcb.StopBits = stopbits;

    //�޾ƿ� ������ ��Ʈ ���� ���߱�
    if (!SetCommState(m_hComm, &m_dcb)) {
        strNotice.Format("SetCommState() Failed, Close Port(%s), Error#(%xh)", strPortName);
        MessageBox(strNotice, NULL, MB_OK | MB_ICONERROR);
        Disconnect();
        return FALSE;
    }

    COMMTIMEOUTS timeouts;
    timeouts.ReadIntervalTimeout = 0;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.ReadTotalTimeoutConstant = 0;
    timeouts.WriteTotalTimeoutMultiplier = 0;
    timeouts.WriteTotalTimeoutConstant = 0;
    if (!SetCommTimeouts(m_hComm, &timeouts))
    {
        strNotice.Format("SetCommTimeouts() Failed, Close Port(%s), Error#(%xh)", strPortName);
        MessageBox(strNotice, NULL, MB_OK | MB_ICONERROR);
    }
        




    return TRUE;
}



UINT CommThread(LPVOID pParam)
{
    CSerialPort* pThread = (CSerialPort*)pParam;
    BYTE inBuff[1024]; // �ѹ��� �о�� �ִ� ����
    DWORD nBytesRead = 0; //������ ���� ����Ʈ ��
    DWORD dwEventMask, dwError = 0 ; // �̺�Ʈ ���� ����
    COMSTAT comStat; // ��Ʈ ���� ���� ���� ����ü

    dwEventMask = EV_RXCHAR;

    if (!SetCommMask(pThread->m_hComm, EV_RXCHAR)) //RXCHAR ���� �޽��� ���� �̺�Ʈ ���� 
    {
        AfxMessageBox(_T("SetCommMask ������ �����߽��ϴ�."));
        return -1; // ������ ������ ����
    }
    
    //�̺�Ʈ�� ���� ������ ��ٸ���
    while (1)
    {
        if (!WaitCommEvent(pThread->m_hComm, &dwEventMask, NULL)) 
        //EV_RXCHAR �̺�Ʈ ���� ������ ��ٸ�
        {
            continue;
        }
        
        ClearCommError(pThread->m_hComm, &dwError, &comStat);
        //��� ���� Ȯ��, ���� ����Ʈ �� Ȯ�� 
        //dwError : ���� ���� ����
        //comStat : ���� ���� ���� ���� -> ���� ����Ʈ �� Ȯ�� ����
        DWORD nByteNum = comStat.cbInQue; // ���� ����Ʈ ��

        if (ReadFile(pThread->m_hComm, &inBuff, nByteNum, &nBytesRead, NULL))
        {
            pThread->ParseReadData(inBuff, nByteNum);
        }

    }


    return 0;
}


void CSerialPort::ParseReadData(BYTE* in, DWORD len)
{
    if (nowBufferPosition + len > MAX_BUFFER_SIZE)
    {
        AfxMessageBox(_T("���۰� ���� á���ϴ�"));
        memset(&m_rxBuffer, 0, sizeof(MAX_BUFFER_SIZE));
        nowBufferPosition = 0;
        return; 
    }

    memcpy(&m_rxBuffer[nowBufferPosition], in, len);
    nowBufferPosition += len;

    for (int i = nowBufferPosition - len; i < nowBufferPosition; i++)
    {
        
    }


}
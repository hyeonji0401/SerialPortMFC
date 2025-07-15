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
    m_rxBuffer = new CByteArray();


}

CSerialPort::~CSerialPort()
{
    if (m_rxBuffer) {
        delete(m_rxBuffer);
        m_rxBuffer = NULL;
    }
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
        Sleep(1000); // 0.1�� ���
    }

    if (m_hComm != INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_hComm); // ��Ʈ �ڵ� �ݱ�
        m_hComm = INVALID_HANDLE_VALUE;
        
    }

    if (m_rxBuffer) { 
        m_rxBuffer->RemoveAll();
    }


}

//��Ʈ ����
BOOL CSerialPort::Connect(CString portName, DCB& dcb, HWND hWnd)
{
    // ���� ������ �ִٸ� ������ ����
    if (m_hComm != INVALID_HANDLE_VALUE)
    {
        Disconnect();
    }

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
    timeouts.ReadIntervalTimeout = MAXWORD;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.ReadTotalTimeoutConstant = 0;
    if (!SetCommTimeouts(m_hComm, &timeouts))
    {
        strNotice.Format("SetCommTimeouts() Failed, Close Port(%s), Error#(%xh)", strPortName);
        MessageBox(strNotice, NULL, MB_OK | MB_ICONERROR);
        Disconnect();
        return FALSE;
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
    OVERLAPPED overlap;

    memset(&overlap, 0, sizeof(overlap));
    overlap.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (overlap.hEvent == NULL)
    {
        return -1;
    }
  
    
    dwEventMask = EV_RXCHAR;
    if (!SetCommMask(pThread->m_hComm, EV_RXCHAR)) //RXCHAR ���� �޽��� ���� �̺�Ʈ ���� 
    {
        CloseHandle(overlap.hEvent);
        return -1; // ������ ������ ����
    }
   
    //�̺�Ʈ�� ���� ������ ��ٸ���
    while (pThread->m_bThreadRunning)
    {
        //EV_RXCHAR �̺�Ʈ ���� ������ ��ٸ�
        if (WaitCommEvent(pThread->m_hComm, &dwEventMask, NULL))
        {
            //��� ���� Ȯ��, ���� ����Ʈ �� Ȯ�� 
            //dwError : ���� ���� ����
            //comStat : ���� ���� ���� ���� -> ���� ����Ʈ �� Ȯ�� ����
            if (ClearCommError(pThread->m_hComm, &dwError, &comStat))
            {
                if (comStat.cbInQue > 0)
                {
                    // �о�� ����Ʈ ���� ���� // ����(inBuff) ũ��� ���� ���� �� �� ���� ���� ����.
                    DWORD dwToRead = min((DWORD)sizeof(inBuff), comStat.cbInQue);

                    // �񵿱� ReadFile ȣ�� // ������ ����Ʈ ����ŭ�� �б⸦ �õ�
                    if (!ReadFile(pThread->m_hComm, inBuff, dwToRead, &nBytesRead, &overlap))
                    {
                        if (GetLastError() == ERROR_IO_PENDING)
                        {
                            // �񵿱� �۾��� ���� ���̸� �Ϸ�� ������ ���
                            GetOverlappedResult(pThread->m_hComm, &overlap, &nBytesRead, TRUE);
                        }
                        else
                        {
                            break;
                        }
                    }
                    if (nBytesRead > 0)
                    {
                        pThread->ParseReadData(inBuff, nBytesRead);
                    }
                }

            }
        }
        else
        {
            if (GetLastError() == ERROR_INVALID_HANDLE) {
                pThread->m_bThreadRunning = FALSE;
            }
        }
        


    }
    CloseHandle(overlap.hEvent);
    return 0;
}



void CSerialPort::ParseReadData(BYTE* in, DWORD len)
{
    int i;


    if ((m_rxBuffer->GetSize() + len) > MAX_BUFFER_SIZE) //���� ���� �����ͷ� ���� ���۰� ������ ���
    {
        ::PostMessage(m_hTargetWnd, WM_USER_BUFFER_FULL, 0, 0);
        return;
    }

    int nOldSize = m_rxBuffer->GetSize(); // ���� ���� ũ�� ����
    m_rxBuffer->SetSize(nOldSize + len);  // ���ο� ������ ���̸�ŭ ���� ũ�� �ø���
    memcpy(m_rxBuffer->GetData() + nOldSize, in, len); // �þ ������ ���� ��ġ�� ������ ����

    while (TRUE)
    {
        int nEndOfPacketPosition = -1;
        for (i = 0; i < m_rxBuffer->GetSize(); i++)
        {
            if (m_rxBuffer->GetAt(i) == '\n')
            {
                nEndOfPacketPosition = i;
                break;
            }
        }

        if (nEndOfPacketPosition != (-1)) //CR/LF�� �߰��� ���
        {

            int nPacketlen = nEndOfPacketPosition;
            // \n���� \r�� ���� �� Ȯ��
            if (m_rxBuffer->GetAt(nPacketlen - 1) == '\r' && (nPacketlen > 0))
            {
                nPacketlen--;
            }
            //������ ����� �����Ͱ� �ִ��� Ȯ��
            if (nPacketlen > 0)
            {
                char* pPacketData = new char[nPacketlen + 1];

                memcpy(pPacketData, m_rxBuffer->GetData(), nPacketlen);
                pPacketData[nPacketlen] = '\0'; // C-String�� ���� ǥ��

                // UI ������� �޽��� ���� (�̸� �����ص� ������ �ڵ� ���)
                ::PostMessage(m_hTargetWnd, WM_USER_RX_DATA, (WPARAM)nPacketlen, (LPARAM)pPacketData);
            }

            m_rxBuffer->RemoveAt(0, nEndOfPacketPosition + 1); //���� ����

        }
        else
        {
            break;
        }
    }


}
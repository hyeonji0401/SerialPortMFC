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

    isSLIP = FALSE;
    isCRC = FALSE;


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
        if (m_hThread != NULL) { //�����尡 ���� ������ ���
            WaitForSingleObject(m_hThread, 2000);
            CloseHandle(m_hThread);
            m_hThread = NULL;
        }
 
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

    m_hComm = CreateFile( //�ø��� ��Ʈ ��ġ�� ����ó�� ���� �ڵ��� ����
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
BOOL CSerialPort::SetupPort(DWORD baudrate, BYTE byteSize, BYTE parity, BYTE stopbits, class CEdit &settingInfo)
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

    //��Ʈ ���� �ʱ�ȭ
    if (!PurgeComm(m_hComm, PURGE_RXCLEAR | PURGE_TXCLEAR))
    {
        MessageBox(_T("��Ʈ ���۸� ���� �� �����߽��ϴ�."), NULL, MB_OK | MB_ICONERROR);
    }

    if (settingInfo)
    {
        char	temp[20], strTemp[100];
        CString	info;

        memset(temp, 0, sizeof(temp));
        memset(strTemp, 0, sizeof(strTemp));

        sprintf_s(strTemp, "%s-%d Bps-", strPortName, baudrate);

        sprintf_s(temp, "%d-", byteSize);
        strcat_s(strTemp, temp);

        switch (parity) {
        case 0: sprintf_s(temp, "No-"); break;
        case 1: sprintf_s(temp, "Odd-"); break;
        case 2: sprintf_s(temp, "Even-"); break;
        case 3: sprintf_s(temp, "Mark-"); break;
        case 4: sprintf_s(temp, "Space-"); break;
        }
        strcat_s(strTemp, temp);

        switch (stopbits) {
        case 0: sprintf_s(temp, "1\r\n"); break;
        case 1: sprintf_s(temp, "1.5\r\n"); break;
        case 2: sprintf_s(temp, "2\r\n"); break;
        }
        strcat_s(strTemp, temp);

        info.Format("%s", strTemp);

        settingInfo.SetWindowText(info);
        settingInfo.Invalidate();
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
    OVERLAPPED overlap; // �񵿱� �Է� �� ��¿� ���Ǵ� ����

    memset(&overlap, 0, sizeof(overlap));
    overlap.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL); // �۾��� �Ϸ�ɶ� �ý��׸漭 ��ȣ�� �޴� ���·� �����Ǵ� �̺�Ʈ�� ���� �ڵ�
                                // 1. ���� �ڵ��� �ڽ� ���μ����� ��ӹ��� ���ΰ�
                                // 2. �̺�Ʈ�� �ڵ����� ������ ���ΰ�
                                // 3. �ʱⰪ�� �ñ׳η� �� ���ΰ� 
                               // 4. �̺�Ʈ ��ü �̸� ����
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

        //EV_RXCHAR �̺�Ʈ �߻� ���� ��û
        if (!WaitCommEvent(pThread->m_hComm, &dwEventMask, &overlap))
        {
            if (GetLastError() != ERROR_IO_PENDING)
            {
                break;
            }
        }

        //�̺�Ʈ �߻� �ñ��� ���
        if (WaitForSingleObject(overlap.hEvent, INFINITE) == WAIT_OBJECT_0) // ������ ��ü�� ��ȣ ���°� �ǰų� �ð� ���� ������ ����Ҷ��� ��ٸ�
                                                //���� ���   //��ٸ��� evnet�� ��ȣ���°� ��       
        {
            // �̺�Ʈ ó�� ����
            ResetEvent(overlap.hEvent); //�̺�Ʈ ���� �Ϸ������� ���� �̺�Ʈ�� ���� OFF

            //��� ���� Ȯ��, ���� ����Ʈ �� Ȯ�� 
            //dwError : ���� ���� ����
            //comStat : ���� ���� ���� ���� -> ���� ����Ʈ �� Ȯ�� ����
            if (pThread->m_bThreadRunning && ClearCommError(pThread->m_hComm, &dwError, &comStat))
            {
                if (comStat.cbInQue > 0)
                {
                    // �о�� ����Ʈ ���� ���� // ����(inBuff) ũ��� ���� ���� �� �� ���� ���� ����.
                    DWORD dwToRead = min((DWORD)sizeof(inBuff), comStat.cbInQue);

                    // �񵿱� ReadFile ȣ�� // ������ ����Ʈ ����ŭ�� �б⸦ �õ�
                    if (!ReadFile(pThread->m_hComm, inBuff, dwToRead, &nBytesRead, &overlap)) 
                        // ������ �о���� ���� OR �񵿱� ����� �Ϸ� �� 0�� ��ȯ
                    {
                        if (GetLastError() == ERROR_IO_PENDING)  //ERROR_IO_PENDING�� ������ �ִٸ� ���а� �ƴ϶� �񵿱� ����� �ϷḦ �ǹ���
                        {
                            // �񵿱� �۾��� ���� ���̸� �Ϸ�� ������ ���
                            // ������ �μ��� ������� �Ϸ�� ������ ����� ���ΰ� �ƴѰ��� ����
                            //�� �μ��� TRUE �̸� �񵿱� ������� �߰��� �����ϰ� ������� �Ϸ�� ������ �������� �ʴ´�
                            //����� ������ �����ϰ� �ٷ� �����Ϸ��� FALSE���� �ϸ� �� �Լ��� �Ϸ� �������� ����Ϸ��� TRUE�� �ָ� ��
                            //[��ó] ���� ����� - �񵿱� ����� FILE_FLAG_OVERLAPPED, OVERLAPPED, GetOverlappedResult | �ۼ��� �޸�ī��
                            GetOverlappedResult(pThread->m_hComm, &overlap, &nBytesRead, TRUE); //���� ��� �޾ƿ���
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

    }
        
    CloseHandle(overlap.hEvent);
    return 0;
}

void CSerialPort::ClearRxBuffer()
{
    if (m_rxBuffer) // �����Ͱ� ��ȿ���� Ȯ��
    {
        m_rxBuffer->RemoveAll(); // ������ ��� ������ ����
    }
}

/*
void CSerialPort::ParseReadData(BYTE* in, DWORD len)
{
    if (isCRC)
    {
        ParseReadDataCRC(in, len); // SLIP ���� SLIP �ļ� ȣ��
    }
    else
    {
        ParseReadDataLF(in, len); // �ƴϸ� �Ϲ� �ļ� ȣ��
    }
}*/


void CSerialPort::ParseReadData(BYTE* in, DWORD len)
{
    //  SLIP ��尡 ���� �ִ��� Ȯ��
    if (isSLIP)
    {
        ParseReadDataChecksum(in, len);
        return; // SLIP �ļ��� ���� �������Ƿ� ���⼭ ����
    }

    if (isCRC)
    {
        ParseReadDataCRC(in, len);
        return;
    }

    // SLIP, CRC ��尡 �ƴ϶�� (�ܼ� ǥ�� ���),
    //  �ƹ��͵� �ؼ����� �ʰ� ���� �����͸� �״�� UI ������� ����
    if (len > 0)
    {
        // UI �����忡�� delete[] �� �� �ֵ��� �޸𸮸� ���� �Ҵ��Ͽ� �����͸� ����
        char* pPacketData = new char[len];
        memcpy(pPacketData, in, len);

        // UI ������� ������ ����� �״�� ����
        ::PostMessage(m_hTargetWnd, WM_USER_RX_DATA, (WPARAM)len, (LPARAM)pPacketData);
    }
}


//CRC ��� ���̺� :  CRC ����� �ſ� ������ ����� ���� ����ϴ� �̸� ����� ���� ������
uint8_t crc8_table[256] = {
    0x00, 0x07, 0x0E, 0x09, 0x1C, 0x1B, 0x12, 0x15, 0x38, 0x3F, 0x36, 0x31, 0x24, 0x23, 0x2A, 0x2D,
    0x70, 0x77, 0x7E, 0x79, 0x6C, 0x6B, 0x62, 0x65, 0x48, 0x4F, 0x46, 0x41, 0x54, 0x53, 0x5A, 0x5D,
    0xE0, 0xE7, 0xEE, 0xE9, 0xFC, 0xFB, 0xF2, 0xF5, 0xD8, 0xDF, 0xD6, 0xD1, 0xC4, 0xC3, 0xCA, 0xCD,
    0x90, 0x97, 0x9E, 0x99, 0x8C, 0x8B, 0x82, 0x85, 0xA8, 0xAF, 0xA6, 0xA1, 0xB4, 0xB3, 0xBA, 0xBD,
    0xC7, 0xC0, 0xC9, 0xCE, 0xDB, 0xDC, 0xD5, 0xD2, 0xFF, 0xF8, 0xF1, 0xF6, 0xE3, 0xE4, 0xED, 0xEA,
    0xB7, 0xB0, 0xB9, 0xBE, 0xAB, 0xAC, 0xA5, 0xA2, 0x8F, 0x88, 0x81, 0x86, 0x93, 0x94, 0x9D, 0x9A,
    0x27, 0x20, 0x29, 0x2E, 0x3B, 0x3C, 0x35, 0x32, 0x1F, 0x18, 0x11, 0x16, 0x03, 0x04, 0x0D, 0x0A,
    0x57, 0x50, 0x59, 0x5E, 0x4B, 0x4C, 0x45, 0x42, 0x6F, 0x68, 0x61, 0x66, 0x73, 0x74, 0x7D, 0x7A,
    0x89, 0x8E, 0x87, 0x80, 0x95, 0x92, 0x9B, 0x9C, 0xB1, 0xB6, 0xBF, 0xB8, 0xAD, 0xAA, 0xA3, 0xA4,
    0xF9, 0xFE, 0xF7, 0xF0, 0xE5, 0xE2, 0xEB, 0xEC, 0xC1, 0xC6, 0xCF, 0xC8, 0xDD, 0xDA, 0xD3, 0xD4,
    0x69, 0x6E, 0x67, 0x60, 0x75, 0x72, 0x7B, 0x7C, 0x51, 0x56, 0x5F, 0x58, 0x4D, 0x4A, 0x43, 0x44,
    0x19, 0x1E, 0x17, 0x10, 0x05, 0x02, 0x0B, 0x0C, 0x21, 0x26, 0x2F, 0x28, 0x3D, 0x3A, 0x33, 0x34,
    0x4E, 0x49, 0x40, 0x47, 0x52, 0x55, 0x5C, 0x5B, 0x76, 0x71, 0x78, 0x7F, 0x6A, 0x6D, 0x64, 0x63,
    0x3E, 0x39, 0x30, 0x37, 0x22, 0x25, 0x2C, 0x2B, 0x06, 0x01, 0x08, 0x0F, 0x1A, 0x1D, 0x14, 0x13,
    0xAE, 0xA9, 0xA0, 0xA7, 0xB2, 0xB5, 0xBC, 0xBB, 0x96, 0x91, 0x98, 0x9F, 0x8A, 0x8D, 0x84, 0x83,
    0xDE, 0xD9, 0xD0, 0xD7, 0xC2, 0xC5, 0xCC, 0xCB, 0xE6, 0xE1, 0xE8, 0xEF, 0xFA, 0xFD, 0xF4, 0xF3
};

// ���̺��� ����Ͽ� CRC-8 ���� ����ϴ� �Լ�
uint8_t CSerialPort::CalculateCRC8(const unsigned char* data, int length)
{
    unsigned char crc = 0; // �ʱⰪ�� 0
    while (length--)
    {
        crc = crc8_table[crc ^ *data++]; //�����Ϳ� XOR ����
    }
    return crc;
}


// CRC
void CSerialPort::ParseReadDataCRC(BYTE* in, DWORD len)
{
    int i = 0;

    if ((m_rxBuffer->GetSize() + len) > MAX_BUFFER_SIZE) //���� ���� �����ͷ� ���� ���۰� ������ ���
    {
        ::PostMessage(m_hTargetWnd, WM_USER_BUFFER_FULL, 0, 0);
        return;
    }

    int nOldSize = m_rxBuffer->GetSize();
    m_rxBuffer->SetSize(nOldSize + len);
    memcpy(m_rxBuffer->GetData() + nOldSize, in, len);

    int nScanStartPosition = 0;

    while (TRUE)
    {
        // ��Ŷ�� ����(0x02)�� ã��
        int nStartOfPacket = -1;
        for (i = nScanStartPosition; i < m_rxBuffer->GetSize(); i++) {
            if (m_rxBuffer->GetAt(i) == 0x02) {
                nStartOfPacket = i;
                break;
            }
        }
        if (nStartOfPacket == -1) break;

        // ��Ŷ�� ��(���� 0x03)�� ã��
        int nEndOfPacket = -1;
        for (i = nStartOfPacket + 1; i < m_rxBuffer->GetSize(); i++) {
            if (m_rxBuffer->GetAt(i) == 0x03) {
                nEndOfPacket = i;
                break;
            }
        }
        if (nEndOfPacket == -1) break;

        // protocol ���ڵ� 
        CByteArray decodedPacket; // ���ڵ��� �����͸� ���� �ӽ� ����
        for (i = nStartOfPacket + 1; i < nEndOfPacket; i++)
        {
            if (m_rxBuffer->GetAt(i) == 0x1B) // ESC ���ڸ� ������
            {
                i++; // ���� ����Ʈ�� Ȯ��
                if (i < nEndOfPacket)
                {
                    if (m_rxBuffer->GetAt(i) == 0xE2) {
                        decodedPacket.Add(0x02); 
                    }
                    else if (m_rxBuffer->GetAt(i) == 0xE3) {
                        decodedPacket.Add(0x03); 
                    }
                    else if (m_rxBuffer->GetAt(i) == 0xE1)
                    {
                        decodedPacket.Add(0x1B);
                    }
                }
            }
            else // �Ϲ� �����ʹ� �״�� ����
            {
                decodedPacket.Add(m_rxBuffer->GetAt(i));
            }
        }



        // CRC ��� �� ������ ����
        if (decodedPacket.GetSize() >= 2) // �ּ� ����: Length(1) + CRC(1)
        {
            BYTE nDataLength = decodedPacket.GetAt(0);

            // ���� ������ ���̿� ��Ŷ�� ��ϵ� ���̰� �´��� Ȯ��
            if (decodedPacket.GetSize() == (nDataLength + 2)) //���� ������ ���� + Length(1) + CRC(1)
            {
                BYTE* pData = decodedPacket.GetData() + 1; //������ ������
                BYTE receivedCRC = decodedPacket.GetAt(1 + nDataLength); //���� ������ ����

                //  CRC-8 (���ڵ��� ������ ����)
                BYTE calculatedCRC = CalculateCRC8(pData, nDataLength);

                // CRC ����
                if (calculatedCRC == receivedCRC)
                {
                    // ���� �����͸� UI ������� ����
                    char* pPacketData = new char[nDataLength + 1];
                    memcpy(pPacketData, pData, nDataLength);
                    pPacketData[nDataLength] = '\0';
                    ::PostMessage(m_hTargetWnd, WM_USER_RX_DATA, (WPARAM)nDataLength, (LPARAM)pPacketData);
                }
            }
        }

        // ó���� ���� ��Ŷ��ŭ ��ĵ ��ġ �̵�
        nScanStartPosition = nEndOfPacket + 1;
    }

    // ó���� ��� �����͸� ���� �տ��� �� ���� ����
    if (nScanStartPosition > 0)
    {
        m_rxBuffer->RemoveAt(0, nScanStartPosition);
    }
}

// ���� CR/LF�� ���� ��� 
/*
void CSerialPort::ParseReadDataLF(BYTE* in, DWORD len)
{
    int i=0;


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
                nEndOfPacketPosition = i; //��Ŷ �� ��ġ ����
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
                BYTE* pPacketData = new BYTE[nPacketlen + 1];

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


}*/

void CSerialPort::ParseReadDataChecksum(BYTE* in, DWORD len)
{
    int i=0;

    if ((m_rxBuffer->GetSize() + len) > MAX_BUFFER_SIZE) //���� ���� �����ͷ� ���� ���۰� ������ ���
    {
        ::PostMessage(m_hTargetWnd, WM_USER_BUFFER_FULL, 0, 0);
        return;
    }

    int nOldSize = m_rxBuffer->GetSize();
    m_rxBuffer->SetSize(nOldSize + len);
    memcpy(m_rxBuffer->GetData() + nOldSize, in, len);

    int nScanStartPosition = 0;

    while (TRUE)
    {
        // ��Ŷ�� ����(0xC0)�� ã��
        int nStartOfPacket = -1;
        for (i = nScanStartPosition; i < m_rxBuffer->GetSize(); i++) {
            if (m_rxBuffer->GetAt(i) == 0xC0) {
                nStartOfPacket = i;
                break;
            }
        }
        if (nStartOfPacket == -1) break;

        // ��Ŷ�� ��(���� 0xC0)�� ã��
        int nEndOfPacket = -1;
        for (i = nStartOfPacket + 1; i < m_rxBuffer->GetSize(); i++) {
            if (m_rxBuffer->GetAt(i) == 0xC0) {
                nEndOfPacket = i;
                break;
            }
        }
        if (nEndOfPacket == -1) break;

        // SLIP ���ڵ� 
        CByteArray decodedPacket; // ���ڵ��� �����͸� ���� �ӽ� ����
        for (i = nStartOfPacket + 1; i < nEndOfPacket; i++)
        {
            if (m_rxBuffer->GetAt(i) == 0xDB) // ESC ���ڸ� ������
            {
                i++; // ���� ����Ʈ�� Ȯ��
                if (i < nEndOfPacket)
                {
                    if (m_rxBuffer->GetAt(i) == 0xDC) {
                        decodedPacket.Add(0xC0); // DB DC -> C0
                    }
                    else if (m_rxBuffer->GetAt(i) == 0xDD) {
                        decodedPacket.Add(0xDB); // DB DD -> DB
                    }
                }
            }
            else // �Ϲ� �����ʹ� �״�� ����
            {
                decodedPacket.Add(m_rxBuffer->GetAt(i));
            }
        }



        // üũ�� ��� �� ������ ����
        if (decodedPacket.GetSize() >= 2) // �ּ� ����: Length(1) + Checksum(1)
        {
            BYTE nDataLength = decodedPacket.GetAt(0);

            // ���� ������ ���̿� ��Ŷ�� ��ϵ� ���̰� �´��� Ȯ��
            if (decodedPacket.GetSize() == (nDataLength + 2)) //���� ������ ���� + Length(1) + Checksum(1)
            {
                BYTE* pData = decodedPacket.GetData() + 1; //������ ������
                BYTE nChecksum = decodedPacket.GetAt(1 + nDataLength); //���� ������ ����

                //  üũ�� ��� (���ڵ��� ������ ����)
                BYTE calculatedChecksum = 0;
                //BYTE�� ũ�� : BYTE��� ������ Ÿ���� ��Ȯ�� 1����Ʈ(8��Ʈ) ũ��
                //255�� ���� ���(�����÷�) �ڵ����� 0���� �����ϰ� �� -> 256�� ���� �ʿ����
                for (int i = 0; i < nDataLength; i++)
                {
                    calculatedChecksum += pData[i]; //������ �ջ�
                }

                // üũ�� ����
                if (calculatedChecksum == nChecksum)
                {
                    // ���� �����͸� UI ������� ����
                    char* pPacketData = new char[nDataLength + 1];
                    memcpy(pPacketData, pData, nDataLength);
                    pPacketData[nDataLength] = '\0';
                    ::PostMessage(m_hTargetWnd, WM_USER_RX_DATA, (WPARAM)nDataLength, (LPARAM)pPacketData);
                }
            }
        }

        // ó���� ���� ��Ŷ��ŭ ��ĵ ��ġ �̵�
        nScanStartPosition = nEndOfPacket + 1;
    }

    // ó���� ��� �����͸� ���� �տ��� �� ���� ����
    if (nScanStartPosition > 0)
    {
        m_rxBuffer->RemoveAt(0, nScanStartPosition);
    }
}
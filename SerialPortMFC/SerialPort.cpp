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
    if (isCRC)
    {
        ParseReadDataCRC(in, len);
        return; // SLIP �ļ��� ���� �������Ƿ� ���⼭ ����
    }

    // SLIP ��尡 �ƴ϶�� (�ܼ� ǥ�� ���),
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

void CSerialPort::ParseReadDataCRC(BYTE* in, DWORD len)
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
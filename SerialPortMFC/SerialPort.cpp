// SerialPort.cpp
#include "pch.h" // 또는 "stdafx.h"
#include "SerialPort.h"


CSerialPort::CSerialPort()
{
    // 핸들을 유효하지 않은 값으로 초기화
    m_hComm = INVALID_HANDLE_VALUE;
    m_bThreadRunning = FALSE;
    m_hTargetWnd = NULL;

    //버퍼 초기화
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


//연결 가능한 포트 이름 받아오기
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

//포트 연결 해제
void CSerialPort::Disconnect()
{
    if (m_bThreadRunning)
    {
        m_bThreadRunning = FALSE;
        if (m_hThread != NULL) { //스레드가 끝날 때까지 대기
            WaitForSingleObject(m_hThread, 2000);
            CloseHandle(m_hThread);
            m_hThread = NULL;
        }
 
    }

    if (m_hComm != INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_hComm); // 포트 핸들 닫기
        m_hComm = INVALID_HANDLE_VALUE;
        
    }

    if (m_rxBuffer) { 
        m_rxBuffer->RemoveAll();
    }


}

//포트 연결
BOOL CSerialPort::Connect(CString portName, DCB& dcb, HWND hWnd)
{
    // 기존 연결이 있다면 완전히 해제
    if (m_hComm != INVALID_HANDLE_VALUE)
    {
        Disconnect();
    }

    // 포트 이름 포맷팅 및 열기
    CString formattedPortName;
    formattedPortName.Format(_T("\\\\.\\%s"), portName); // CreateFile 함수가 COM10 이상의 포트도 인식할 수 있도록 포트 이름을 포맷팅
    strPortName = portName;

    m_hComm = CreateFile( //시리얼 포트 장치를 파일처럼 열어 핸들을 얻음
        formattedPortName,
        GENERIC_READ | GENERIC_WRITE, //포트 엑세스 타입 지정
        0, //No Shared
        NULL, //포트에 디폴트 안정 속성 할당
        OPEN_EXISTING, //기존 파일에서 호출
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, //비동기 통신
        NULL //템플릿 파일에 대한 핸들 지정 -> 포트 열 때는 필요없음
    );

    m_hTargetWnd = hWnd;      // 메시지를 받을 윈도우 핸들 저장
    m_bThreadRunning = TRUE;  // 스레드 동작 깃발 올리기

    if (m_hComm == INVALID_HANDLE_VALUE) {
        // 포트 열기 실패
        return FALSE;
    }

 
    return TRUE;
}

//포트 설정
BOOL CSerialPort::SetupPort(DWORD baudrate, BYTE byteSize, BYTE parity, BYTE stopbits, class CEdit &settingInfo)
{
    //실시간 포트 설정 받아오기
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

    //받아온 값으로 포트 설정 맞추기
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

    //포트 버퍼 초기화
    if (!PurgeComm(m_hComm, PURGE_RXCLEAR | PURGE_TXCLEAR))
    {
        MessageBox(_T("포트 버퍼를 비우는 데 실패했습니다."), NULL, MB_OK | MB_ICONERROR);
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
    BYTE inBuff[1024]; // 한번에 읽어올 최대 버퍼
    DWORD nBytesRead = 0; //실제로 읽은 바이트 수
    DWORD dwEventMask, dwError = 0 ; // 이벤트 수신 변수
    COMSTAT comStat; // 포트 상태 정보 수신 구조체
    OVERLAPPED overlap; // 비동기 입력 및 출력에 사용되는 정보

    memset(&overlap, 0, sizeof(overlap));
    overlap.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL); // 작업이 완료될때 시스테멩서 신호를 받는 상태로 설정되는 이벤트에 대한 핸들
                                // 1. 생성 핸들을 자식 프로세스가 상속받을 것인가
                                // 2. 이벤트를 자동으로 리셋할 것인가
                                // 3. 초기값을 시그널로 할 것인가 
                               // 4. 이벤트 개체 이름 없음
    if (overlap.hEvent == NULL)
    {
        return -1;
    }
    
    dwEventMask = EV_RXCHAR;
    if (!SetCommMask(pThread->m_hComm, EV_RXCHAR)) //RXCHAR 수신 메시지 도착 이벤트 감시 
    {
        CloseHandle(overlap.hEvent);
        return -1; // 스레드 비정상 종료
    }
   
    //이벤트가 들어올 때까지 기다리기
    while (pThread->m_bThreadRunning)
    {

        //EV_RXCHAR 이벤트 발생 감지 요청
        if (!WaitCommEvent(pThread->m_hComm, &dwEventMask, &overlap))
        {
            if (GetLastError() != ERROR_IO_PENDING)
            {
                break;
            }
        }

        //이벤트 발생 시까지 대기
        if (WaitForSingleObject(overlap.hEvent, INFINITE) == WAIT_OBJECT_0) // 지정된 개체가 신호 상태가 되거나 시간 제한 간격이 경과할때까 기다림
                                                //무한 대기   //기다리던 evnet가 신호상태가 됨       
        {
            // 이벤트 처리 리셋
            ResetEvent(overlap.hEvent); //이벤트 수신 완료했으니 다음 이벤트를 위해 OFF

            //통신 에러 확인, 들어온 바이트 수 확인 
            //dwError : 에러 수신 변수
            //comStat : 상태 정보 수신 변수 -> 들어온 바이트 수 확인 가능
            if (pThread->m_bThreadRunning && ClearCommError(pThread->m_hComm, &dwError, &comStat))
            {
                if (comStat.cbInQue > 0)
                {
                    // 읽어올 바이트 수를 결정 // 버퍼(inBuff) 크기와 실제 쌓인 양 중 작은 값을 선택.
                    DWORD dwToRead = min((DWORD)sizeof(inBuff), comStat.cbInQue);

                    // 비동기 ReadFile 호출 // 결정된 바이트 수만큼만 읽기를 시도
                    if (!ReadFile(pThread->m_hComm, inBuff, dwToRead, &nBytesRead, &overlap)) 
                        // 데이터 읽어오기 실패 OR 비동기 입출력 완료 시 0을 반환
                    {
                        if (GetLastError() == ERROR_IO_PENDING)  //ERROR_IO_PENDING을 가지고 있다면 실패가 아니라 비동기 입출력 완료를 의미함
                        {
                            // 비동기 작업이 진행 중이면 완료될 때까지 대기
                            // 마지막 인수는 입출력이 완료될 때까지 대기할 것인가 아닌가를 지정
                            //이 인수가 TRUE 이면 비동기 입출력을 중간에 포기하고 입출력이 완료될 때까지 리턴하지 않는다
                            //입출력 과정만 조사하고 바로 리턴하려면 FALSE여야 하며 이 함수로 완료 시점까지 대기하려면 TRUE를 주면 됨
                            //[출처] 파일 입출력 - 비동기 입출력 FILE_FLAG_OVERLAPPED, OVERLAPPED, GetOverlappedResult | 작성자 메르카츠
                            GetOverlappedResult(pThread->m_hComm, &overlap, &nBytesRead, TRUE); //실제 결과 받아오기
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
    if (m_rxBuffer) // 포인터가 유효한지 확인
    {
        m_rxBuffer->RemoveAll(); // 버퍼의 모든 내용을 삭제
    }
}

/*
void CSerialPort::ParseReadData(BYTE* in, DWORD len)
{
    if (isCRC)
    {
        ParseReadDataCRC(in, len); // SLIP 모드면 SLIP 파서 호출
    }
    else
    {
        ParseReadDataLF(in, len); // 아니면 일반 파서 호출
    }
}*/


void CSerialPort::ParseReadData(BYTE* in, DWORD len)
{
    //  SLIP 모드가 켜져 있는지 확인
    if (isSLIP)
    {
        ParseReadDataChecksum(in, len);
        return; // SLIP 파서가 일을 마쳤으므로 여기서 종료
    }

    if (isCRC)
    {
        ParseReadDataCRC(in, len);
        return;
    }

    // SLIP, CRC 모드가 아니라면 (단순 표시 모드),
    //  아무것도 해석하지 않고 받은 데이터를 그대로 UI 스레드로 보냄
    if (len > 0)
    {
        // UI 스레드에서 delete[] 할 수 있도록 메모리를 새로 할당하여 데이터를 복사
        char* pPacketData = new char[len];
        memcpy(pPacketData, in, len);

        // UI 스레드로 데이터 덩어리를 그대로 전송
        ::PostMessage(m_hTargetWnd, WM_USER_RX_DATA, (WPARAM)len, (LPARAM)pPacketData);
    }
}


//CRC 룩업 테이블 :  CRC 계산을 매우 빠르게 만들기 위해 사용하는 미리 만들어 놓은 정답지
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

// 테이블을 사용하여 CRC-8 값을 계산하는 함수
uint8_t CSerialPort::CalculateCRC8(const unsigned char* data, int length)
{
    unsigned char crc = 0; // 초기값은 0
    while (length--)
    {
        crc = crc8_table[crc ^ *data++]; //데이터와 XOR 연산
    }
    return crc;
}


// CRC
void CSerialPort::ParseReadDataCRC(BYTE* in, DWORD len)
{
    int i = 0;

    if ((m_rxBuffer->GetSize() + len) > MAX_BUFFER_SIZE) //새로 들어올 데이터로 인해 버퍼가 가득찬 경우
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
        // 패킷의 시작(0x02)을 찾음
        int nStartOfPacket = -1;
        for (i = nScanStartPosition; i < m_rxBuffer->GetSize(); i++) {
            if (m_rxBuffer->GetAt(i) == 0x02) {
                nStartOfPacket = i;
                break;
            }
        }
        if (nStartOfPacket == -1) break;

        // 패킷의 끝(다음 0x03)을 찾음
        int nEndOfPacket = -1;
        for (i = nStartOfPacket + 1; i < m_rxBuffer->GetSize(); i++) {
            if (m_rxBuffer->GetAt(i) == 0x03) {
                nEndOfPacket = i;
                break;
            }
        }
        if (nEndOfPacket == -1) break;

        // protocol 디코딩 
        CByteArray decodedPacket; // 디코딩된 데이터를 담을 임시 버퍼
        for (i = nStartOfPacket + 1; i < nEndOfPacket; i++)
        {
            if (m_rxBuffer->GetAt(i) == 0x1B) // ESC 문자를 만나면
            {
                i++; // 다음 바이트를 확인
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
            else // 일반 데이터는 그대로 복사
            {
                decodedPacket.Add(m_rxBuffer->GetAt(i));
            }
        }



        // CRC 계산 및 데이터 추출
        if (decodedPacket.GetSize() >= 2) // 최소 길이: Length(1) + CRC(1)
        {
            BYTE nDataLength = decodedPacket.GetAt(0);

            // 실제 데이터 길이와 패킷에 기록된 길이가 맞는지 확인
            if (decodedPacket.GetSize() == (nDataLength + 2)) //실제 데이터 길이 + Length(1) + CRC(1)
            {
                BYTE* pData = decodedPacket.GetData() + 1; //데이터 포인터
                BYTE receivedCRC = decodedPacket.GetAt(1 + nDataLength); //가장 마지막 글자

                //  CRC-8 (디코딩된 데이터 기준)
                BYTE calculatedCRC = CalculateCRC8(pData, nDataLength);

                // CRC 검증
                if (calculatedCRC == receivedCRC)
                {
                    // 순수 데이터만 UI 스레드로 전송
                    char* pPacketData = new char[nDataLength + 1];
                    memcpy(pPacketData, pData, nDataLength);
                    pPacketData[nDataLength] = '\0';
                    ::PostMessage(m_hTargetWnd, WM_USER_RX_DATA, (WPARAM)nDataLength, (LPARAM)pPacketData);
                }
            }
        }

        // 처리가 끝난 패킷만큼 스캔 위치 이동
        nScanStartPosition = nEndOfPacket + 1;
    }

    // 처리된 모든 데이터를 버퍼 앞에서 한 번에 제거
    if (nScanStartPosition > 0)
    {
        m_rxBuffer->RemoveAt(0, nScanStartPosition);
    }
}

// 끝이 CR/LF만 오는 경우 
/*
void CSerialPort::ParseReadDataLF(BYTE* in, DWORD len)
{
    int i=0;


    if ((m_rxBuffer->GetSize() + len) > MAX_BUFFER_SIZE) //새로 들어올 데이터로 인해 버퍼가 가득찬 경우
    {
        ::PostMessage(m_hTargetWnd, WM_USER_BUFFER_FULL, 0, 0);
        return;
    }

    int nOldSize = m_rxBuffer->GetSize(); // 기존 버퍼 크기 저장
    m_rxBuffer->SetSize(nOldSize + len);  // 새로운 데이터 길이만큼 버퍼 크기 늘리기
    memcpy(m_rxBuffer->GetData() + nOldSize, in, len); // 늘어난 공간의 시작 위치에 데이터 복사

    while (TRUE)
    {
        int nEndOfPacketPosition = -1;
        for (i = 0; i < m_rxBuffer->GetSize(); i++)
        {
            if (m_rxBuffer->GetAt(i) == '\n')
            {
                nEndOfPacketPosition = i; //패킷 끝 위치 저장
                break;
            }
        }

        if (nEndOfPacketPosition != (-1)) //CR/LF를 발견한 경우
        {

            int nPacketlen = nEndOfPacketPosition;
            // \n다음 \r이 오는 지 확인
            if (m_rxBuffer->GetAt(nPacketlen - 1) == '\r' && (nPacketlen > 0))
            {
                nPacketlen--;
            }
            //실제로 출력할 데이터가 있는지 확인
            if (nPacketlen > 0)
            {
                BYTE* pPacketData = new BYTE[nPacketlen + 1];

                memcpy(pPacketData, m_rxBuffer->GetData(), nPacketlen); 
                pPacketData[nPacketlen] = '\0'; // C-String의 끝을 표시

                // UI 스레드로 메시지 전송 (미리 저장해둔 윈도우 핸들 사용)
                ::PostMessage(m_hTargetWnd, WM_USER_RX_DATA, (WPARAM)nPacketlen, (LPARAM)pPacketData);
            }

            m_rxBuffer->RemoveAt(0, nEndOfPacketPosition + 1); //버퍼 비우기

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

    if ((m_rxBuffer->GetSize() + len) > MAX_BUFFER_SIZE) //새로 들어올 데이터로 인해 버퍼가 가득찬 경우
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
        // 패킷의 시작(0xC0)을 찾음
        int nStartOfPacket = -1;
        for (i = nScanStartPosition; i < m_rxBuffer->GetSize(); i++) {
            if (m_rxBuffer->GetAt(i) == 0xC0) {
                nStartOfPacket = i;
                break;
            }
        }
        if (nStartOfPacket == -1) break;

        // 패킷의 끝(다음 0xC0)을 찾음
        int nEndOfPacket = -1;
        for (i = nStartOfPacket + 1; i < m_rxBuffer->GetSize(); i++) {
            if (m_rxBuffer->GetAt(i) == 0xC0) {
                nEndOfPacket = i;
                break;
            }
        }
        if (nEndOfPacket == -1) break;

        // SLIP 디코딩 
        CByteArray decodedPacket; // 디코딩된 데이터를 담을 임시 버퍼
        for (i = nStartOfPacket + 1; i < nEndOfPacket; i++)
        {
            if (m_rxBuffer->GetAt(i) == 0xDB) // ESC 문자를 만나면
            {
                i++; // 다음 바이트를 확인
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
            else // 일반 데이터는 그대로 복사
            {
                decodedPacket.Add(m_rxBuffer->GetAt(i));
            }
        }



        // 체크섬 계산 및 데이터 추출
        if (decodedPacket.GetSize() >= 2) // 최소 길이: Length(1) + Checksum(1)
        {
            BYTE nDataLength = decodedPacket.GetAt(0);

            // 실제 데이터 길이와 패킷에 기록된 길이가 맞는지 확인
            if (decodedPacket.GetSize() == (nDataLength + 2)) //실제 데이터 길이 + Length(1) + Checksum(1)
            {
                BYTE* pData = decodedPacket.GetData() + 1; //데이터 포인터
                BYTE nChecksum = decodedPacket.GetAt(1 + nDataLength); //가장 마지막 글자

                //  체크섬 계산 (디코딩된 데이터 기준)
                BYTE calculatedChecksum = 0;
                //BYTE의 크기 : BYTE라는 데이터 타입은 정확히 1바이트(8비트) 크기
                //255를 넘을 경우(오버플로) 자동으로 0부터 시작하게 됨 -> 256을 나눌 필요없음
                for (int i = 0; i < nDataLength; i++)
                {
                    calculatedChecksum += pData[i]; //데이터 합산
                }

                // 체크섬 검증
                if (calculatedChecksum == nChecksum)
                {
                    // 순수 데이터만 UI 스레드로 전송
                    char* pPacketData = new char[nDataLength + 1];
                    memcpy(pPacketData, pData, nDataLength);
                    pPacketData[nDataLength] = '\0';
                    ::PostMessage(m_hTargetWnd, WM_USER_RX_DATA, (WPARAM)nDataLength, (LPARAM)pPacketData);
                }
            }
        }

        // 처리가 끝난 패킷만큼 스캔 위치 이동
        nScanStartPosition = nEndOfPacket + 1;
    }

    // 처리된 모든 데이터를 버퍼 앞에서 한 번에 제거
    if (nScanStartPosition > 0)
    {
        m_rxBuffer->RemoveAt(0, nScanStartPosition);
    }
}
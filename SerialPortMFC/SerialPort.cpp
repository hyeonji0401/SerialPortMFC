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
        Sleep(1000); // 0.1초 대기
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

    m_hComm = CreateFile( //시리얼 포트 장치를 파일처럼 열어 핸들(m_hDevSerial)을 얻음
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
BOOL CSerialPort::SetupPort(DWORD baudrate, BYTE byteSize, BYTE parity, BYTE stopbits)
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

    return TRUE;
}



UINT CommThread(LPVOID pParam)
{
    CSerialPort* pThread = (CSerialPort*)pParam;
    BYTE inBuff[1024]; // 한번에 읽어올 최대 버퍼
    DWORD nBytesRead = 0; //실제로 읽은 바이트 수
    DWORD dwEventMask, dwError = 0 ; // 이벤트 수신 변수
    COMSTAT comStat; // 포트 상태 정보 수신 구조체
    OVERLAPPED overlap;

    memset(&overlap, 0, sizeof(overlap));
    overlap.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
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
        //EV_RXCHAR 이벤트 들어올 때까지 기다림
        if (WaitCommEvent(pThread->m_hComm, &dwEventMask, NULL))
        {
            //통신 에러 확인, 들어온 바이트 수 확인 
            //dwError : 에러 수신 변수
            //comStat : 상태 정보 수신 변수 -> 들어온 바이트 수 확인 가능
            if (ClearCommError(pThread->m_hComm, &dwError, &comStat))
            {
                if (comStat.cbInQue > 0)
                {
                    // 읽어올 바이트 수를 결정 // 버퍼(inBuff) 크기와 실제 쌓인 양 중 작은 값을 선택.
                    DWORD dwToRead = min((DWORD)sizeof(inBuff), comStat.cbInQue);

                    // 비동기 ReadFile 호출 // 결정된 바이트 수만큼만 읽기를 시도
                    if (!ReadFile(pThread->m_hComm, inBuff, dwToRead, &nBytesRead, &overlap))
                    {
                        if (GetLastError() == ERROR_IO_PENDING)
                        {
                            // 비동기 작업이 진행 중이면 완료될 때까지 대기
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
                nEndOfPacketPosition = i;
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
                char* pPacketData = new char[nPacketlen + 1];

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


}
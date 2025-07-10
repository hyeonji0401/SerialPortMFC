// SerialPort.h
#pragma once

#include <Windows.h> // HANDLE과 같은 Windows API 타입을 사용하기 위해 
#include <vector>    // 사용 가능한 포트 목록을 저장하기 위해
#include <string>    // 포트 이름을 다루기 위해 
#include <afx.h> // cbytearray

#define WM_USER_RX_DATA (WM_USER + 1)

#define MAX_BUFFER_SIZE (1024 * 1024) // 최대 수신 버퍼 크기

UINT CommThread(void* pParam);

class CSerialPort : public CWnd //윈도우 메시지 시스템을 직접 사용하기 위해서
{
public:
    CSerialPort();  // 생성자
    ~CSerialPort(); // 소멸자


private:

    

public:
    HANDLE m_hComm; // 시리얼 포트 핸들
    DCB m_dcb;

    BOOL m_bThreadRunning;
    HWND m_hTargetWnd;



    std::vector<std::string> GetAvailablePorts(); // 사용 가능한 시리얼 포트 목록을 반환하는 함수
    BOOL Connect(CString portName, DCB& dcb,HWND hWnd); //연결
    void Disconnect(); //해제
    BOOL SetupPort(DWORD baudrate, BYTE byteSize, BYTE parity, BYTE stopbits); //설정
    CString strNotice;  
    CString strPortName; 
    UINT CommThread(LPVOID pParam);
    CHAR m_rxBuffer[MAX_BUFFER_SIZE];
    void ParseReadData(BYTE* in, DWORD len);
    int nowBufferPosition;
};
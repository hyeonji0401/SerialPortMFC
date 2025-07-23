// SerialPort.h
#pragma once

#include <Windows.h> // HANDLE과 같은 Windows API 타입을 사용하기 위해 
#include <vector>    // 사용 가능한 포트 목록을 저장하기 위해
#include <string>    // 포트 이름을 다루기 위해 
#include <afx.h> // cbytearray

#define WM_USER_RX_DATA (WM_USER + 1)
#define WM_USER_BUFFER_FULL (WM_USER + 2)

#define MAX_BUFFER_SIZE (1024 * 1024) // 최대 수신 버퍼 크기

UINT CommThread(LPVOID pParam);


class CSerialPort : public CWnd //윈도우 메시지 시스템을 직접 사용하기 위해서
{
public:
    CSerialPort();  // 생성자
    ~CSerialPort(); // 소멸자


private:

    

public:
    HANDLE m_hThread; //스레드 핸들
    HANDLE m_hComm; // 시리얼 포트 핸들
    DCB m_dcb;
    BOOL m_bThreadRunning; //스레드 동작 여부
    HWND m_hTargetWnd; //UI 핸들
    CString strNotice;
    CString strPortName;

    std::vector<std::string> GetAvailablePorts(); // 사용 가능한 시리얼 포트 목록을 반환하는 함수
    BOOL Connect(CString portName, DCB& dcb,HWND hWnd); //연결
    void Disconnect(); //해제
    BOOL SetupPort(DWORD baudrate, BYTE byteSize, BYTE parity, BYTE stopbits, class CEdit& settingInfo); //설정
    
    CByteArray* m_rxBuffer;
    void ParseReadData(BYTE* in, DWORD len);
    void ParseReadDataLF(BYTE* in, DWORD len);
    void ParseReadDataChecksum(BYTE* in, DWORD len);
    BOOL isSLIP;
    void ClearRxBuffer();
};
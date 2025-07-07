// SerialPort.h
#pragma once

#include <Windows.h> // HANDLE과 같은 Windows API 타입을 사용하기 위해 필요합니다.
#include <vector>    // 사용 가능한 포트 목록을 저장하기 위해 필요합니다.
#include <string>    // 포트 이름을 다루기 위해 필요합니다.

class CSerialPort
{
public:
    CSerialPort();  // 생성자
    ~CSerialPort(); // 소멸자

private:
    HANDLE m_hComm; // 시리얼 포트 핸들
    bool   m_bConnected;
    

public:
    // 사용 가능한 시리얼 포트 목록을 반환하는 함수
    std::vector<std::string> GetAvailablePorts();
    BOOL Connect(CString portName, DWORD baudRate, BYTE parity, BYTE dataBits, BYTE stopBits, BYTE flowControl);
    void Disconnect();
    bool IsConnected();
};
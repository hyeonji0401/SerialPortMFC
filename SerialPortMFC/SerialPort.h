// SerialPort.h
#pragma once

#include <Windows.h> // HANDLE�� ���� Windows API Ÿ���� ����ϱ� ���� 
#include <vector>    // ��� ������ ��Ʈ ����� �����ϱ� ����
#include <string>    // ��Ʈ �̸��� �ٷ�� ���� 
#include <afx.h> // cbytearray

#define WM_USER_RX_DATA (WM_USER + 1)
#define WM_USER_BUFFER_FULL (WM_USER + 2)

#define MAX_BUFFER_SIZE (1024 * 1024) // �ִ� ���� ���� ũ��

UINT CommThread(LPVOID pParam);


class CSerialPort : public CWnd //������ �޽��� �ý����� ���� ����ϱ� ���ؼ�
{
public:
    CSerialPort();  // ������
    ~CSerialPort(); // �Ҹ���


private:

    

public:
    HANDLE m_hThread; //������ �ڵ�
    HANDLE m_hComm; // �ø��� ��Ʈ �ڵ�
    DCB m_dcb;
    BOOL m_bThreadRunning; //������ ���� ����
    HWND m_hTargetWnd; //UI �ڵ�
    CString strNotice;
    CString strPortName;

    std::vector<std::string> GetAvailablePorts(); // ��� ������ �ø��� ��Ʈ ����� ��ȯ�ϴ� �Լ�
    BOOL Connect(CString portName, DCB& dcb,HWND hWnd); //����
    void Disconnect(); //����
    BOOL SetupPort(DWORD baudrate, BYTE byteSize, BYTE parity, BYTE stopbits, class CEdit& settingInfo); //����
    
    CByteArray* m_rxBuffer;
    void ParseReadData(BYTE* in, DWORD len);
    void ParseReadDataLF(BYTE* in, DWORD len);
    void ParseReadDataChecksum(BYTE* in, DWORD len);
    BOOL isSLIP;
    void ClearRxBuffer();
};
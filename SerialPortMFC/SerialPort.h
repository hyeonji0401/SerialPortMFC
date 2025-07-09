// SerialPort.h
#pragma once

#include <Windows.h> // HANDLE�� ���� Windows API Ÿ���� ����ϱ� ���� 
#include <vector>    // ��� ������ ��Ʈ ����� �����ϱ� ����
#include <string>    // ��Ʈ �̸��� �ٷ�� ���� 
#include <afx.h> // cbytearray


UINT CommThread(void* pParam);

class CSerialPort : public CWnd //������ �޽��� �ý����� ���� ����ϱ� ���ؼ�
{
public:
    CSerialPort();  // ������
    ~CSerialPort(); // �Ҹ���


private:

    

public:
    HANDLE m_hComm; // �ø��� ��Ʈ �ڵ�
    DCB m_dcb;

    BOOL m_bThreadRunning;
    HWND m_hTargetWnd;

    std::vector<std::string> GetAvailablePorts(); // ��� ������ �ø��� ��Ʈ ����� ��ȯ�ϴ� �Լ�
    BOOL Connect(CString portName, DCB& dcb,HWND hWnd); //����
    void Disconnect(); //����
    BOOL SetupPort(DWORD baudrate, BYTE byteSize, BYTE parity, BYTE stopbits); //����
    CString strNotice;  
    CString strPortName; 
    UINT CommThread(LPVOID pParam);
    CByteArray m_rxBuffer;
};
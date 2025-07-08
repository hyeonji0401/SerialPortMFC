// SerialPort.h
#pragma once

#include <Windows.h> // HANDLE�� ���� Windows API Ÿ���� ����ϱ� ���� �ʿ��մϴ�.
#include <vector>    // ��� ������ ��Ʈ ����� �����ϱ� ���� �ʿ��մϴ�.
#include <string>    // ��Ʈ �̸��� �ٷ�� ���� �ʿ��մϴ�.

class CSerialPort
{
public:
    CSerialPort();  // ������
    ~CSerialPort(); // �Ҹ���

private:
    HANDLE m_hComm; // �ø��� ��Ʈ �ڵ�
    bool   m_bConnected;
    

public:
    // ��� ������ �ø��� ��Ʈ ����� ��ȯ�ϴ� �Լ�
    std::vector<std::string> GetAvailablePorts();
    BOOL Connect(CString portName, DCB& dcb, CWnd* pParent);
    void Disconnect();
    bool IsConnected();
    BOOL Connect(CString portName, DCB& dcb);
};
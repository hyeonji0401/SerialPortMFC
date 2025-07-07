
// SerialPortMFCDlg.h: 헤더 파일
//

#pragma once
#include "SerialPort.h" 

// CSerialPortMFCDlg 대화 상자
class CSerialPortMFCDlg : public CDialogEx
{
// 생성입니다.
public:
	CSerialPortMFCDlg(CWnd* pParent = nullptr);	// 표준 생성자입니다.

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_SERIALPORTMFC_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 지원입니다.


// 구현입니다.
protected:
	HICON m_hIcon;
	CSerialPort m_serialPort;

	// 생성된 메시지 맵 함수
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	CComboBox m_cmb_PortName;
	CComboBox c_cmb_Baud;
	CComboBox m_cmb_Data;
	CComboBox m_cmb_Parity;
	CComboBox m_cmb_Stop;
	CComboBox m_cmb_Flow;
	afx_msg void OnClickedBtnConnect();
};


// SerialPortMFCDlg.h: 헤더 파일
//

#pragma once
#include "SerialPort.h" 
#define WM_USER_RX_DATA (WM_USER + 1) //사용자 메시지 영역 

// CSerialPortMFCDlg 대화 상자
class CSerialPortMFCDlg : public CDialogEx
{
// 생성입니다.
public: 
	class CSerialPort *m_serialPort;
	DCB m_ComDCB; 
	COMMCONFIG m_commConfig;   // 포트 설정값을 저장할 구조체


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


	// 생성된 메시지 맵 함수
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
	afx_msg LRESULT OnReceiveData(WPARAM wParam, LPARAM lParam); //메시지 처리기
	//afx_msg : MFC 메시지 맵과 연결될 메시지 처리 전용 함수라는 의미
	//LRESULT : 메시지 처리가 끝난 후 결과를 알려주는 값 반환, 별일 없으면 0 반환
	// wParam : 데이터 길이
	// lParam : 데이터 실제 위치
public:
	CComboBox m_cmb_PortName;
	afx_msg void OnClickedBtnConnect();
	CButton m_btn_setting;
	afx_msg void OnClickedBtnSetting();
	CString m_edit_rev;
};

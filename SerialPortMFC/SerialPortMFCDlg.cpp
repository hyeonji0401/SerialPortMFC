
// SerialPortMFCDlg.cpp: 구현 파일
//

#include "pch.h"
#include "framework.h"
#include "SerialPortMFC.h"
#include "SerialPortMFCDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 응용 프로그램 정보에 사용되는 CAboutDlg 대화 상자입니다.

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

// 구현입니다.
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CSerialPortMFCDlg 대화 상자



CSerialPortMFCDlg::CSerialPortMFCDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_SERIALPORTMFC_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CSerialPortMFCDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_PORTNAME, m_cmb_PortName);
	DDX_Control(pDX, IDC_COMBO_BAUD, c_cmb_Baud);
	DDX_Control(pDX, IDC_COMBO_DATA, m_cmb_Data);
	DDX_Control(pDX, IDC_COMBO_PARITY, m_cmb_Parity);
	DDX_Control(pDX, IDC_COMBO_STOP, m_cmb_Stop);
	DDX_Control(pDX, IDC_COMBO_FLOW, m_cmb_Flow);
}

BEGIN_MESSAGE_MAP(CSerialPortMFCDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_CONNECT, &CSerialPortMFCDlg::OnClickedBtnConnect)
END_MESSAGE_MAP()


// CSerialPortMFCDlg 메시지 처리기

BOOL CSerialPortMFCDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 시스템 메뉴에 "정보..." 메뉴 항목을 추가합니다.

	// IDM_ABOUTBOX는 시스템 명령 범위에 있어야 합니다.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 이 대화 상자의 아이콘을 설정합니다.  응용 프로그램의 주 창이 대화 상자가 아닐 경우에는
	//  프레임워크가 이 작업을 자동으로 수행합니다.
	SetIcon(m_hIcon, TRUE);			// 큰 아이콘을 설정합니다.
	SetIcon(m_hIcon, FALSE);		// 작은 아이콘을 설정합니다.

	// TODO: 여기에 추가 초기화 작업을 추가합니다.
	 // 사용 가능한 포트 목록을 가져옴
	GetDlgItem(IDC_BTN_CONNECT)->SetWindowText(_T("연결"));
	std::vector<std::string> ports = m_serialPort.GetAvailablePorts();

	// 포트 콤보박스에 목록을 추가
	for (const auto& port : ports)
	{
		m_cmb_PortName.AddString(CString(port.c_str()));
	}

	// 첫 번째 포트를 기본값으로 선택
	if (!ports.empty())
	{
		m_cmb_PortName.SetCurSel(0);
	}

	//콤보 박스 초기화
	c_cmb_Baud.AddString(_T("9600"));
	c_cmb_Baud.AddString(_T("19200"));
	c_cmb_Baud.AddString(_T("38400"));
	c_cmb_Baud.AddString(_T("57600"));
	c_cmb_Baud.AddString(_T("115200"));
	c_cmb_Baud.SetCurSel(0); 

	m_cmb_Data.AddString(_T("7"));
	m_cmb_Data.AddString(_T("8"));
	m_cmb_Data.SetCurSel(0);

	m_cmb_Parity.AddString(_T("None"));
	m_cmb_Parity.AddString(_T("Odd"));
	m_cmb_Parity.AddString(_T("Even"));
	m_cmb_Parity.AddString(_T("Mark"));
	m_cmb_Parity.AddString(_T("Space"));
	m_cmb_Parity.SetCurSel(0);

	m_cmb_Stop.AddString(_T("1"));
	m_cmb_Stop.AddString(_T("1.5"));
	m_cmb_Stop.AddString(_T("2"));
	m_cmb_Stop.SetCurSel(0);

	m_cmb_Flow.AddString(_T("None"));
	m_cmb_Flow.AddString(_T("hardware"));
	m_cmb_Flow.AddString(_T("Xon/Xoff"));
	m_cmb_Flow.SetCurSel(0);
	

	return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
}

void CSerialPortMFCDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 대화 상자에 최소화 단추를 추가할 경우 아이콘을 그리려면
//  아래 코드가 필요합니다.  문서/뷰 모델을 사용하는 MFC 애플리케이션의 경우에는
//  프레임워크에서 이 작업을 자동으로 수행합니다.

void CSerialPortMFCDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 그리기를 위한 디바이스 컨텍스트입니다.

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 클라이언트 사각형에서 아이콘을 가운데에 맞춥니다.
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 아이콘을 그립니다.
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// 사용자가 최소화된 창을 끄는 동안에 커서가 표시되도록 시스템에서
//  이 함수를 호출합니다.
HCURSOR CSerialPortMFCDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CSerialPortMFCDlg::OnClickedBtnConnect()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	if (m_serialPort.IsConnected())
	{
		// --- 이미 연결된 상태일 경우 -> 연결 해제 로직 수행
		m_serialPort.Disconnect();
		AfxMessageBox(_T("연결을 해제했습니다."));

		// 버튼 텍스트를 연결로 변경
		GetDlgItem(IDC_BTN_CONNECT)->SetWindowText(_T("연결"));

		// 설정 컨트롤들을 다시 활성화
		m_cmb_PortName.EnableWindow(TRUE);
		c_cmb_Baud.EnableWindow(TRUE);
		m_cmb_Data.EnableWindow(TRUE);
		m_cmb_Parity.EnableWindow(TRUE);
		m_cmb_Stop.EnableWindow(TRUE);
		m_cmb_Flow.EnableWindow(TRUE);
	}
	else
	{
	
		CString strPort, strBaud;
		m_cmb_PortName.GetLBText(m_cmb_PortName.GetCurSel(), strPort);
		c_cmb_Baud.GetLBText(c_cmb_Baud.GetCurSel(), strBaud);
		DWORD dwBaud = _ttoi(strBaud);

		
		CString strData;
		m_cmb_Data.GetLBText(m_cmb_Data.GetCurSel(), strData);
		BYTE byData = _ttoi(strData);

		BYTE byParity = m_cmb_Parity.GetCurSel();

			BYTE byStop;
		int nStopIndex = m_cmb_Stop.GetCurSel();
		switch (nStopIndex)
		{
		case 0: byStop = ONESTOPBIT;   break; // 인덱스 0 -> 1 스톱 비트
		case 1: byStop = ONE5STOPBITS; break; // 인덱스 1 -> 1.5 스톱 비트
		case 2: byStop = TWOSTOPBITS;  break; // 인덱스 2 -> 2 스톱 비트
		default: byStop = ONESTOPBIT;

		}
		BYTE byFlow = m_cmb_Flow.GetCurSel();

		
		if (m_serialPort.Connect(strPort, dwBaud, byParity, byData, byStop, byFlow))
		{
			AfxMessageBox(_T("연결에 성공했습니다."));
			GetDlgItem(IDC_BTN_CONNECT)->SetWindowText(_T("해제"));
			m_cmb_PortName.EnableWindow(FALSE);
			c_cmb_Baud.EnableWindow(FALSE);
			m_cmb_Data.EnableWindow(FALSE);
			m_cmb_Parity.EnableWindow(FALSE);
			m_cmb_Stop.EnableWindow(FALSE);
			m_cmb_Flow.EnableWindow(FALSE);
		}
		else
		{
			AfxMessageBox(_T("연결에 실패했습니다."));
		}
	}

}

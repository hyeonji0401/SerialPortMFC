
// SerialPortMFCDlg.cpp: 구현 파일
//

#include "pch.h"
#include "framework.h"
#include "SerialPortMFC.h"
#include "SerialPortMFCDlg.h"
#include "afxdialogex.h"
#include <winspool.h>   
#include <fstream>
#include <iostream>
#include <string>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <afxstr.h>
#pragma comment(lib, "winspool.lib")

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
	, m_edit_rev(_T(""))
	, m_edit_path(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_serialPort = NULL;
}

void CSerialPortMFCDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_PORTNAME, m_cmb_PortName);
	DDX_Control(pDX, IDC_BTN_SETTING, m_btn_setting);
	DDX_Text(pDX, IDC_EDIT_REV, m_edit_rev);
	DDX_Control(pDX, IDC_BUTTON_CLR, m_btn_clear);
	DDX_Control(pDX, IDC_EDIT_SETTING, m_edit_setting);
	DDX_Control(pDX, IDC_BTN_PATH, m_btn_path);
	DDX_Text(pDX, IDC_EDIT_PATH, m_edit_path);
	DDX_Control(pDX, IDC_RADIO_HEX, m_radio_hex);
	DDX_Control(pDX, IDC_RADIO_ASCII, m_radio_ascii);
}

BEGIN_MESSAGE_MAP(CSerialPortMFCDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_CONNECT, &CSerialPortMFCDlg::OnClickedBtnConnect)
	ON_BN_CLICKED(IDC_BTN_SETTING, &CSerialPortMFCDlg::OnClickedBtnSetting)
	ON_BN_CLICKED(IDC_BUTTON_CLR, &CSerialPortMFCDlg::OnClickedButtonClr)
	ON_MESSAGE(WM_USER_RX_DATA, &CSerialPortMFCDlg::OnReceiveData)
	ON_MESSAGE(WM_USER_BUFFER_FULL, &CSerialPortMFCDlg::OnBufferFull)
	ON_BN_CLICKED(IDC_BTN_PATH, &CSerialPortMFCDlg::OnBnClickedBtnPath)
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
	m_radio_ascii.SetCheck(BST_CHECKED);

	//저장된 가장 최근 파일 저장 장소 불러옴
	m_edit_path = AfxGetApp()->GetProfileString(_T("Settings"), _T("LastSavePath"), _T(""));

	// 만약 저장된 경로가 없다면 현재 프로그램 폴더를 기본 경로로 설정
	if (m_edit_path.IsEmpty())
	{
		TCHAR szPath[MAX_PATH];
		GetModuleFileName(NULL, szPath, MAX_PATH);
		PathRemoveFileSpec(szPath); // 파일 이름 제외하고 경로만 남김
		m_edit_path = szPath;
	}
	UpdateData(FALSE);


	GetDlgItem(IDC_BTN_CONNECT)->SetWindowText(_T("연결"));
	std::vector<std::string> ports = m_serialPort->GetAvailablePorts();

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


	ZeroMemory(&m_ComDCB, sizeof(m_ComDCB));
	m_ComDCB.DCBlength = sizeof(m_ComDCB);

	m_bIsSettingDone = FALSE;


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
	// 이미 연결된 상태인지 확인 -> 그렇다면 연결 해제 수행
	if (m_serialPort)
	{
		m_serialPort->Disconnect();
		delete m_serialPort;
		m_serialPort = NULL;
		AfxMessageBox(_T("연결을 해제했습니다."));

		if (m_file.is_open())
		{
			m_file.close();
		}

		// UI를 연결 전 상태로 복구
		GetDlgItem(IDC_BTN_CONNECT)->SetWindowText(_T("연결"));
		m_cmb_PortName.EnableWindow(TRUE);
		GetDlgItem(IDC_BTN_SETTING)->EnableWindow(TRUE);
		m_edit_rev.Empty();
		m_edit_setting.SetWindowText(_T(" "));
		m_edit_setting.Invalidate();
		UpdateData(FALSE);
		return;
	}
	
	

	// 콤보박스에서 포트 이름 가져오기
	CString strPort;
	m_cmb_PortName.GetLBText(m_cmb_PortName.GetCurSel(), strPort);

	//포트가 선택되지 않았을 경우
	if (strPort.IsEmpty()) {
		AfxMessageBox(_T("포트를 선택하세요."));
		return;
	}

	if (m_bIsSettingDone == FALSE)
	{
		AfxMessageBox(_T("먼저 '설정' 버튼을 눌러 포트 설정을 완료해주세요."), MB_ICONINFORMATION);
		return; // 연결 시도 중단
	}

	//포트 생성
	m_serialPort = new CSerialPort;
	

	//포트 연결
	if (m_serialPort->Connect(strPort, m_commConfig.dcb, this->m_hWnd))
	{
		// 연결 성공 직후, 우리가 원하는 통신 설정으로 포트를 구성
		// m_ComDCB에 저장된 값 (혹은 기본값)을 사용
		if (!m_serialPort->SetupPort(m_ComDCB.BaudRate, m_ComDCB.ByteSize, m_ComDCB.Parity, m_ComDCB.StopBits, m_edit_setting))
		{
			// 설정에 실패하면 연결을 즉시 끊습니다.
			AfxMessageBox(_T("포트 설정에 실패했습니다. 연결을 해제합니다."));
			m_serialPort->Disconnect();
			delete m_serialPort;
			m_serialPort = NULL;
			GetDlgItem(IDC_BTN_CONNECT)->SetWindowText(_T("연결"));
			m_edit_rev.Empty();
			m_edit_setting.SetWindowText(_T(" "));
			m_edit_setting.Invalidate();
			UpdateData(FALSE);
			return;
		}


		// 오늘 날짜로 파일 경로 생성
		auto now = std::chrono::system_clock::now();
		std::time_t now_c = std::chrono::system_clock::to_time_t(now);
		std::tm now_tm;
		localtime_s(&now_tm, &now_c);
		char filenameBuffer[20];
		strftime(filenameBuffer, sizeof(filenameBuffer), "%Y-%m-%d.csv", &now_tm);
		CString strFullPath;
		UpdateData(TRUE);
		strFullPath.Format(_T("%s%s"), m_edit_path, CString(filenameBuffer));

		m_file.open(CT2A(strFullPath), std::ios::app);
		if (!m_file.is_open())
		{
			AfxMessageBox(_T("로그 파일을 열 수 없습니다."));
		}

		AfxMessageBox(_T("연결에 성공했습니다."));

		// UI를 연결 후 상태로 변경
		GetDlgItem(IDC_BTN_CONNECT)->SetWindowText(_T("해제"));
		m_cmb_PortName.EnableWindow(FALSE);
		GetDlgItem(IDC_BTN_SETTING)->EnableWindow(FALSE);



		CWinThread* pThread = AfxBeginThread(CommThread, m_serialPort); //스레드 시작
		m_serialPort->m_hThread = pThread->m_hThread; //핸들 저장

	}
	else //연결 실패
	{
		AfxMessageBox(_T("연결에 실패했습니다."));
		delete m_serialPort;
		m_serialPort = NULL;
		GetDlgItem(IDC_BTN_CONNECT)->SetWindowText(_T("연결"));
		m_edit_rev.Empty();
		m_edit_setting.SetWindowText(_T(" "));
		m_edit_setting.Invalidate();
		UpdateData(FALSE);
		return;

	}
	

}


void CSerialPortMFCDlg::OnClickedBtnSetting()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	CString strPort;
	CString	strNotice;
	COMMCONFIG comConfig;

	m_cmb_PortName.GetLBText(m_cmb_PortName.GetCurSel(), strPort);

	if (strPort.IsEmpty()) {
		AfxMessageBox(_T("먼저 포트를 선택하세요."));
		return;
	}
	if (m_serialPort) //포트가 열려있을 때
	{
		if (!GetCommState(m_serialPort->m_hComm, &m_ComDCB)) {
			strNotice.Format("GetCommState() Failed, Close Port(%s), Error#(%xh)", m_serialPort->strPortName);
			MessageBox(strNotice, NULL, MB_OK | MB_ICONERROR); //확인, 에러 아이콘
			m_serialPort->Disconnect();
			m_serialPort = NULL;
			GetDlgItem(IDC_BTN_CONNECT)->SetWindowText(_T("연결"));
			m_edit_rev.Empty();
			m_edit_setting.SetWindowText(" ");
			m_edit_setting.Invalidate();
			UpdateData(FALSE);
			return;
		}

		memcpy(&comConfig.dcb, &m_ComDCB, sizeof(m_ComDCB));

		// 윈도우의 표준 포트 설정 대화상자를 호출함
		if (!CommConfigDialog(strPort, m_hWnd, &comConfig))
		{
			return;
		}
		else {
			
			AfxMessageBox(_T("포트 설정이 완료되었습니다."));
		}

		if (!(m_serialPort->SetupPort(m_ComDCB.BaudRate, m_ComDCB.ByteSize, m_ComDCB.Parity, m_ComDCB.StopBits, m_edit_setting)))
		{
			delete m_serialPort;
			m_serialPort = NULL;
			GetDlgItem(IDC_BTN_CONNECT)->SetWindowText(_T("연결"));
			m_edit_rev.Empty();
			m_edit_setting.SetWindowText(_T(" "));
			m_edit_setting.Invalidate();
			UpdateData(FALSE);
			return;
		}
		
		
	}
	else {  //포트가 열려있지 않을 때
		memcpy(&comConfig.dcb, &m_ComDCB, sizeof(m_ComDCB));
		if (!CommConfigDialog(strPort, m_hWnd, &comConfig)) {
			return;
		}
		memcpy(&m_ComDCB, &comConfig.dcb, sizeof(m_ComDCB));
		m_bIsSettingDone = TRUE;
	}


	
}

void CSerialPortMFCDlg::saveData(const std::string& receivedString)
{
	if (m_file.is_open())
	{
		std::string text = receivedString;

		// 현재 시간 정보 얻기
		auto now = std::chrono::system_clock::now();
		std::time_t now_c = std::chrono::system_clock::to_time_t(now);
		std::tm now_tm;
		localtime_s(&now_tm, &now_c);

		// 로그에 기록
		m_file << text << "," << std::put_time(&now_tm, "%Y-%m-%d %H:%M:%S") << std::endl;
	}
}


void CSerialPortMFCDlg::OnBnClickedBtnPath()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.

	BROWSEINFO BrInfo;
	TCHAR szBuffer[512];                                      // 경로저장 버퍼 

	::ZeroMemory(&BrInfo, sizeof(BROWSEINFO));
	::ZeroMemory(szBuffer, 512);

	BrInfo.hwndOwner = GetSafeHwnd(); 
	BrInfo.lpszTitle = _T("파일이 저장될 폴더를 선택하세요");
	BrInfo.ulFlags = BIF_NEWDIALOGSTYLE | BIF_EDITBOX | BIF_RETURNONLYFSDIRS; 
	LPITEMIDLIST pItemIdList = ::SHBrowseForFolder(&BrInfo);
	::SHGetPathFromIDList(pItemIdList, szBuffer);				// 파일경로 읽어오기

	CString str;
	str.Format(_T("%s"), szBuffer);
	m_edit_path = str+'\\';
	UpdateData(FALSE);

	AfxGetApp()->WriteProfileString(_T("Settings"), _T("LastSavePath"), m_edit_path); //레지스트리에 파일 경로 저장

}


// 수신한 데이터 UI에 띄워주기
LRESULT CSerialPortMFCDlg::OnReceiveData(WPARAM wParam, LPARAM lParam)
//LRESULT : long과 같지만 리턴값을 명시하기 위해 정의된 이름
//WParam : unsinged int / word parameter => 콜백 함수를 호출할 때 부가적으로 주는 메시지를 가지는 변수
//LParam : long / Long parameter ''
{
	// 스레드가 보내준 데이터의 길이(wParam)와 데이터의 실제 위치(lParam)를 받음
	int length = (int)wParam;
	BYTE* data = (BYTE*)lParam;
	//BYTE = unsigned char 값에는 부호가 없음 
	//근데 CString으로 바꾸려면 char*로 어차피 바꿔줘야하므로 그냥 char* 사용

	UpdateData(TRUE);

	CString strToShow;

	if (m_radio_hex.GetCheck() == BST_CHECKED) //HEX를 선택했을 경우
	{
		for (int i = 0; i < length; i++)
		{
			CString temp;
			temp.Format(_T("%02X "), data[i]);
			strToShow += temp;
		}
	}
	else if (m_radio_ascii.GetCheck()==BST_CHECKED) //ASCII를 선택했을 경우
	{
		strToShow = CString((char*)data, length);
	}

	// 수신 에디트 컨트롤의 포인터를 얻어옴
	CEdit* pEditRev = (CEdit*)GetDlgItem(IDC_EDIT_REV);
	if (pEditRev == NULL) {
		delete[] data;
		return 0;
	}

	//  에디트 컨트롤의 텍스트가 너무 길어지면 자동으로 비워주기
	const int MAX_EDIT_LEN = 1024 * 1024;
	if (pEditRev->GetWindowTextLength() > MAX_EDIT_LEN)
	{
		pEditRev->SetWindowText(_T("")); // 텍스트 클리어
	}

	// 현재 텍스트의 끝 부분으로 커서를 이동
	int nLen = pEditRev->GetWindowTextLength();
	pEditRev->SetSel(nLen, nLen);

	// 새로 받은 텍스트를 직접 추가
	pEditRev->ReplaceSel(strToShow+"\r\n");
	pEditRev->LineScroll(pEditRev->GetLineCount());

	std::string str = strToShow.GetString();
	saveData(str);

	// 스레드에서 new로 할당했던 메모리를 여기서 반드시 해제해야 함
	delete[] data;

	return 0;
}


void CSerialPortMFCDlg::OnClickedButtonClr()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	m_edit_rev.Empty();
	UpdateData(FALSE);

}


LRESULT CSerialPortMFCDlg::OnBufferFull(WPARAM wParam, LPARAM lParam)
{
	AfxMessageBox(_T("버퍼가 가득 찼습니다. 연결을 해제합니다."));

	// 연결 해제
	OnClickedBtnConnect();

	return 0;
}





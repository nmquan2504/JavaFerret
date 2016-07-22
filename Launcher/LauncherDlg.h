
// LauncherDlg.h : header file
//

#pragma once

typedef BOOL (*FPTR_InitWinHook)(DWORD dwMode);

// CLauncherDlg dialog
class CLauncherDlg : public CDialogEx
{
// Construction
public:
	CLauncherDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_LAUNCHER_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

public:
	static void LogString(char *msg, ...);

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnBnClickedButton3();
};

#if !defined(AFX_COMMONPARAMSPAGE_H__33B73994_EEEA_11D1_85AA_0060977E8CAC__INCLUDED_)
#define AFX_COMMONPARAMSPAGE_H__33B73994_EEEA_11D1_85AA_0060977E8CAC__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// CommonParamsPage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CCommonParamsPage dialog

#include "onechar.h"

#include <vector>

class CCommonParamsPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CCommonParamsPage)

// Construction
public:
	CCommonParamsPage();
	~CCommonParamsPage();

// Dialog Data
	//{{AFX_DATA(CCommonParamsPage)
	enum { IDD = IDD_PARAMS_PAGE };
	COneChar	m_cCommandDelimiter;
	COneChar	m_cCommandChar;
	int		m_nStartLine;
	UINT	m_nMinLen;
	CString	m_strCommandChar;
	CString	m_strCommandDelimiter;
	UINT	m_nHistorySize;
	BOOL	m_bDisplayCommands;
	BOOL	m_bClearInput;
	BOOL	m_bTokenInput;
	BOOL	m_bKillOneToken;
	BOOL	m_bScrollEnd;
	BOOL	m_bConnectBeep;
	BOOL	m_bAutoReconnect;
	BOOL	m_bSplitOnBackscroll;
	BOOL	m_bMinimizeToTray;
	int		m_nTrigDelay;
	CComboBox m_cCodePage;
	BOOL	m_bLineWrap;
	BOOL	m_bShowTimestamps;
	BOOL	m_bSelectRect;
	BOOL	m_bRemoveESC;
	BOOL	m_bShowHidden;
	BOOL	m_bShowPing;
	BOOL	m_bStickScrollbar;
	BOOL	m_bExtAnsiColors;
	int		m_nUserInputHide;
	//}}AFX_DATA

	std::vector<int> m_vIndexToCPID;
	int m_nMudCodePage;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CCommonParamsPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CCommonParamsPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnSelchangeCodePage();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_COMMONPARAMSPAGE_H__33B73994_EEEA_11D1_85AA_0060977E8CAC__INCLUDED_)

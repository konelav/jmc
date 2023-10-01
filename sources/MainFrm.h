// MainFrm.h : interface of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#include "editbar.h"
#include "cooldialogbar.h"
#include "ansiwnd.h"

//vls-begin// multiple output
#include "ttcoreex\\tintinx.h" // for MAX_OUTPUT
//vls-end//

static UINT outputwindows[MAX_OUTPUT] =
{
        ID_VIEW_OUTPUTWINDOW,   ID_VIEW_OUTPUTWINDOW1,
        ID_VIEW_OUTPUTWINDOW2,  ID_VIEW_OUTPUTWINDOW3,
        ID_VIEW_OUTPUTWINDOW4,  ID_VIEW_OUTPUTWINDOW5,
        ID_VIEW_OUTPUTWINDOW6,  ID_VIEW_OUTPUTWINDOW7,
        ID_VIEW_OUTPUTWINDOW8,  ID_VIEW_OUTPUTWINDOW9
};

class CInvertSplit : public CSplitterWnd 
{
public:
    BOOL SplitRow( );
    void SavePosition();

    int m_nUpSize, m_nDownSize;    

    BOOL m_bInited;

protected:

protected:
	//{{AFX_MSG(CInvertSplit)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};


/////////////////////////////////////////////////////////////////////////////
// COutputBar window

class COutputBar : public CCoolDialogBar
{
// Construction
public:
	COutputBar();

// Attributes
public:
    CAnsiWnd m_wndAnsi;

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(COutputBar)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~COutputBar();

	// Generated message map functions
protected:
	//{{AFX_MSG(COutputBar)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

class CJMCStatus : public CStatusBar
{
protected:
    CBitmap m_bmpConnected, m_bmpLogged, m_bmpMarked;

protected:
	int m_nYsize;

	void HandleMouseEvent(const wchar_t *event, UINT nFlags, CPoint point);

public:    
    CJMCStatus();
	void SetFontHeight(int h) {m_nYsize = h;};
    virtual void DrawItem( LPDRAWITEMSTRUCT lpDrawItemStruct );
	virtual CSize CalcFixedLayout(BOOL bStretch, BOOL bHorz);

protected:
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnMButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMButtonDblClk(UINT nFlags, CPoint point);

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

class CMainFrame : public CFrameWnd
{
protected: // create from serialization only
	CMainFrame();
	DECLARE_DYNCREATE(CMainFrame)

    CString m_strInfo1, m_strInfo2, m_strInfo3, m_strInfo4, m_strInfo5;

// Attributes
public:
    CEditBar m_editBar;
	CJMCStatus  m_wndStatusBar;
//vls-begin// multiple output
//    COutputBar m_coolBar;
    COutputBar m_coolBar[MAX_OUTPUT];
	DWORD m_Docks[MAX_OUTPUT];
//vls-end//

    virtual void OnUpdateFrameTitle(BOOL);
    
    CInvertSplit m_wndSplitter;


// Operations
public:
	void RestorePosition();
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMainFrame)
	public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CMainFrame();

    // virtual void OnUpdateFrameTitle(BOOL);
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:  // control bar embedded members
	CToolBar    m_wndToolBar;

// Generated message map functions
protected:
	//{{AFX_MSG(CMainFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnOptionsOptions();
	afx_msg void OnDestroy();
	afx_msg void OnUnsplit();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnEditJmcobjects();
	//}}AFX_MSG
    afx_msg void OnUpdateLogged(CCmdUI* pUI);
    afx_msg void OnUpdateConnected(CCmdUI* pUI);
    afx_msg void OnUpdatePath(CCmdUI* pUI);
    afx_msg void OnUpdateTicker(CCmdUI* pUI);
    afx_msg BOOL OnBarCheckEx(UINT nID);

    afx_msg void OnUpdateInfo1(CCmdUI* pUI);
    afx_msg void OnUpdateInfo2(CCmdUI* pUI);
    afx_msg void OnUpdateInfo3(CCmdUI* pUI);
    afx_msg void OnUpdateInfo4(CCmdUI* pUI);
    afx_msg void OnUpdateInfo5(CCmdUI* pUI);
    
    afx_msg LONG OnTabAdded( UINT wParam, LONG lParam);
    afx_msg LONG OnTabDeleted( UINT wParam, LONG lParam);

//vls-begin// #quit
    afx_msg LONG OnQuitCommand(UINT wParam, LONG lParam);
//vls-end//

    afx_msg LONG OnHideWindow(UINT wParam, LONG lParam);
    afx_msg LONG OnRestoreWindow(UINT wParam, LONG lParam);
    afx_msg LONG OnHideWindowToSystemTray(UINT wParam, LONG lParam);
    afx_msg LONG OnRestoreWindowFromSystemTray(UINT wParam, LONG lParam);

//vls-begin// #reloadscripts
    afx_msg LONG OnReloadScripts(UINT wParam, LONG lParam);
//vls-end//

//vls-begin// multiple output
    afx_msg LONG OnShowOutput(UINT wParam, LONG lParam);
    afx_msg LONG OnNameOutput(UINT wParam, LONG lParam);
    afx_msg LONG OnDockOutput(UINT wParam, LONG lParam);
    afx_msg LONG OnPosWOutput(UINT wParam, LONG lParam);
	afx_msg LONG OnSizeWOutput(UINT wParam, LONG lParam);
//vls-end//

    afx_msg LONG OnCleanInput(UINT wParam, LONG lParam);

	afx_msg LONG OnUpdStat1(UINT wParam, LONG lParam);
	afx_msg LONG OnUpdStat2(UINT wParam, LONG lParam);
	afx_msg LONG OnUpdStat3(UINT wParam, LONG lParam);
	afx_msg LONG OnUpdStat4(UINT wParam, LONG lParam);
	afx_msg LONG OnUpdStat5(UINT wParam, LONG lParam);

	afx_msg LONG OnUpdPing(UINT wParam, LONG lParam);

	afx_msg LONG OnTrayMessage(UINT wParam, LONG lParam);
	afx_msg void OnSysCommand(UINT wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
};

extern CMainFrame* pMainWnd;

/////////////////////////////////////////////////////////////////////////////

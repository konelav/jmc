// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "afxpriv.h"
#include "smc.h"
#include "MainFrm.h"
#include "smcDoc.h"
#include "smcView.h"
#include "Tray.h"

#include "CommonParamsPage.h"
#include "CharSubstPage.h"
#include "SmcPropertyDlg.h"
#include "ProfilePage.h"
#include "LogParamsPage.h"
#include "ScriptPage.h"
#include "JmcObjectsDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//vls-begin// multiple output
//vls-end//

int shouldMinimizeToTray = -1;

static UINT indicators[] =
{
	    ID_SEPARATOR,           // status line indicator
        ID_TICKER,
        ID_INDICATOR_INFO1,
        ID_INDICATOR_INFO2,
        ID_INDICATOR_INFO3,
        ID_INDICATOR_INFO4,
        ID_INDICATOR_INFO5,
        ID_INDICATOR_CONNECTED,
	    ID_INDICATOR_LOGGED,
        ID_PATH_WRITING
};

enum INDICATOR_NUM{
        NUM_INDICATOR_INFO1 = 2,
        NUM_INDICATOR_INFO2,
        NUM_INDICATOR_INFO3,
        NUM_INDICATOR_INFO4,
        NUM_INDICATOR_INFO5,
        NUM_INDICATOR_CONNECTED,
	    NUM_INDICATOR_LOGGED,
        NUM_PATH_WRITING, 
        NUM_TICKER=1
};

CTray sysTray;

BEGIN_MESSAGE_MAP(CJMCStatus, CStatusBar)
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_RBUTTONDBLCLK()
	ON_WM_MBUTTONDOWN()
	ON_WM_MBUTTONUP()
	ON_WM_MBUTTONDBLCLK()
END_MESSAGE_MAP()

CJMCStatus::CJMCStatus()
{
    m_bmpConnected.LoadBitmap (IDB_CONNECTED);
    m_bmpLogged.LoadBitmap (IDB_LOGGED);
    m_bmpMarked.LoadBitmap (IDB_MARKED);
	m_nYsize = -1;
}

CSize CJMCStatus::CalcFixedLayout(BOOL bStretch, BOOL bHorz) {
	CSize ret = CStatusBar::CalcFixedLayout( bStretch, bHorz );
	if (m_nYsize > 0) {
		CRect border = GetBorders();
		ret.cy = m_nYsize + border.top + border.bottom + 2;
	}
	return ret;
}

void CJMCStatus::OnLButtonDown(UINT nFlags, CPoint point) {
	HandleMouseEvent(L"LDown", nFlags, point);
}
void CJMCStatus::OnLButtonUp(UINT nFlags, CPoint point) {
	HandleMouseEvent(L"LUp", nFlags, point);
}
void CJMCStatus::OnLButtonDblClk(UINT nFlags, CPoint point) {
	HandleMouseEvent(L"LDblClk", nFlags, point);
}
void CJMCStatus::OnRButtonDown(UINT nFlags, CPoint point) {
	HandleMouseEvent(L"RDown", nFlags, point);
}
void CJMCStatus::OnRButtonUp(UINT nFlags, CPoint point) {
	HandleMouseEvent(L"RUp", nFlags, point);
}
void CJMCStatus::OnRButtonDblClk(UINT nFlags, CPoint point) {
	HandleMouseEvent(L"RDblClk", nFlags, point);
}
void CJMCStatus::OnMButtonDown(UINT nFlags, CPoint point) {
	HandleMouseEvent(L"MDown", nFlags, point);
}
void CJMCStatus::OnMButtonUp(UINT nFlags, CPoint point) {
	HandleMouseEvent(L"MUp", nFlags, point);
}
void CJMCStatus::OnMButtonDblClk(UINT nFlags, CPoint point) {
	HandleMouseEvent(L"MDblClk", nFlags, point);
}

void CJMCStatus::HandleMouseEvent(const wchar_t *event, UINT nFlags, CPoint point) {
	CRect rect;
	int index = 0;
	const wchar_t *strInfo = L"";

	GetItemRect(NUM_INDICATOR_INFO1, &rect);
	if (rect.PtInRect(point)) {
		index = 1;
		strInfo = ((CMainFrame*)AfxGetMainWnd())->m_strInfo1;
	}
	GetItemRect(NUM_INDICATOR_INFO2, &rect);
	if (rect.PtInRect(point)) {
		index = 2;
		strInfo = ((CMainFrame*)AfxGetMainWnd())->m_strInfo2;
	}
	GetItemRect(NUM_INDICATOR_INFO3, &rect);
	if (rect.PtInRect(point)) {
		index = 3;
		strInfo = ((CMainFrame*)AfxGetMainWnd())->m_strInfo3;
	}
	GetItemRect(NUM_INDICATOR_INFO4, &rect);
	if (rect.PtInRect(point)) {
		index = 4;
		strInfo = ((CMainFrame*)AfxGetMainWnd())->m_strInfo4;
	}
	GetItemRect(NUM_INDICATOR_INFO5, &rect);
	if (rect.PtInRect(point)) {
		index = 5;
		strInfo = ((CMainFrame*)AfxGetMainWnd())->m_strInfo5;
	}

	if (index) {
		if ( WaitForSingleObject (eventGuiAction, 0 ) == WAIT_TIMEOUT ) {
			swprintf(strGuiAction, L"status %ls %d %d %ls", event, index, nFlags, strInfo);
			SetEvent(eventGuiAction);
		}
	}
}

extern int LengthWithoutANSI(const wchar_t* str);
extern void HandleCSI(const COLORREF *FgColors, const COLORREF *BgColors,
					  BOOL ExtAnsiColors, BOOL DarkOnly, BOOL Invert, BOOL ShowHiddenFg, BOOL ShowHiddenBg,
					  BOOL &AnsiBold, int &CurrentFg, int &CurrentBg,
					  COLORREF &AnsiColorFg, COLORREF &AnsiColorBg,
					  COLORREF &ColorFg, COLORREF &ColorBg,
					  const wchar_t **str);

//vls-begin// multiple output
static void __stdcall GetOutputName(int wnd, wchar_t *name, int maxlen)
{
    if (wnd >= 0 && wnd < MAX_OUTPUT && name)
    {
        int len;
        CString cs;
        CMainFrame* pMainFrm = (CMainFrame*)AfxGetMainWnd();
        cs = pMainFrm->m_coolBar[wnd].m_sTitle;
        len = min(maxlen, cs.GetLength()) - 1;
        wcsncpy(name, cs, len);
        name[len] = L'\0';
    }
}
//vls-end//

static void __stdcall GetWindowSizeFunc(int wnd, int &width, int &height)
{
	width = height = 0;
	if (wnd >= 0 && wnd < MAX_OUTPUT) {
        CMainFrame* pMainFrm = (CMainFrame*)AfxGetMainWnd();
		if (pMainFrm) {
			height = pMainFrm->m_coolBar[wnd].m_wndAnsi.m_nPageSize;
			width = pMainFrm->m_coolBar[wnd].m_wndAnsi.m_nLineWidth;
		}
    } else {
		CMainFrame* pMainFrm = (CMainFrame*)AfxGetMainWnd();
		if (pMainFrm) {
			CSmcView* pView = (CSmcView*)pMainFrm->GetActiveView();
			if (pView) {
				height = pView->m_nPageSize;
				width = pView->m_nLineWidth;
			}
		}
	}
}
static void __stdcall SetWindowSizeFunc(int wnd, int width, int height)
{
	if (wnd < 0 || wnd >= MAX_OUTPUT)
		wnd = MAX_OUTPUT;
	CMainFrame* pMainFrm = (CMainFrame*)AfxGetMainWnd();
	pMainFrm->PostMessage(WM_USER+506, MAKELPARAM(width, height));
}

static void DrawColoredText(LPDRAWITEMSTRUCT lpDrawItemStruct, const wchar_t* strText)
{
    CSmcDoc* pDoc = (CSmcDoc*) (((CMainFrame*)AfxGetMainWnd())->GetActiveDocument());
    if ( !pDoc) 
        return;
    ASSERT( pDoc->IsKindOf( RUNTIME_CLASS( CSmcDoc ) ) );

    int nBg = 0, nFg = 7;
	BOOL bold = FALSE;
	COLORREF ansiBg, ansiFg, colorF, colorB;

	SelectObject(lpDrawItemStruct->hDC ,pDoc->m_fntText.GetSafeHandle ());

	CRect rect = lpDrawItemStruct->rcItem;
	int shift = 0;

	HandleCSI(pDoc->m_ForeColors, pDoc->m_BackColors, pDoc->m_bExtAnsiColors, pDoc->m_bDarkOnly,
			  FALSE, FALSE, FALSE, bold, nFg, nBg, ansiFg, ansiBg,
			  colorF, colorB, 0);

	const wchar_t* ptr = strText;
	while (*ptr) {
		if (*ptr == ESC_SEQUENCE_MARK) {
			HandleCSI(pDoc->m_ForeColors, pDoc->m_BackColors, pDoc->m_bExtAnsiColors, pDoc->m_bDarkOnly,
					  FALSE, FALSE, FALSE, bold, nFg, nBg, ansiFg, ansiBg,
					  colorF, colorB, &ptr);
		}
		
		const wchar_t *end = ptr;
		while (*end && *end != ESC_SEQUENCE_MARK)
			end++;

		SetTextColor(lpDrawItemStruct->hDC , colorF);
		SetBkColor(lpDrawItemStruct->hDC , colorB);

		if (end > ptr) {
			int len = end - ptr;
			CRect rctext(0, 0, 0, 0);

			DrawText(lpDrawItemStruct->hDC, ptr, len, &rctext, DT_LEFT | DT_SINGLELINE | DT_NOCLIP | DT_CALCRECT | DT_NOPREFIX );

			CRect rc = rect;
			rc.left += shift;
			ExtTextOut(lpDrawItemStruct->hDC, rc.left , rc.top, ETO_OPAQUE, &rc, ptr, len, NULL);

			shift += rctext.Width();
		}

		ptr = end;
	}

	if (shift == 0) {
		SetTextColor(lpDrawItemStruct->hDC , colorF);
		SetBkColor(lpDrawItemStruct->hDC , colorB);
		ExtTextOut(lpDrawItemStruct->hDC, rect.left , rect.top, ETO_OPAQUE, &rect, L"", 0, NULL);
	}
}

void CJMCStatus::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
    static int count = 0;
    HBITMAP hb;
    switch ( lpDrawItemStruct->itemID  ) {
    case NUM_INDICATOR_INFO1:
        {
            DrawColoredText(lpDrawItemStruct, ((CMainFrame*)AfxGetMainWnd())->m_strInfo1);
        }
        return;
    case NUM_INDICATOR_INFO2:
        {
            DrawColoredText(lpDrawItemStruct, ((CMainFrame*)AfxGetMainWnd())->m_strInfo2);
        }
        return;
    case NUM_INDICATOR_INFO3:
        {
            DrawColoredText(lpDrawItemStruct, ((CMainFrame*)AfxGetMainWnd())->m_strInfo3);
        }
        return;
    case NUM_INDICATOR_INFO4:
        {
            DrawColoredText(lpDrawItemStruct, ((CMainFrame*)AfxGetMainWnd())->m_strInfo4);
        }
        return;
    case NUM_INDICATOR_INFO5:
        {
            DrawColoredText(lpDrawItemStruct, ((CMainFrame*)AfxGetMainWnd())->m_strInfo5);
        }
        return;
    case NUM_INDICATOR_CONNECTED:
        {
            if ( IsConnected() ) {
                hb = (HBITMAP)m_bmpConnected;
                break;
            }
        }
        return;
    case NUM_INDICATOR_LOGGED:
        {
            if ( IsLogging() ) {
                hb = (HBITMAP)m_bmpLogged;
                break;
            }
        }
        return;
    case NUM_PATH_WRITING:
        {
            if ( IsPathing () ) {
                hb = (HBITMAP)m_bmpMarked;
                break;
            }
        }
        return;
    default:
        return;
    }
    HDC dc = CreateCompatibleDC (lpDrawItemStruct->hDC );
    SelectObject(dc, hb);
    BitBlt(lpDrawItemStruct->hDC, lpDrawItemStruct->rcItem.left, 
        lpDrawItemStruct->rcItem.top, 
        lpDrawItemStruct->rcItem.right - lpDrawItemStruct->rcItem.left, 
        lpDrawItemStruct->rcItem.bottom- lpDrawItemStruct->rcItem.top, 
        dc, 0, 0 , SRCCOPY);

    DeleteDC(dc);
    return;
}


BEGIN_MESSAGE_MAP(CInvertSplit , CSplitterWnd)
	//{{AFX_MSG_MAP(CInvertSplit)
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)


BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_WM_CREATE()
	ON_WM_SYSCOMMAND()
	ON_COMMAND(ID_OPTIONS_OPTIONS, OnOptionsOptions)
	ON_WM_DESTROY()
	ON_COMMAND(ID_UNSPLIT, OnUnsplit)
	ON_WM_SIZE()
	ON_COMMAND(ID_EDIT_JMCOBJECTS, OnEditJmcobjects)
	ON_WM_CTLCOLOR()
	//}}AFX_MSG_MAP
    ON_UPDATE_COMMAND_UI(ID_INDICATOR_CONNECTED, OnUpdateConnected)
    ON_UPDATE_COMMAND_UI(ID_INDICATOR_LOGGED, OnUpdateLogged)
    ON_UPDATE_COMMAND_UI(ID_PATH_WRITING, OnUpdatePath)
    ON_UPDATE_COMMAND_UI(ID_TICKER, OnUpdateTicker)
	ON_COMMAND_EX(ID_VIEW_OUTPUTWINDOW, OnBarCheckEx)
//vls-begin// multiple output
	ON_COMMAND_EX(ID_VIEW_OUTPUTWINDOW1, OnBarCheckEx)
	ON_COMMAND_EX(ID_VIEW_OUTPUTWINDOW2, OnBarCheckEx)
	ON_COMMAND_EX(ID_VIEW_OUTPUTWINDOW3, OnBarCheckEx)
	ON_COMMAND_EX(ID_VIEW_OUTPUTWINDOW4, OnBarCheckEx)
	ON_COMMAND_EX(ID_VIEW_OUTPUTWINDOW5, OnBarCheckEx)
	ON_COMMAND_EX(ID_VIEW_OUTPUTWINDOW6, OnBarCheckEx)
	ON_COMMAND_EX(ID_VIEW_OUTPUTWINDOW7, OnBarCheckEx)
	ON_COMMAND_EX(ID_VIEW_OUTPUTWINDOW8, OnBarCheckEx)
	ON_COMMAND_EX(ID_VIEW_OUTPUTWINDOW9, OnBarCheckEx)
	ON_UPDATE_COMMAND_UI(ID_VIEW_OUTPUTWINDOW1, OnUpdateControlBarMenu)
	ON_UPDATE_COMMAND_UI(ID_VIEW_OUTPUTWINDOW2, OnUpdateControlBarMenu)
	ON_UPDATE_COMMAND_UI(ID_VIEW_OUTPUTWINDOW3, OnUpdateControlBarMenu)
	ON_UPDATE_COMMAND_UI(ID_VIEW_OUTPUTWINDOW4, OnUpdateControlBarMenu)
	ON_UPDATE_COMMAND_UI(ID_VIEW_OUTPUTWINDOW5, OnUpdateControlBarMenu)
	ON_UPDATE_COMMAND_UI(ID_VIEW_OUTPUTWINDOW6, OnUpdateControlBarMenu)
	ON_UPDATE_COMMAND_UI(ID_VIEW_OUTPUTWINDOW7, OnUpdateControlBarMenu)
	ON_UPDATE_COMMAND_UI(ID_VIEW_OUTPUTWINDOW8, OnUpdateControlBarMenu)
	ON_UPDATE_COMMAND_UI(ID_VIEW_OUTPUTWINDOW9, OnUpdateControlBarMenu)
//vls-end// multiple output
	ON_UPDATE_COMMAND_UI(ID_VIEW_OUTPUTWINDOW, OnUpdateControlBarMenu)

	ON_UPDATE_COMMAND_UI(ID_INDICATOR_INFO1, OnUpdateInfo1)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_INFO2, OnUpdateInfo2)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_INFO3, OnUpdateInfo3)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_INFO4, OnUpdateInfo4)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_INFO5, OnUpdateInfo5)

    ON_MESSAGE(WM_USER+200, OnTabAdded)
    ON_MESSAGE(WM_USER+201, OnTabDeleted)

//vls-begin// #reloadscripts
    ON_MESSAGE(WM_USER+300, OnReloadScripts)
//vls-end//

//vls-begin// #quit
    ON_MESSAGE(WM_USER+400, OnQuitCommand)
//vls-end//

    ON_MESSAGE(WM_USER+410, OnHideWindow)
    ON_MESSAGE(WM_USER+420, OnRestoreWindow)
    ON_MESSAGE(WM_USER+430, OnHideWindowToSystemTray)
    ON_MESSAGE(WM_USER+440, OnRestoreWindowFromSystemTray)

//vls-begin// multiple output
    ON_MESSAGE(WM_USER+500, OnShowOutput)
    ON_MESSAGE(WM_USER+501, OnNameOutput)
    ON_MESSAGE(WM_USER+502, OnDockOutput)
    ON_MESSAGE(WM_USER+505, OnPosWOutput)
	ON_MESSAGE(WM_USER+506, OnSizeWOutput)
//vls-end//

    ON_MESSAGE(WM_USER+600, OnCleanInput)
//* en status refreshing
    ON_MESSAGE(WM_USER+651, OnUpdStat1)
    ON_MESSAGE(WM_USER+652, OnUpdStat2)
    ON_MESSAGE(WM_USER+653, OnUpdStat3)
    ON_MESSAGE(WM_USER+654, OnUpdStat4)
    ON_MESSAGE(WM_USER+655, OnUpdStat5)
//*/en

	ON_MESSAGE(WM_USER+680, OnUpdPing)

	// sysTray command
    ON_MESSAGE(WM_USER+701, OnTrayMessage)

END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{

    m_wndSplitter.m_bInited = FALSE;
	
    m_wndSplitter.m_nUpSize = ::GetPrivateProfileInt(L"Main" , L"UpSize" , 300, szGLOBAL_PROFILE);
    m_wndSplitter.m_nDownSize = ::GetPrivateProfileInt(L"Main" , L"DownSize" , 100, szGLOBAL_PROFILE);
	nScrollSize = ::GetPrivateProfileInt(L"Options" , L"Scroll" , 300, szGLOBAL_PROFILE);
    bDisplayCommands  = ::GetPrivateProfileInt(L"Options" , L"DisplayCommands" , 0, szGLOBAL_PROFILE);
    bDisplayInput  = ::GetPrivateProfileInt(L"Options" , L"DisplayInput" , 1, szGLOBAL_PROFILE);
	bInputOnNewLine  = ::GetPrivateProfileInt(L"Options" , L"InputOnNewLine" , 0, szGLOBAL_PROFILE);
	bDisplayPing  = ::GetPrivateProfileInt(L"Options" , L"DisplayPing" , 1, szGLOBAL_PROFILE);
    bMinimizeToTray  = ::GetPrivateProfileInt(L"Options" , L"MinimizeToTray" , 0, szGLOBAL_PROFILE);
    MoreComingDelay  = ::GetPrivateProfileInt(L"Options" , L"MoreComingDelay" , 100, szGLOBAL_PROFILE);
}

CMainFrame::~CMainFrame()
{
    ::WritePrivateProfileInt(L"Main" , L"UpSize" , m_wndSplitter.m_nUpSize, szGLOBAL_PROFILE);
    ::WritePrivateProfileInt(L"Main" , L"DownSize" , m_wndSplitter.m_nDownSize, szGLOBAL_PROFILE);
    ::WritePrivateProfileInt(L"Options" , L"DisplayCommands" , bDisplayCommands, szGLOBAL_PROFILE);
    ::WritePrivateProfileInt(L"Options" , L"DisplayInput" , bDisplayInput , szGLOBAL_PROFILE);
	::WritePrivateProfileInt(L"Options" , L"InputOnNewLine" , bInputOnNewLine , szGLOBAL_PROFILE);
	::WritePrivateProfileInt(L"Options" , L"DisplayPing" , bDisplayPing , szGLOBAL_PROFILE);
    ::WritePrivateProfileInt(L"Options" , L"MinimizeToTray" , bMinimizeToTray, szGLOBAL_PROFILE);
    ::WritePrivateProfileInt(L"Options" , L"MoreComingDelay" , MoreComingDelay , szGLOBAL_PROFILE);
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
    
	if (!m_wndToolBar.CreateEx
		(this,
		TBSTYLE_FLAT,
		WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY
		| CBRS_SIZE_DYNAMIC) ||
		!m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}

	if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicators,
		  sizeof(indicators)/sizeof(UINT)))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}


    UINT Style, ID;
    int Size;
    m_wndStatusBar.GetPaneInfo(0 , ID, Style, Size);
    m_wndStatusBar.SetPaneInfo(0 , ID, Style, 30);

    m_wndStatusBar.GetPaneInfo(NUM_INDICATOR_INFO1 , ID, Style, Size);
    m_wndStatusBar.SetPaneInfo(NUM_INDICATOR_INFO1, ID, Style, 150);
    m_wndStatusBar.GetStatusBarCtrl().SetText(0, NUM_INDICATOR_INFO1, SBT_OWNERDRAW );

    m_wndStatusBar.GetPaneInfo(NUM_INDICATOR_INFO2 , ID, Style, Size);
    m_wndStatusBar.SetPaneInfo(NUM_INDICATOR_INFO2, ID, Style, 150);
    m_wndStatusBar.GetStatusBarCtrl().SetText(0, NUM_INDICATOR_INFO2, SBT_OWNERDRAW );

    m_wndStatusBar.GetPaneInfo(NUM_INDICATOR_INFO3 , ID, Style, Size);
    m_wndStatusBar.SetPaneInfo(NUM_INDICATOR_INFO3, ID, Style, 150);
    m_wndStatusBar.GetStatusBarCtrl().SetText(0, NUM_INDICATOR_INFO3, SBT_OWNERDRAW );

    m_wndStatusBar.GetPaneInfo(NUM_INDICATOR_INFO4 , ID, Style, Size);
    m_wndStatusBar.SetPaneInfo(NUM_INDICATOR_INFO4, ID, Style, 150);
    m_wndStatusBar.GetStatusBarCtrl().SetText(0, NUM_INDICATOR_INFO4, SBT_OWNERDRAW );

    m_wndStatusBar.GetPaneInfo(NUM_INDICATOR_INFO5 , ID, Style, Size);
    m_wndStatusBar.SetPaneInfo(NUM_INDICATOR_INFO5, ID, Style, 150);
    m_wndStatusBar.GetStatusBarCtrl().SetText(0, NUM_INDICATOR_INFO5, SBT_OWNERDRAW );
    

    m_wndStatusBar.GetPaneInfo(NUM_INDICATOR_CONNECTED , ID, Style, Size);
    m_wndStatusBar.SetPaneInfo(NUM_INDICATOR_CONNECTED , ID, Style, 14);
    m_wndStatusBar.GetStatusBarCtrl ().SetText(0, NUM_INDICATOR_CONNECTED, SBT_OWNERDRAW );


    m_wndStatusBar.GetPaneInfo(NUM_INDICATOR_LOGGED , ID, Style, Size);
    m_wndStatusBar.SetPaneInfo(NUM_INDICATOR_LOGGED , ID, Style, 14);
    m_wndStatusBar.GetStatusBarCtrl ().SetText(0, NUM_INDICATOR_LOGGED, SBT_OWNERDRAW );


    m_wndStatusBar.GetPaneInfo(NUM_PATH_WRITING , ID, Style, Size);
    m_wndStatusBar.SetPaneInfo(NUM_PATH_WRITING , ID, Style, 14);
    m_wndStatusBar.GetStatusBarCtrl ().SetText(0, NUM_PATH_WRITING, SBT_OWNERDRAW );


    m_wndStatusBar.GetPaneInfo(NUM_TICKER , ID, Style, Size);
    m_wndStatusBar.SetPaneInfo(NUM_TICKER , ID, Style, 20);

    // TODO: Remove this if you don't want tool tips or a resizeable toolbar
	m_wndToolBar.SetBarStyle(m_wndToolBar.GetBarStyle() |
		CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);

	// TODO: Delete these three lines if you don't want the toolbar to
	//  be dockable
	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
	EnableDocking(CBRS_ALIGN_ANY);
	DockControlBar(&m_wndToolBar);

    m_editBar.Create(this , CBRS_BOTTOM  , 100 );
	
//vls-begin// multiple output
//    CString t;
//    t.LoadString(IDS_OUTPUT);
//    LPCSTR strTitle = t;
//
//	if (!m_coolBar.Create(this, strTitle, CSize(200, 100), ID_VIEW_OUTPUTWINDOW,  WS_CHILD | WS_VISIBLE | CBRS_TOP) )
//	{
//		TRACE0("Failed to create output window\n");
//		return -1;      // fail to create
//	}
//
//    m_coolBar.SetBarStyle(m_coolBar.GetBarStyle()|CBRS_TOOLTIPS|CBRS_FLYBY|CBRS_SIZE_DYNAMIC);
//	m_coolBar.EnableDocking(CBRS_ALIGN_ANY);
//	DockControlBar(&m_coolBar);
//	LoadBarState("JMC");
//    m_coolBar.Load();
    CString t;
    t.LoadString(IDS_OUTPUT);
    
    for (int i = 0; i < MAX_OUTPUT; i++) {
        CString str;
        str.Format(t, i);
        LPWSTR strTitle = (LPWSTR)(const wchar_t*)str;

        m_coolBar[i].m_wndCode = i;
        m_coolBar[i].m_wndAnsi.m_wndCode = i;
        if (!m_coolBar[i].Create(this, strTitle, CSize(200, 100), outputwindows[i],  WS_CHILD | WS_VISIBLE | CBRS_TOP) )
        {
            TRACE0("Failed to create output window\n");
            return -1;      // fail to create
        }
        m_coolBar[i].SetBarStyle(m_coolBar[i].GetBarStyle()|CBRS_TOOLTIPS|CBRS_FLYBY|CBRS_SIZE_DYNAMIC);
        m_coolBar[i].Load();
        DockControlBar(&m_coolBar[i]);
		m_coolBar[i].m_pDockContext->ToggleDocking();
		CControlBar* pBar = GetControlBar(outputwindows[i]);
		wposes[i][0] = m_coolBar[i].m_mX;
		wposes[i][1] = m_coolBar[i].m_mY;
		if(pBar->IsFloating())
		{
			FloatControlBar(pBar,CPoint(m_coolBar[i].m_mX,m_coolBar[i].m_mY),0);
		}

    }
    LoadBarState(L"JMC");
    InitOutputNameFunc(GetOutputName);
	InitWindowSizeFunc(GetWindowSizeFunc, SetWindowSizeFunc);
	
//vls-end//
    
    SetTimer(1, 1000, NULL);

    // Load history here 
    CFile histFile;
    if ( histFile.Open(L"history.dat", CFile::modeRead ) ) {
        CArchive ar(&histFile, CArchive::load );
        m_editBar.GetHistory().Serialize (ar);
        m_editBar.m_nCurrItem = m_editBar.GetHistory().GetCount();
    }

	wchar_t trayTitle[BUFFER_SIZE] = L"";
	
	CSmcDoc* pDoc = (CSmcDoc*)GetActiveDocument();
	if ( pDoc ) {
		CString text;
		text.Format(IDS_JABA_TITLE, pDoc->m_strProfileName);
		wcscpy(trayTitle, text);
	}

	sysTray = CTray(IDR_MAINFRAME, trayTitle);

//	GetDlgItem(ID_VIEW_MUDEMULATOR)->SetWindowText("Emulation");
    return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs
    // cs.style -= WS_VISIBLE;
	return CFrameWnd::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers

void CMainFrame::OnOptionsOptions() 
{
    
    CSmcPropertyDlg dlg(IDS_OPTIONS, this);
    CCommonParamsPage pg1;    // Common params
    CCharSubstPage pg2;
    CProfilePage pg3;
    CLogParamsPage pg4;
    CScriptPage pg5;

	CSmcDoc* pDoc = (CSmcDoc*)GetActiveDocument();
	ASSERT_KINDOF(CSmcDoc, pDoc);
    
    // Fill common pparams
    pg1.m_nStartLine = m_editBar.m_nCursorPosWhileListing;
    pg1.m_nMinLen = m_editBar.m_nMinStrLen;
    pg1.m_strCommandChar = pDoc->m_cCommandChar;
    pg1.m_strCommandDelimiter = cCommandDelimiter;
    pg1.m_nHistorySize = m_editBar.GetHistorySize();
    pg1.m_bDisplayCommands = bDisplayCommands;
	pg1.m_bShowPing = bDisplayPing;
    pg1.m_bClearInput = m_editBar.m_bClearInput;
    pg1.m_bTokenInput = m_editBar.m_bTokenInput;
    pg1.m_bScrollEnd = m_editBar.m_bScrollEnd;
    pg1.m_bKillOneToken = m_editBar.m_bKillOneToken;
    pg1.m_bConnectBeep = bConnectBeep;
    pg1.m_bAutoReconnect = bAutoReconnect;
    pg1.m_bSplitOnBackscroll = pDoc->m_bSplitOnBackscroll;
	pg1.m_bMinimizeToTray = bMinimizeToTray;
    pg1.m_nTrigDelay = MoreComingDelay;
	pg1.m_bLineWrap = pDoc->m_bLineWrap;
	pg1.m_bShowTimestamps = pDoc->m_bShowTimestamps;
	pg1.m_bStickScrollbar = pDoc->m_bStickScrollbar;
	pg1.m_bSelectRect = pDoc->m_bRectangleSelection;
	pg1.m_bRemoveESC = pDoc->m_bRemoveESCSelection;
	pg1.m_bShowHidden = pDoc->m_bShowHiddenText;
	pg1.m_bExtAnsiColors = pDoc->m_bExtAnsiColors;

	if (!bDisplayInput)
		pg1.m_nUserInputHide = 0;
	else if (!bInputOnNewLine)
		pg1.m_nUserInputHide = 1;
	else 
		pg1.m_nUserInputHide = 2;

	pg1.m_nMudCodePage = MudCodePage;


    // Fill subst params
    pg2.m_bAllowSubst = bSubstitution;
    EnterCriticalSection(&secSubstSection);
        memcpy(pg2.m_charsSubst, substChars , SUBST_ARRAY_SIZE);
    LeaveCriticalSection(&secSubstSection);

    if ( bIACSendSingle ) 
        pg2.m_nSendSingle  = 0;
    else 
        pg2.m_nSendSingle  = 1;
    if ( bIACReciveSingle ) 
        pg2.m_nReciveSingle  = 0;
    else 
        pg2.m_nReciveSingle  = 1;

	pg3.m_strCommand		= pDoc->m_strSaveCommand;
	pg3.m_strSaveName		= pDoc->m_strDefSaveFile;
	pg3.m_strStartFileName	= pDoc->m_strDefSetFile;
    pg3.m_strLangFile       = langfile;
    pg3.m_strLangSect       = langsect;
    

    // Log options
	if (bANSILog) {
		pg4.m_LogType = 2;
	} else if (bHTML) {
		pg4.m_LogType = 1;
	} else {
		pg4.m_LogType = 0;
	}

    pg4.m_bRMASupport = bRMASupport;
    pg4.m_nAppendMode = bDefaultLogMode ? 1 : 0 ;
    pg4.m_bAppendLogTitle = bAppendLogTitle;
	pg4.m_bHTMLTimestamps = bHTML ? bHTMLTimestamps : FALSE;
	pg4.m_bTextTimestamps = (!bHTML) ? bTextTimestamps : FALSE;
	pg4.m_nLogAs = bLogAsUserSeen ? 1 : 0;

	pg4.m_nLogCodePage = LogCodePage;
	pg4.m_strLogFlushMinPeriodSec.Format(L"%d", LogFlushMinPeriodSec);
	
    memcpy(&pg5.m_guidLang ,  &theApp.m_guidScriptLang, sizeof(GUID));
    pg5.m_bAllowDebug = bAllowDebug;
    pg5.m_nErrorOutput = nScripterrorOutput;


    dlg.AddPage(&pg1);
    dlg.AddPage(&pg2);
    dlg.AddPage(&pg3);
    dlg.AddPage(&pg4);
    dlg.AddPage(&pg5);
    // End fill common pparams

    if ( dlg.DoModal() == IDOK ) {
        m_editBar.m_nCursorPosWhileListing = pg1.m_nStartLine ;
        m_editBar.m_nMinStrLen = pg1.m_nMinLen;
		m_editBar.SetHistorySize(pg1.m_nHistorySize);
        ASSERT(pg1.m_strCommandChar.GetLength());
        cCommandChar = pDoc->m_cCommandChar = pg1.m_strCommandChar[0];
        cCommandDelimiter = pg1.m_strCommandDelimiter[0];
        bDisplayCommands = pg1.m_bDisplayCommands;
        bDisplayInput = (pg1.m_nUserInputHide != 0);
		bInputOnNewLine = (pg1.m_nUserInputHide == 2);
		bDisplayPing = pg1.m_bShowPing;
		bMinimizeToTray = pg1.m_bMinimizeToTray;
        m_editBar.m_bClearInput = pg1.m_bClearInput;
        m_editBar.m_bTokenInput = pg1.m_bTokenInput;
        m_editBar.m_bScrollEnd = pg1.m_bScrollEnd;
        m_editBar.m_bKillOneToken = pg1.m_bKillOneToken;
        bConnectBeep = pg1.m_bConnectBeep;
        bAutoReconnect = pg1.m_bAutoReconnect;
        pDoc->m_bSplitOnBackscroll = pg1.m_bSplitOnBackscroll;
        if ( !pg1.m_bSplitOnBackscroll ) 
            OnUnsplit();
		pDoc->m_bLineWrap = pg1.m_bLineWrap;
		pDoc->m_bShowTimestamps = pg1.m_bShowTimestamps;
		pDoc->m_bStickScrollbar = pg1.m_bStickScrollbar;
		pDoc->m_bRectangleSelection = pg1.m_bSelectRect;
		pDoc->m_bRemoveESCSelection = pg1.m_bRemoveESC;
		pDoc->m_bShowHiddenText = pg1.m_bShowHidden;
		pDoc->m_bExtAnsiColors = pg1.m_bExtAnsiColors;

        MoreComingDelay = pg1.m_nTrigDelay;

		MudCodePage = pg1.m_nMudCodePage;

        bSubstitution = pg2.m_bAllowSubst ;
        EnterCriticalSection(&secSubstSection);
            memcpy(substChars ,pg2.m_charsSubst,  SUBST_ARRAY_SIZE);
        LeaveCriticalSection(&secSubstSection);

        bIACSendSingle = (pg2.m_nSendSingle == 0);
        bIACReciveSingle = (pg2.m_nReciveSingle == 0);

        
        pDoc->m_strSaveCommand = pg3.m_strCommand;
		pDoc->m_strDefSaveFile = pg3.m_strSaveName;
		pDoc->m_strDefSetFile = pg3.m_strStartFileName;
		wcscpy(langfile, pg3.m_strLangFile);
		wcscpy(langsect, pg3.m_strLangSect);

        // Log settings save
		bANSILog = pg4.m_LogType == 2;
        bHTML = pg4.m_LogType == 1; 
		bHTMLTimestamps = bHTML ? pg4.m_bHTMLTimestamps : FALSE;
        bRMASupport = bANSILog ? pg4.m_bRMASupport : FALSE;
		bTextTimestamps = (!bHTML) ? pg4.m_bTextTimestamps : FALSE;
        bDefaultLogMode = pg4.m_nAppendMode ;
		bLogAsUserSeen = pg4.m_nLogAs;
        bAppendLogTitle = pg4.m_bAppendLogTitle;
		LogCodePage = pg4.m_nLogCodePage;
		LogFlushMinPeriodSec = _wtoi(pg4.m_strLogFlushMinPeriodSec);

        if ( memcmp(&theApp.m_guidScriptLang, &pg5.m_guidLang , sizeof(GUID) ) ) {
            memcpy(&theApp.m_guidScriptLang, &pg5.m_guidLang , sizeof(GUID) ) ;
            PostMessage(WM_COMMAND, ID_SCRIPTING_RELOADSCRIPT, 0 ); 
        }
        bAllowDebug =pg5.m_bAllowDebug;
        nScripterrorOutput = pg5.m_nErrorOutput;
    }
}

void CMainFrame::OnUpdateFrameTitle(BOOL)
{
    CString text;
    CSmcDoc* pDoc = (CSmcDoc*)GetActiveDocument();
    if ( pDoc ) {
        text.Format(IDS_JABA_TITLE, pDoc->m_strProfileName);
        SetWindowText(text);
    }
}

void CMainFrame::OnUpdateLogged(CCmdUI* pUI)
{
    wchar_t buff[32];
    int Data;
    int val  = m_wndStatusBar.GetStatusBarCtrl ().GetText(buff, NUM_INDICATOR_LOGGED, &Data);

    BOOL bLog = IsLogging();
    if (  (bLog && val ) || (!bLog && !val)  ) 
        return ;

    m_wndStatusBar.GetStatusBarCtrl ().SetText((wchar_t*)bLog, NUM_INDICATOR_LOGGED, SBT_OWNERDRAW );
}

void CMainFrame::OnUpdateConnected(CCmdUI* pUI)
{
    wchar_t buff[32];
    int Data;
    int val  = m_wndStatusBar.GetStatusBarCtrl ().GetText(buff, NUM_INDICATOR_CONNECTED, &Data);

    BOOL bLog = IsConnected ();
    if (  (bLog && val ) || (!bLog && !val)  ) 
        return ;

    m_wndStatusBar.GetStatusBarCtrl ().SetText((wchar_t*)bLog, NUM_INDICATOR_CONNECTED, SBT_OWNERDRAW );
}

void CMainFrame::OnUpdatePath(CCmdUI* pUI)
{
    wchar_t buff[32];
    int Data;
    int val  = m_wndStatusBar.GetStatusBarCtrl ().GetText(buff, NUM_PATH_WRITING, &Data);

    BOOL bLog = IsPathing ();
    if (  (bLog && val ) || (!bLog && !val)  ) 
        return ;

    m_wndStatusBar.GetStatusBarCtrl ().SetText((wchar_t*)bLog, NUM_PATH_WRITING, SBT_OWNERDRAW );
}

void CMainFrame::OnUpdateTicker(CCmdUI* pUI)
{
    BOOL bStatus = bTickStatus;
    int toTick = iSecToTick;

    if ( bStatus ) {
        CString str;
        str.Format(L"%d", toTick);
        pUI->SetText(str);
    }
    else
        pUI->SetText(L"OFF");
}

void CMainFrame::OnUpdateInfo1(CCmdUI* pUI)
{
    EnterCriticalSection(&secStatusSection);
    if ( m_strInfo1 != strInfo1  ) {
        m_strInfo1 = strInfo1;

		int Width = LengthWithoutANSI(m_strInfo1) * pDoc->m_nCharX;
		UINT Style, ID;
		int Size;
		m_wndStatusBar.GetPaneInfo(NUM_INDICATOR_INFO1 , ID, Style, Size);
		if ( Size < Width )
			m_wndStatusBar.SetPaneInfo(NUM_INDICATOR_INFO1, ID, Style, Width);

        m_wndStatusBar.GetStatusBarCtrl ().SetText(m_strInfo1, NUM_INDICATOR_INFO1, SBT_OWNERDRAW );
    }
    LeaveCriticalSection(&secStatusSection);
}

void CMainFrame::OnUpdateInfo2(CCmdUI* pUI)
{
    EnterCriticalSection(&secStatusSection);
    if ( m_strInfo2 != strInfo2  ) 
	{
        m_strInfo2 = strInfo2;

		int Width = LengthWithoutANSI(m_strInfo2) * pDoc->m_nCharX;
		UINT Style, ID;
		int Size;
		m_wndStatusBar.GetPaneInfo(NUM_INDICATOR_INFO2 , ID, Style, Size);
		if ( Size < Width )
			m_wndStatusBar.SetPaneInfo(NUM_INDICATOR_INFO2, ID, Style, Width);

        m_wndStatusBar.GetStatusBarCtrl ().SetText(m_strInfo2, NUM_INDICATOR_INFO2, SBT_OWNERDRAW );
    }
    LeaveCriticalSection(&secStatusSection);
}

void CMainFrame::OnUpdateInfo3(CCmdUI* pUI)
{
    EnterCriticalSection(&secStatusSection);
    if ( m_strInfo3 != strInfo3  ) {
        m_strInfo3 = strInfo3;

		int Width = LengthWithoutANSI(m_strInfo3) * pDoc->m_nCharX;
		UINT Style, ID;
		int Size;
		m_wndStatusBar.GetPaneInfo(NUM_INDICATOR_INFO3 , ID, Style, Size);
		if ( Size < Width )
			m_wndStatusBar.SetPaneInfo(NUM_INDICATOR_INFO3, ID, Style, Width);

        m_wndStatusBar.GetStatusBarCtrl ().SetText(m_strInfo3, NUM_INDICATOR_INFO3, SBT_OWNERDRAW );
    }
    LeaveCriticalSection(&secStatusSection);
}

void CMainFrame::OnUpdateInfo4(CCmdUI* pUI)
{
    EnterCriticalSection(&secStatusSection);
    if ( m_strInfo4 != strInfo4  ) {
        m_strInfo4 = strInfo4;

		int Width = LengthWithoutANSI(m_strInfo4) * pDoc->m_nCharX;
		UINT Style, ID;
		int Size;
		m_wndStatusBar.GetPaneInfo(NUM_INDICATOR_INFO4 , ID, Style, Size);
		if ( Size < Width )
			m_wndStatusBar.SetPaneInfo(NUM_INDICATOR_INFO4, ID, Style, Width);

        m_wndStatusBar.GetStatusBarCtrl ().SetText(m_strInfo4, NUM_INDICATOR_INFO4, SBT_OWNERDRAW );
    }
    LeaveCriticalSection(&secStatusSection);
}

void CMainFrame::OnUpdateInfo5(CCmdUI* pUI)
{
    EnterCriticalSection(&secStatusSection);
    if ( m_strInfo5 != strInfo5  ) {
        m_strInfo5 = strInfo5;

		int Width = LengthWithoutANSI(m_strInfo5) * pDoc->m_nCharX;
		UINT Style, ID;
		int Size;
		m_wndStatusBar.GetPaneInfo(NUM_INDICATOR_INFO5 , ID, Style, Size);
		if ( Size < Width )
			m_wndStatusBar.SetPaneInfo(NUM_INDICATOR_INFO5, ID, Style, Width);

        m_wndStatusBar.GetStatusBarCtrl ().SetText(m_strInfo5, NUM_INDICATOR_INFO5, SBT_OWNERDRAW );
    }
    LeaveCriticalSection(&secStatusSection);
}


void CMainFrame::OnDestroy() 
{
    
    WINDOWPLACEMENT wp;
	memset (&wp, 0 , sizeof(wp));
	wp.length = sizeof(wp);
	GetWindowPlacement(&wp);
	if ( wp.showCmd == SW_SHOWMINIMIZED ) 
		wp.showCmd = SW_SHOW;
    ::WritePrivateProfileBinary(L"View" , L"WindowPlacement" ,(LPBYTE)&wp, sizeof(wp), szGLOBAL_PROFILE);

	CWinApp* pApp = AfxGetApp();
    const wchar_t* pProfSave= pApp->m_pszProfileName;
	pApp->m_pszProfileName = szGLOBAL_PROFILE;
	SaveBarState(L"View");
	pApp->m_pszProfileName = pProfSave;
	
    if ( m_wndSplitter.GetRowCount() > 1 ) {
        m_wndSplitter.SavePosition();
    }

    SaveBarState(L"JMC");
//vls-begin// multiple output
//    m_coolBar.Save();
    for (int i = 0; i < MAX_OUTPUT; i++)
        m_coolBar[i].Save();
//vls-end//

    // save history 
    CFile histFile;
//vls-begin// base dir
//    if ( histFile.Open("history.dat", CFile::modeCreate | CFile::modeWrite ) ) {
    CString strFile(szBASE_DIR);
    strFile += L"\\history.dat";
    if ( histFile.Open(strFile, CFile::modeCreate | CFile::modeWrite ) ) {
//vls-end//
        CArchive ar(&histFile, CArchive::store);
        m_editBar.GetHistory().Serialize (ar);
    }

    pMainWnd = NULL;
    CFrameWnd::OnDestroy();
}

void CMainFrame::RestorePosition()
{
	// Loading state of control bars
	CWinApp* pApp = AfxGetApp();
    const wchar_t* pProfSave= pApp->m_pszProfileName;
	pApp->m_pszProfileName = szGLOBAL_PROFILE;
	LoadBarState(L"View");
	pApp->m_pszProfileName = pProfSave;
    UINT  nSize;
    LPBYTE pData;
    if ( ::GetPrivateProfileBinary (L"View", L"WindowPlacement", &pData, &nSize, szGLOBAL_PROFILE) ) {
		WINDOWPLACEMENT wp;
		memcpy(&wp, pData , nSize);
		delete pData;
		SetWindowPlacement(&wp);
	}
}

BOOL CMainFrame::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext) 
{
		if (!m_wndSplitter.Create(this,2,1, CSize(10, 10), pContext, 
            WS_CHILD | WS_VISIBLE | /*WS_VSCROLL | */SPLS_DYNAMIC_SPLIT))
		{
			TRACE0("Failed to create split bar ");
			return FALSE;    // failed to create
		}

        pMainWnd = this;
		m_wndSplitter.m_bInited = TRUE;
        return TRUE;
}


BOOL CInvertSplit::SplitRow()
{
    CRect rect;

    int TotalSize, MinSize;
    GetRowInfo(0, TotalSize, MinSize);

    TotalSize -=  m_cySplitter;
    int Before = TotalSize*m_nUpSize/(m_nDownSize+m_nUpSize) + m_cyBorder;

    BOOL bRet = CSplitterWnd::SplitRow(Before);

    if ( !bRet ) 
        return FALSE;

    CSmcView* pView = (CSmcView*)GetPane(0, 0 );
    CSmcView* pMainView = (CSmcView*)GetPane(1, 0 );
    CSmcDoc* pDoc = pView->GetDocument();

    pMainView->GetClientRect(&rect);
    int lineCount = rect.Height() / pDoc->m_nYsize;
    // now scroll this lines
    int minpos, maxpos;
    pView->GetScrollRange(SB_VERT, &minpos, &maxpos);
    pView->SetScrollPos(SB_VERT, maxpos - lineCount, TRUE);

    pView->InvalidateRect(NULL, FALSE);
    pView->UpdateWindow();

    // Copy contents of old view to the new view 
    pMainView->m_strList.RemoveAll();
    pMainView->m_strList.AddHead(&pView->m_strList);
	pMainView->m_TotalLinesReceived = pView->m_TotalLinesReceived;
    pMainView->m_nCurrentBg  = pView->m_nCurrentBg;
    pMainView->m_nCurrentFg  = pView->m_nCurrentFg;
	pMainView->m_AnsiBGColor  = pView->m_AnsiBGColor;
    pMainView->m_AnsiFGColor  = pView->m_AnsiFGColor;
    pMainView->m_bAnsiBold = pView->m_bAnsiBold;
    pMainView->InvalidateRect(NULL, FALSE);
    pMainView->UpdateWindow();

    return bRet;
}


BOOL CMainFrame::PreTranslateMessage(MSG* pMsg) 
{
	if ( pMsg->message == WM_KEYDOWN ) {
        switch ( pMsg->wParam ) {
		case VK_PAUSE:
			{
				OnUnsplit();
			}
			return TRUE;
        case VK_ESCAPE:
            {
                if ( m_wndSplitter.GetRowCount() == 1 ) {
                    CEdit* pEdit = (CEdit*)m_editBar.GetDlgItem(IDC_EDIT);
                    pEdit->SetWindowText(L"");
                }
                else 
                    OnUnsplit();
            }
            break;
        case VK_PRIOR:
            {
                CSmcDoc* pDoc = (CSmcDoc*)GetActiveDocument();
                // first check second pane exist etc 
                if ( m_wndSplitter.GetRowCount() == 1 && pDoc->m_bSplitOnBackscroll ) {
                    m_wndSplitter.SplitRow();
                } else {
                    CWnd* pWnd = m_wndSplitter.GetPane(0, 0 );
                    if ( pWnd ) 
                        pWnd->PostMessage(WM_VSCROLL , MAKELONG(SB_PAGEUP, 0), 0);
                }
            }
            return TRUE;
        case VK_NEXT:
            {
                CWnd* pWnd = m_wndSplitter.GetPane(0, 0 );
                if ( pWnd ) {
                    pWnd->SendMessage(WM_VSCROLL , MAKELONG(SB_PAGEDOWN, 0), 0);
                    int minpos, maxpos, pos;
                    pWnd->GetScrollRange(SB_VERT, &minpos, &maxpos);
                    pos = pWnd->GetScrollPos(SB_VERT);
                    if ( pos == maxpos ) 
                        OnUnsplit() ;
                }
                
            }
            return TRUE;
        case VK_UP:
            if ( GetKeyState(VK_CONTROL) < 0 )  {
                CSmcDoc* pDoc = (CSmcDoc*)GetActiveDocument();
                // first check second pane exist etc 
                if ( m_wndSplitter.GetRowCount() == 1 && pDoc->m_bSplitOnBackscroll ) {
                    m_wndSplitter.SplitRow();
                } else {
                    CWnd* pWnd = m_wndSplitter.GetPane(0, 0 );
                    if ( pWnd ) 
                        pWnd->PostMessage(WM_VSCROLL , MAKELONG(SB_LINEUP, 0), 0);
                }
            }
            return TRUE;
        case VK_DOWN:
            if ( GetKeyState(VK_CONTROL) < 0 )  {
                CWnd* pWnd = m_wndSplitter.GetPane(0, 0 );
                if ( pWnd ) {
                    pWnd->SendMessage(WM_VSCROLL , MAKELONG(SB_LINEDOWN, 0), 0);
                    int minpos, maxpos, pos;
                    pWnd->GetScrollRange(SB_VERT, &minpos, &maxpos);
                    pos = pWnd->GetScrollPos(SB_VERT);
                    if ( pos == maxpos ) 
                        OnUnsplit() ;
                }
                
            }
            return TRUE;
        default:
            break;
        }                                                                           
    }
	return CFrameWnd::PreTranslateMessage(pMsg);
}

void CMainFrame::OnUnsplit() 
{
	if ( m_wndSplitter.GetRowCount() == 1 ) 
        return;

    m_wndSplitter.SavePosition();

    m_wndSplitter.DeleteRow(0);

    CWnd* pWnd = m_wndSplitter.GetPane(0, 0);
    pWnd->InvalidateRect(NULL, FALSE);
    pWnd->UpdateWindow();

}


void CInvertSplit::SavePosition()
{
	if ( GetRowCount() == 1 ) 
        return;

    int ideal;
    GetRowInfo(0, m_nUpSize, ideal);
    GetRowInfo(1, m_nDownSize, ideal);
}


void CMainFrame::OnSize(UINT nType, int cx, int cy) 
{

	if (nType == SIZE_MINIMIZED) {
		if ((shouldMinimizeToTray == TRUE) || ((shouldMinimizeToTray == -1) && bMinimizeToTray)) {
			ShowWindow(SW_HIDE);
			sysTray.add();
		}
	} else if (nType == SIZE_RESTORED || nType == SIZE_MAXIMIZED) {
		shouldMinimizeToTray = -1;
		if (sysTray.isInTray()) {
			sysTray.remove();
		}
	}

	CFrameWnd::OnSize(nType, cx, cy);
}

void CInvertSplit::OnSize(UINT nType, int cx, int cy) 
{
    
    CSplitterWnd::OnSize(nType, cx, cy);

    if ( ! m_bInited ) 
        return;

    int ideal, Up, Down;
    GetRowInfo(0, Up, ideal);
    GetRowInfo(1, Down, ideal);
    
    Up = m_nUpSize*(Up+Down)/(m_nDownSize+m_nUpSize);
    SetRowInfo(0, Up, 10);
    RecalcLayout();

}

/////////////////////////////////////////////////////////////////////////////
// COutputBar

COutputBar::COutputBar()
{
}

COutputBar::~COutputBar()
{
}

BEGIN_MESSAGE_MAP(COutputBar, CCoolDialogBar)
	//{{AFX_MSG_MAP(COutputBar)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()



/////////////////////////////////////////////////////////////////////////////
// COutputBar message handlers

int COutputBar::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CCoolDialogBar::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	CRect rect;
    GetClientRect(&rect);
    VERIFY(m_wndAnsi.Create(NULL, L"", WS_CHILD | WS_VISIBLE, rect, this, 97));
	
	return 0;
}

void COutputBar::OnSize(UINT nType, int cx, int cy) 
{
	CCoolDialogBar::OnSize(nType, cx, cy);
	
    if ( m_wndAnsi.GetSafeHwnd() == NULL ) 
        return ;

	m_wndAnsi.ModifyStyleEx(WS_EX_CLIENTEDGE, 0,SWP_DRAWFRAME );

    m_wndAnsi.SetWindowPos(NULL, 0 , 0 , cx, cy, SWP_NOZORDER | SWP_NOMOVE);

}
void COutputBar::OnDestroy()
{
	if(IsFloating() && 
	   GetParent() && 
	   GetParent()->GetParent() &&
	   IsWindow(GetParent()->GetParent()->m_hWnd)) {
		CRect rect;
		GetParent()->GetParent()->GetWindowRect(&rect);
		m_mX = rect.left;
		m_mY = rect.top;
	}
	CCoolDialogBar::OnDestroy();
}

BOOL CMainFrame::OnBarCheckEx(UINT nID)
{
    int count = m_wndSplitter.GetRowCount ();
    for ( int i = 0 ; i < count ; i++ ) {
        (m_wndSplitter.GetPane(i, 0))->InvalidateRect(NULL);
    }

//vls-begin// multiple output
    CControlBar* pBar = GetControlBar(nID);

    if (pBar != NULL) {
        BOOL bVisible = (pBar->GetStyle() & WS_VISIBLE);
        for (int i = 0; i < MAX_OUTPUT; i++) {
            if (nID == outputwindows[i]) {
                m_coolBar[i].m_bFlag = !bVisible;
                break;
            }
        }
    }


//vls-end//

    return OnBarCheck(nID);
}

#include "AliasPage.h"
#include "JmcGroupPage.h"
#include "JMCActionsPage.h"
#include "JmcHlightPage.h"
#include "JmcHotkeyPage.h"
//vls-begin// subst page
#include "JmcSubstPage.h"
//vls-end//
//vls-begin// script files
#include "JmcScriptFilesPage.h"
//vls-end//

void CMainFrame::OnEditJmcobjects() 
{
	WaitForSingleObject (eventAllObjectEvent, INFINITE);
    CJmcObjectsDlg dlg(IDS_JMC_OBJECTS, this);
    CAliasPage pg1;
    CJMCActionsPage pg2;
    CJmcHlightPage pg3;
    CJmcHotkeyPage pg4;
    CJmcGroupPage pg5;

//vls-begin// subst page
    CJmcSubstPage pg6;
//vls-end//

//vls-begin// script files
    CJmcScriptFilesPage pg7;
//vls-end//

    dlg.AddPage(&pg1);
    dlg.AddPage(&pg2);
    dlg.AddPage(&pg3);
    dlg.AddPage(&pg4);
    dlg.AddPage(&pg5);
//vls-begin// subst page
    dlg.AddPage(&pg6);
//vls-end//
//vls-begin// script files
    dlg.AddPage(&pg7);
//vls-end//

    dlg.DoModal();

    SetEvent(eventAllObjectEvent );

//vls-begin// script files
    if (bScriptFileListChanged) {
        PostMessage(WM_COMMAND, ID_SCRIPTING_RELOADSCRIPT, 0);
    }
//vls-end//
}

LONG CMainFrame::OnTabAdded( UINT wParam, LONG lParam)
{
    HGLOBAL hg = (HGLOBAL)lParam;
    wchar_t* p = (wchar_t*)GlobalLock(hg);
    CSmcDoc* pDoc = (CSmcDoc*) (((CMainFrame*)AfxGetMainWnd())->GetActiveDocument());

    POSITION pos = pDoc->m_lstTabWords.GetHeadPosition ();
    while (pos ) {
        CString str = pDoc->m_lstTabWords.GetAt(pos);
        if ( !wcsicmp(p, str) ){
            pDoc->m_lstTabWords.RemoveAt (pos);
            break;
        }
        pDoc->m_lstTabWords.GetNext(pos);
    }
    pDoc->m_lstTabWords.AddHead(p);

    GlobalUnlock (hg);
    GlobalFree(hg);

    return 0;
}

LONG CMainFrame::OnTabDeleted( UINT wParam, LONG lParam)
{
    HGLOBAL hg = (HGLOBAL)lParam;
    wchar_t* p = (wchar_t*)GlobalLock(hg);
    CSmcDoc* pDoc = (CSmcDoc*) (((CMainFrame*)AfxGetMainWnd())->GetActiveDocument());

    POSITION pos = pDoc->m_lstTabWords.GetHeadPosition ();
    while (pos ) {
        CString str = pDoc->m_lstTabWords.GetAt(pos);
        if ( !wcsicmp(p, str) ){
            pDoc->m_lstTabWords.RemoveAt (pos);
            break;
        }
        pDoc->m_lstTabWords.GetNext(pos);
    }

    GlobalUnlock (hg);
    GlobalFree(hg);

    return 0;
}

//vls-begin// #reloadscripts
LONG CMainFrame::OnReloadScripts(UINT wParam, LONG lParam)
{
    // just emulating menu: scripts->reload
    return SendMessage(WM_COMMAND, ID_SCRIPTING_RELOADSCRIPT, 0);
}
//vls-end//

//vls-begin// #quit
LONG CMainFrame::OnQuitCommand(UINT wParam, LONG lParam)
{
    // just emulating menu: file->exit
    return SendMessage(WM_COMMAND, ID_APP_EXIT, 0);
}
//vls-end//

//vls-begin// multiple output
LONG CMainFrame::OnShowOutput(UINT wParam, LONG lParam)
{
    int wnd = (int)wParam;
    int opt = (int)lParam;
    UINT nId = outputwindows[wnd];
    CControlBar* pBar = GetControlBar(nId);

    if (pBar != NULL && opt > 0) {
        BOOL bVisible = (pBar->GetStyle() & WS_VISIBLE);
        if ( (opt>1 && bVisible) || (opt==1 && !bVisible) )
            return 0;
    }
    return SendMessage(WM_COMMAND, nId, 0);
}

LONG CMainFrame::OnNameOutput(UINT wParam, LONG lParam)
{
    int wnd = (int)wParam;
    HGLOBAL hg = (HGLOBAL)lParam;

    wchar_t* p = (wchar_t*)GlobalLock(hg);
    CString cs;
    if (p && p[0]) {
        cs = p;
    } else {
        CString t;
        t.LoadString(IDS_OUTPUT);
        cs.Format(t, wnd);
    }
    if (wnd >=0 && wnd < MAX_OUTPUT)
	{
        m_coolBar[wnd].SetTitle(cs);
		//* en
		if(m_coolBar[wnd].IsFloating())
		{
			CControlBar * pBar = GetControlBar(outputwindows[wnd]);
			FloatControlBar(pBar, CPoint(m_coolBar[wnd].m_mX,m_coolBar[wnd].m_mY),0);
		}
		//*/en
	}

    GlobalUnlock(hg);
    GlobalFree(hg);

    return 0;
}
//* en
LONG CMainFrame::OnCleanInput(UINT wParam, LONG lParam)
{
    return m_editBar.CleanLine()?1:0;
}

LONG CMainFrame::OnUpdStat1(UINT wParam, LONG lParam)
{
	CMainFrame::OnUpdateInfo1(NULL);

	return 1;
}

LONG CMainFrame::OnUpdStat2(UINT wParam, LONG lParam)
{
	CMainFrame::OnUpdateInfo2(NULL);
	return 1;
}

LONG CMainFrame::OnUpdStat3(UINT wParam, LONG lParam)
{
	CMainFrame::OnUpdateInfo3(NULL);
	return 1;
}

LONG CMainFrame::OnUpdStat4(UINT wParam, LONG lParam)
{
	CMainFrame::OnUpdateInfo4(NULL);
	return 1;
}

LONG CMainFrame::OnUpdStat5(UINT wParam, LONG lParam)
{
	CMainFrame::OnUpdateInfo5(NULL);
	return 1;
}

LONG CMainFrame::OnUpdPing(UINT wParam, LONG lParam)
{
	int mud_ping = (int)wParam;
	int proxy_ping = (int)lParam;
	static wchar_t mud_buf[64], proxy_buf[64], msg_buf[128];

	switch(mud_ping) {
	case -4:
		swprintf(mud_buf, L"");
		break;
	case -3:
		swprintf(mud_buf, L"ping error");
		break;
	case -2:
		swprintf(mud_buf, L"no connection");
		break;
	case -1:
		swprintf(mud_buf, L"PING TIMEOUT");
		break;
	case 0:
		swprintf(mud_buf, L"ping <1 ms");
		break;
	default:
		swprintf(mud_buf, L"ping %d ms", mud_ping);
		break;
	}

	switch(proxy_ping) {
	case -4:
		swprintf(proxy_buf, L"");
		break;
	case -3:
		swprintf(proxy_buf, L" (proxy: error)");
		break;
	case -2:
		swprintf(proxy_buf, L"");
		break;
	case -1:
		swprintf(proxy_buf, L" (proxy: TIMEOUT)");
		break;
	case 0:
		swprintf(proxy_buf, L" (proxy: <1 ms)");
		break;
	default:
		swprintf(proxy_buf, L" (proxy: %d ms)", proxy_ping);
		break;
	}

	swprintf(msg_buf, L"%ls%ls", mud_buf, proxy_buf);

	m_wndStatusBar.SetPaneText(0, msg_buf);
	
	return 1;
}


LONG CMainFrame::OnDockOutput(UINT wParam, LONG lParam)
{
    int wnd = (int)wParam;

    DWORD cs;
	UINT nDockBarID;
	switch ( lParam ) {
	default:
		cs = 0;
		nDockBarID = 0;
		break;
	case 1:
		cs = CBRS_ALIGN_ANY;
		nDockBarID = 0;
		break;
	case 2:
		cs = CBRS_ALIGN_LEFT;
		nDockBarID = AFX_IDW_DOCKBAR_LEFT;
		break;
	case 3:
		cs = CBRS_ALIGN_TOP;
		nDockBarID = AFX_IDW_DOCKBAR_TOP;
		break;
	case 4:
		cs = CBRS_ALIGN_RIGHT;
		nDockBarID = AFX_IDW_DOCKBAR_RIGHT;
		break;
	case 5:
		cs = CBRS_ALIGN_BOTTOM;
		nDockBarID = AFX_IDW_DOCKBAR_BOTTOM;
		break;
	}
	
    if (wnd >=0 && wnd < MAX_OUTPUT)
	{
        m_coolBar[wnd].EnableDocking(cs);
		if(cs) {
			DockControlBar(&m_coolBar[wnd], nDockBarID);
		} else {
			FloatControlBar(&m_coolBar[wnd], CPoint(m_coolBar[wnd].m_mX, m_coolBar[wnd].m_mY));
		}
	    m_coolBar[wnd].m_Dock = cs;
	}

    return 0;
}

LONG CMainFrame::OnPosWOutput(UINT wParam, LONG lParam)
{
    int p1,p2;
    int wnd = (int)wParam;

	p1 = lParam & 32767;
	p2 = lParam>>16;
    UINT nId = outputwindows[wnd];
    CControlBar* pBar = GetControlBar(nId);

    if ((pBar != NULL) && (m_coolBar[wnd])) {
        FloatControlBar(pBar,CPoint(p1,p2),0);
		m_coolBar[wnd].m_mX = p1;
		m_coolBar[wnd].m_mY = p2;
    }
  
    return 0;
}

LONG CMainFrame::OnSizeWOutput(UINT wParam, LONG lParam)
{
    int p1,p2;
    int wnd = (int)wParam;

	p1 = lParam & 32767;
	p2 = lParam>>16;

	if (wnd >= 0 && wnd < MAX_OUTPUT) {
		UINT nId = outputwindows[wnd];
		CControlBar* pBar = GetControlBar(nId);

		if ((pBar != NULL) && (m_coolBar[wnd])) {
			m_coolBar[wnd].Resize(p1 * pDoc->m_nCharX, p2 * pDoc->m_nYsize);

			if(m_coolBar[wnd].IsFloating()) {
				FloatControlBar(&m_coolBar[wnd],CPoint(m_coolBar[wnd].m_mX,m_coolBar[wnd].m_mY),0);
			} else {
				CSmcView* pView = (CSmcView*)GetActiveView();
				RecalcLayout();
				pView->RedrawWindow();
			}
		}
	} else {
		//m_coolBar[wnd].Resize(p1 * pDoc->m_nCharX, p2 * pDoc->m_nYsize);
	}
  
    return 0;
}
//*/en

//vls-begin// mouse wheel
//DEL BOOL CMainFrame::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt) 
//DEL {
//DEL 	if (!(GetKeyState(VK_SHIFT)&0x8000 || GetKeyState(VK_CONTROL)&0x8000 || GetKeyState(VK_MENU)&0x8000))
//DEL         return CFrameWnd::OnMouseWheel(nFlags, zDelta, pt);
//DEL     CWnd* pWnd = m_wndSplitter.GetPane(0, 0 );
//DEL     WPARAM wParam = MAKELONG(zDelta < 0 ? SB_PAGEDOWN : SB_PAGEUP, 0);
//DEL 	if(GetKeyState(VK_SHIFT)&0x8000 || GetKeyState(VK_MENU)&0x8000)
//DEL 		wParam = MAKELONG(zDelta < 0 ? SB_LINEDOWN : SB_LINEUP, 0);
//DEL 
//DEL     if (zDelta > 0) 
//DEL         if ( m_wndSplitter.GetRowCount() == 1 && pDoc->m_bSplitOnBackscroll )
//DEL             m_wndSplitter.SplitRow();
//DEL 
//DEL 	if ( pWnd )
//DEL         pWnd->SendMessage(WM_VSCROLL , wParam, 0);
//DEL 
//DEL     if (zDelta < 0) 
//DEL 	{
//DEL         int minpos, maxpos, pos;
//DEL         pWnd->GetScrollRange(SB_VERT, &minpos, &maxpos);
//DEL         pos = pWnd->GetScrollPos(SB_VERT);
//DEL         if ( pos == maxpos ) 
//DEL             OnUnsplit() ;
//DEL     }
//DEL 	
//DEL 	return CFrameWnd::OnMouseWheel(nFlags, zDelta, pt);//There should be no internal forwarding of the message!!! Possible error
//DEL }
//vls-end//

LONG CMainFrame::OnTrayMessage(UINT wParam, LONG lParam)
{
	if(lParam == WM_LBUTTONDOWN)
	{
		ShowWindow(SW_SHOW);
		ShowWindow(SW_RESTORE);
	}

	return 1;
}

LONG CMainFrame::OnHideWindow(UINT wParam, LONG lParam)
{

	shouldMinimizeToTray = FALSE;

	ShowWindow(SW_MINIMIZE);

	return 1;
}

LONG CMainFrame::OnRestoreWindow(UINT wParam, LONG lParam)
{
	ShowWindow(SW_RESTORE);

	return 1;
}

LONG CMainFrame::OnHideWindowToSystemTray(UINT wParam, LONG lParam)
{
	shouldMinimizeToTray = TRUE;

	ShowWindow(SW_MINIMIZE);

	return 1;
}

LONG CMainFrame::OnRestoreWindowFromSystemTray(UINT wParam, LONG lParam)
{
	if (shouldMinimizeToTray) {
		ShowWindow(SW_SHOW);
	}

	ShowWindow(SW_RESTORE);		

	return 1;
}

void CMainFrame::OnSysCommand(UINT wParam, LPARAM lParam){
	if(wParam==SC_KEYMENU && (lParam>>16)<=0) return;
	CFrameWnd::OnSysCommand(wParam, lParam);
}


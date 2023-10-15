// AnsiWnd.cpp : implementation file
//

#include "stdafx.h"
#include "smc.h"
#include "MAinFrm.h"
#include "smcdoc.h"
#include "AnsiWnd.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAnsiWnd

CAnsiWnd::CAnsiWnd()
{
    m_bAnsiBold = FALSE;
    m_nCurrentBg = 0;
    m_nCurrentFg = 7;

	m_TotalLinesReceived = 0;

    m_bSelected = FALSE;

}

CAnsiWnd::~CAnsiWnd()
{
}


BEGIN_MESSAGE_MAP(CAnsiWnd, CWnd)
	//{{AFX_MSG_MAP(CAnsiWnd)
	ON_WM_PAINT()
	ON_WM_SETFOCUS()
	ON_WM_SIZE()
	ON_WM_VSCROLL()
	ON_WM_ERASEBKGND()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_RBUTTONDBLCLK()
	ON_WM_MBUTTONDOWN()
	ON_WM_MBUTTONUP()
	ON_WM_MBUTTONDBLCLK()
	ON_WM_MOUSEMOVE()
	ON_WM_CAPTURECHANGED()
	ON_WM_CREATE()
	ON_WM_MOUSEWHEEL()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CAnsiWnd message handlers

BOOL CAnsiWnd::PreCreateWindow(CREATESTRUCT& cs)
{
    cs.style += WS_VSCROLL;
	return CWnd::PreCreateWindow(cs);
}

extern int LengthWithoutANSI(const wchar_t* str);
extern void HandleCSI(const COLORREF *FgColors, const COLORREF *BgColors,
					  BOOL ExtAnsiColors, BOOL DarkOnly, BOOL Invert, BOOL ShowHiddenFg, BOOL ShowHiddenBg,
					  BOOL &AnsiBold, int &CurrentFg, int &CurrentBg,
					  COLORREF &AnsiColorFg, COLORREF &AnsiColorBg,
					  COLORREF &ColorFg, COLORREF &ColorBg,
					  const wchar_t **str);
static int NumOfLines(int StrLength, int LineWidth) 
{
	if (LineWidth <= 0)
		return 0;
	int ret = StrLength / LineWidth;
	if ((StrLength == 0) || (StrLength % LineWidth))
		ret++;
	return ret;
}
void CAnsiWnd::OnPaint() 
{
	CPaintDC dc(this); // device context for painting

    CRect rect;
    GetClientRect(&rect);

    CRgn rgn;
    rgn.CreateRectRgn(rect.left, rect.top, rect.right, rect.bottom);
    dc.SelectClipRgn(&rgn);

    int ScrollIndex = GetScrollPos(SB_VERT);
	int last_line = min(ScrollIndex + m_nPageSize, nScrollSize - 1);

	POSITION pos = m_strList.FindIndex(last_line);
    ASSERT(pos);

	dc.SetBkMode(OPAQUE);
    CFont* pOldFont = dc.SelectObject(&pDoc->m_fntText);

	int top = rect.top;

	m_LineCountsList.clear();
	for(int n_line = 0, total_lines = 0; pos && total_lines <= m_nPageSize; n_line++) {
        CString str = m_strList.GetPrev(pos);

		int length = LengthWithoutANSI((const wchar_t*)str);
		int lines = pDoc->m_bLineWrap ? NumOfLines(length, m_nLineWidth) : 1;

		if (lines <= 0) //nothing can be drawn
			lines++;

		m_LineCountsList.push_back(lines);
		total_lines += lines;

		rect.top = rect.bottom - pDoc->m_nYsize * lines;
		
		if ( dc.RectVisible(&rect) )
			DrawWithANSI(&dc, rect, &str, m_nPageSize - n_line - 1);

		rect.bottom = rect.top;

		if (rect.bottom <= top)
			break;
    }
    dc.SelectObject(pOldFont);
}

void CAnsiWnd::ConvertCharPosition(int TextRow, int TextCol, int *LineNum, int *CharPos)
{
	int row = m_nPageSize;
	*LineNum = TextRow;
	*CharPos = TextCol;
	for (int i = 0; i < m_LineCountsList.size(); i++) {
		row -= m_LineCountsList[i];
		if (row <= TextRow) {
			*LineNum = m_nPageSize - i - 1;
			*CharPos = TextCol + (m_nLineWidth - 1) * (TextRow - row);
			break;
		}
	}
}

void CAnsiWnd::OnSetFocus(CWnd* pOldWnd) 
{
	CWnd::OnSetFocus(pOldWnd);
	
	CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();
    pFrame->m_editBar.SetFocus();
	
}


int CAnsiWnd::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
    SetClassLong(GetSafeHwnd(), GCL_STYLE, GetClassLong(GetSafeHwnd(), GCL_STYLE)-CS_VREDRAW);
    while ( m_strList.GetCount () < nScrollSize ) 
        m_strList.AddTail("");

	m_TotalLinesReceived = 0;

    // Init colors 
    m_nCurrentBg = 0;
    m_nCurrentFg =7;
    

//===================================================================================================================

    CRect rect;
    GetClientRect(&rect);

    // To init screen dimentions in characters !!!
    
    SetScrollSettings();
    return 0;
}

void CAnsiWnd::OnSize(UINT nType, int cx, int cy) 
{
    CWnd::OnSize(nType, cx, cy);
    
	m_nLastPageSize = m_nPageSize;
    m_nPageSize = min(cy/pDoc->m_nYsize, nScrollSize);
	
    m_nYDiff = cy - m_nPageSize*pDoc->m_nYsize;

	m_nLineWidth = cx/pDoc->m_nCharX;
	if(cx % pDoc->m_nCharX)
		m_nLineWidth++;

    SetScrollSettings(FALSE);
    pDoc->m_nOutputWindowCharsSize[m_wndCode] = m_nLineWidth;
}

void CAnsiWnd::SetScrollSettings(BOOL bResetPosition )
{
    int TotalCount = max(nScrollSize-m_nPageSize, 0 );
    int OldPos = GetScrollPos(SB_VERT);
    SetScrollRange(SB_VERT, 0, TotalCount -1, FALSE);
    if ( bResetPosition ) {
        SetScrollPos(SB_VERT, TotalCount -1 , TRUE);
    } else {
        int LastLine = OldPos + m_nLastPageSize;
        SetScrollPos(SB_VERT, LastLine - m_nPageSize, TRUE);
    }
}

void CAnsiWnd::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
    int Pos = GetScrollPos(SB_VERT);
    CRect rect;
    switch ( nSBCode ) {
    case SB_LINEUP:
        if ( Pos ) {
            Pos --;
            SetScrollPos(SB_VERT, Pos, TRUE);
			InvalidateRect(NULL, FALSE );
			UpdateWindow();
			/*
            CDC* pDC = GetDC();
            GetClientRect(&rect);
            pDC->ScrollDC(0 ,pDoc->m_nYsize, rect , NULL , NULL , NULL );

            pDC->SetBkMode(OPAQUE);
            pOldFont = pDC->SelectObject(&pDoc->m_fntText);

            pDC->SelectObject(&pDoc->m_fntText);
            RedrawOneLine(pDC , Pos);
            RedrawOneLine(pDC , Pos+1);
            pDC->SelectObject(pOldFont);
            ReleaseDC(pDC);
			*/
        }
        break;
    case SB_LINEDOWN:
        if ( Pos < nScrollSize -1-m_nPageSize ) {
            Pos ++;
            SetScrollPos(SB_VERT, Pos, TRUE);
			InvalidateRect(NULL, FALSE );
			UpdateWindow();
			/*
            CDC* pDC = GetDC();
            GetClientRect(&rect);
            pDC->ScrollDC(0 ,-pDoc->m_nYsize, rect , NULL , NULL , NULL );

            pDC->SetBkMode(OPAQUE);
            pOldFont = pDC->SelectObject(&pDoc->m_fntText);

            pDC->SelectObject(&pDoc->m_fntText);

            RedrawOneLine(pDC , Pos+m_nPageSize);
            pDC->SelectObject(pOldFont);
            ReleaseDC(pDC);
			*/
        }
        break;
    case SB_PAGEDOWN:
        if ( Pos < nScrollSize -1-m_nPageSize ) {
            Pos += m_nPageSize;
            if ( Pos > nScrollSize -1-m_nPageSize ) 
                Pos = nScrollSize -1-m_nPageSize;
            SetScrollPos(SB_VERT, Pos, TRUE);
            InvalidateRect(NULL, FALSE );
            UpdateWindow();
        }
        break;
    case SB_PAGEUP:
        if ( Pos ) {
            Pos -= m_nPageSize;
            if ( Pos < 0 ) 
                Pos = 0;
            SetScrollPos(SB_VERT, Pos, TRUE);
            InvalidateRect(NULL, FALSE );
            UpdateWindow();
        }
        break;
    case SB_THUMBPOSITION:
        SetScrollPos(SB_VERT, nPos, TRUE);
        InvalidateRect(NULL, FALSE );
        UpdateWindow();
        break;
    case SB_THUMBTRACK:
        SetScrollPos(SB_VERT, nPos, TRUE);
        InvalidateRect(NULL, FALSE );
        UpdateWindow();
        break;
    default :
        break;
    };
}

void CAnsiWnd::RedrawOneLine(CDC* pDC, int LineNum) // Absolute number of line
{
    int Pos = GetScrollPos(SB_VERT);
    if ( LineNum > Pos+m_nPageSize ) 
        return;

    CRect rect;
    GetClientRect(&rect);
    int Y = rect.bottom - (Pos+m_nPageSize-LineNum+1)*pDoc->m_nYsize;
    POSITION pos = m_strList.FindIndex(LineNum);
    ASSERT(pos);
    CString str = m_strList.GetAt(pos);
    rect.top = Y;
    rect.bottom = rect.top+pDoc->m_nYsize;
    DrawWithANSI(pDC, rect, &str);
}

BOOL CAnsiWnd::OnEraseBkgnd(CDC* pDC) 
{
    return TRUE;
	// return CWnd::OnEraseBkgnd(pDC);
}

void CAnsiWnd::DrawWithANSI(CDC* pDC, CRect& rect, CString* str, int nStrPos)
{
    // Set def colors
    m_nCurrentBg = 0;
    m_nCurrentFg = 7;
    m_bAnsiBold = FALSE;
    CRect OutRect;
    COLORREF colorF, colorB;

    const wchar_t* src = *str;

    int LeftSide =0, TopSide =0;
    // Lets do different drawing code for selected/unselected mode. Doing to to
    // keep high speed of drawing while unselected mode


    if ( m_bSelected && nStrPos <= m_nEndSelectY && nStrPos >= m_nStartSelectY) {
        BOOL  bOldInvert = !pDoc->m_bRectangleSelection && (nStrPos > m_nStartSelectY);
        BOOL bNewInvert = bOldInvert;
        int CharCount = 0;
        do  {
            // Get text to draw
            wchar_t Text[BUFFER_SIZE];
            wchar_t* dest = Text;
            int TextLen = 0;
            while (*src && *src != ESC_SEQUENCE_MARK ) {
                // check for current bold
                if ( (pDoc->m_bRectangleSelection || nStrPos == m_nStartSelectY) && CharCount == m_nStartSelectX) {
                    bNewInvert = TRUE;
                }
                if ( (pDoc->m_bRectangleSelection || nStrPos == m_nEndSelectY) && CharCount == m_nEndSelectX + 1) {
                    bNewInvert = FALSE;
                }
                 if ( bNewInvert != bOldInvert) 
                     break;

                *dest++ = *src++;
                TextLen++;
                CharCount ++;
            }
            *dest = 0;

            // Draw text

            // Skip \n from the end
            while ( TextLen && (Text[TextLen-1] == L'\n' ) )
                TextLen--;

			HandleCSI(pDoc->m_ForeColors, pDoc->m_BackColors, pDoc->m_bExtAnsiColors, pDoc->m_bDarkOnly,
					  bOldInvert, FALSE, FALSE, m_bAnsiBold, m_nCurrentFg, m_nCurrentBg, m_AnsiFGColor, m_AnsiBGColor,
					  colorF, colorB, 0);
            pDC->SetTextColor(colorF);
			pDC->SetBkColor(colorB);

            CRect myRect(0,0,0,0) ;
            int XShift;
            if ( TextLen) {
                pDC->DrawText(Text, TextLen, &myRect, DT_LEFT | DT_SINGLELINE | DT_NOCLIP | DT_CALCRECT | DT_NOPREFIX );
                XShift = myRect.Width();

            } else {
                XShift = 0;
            }

			if ( XShift ) {
				//LeftSide += XShift;
                //pDC->ExtTextOut(OutRect.left, OutRect.top, ETO_OPAQUE, &OutRect, Text, TextLen, NULL);
				int index = 0;
				while ( pDoc->m_bLineWrap && LeftSide + XShift > rect.Width() && TextLen > 0) {
					int len = min((rect.Width() - LeftSide) / pDoc->m_nCharX, TextLen);

					if (len < 1) //nothing can be drawn
						break;

					OutRect = rect;
					OutRect.left += LeftSide;
					OutRect.top += TopSide;
					OutRect.right = rect.right;

					pDC->ExtTextOut(OutRect.left, OutRect.top, ETO_OPAQUE, &OutRect, &(Text[index]), len, NULL);
					index += len;
					TextLen -= len;

					LeftSide = 0;
					TopSide += myRect.Height();
					XShift -=  len * pDoc->m_nCharX;
				}
				OutRect = rect;
				OutRect.left += LeftSide;
				OutRect.top += TopSide;
				OutRect.right = OutRect.left + XShift;

				LeftSide += XShift;
                pDC->ExtTextOut(OutRect.left, OutRect.top, ETO_OPAQUE, &OutRect, &(Text[index]), TextLen, NULL);
			}
			
            // !!!! Look for it ! Every time you draw to the end of string !!!! May be change rectangle ???

            if ( bOldInvert != bNewInvert ) {
                bOldInvert = bNewInvert;
                continue;
            }

            // Now check for ANSI colors
            if ( !*src++ ) // if end of string - get out
                break;

            // check for [ command and digit after it. IF not - skip to end of ESC command
			HandleCSI(pDoc->m_ForeColors, pDoc->m_BackColors, pDoc->m_bExtAnsiColors, pDoc->m_bDarkOnly,
					  bOldInvert, FALSE, FALSE, m_bAnsiBold, m_nCurrentFg, m_nCurrentBg, m_AnsiFGColor, m_AnsiBGColor,
					  colorF, colorB, &src);
        }while ( *src );
        // draw to end of the window
        OutRect = rect;
		OutRect.top += TopSide;
        OutRect.left += LeftSide;
		HandleCSI(pDoc->m_ForeColors, pDoc->m_BackColors, pDoc->m_bExtAnsiColors, pDoc->m_bDarkOnly,
				  !pDoc->m_bRectangleSelection && bOldInvert, FALSE, FALSE, m_bAnsiBold, m_nCurrentFg, m_nCurrentBg, m_AnsiFGColor, m_AnsiBGColor,
				  colorF, colorB, &src);
        pDC->SetTextColor(colorF);
		pDC->SetBkColor(colorB);
        pDC->ExtTextOut(OutRect.left, OutRect.top, ETO_OPAQUE, &OutRect, L"", 0, NULL);
    } else {
        do  {
            // Get text to draw
            wchar_t Text[BUFFER_SIZE];
            wchar_t* dest = Text;
            int TextLen = 0;
            while (*src && *src != ESC_SEQUENCE_MARK ) {
                *dest++ = *src++;
                TextLen++;
            }
            *dest = 0;
            // Draw text

            // Skip \n  from the end
            while ( TextLen && (Text[TextLen-1] == L'\n' ) )
                TextLen--;

			HandleCSI(pDoc->m_ForeColors, pDoc->m_BackColors, pDoc->m_bExtAnsiColors, pDoc->m_bDarkOnly,
					  FALSE, FALSE, TRUE, m_bAnsiBold, m_nCurrentFg, m_nCurrentBg, m_AnsiFGColor, m_AnsiBGColor,
					  colorF, colorB, 0);
            pDC->SetTextColor(colorF);
			pDC->SetBkColor(colorB);

            CRect myRect(0,0,0,0);
            int XShift;
            if ( TextLen) {
                pDC->DrawText(Text, TextLen, &myRect, DT_LEFT | DT_SINGLELINE | DT_NOCLIP | DT_CALCRECT | DT_NOPREFIX);
                XShift = myRect.Width();
            } else {
                XShift = 0;
            }

            if ( XShift ) {
				int index = 0;
				while ( pDoc->m_bLineWrap && LeftSide + XShift > rect.Width() && TextLen > 0) {
					int len = min((rect.Width() - LeftSide) / pDoc->m_nCharX, TextLen);

					OutRect = rect;
					OutRect.left += LeftSide;
					OutRect.top += TopSide;
					OutRect.right = rect.right;

					pDC->ExtTextOut(OutRect.left, OutRect.top, ETO_OPAQUE, &OutRect, &(Text[index]), len, NULL);
					index += len;
					TextLen -= len;

					LeftSide = 0;
					TopSide += myRect.Height();
					XShift -=  len * pDoc->m_nCharX;
				}
				OutRect = rect;
				OutRect.left += LeftSide;
				OutRect.top += TopSide;
				OutRect.right = OutRect.left + XShift;

				LeftSide += XShift;
                pDC->ExtTextOut(OutRect.left, OutRect.top, ETO_OPAQUE, &OutRect, &(Text[index]), TextLen, NULL);
			}

            // Now check for ANSI colors
            if ( !*src++ ) // if end of string - get out
                break;

            // check for [ command and digit after it. IF not - skip to end of ESC command
            HandleCSI(pDoc->m_ForeColors, pDoc->m_BackColors, pDoc->m_bExtAnsiColors, pDoc->m_bDarkOnly,
					  FALSE, FALSE, TRUE, m_bAnsiBold, m_nCurrentFg, m_nCurrentBg, m_AnsiFGColor, m_AnsiBGColor,
					  colorF, colorB, &src);
        }while ( *src );
        OutRect = rect;
        OutRect.left += LeftSide;
		OutRect.top += TopSide;
		HandleCSI(pDoc->m_ForeColors, pDoc->m_BackColors, pDoc->m_bExtAnsiColors, pDoc->m_bDarkOnly,
				  FALSE, FALSE, TRUE, m_bAnsiBold, m_nCurrentFg, m_nCurrentBg, m_AnsiFGColor, m_AnsiBGColor,
				  colorF, colorB, 0);
        pDC->SetTextColor(colorF);
		pDC->SetBkColor(colorB);
        pDC->ExtTextOut(OutRect.left, OutRect.top, ETO_OPAQUE, &OutRect, L"", 0, NULL);
    }
}

static const wchar_t* SkipAnsi(const wchar_t* ptr)
{
	if (*ptr == ESC_SEQUENCE_MARK)
		ptr++;

	if (CSI_START(*ptr))
	{
		ptr++;
		for ( ; !CSI_END(*ptr); ptr++ )
			;
	}
	else
	{
		for ( ; *ptr && !(ptr[0] == ESC_SEQUENCE_MARK && ptr[1] == ESC_SEQUENCE_TERMINATOR); ptr++ )
			;
		if (ptr[0] == ESC_SEQUENCE_MARK)
			ptr++;
	}

	if (*ptr)
		ptr++;
    return ptr;
}

void CAnsiWnd::HandleMouseEvent(const wchar_t *event, UINT nFlags, CPoint point)
{
	int col = point.x/pDoc->m_nCharX;
	int row = (point.y-m_nYDiff)/pDoc->m_nYsize;
	int x, y;
	ConvertCharPosition(row, col, &y, &x);

	int nline = GetScrollPos(SB_VERT) + 1 + y;
	CString line = L"", word = L"";
	if (0 <= nline && nline < m_strList.GetCount()) {
		CString raw_line = m_strList.GetAt(m_strList.FindIndex(nline));
		const wchar_t *ptr = raw_line;
		int xpos = 0;
		bool word_ready = false;
		while (*ptr) {
			if ( *ptr == ESC_SEQUENCE_MARK ) {
				ptr = SkipAnsi(ptr);
                continue;
			}
			line += (*ptr);
			if (iswspace(*ptr)) {
				if (xpos < x)
					word = "";
				else
					word_ready = true;
			}
			else if (!word_ready)
				word += (*ptr);
			xpos++;
			ptr++;
		}
	}

	if ( WaitForSingleObject (eventGuiAction, 0 ) == WAIT_TIMEOUT ) {
		swprintf(strGuiAction, L"txt %ls %d %lc%lc%lc%lc%lc %ls", event, m_wndCode,
			(nFlags & MK_CONTROL) ? L'C' : L'_',
			(nFlags & MK_SHIFT) ? L'S' : L'_',
			(nFlags & MK_LBUTTON) ? L'L' : L'_',
			(nFlags & MK_MBUTTON) ? L'M' : L'_',
			(nFlags & MK_RBUTTON) ? L'R' : L'_',
			word);
		wcscpy(strGuiActionLine, line);
		SetEvent(eventGuiAction);
	}
}

void CAnsiWnd::OnLButtonDown(UINT nFlags, CPoint point) 
{
	HandleMouseEvent(L"LDown", nFlags, point);
	CWnd::OnLButtonDown(nFlags, point);

    SetCapture();
    pDoc->m_bFrozen = TRUE;
    m_bSelected = TRUE;

	int col = point.x/pDoc->m_nCharX;
	int row = (point.y-m_nYDiff)/pDoc->m_nYsize;
	int x, y;
	ConvertCharPosition(row, col, &y, &x);

    m_nStartTrackY = m_nEndTrackY = m_nEndSelectY = m_nStartSelectY = y;
    m_nStartTrackX = m_nEndTrackX = m_nStartSelectX = m_nEndSelectX = x;
}

void CAnsiWnd::OnLButtonUp(UINT nFlags, CPoint point) 
{
	HandleMouseEvent(L"LUp", nFlags, point);
    if ( m_bSelected ) {
        ReleaseCapture();
        m_bSelected = FALSE;
        InvalidateRect(NULL, FALSE);
        UpdateWindow();
        pDoc->m_bFrozen = FALSE;

        // Well, start forming text for Clipboard
        CString ResultStr;

        // Good, getting reall numbers of strings
        int ScrollIndex = GetScrollPos(SB_VERT)+1;
        m_nStartSelectY = max(0, m_nStartSelectY);
        POSITION pos = m_strList.FindIndex(ScrollIndex+m_nStartSelectY);
        ASSERT(pos);
        int i = m_nStartSelectY;
        do { 
            CString tmpStr = m_strList.GetAt(pos);
            const wchar_t* ptr = tmpStr;
            int count = 0;
            if (pDoc->m_bRectangleSelection || i == m_nStartSelectY) {
                // Skip to StartX character
                while ( count < m_nStartSelectX && *ptr){
                    if ( *ptr == ESC_SEQUENCE_MARK ){
                        ptr = SkipAnsi(ptr);
                    }
                    else {
                        count++;
                        ptr++;
                    }
                } 
                
            }
            // characters skipped now copy nessesary info to string
            do {
                if ( *ptr == L'\n' ) {
                    ptr++;
                    continue;
                }
                if ( *ptr == ESC_SEQUENCE_MARK ) {
                    ptr = SkipAnsi(ptr);
                    continue;
                } //* en: do not even try
				if ( !(*ptr))
					break;
                if ( count > m_nEndSelectX && (pDoc->m_bRectangleSelection || i == m_nEndSelectY)) 
                    break;
                ResultStr+= *ptr++;
                count++;
            } while ( *ptr );
            if ( i != m_nEndSelectY ) 
                ResultStr +=L"\r\n";
            i++;
            pos = m_strList.FindIndex(ScrollIndex+i);
        } while ( i<=m_nEndSelectY && pos );
        // Put to clipboard
		if (wcslen(ResultStr) != 0)
		{
			VERIFY(OpenClipboard());

			VERIFY(EmptyClipboard());

			HANDLE hData;

			hData = GlobalAlloc(GMEM_ZEROINIT, (ResultStr.GetLength()+1)*sizeof(wchar_t) );
			wchar_t* buff = (wchar_t*)GlobalLock(hData);
			wcscpy (buff, ResultStr);
			GlobalUnlock(hData);
			SetClipboardData(CF_UNICODETEXT, hData);
			CloseClipboard();
		}
    }
	CWnd::OnLButtonUp(nFlags, point);
}

void CAnsiWnd::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
	HandleMouseEvent(L"LDblClk", nFlags, point);
	CWnd::OnLButtonDblClk(nFlags, point);
}

void CAnsiWnd::OnRButtonDown(UINT nFlags, CPoint point) 
{
	HandleMouseEvent(L"RDown", nFlags, point);
	CWnd::OnRButtonDown(nFlags, point);
}

void CAnsiWnd::OnRButtonUp(UINT nFlags, CPoint point) 
{
	HandleMouseEvent(L"RUp", nFlags, point);
	CWnd::OnRButtonUp(nFlags, point);
}

void CAnsiWnd::OnRButtonDblClk(UINT nFlags, CPoint point) 
{
	HandleMouseEvent(L"RDblClk", nFlags, point);
	CWnd::OnRButtonDblClk(nFlags, point);
}

void CAnsiWnd::OnMButtonDown(UINT nFlags, CPoint point) 
{
	HandleMouseEvent(L"MDown", nFlags, point);
	CWnd::OnMButtonDown(nFlags, point);
}

void CAnsiWnd::OnMButtonUp(UINT nFlags, CPoint point) 
{
	HandleMouseEvent(L"MUp", nFlags, point);
	CWnd::OnMButtonUp(nFlags, point);
}

void CAnsiWnd::OnMButtonDblClk(UINT nFlags, CPoint point) 
{
	HandleMouseEvent(L"MDblClk", nFlags, point);
	CWnd::OnMButtonDblClk(nFlags, point);
}

void CAnsiWnd::OnMouseMove(UINT nFlags, CPoint point) 
{
	if (m_bSelected) {
		int col = point.x/pDoc->m_nCharX;
		int row = (point.y-m_nYDiff)/pDoc->m_nYsize;
		int x, y;
		ConvertCharPosition(row, col, &y, &x);

		if ( m_nEndTrackY != y || m_nEndTrackX != x ) {
			int OldEndTrackX = m_nEndTrackX, OldEndTrackY = m_nEndTrackY;
			m_nEndTrackY = y;
			m_nEndTrackX = x;

			// Now calculate SELECT positions !!!!
			m_nStartSelectY = min(m_nStartTrackY, m_nEndTrackY);
			m_nEndSelectY = max(m_nStartTrackY, m_nEndTrackY);
			if (pDoc->m_bRectangleSelection) {					
				m_nStartSelectX = min(m_nStartTrackX, m_nEndTrackX);
				m_nEndSelectX = max(m_nStartTrackX, m_nEndTrackX);
			} else {
				if (m_nStartSelectY == m_nEndSelectY) {
					m_nStartSelectX = min(m_nStartTrackX, m_nEndTrackX);
					m_nEndSelectX = max(m_nStartTrackX, m_nEndTrackX);
				} else if (m_nStartSelectY == m_nStartTrackY) {
					m_nStartSelectX = m_nStartTrackX;
					m_nEndSelectX = m_nEndTrackX;
				} else {
					m_nStartSelectX = m_nEndTrackX;
					m_nEndSelectX = m_nStartTrackX;
				}
				
			}

			InvalidateRect(NULL, FALSE);
			UpdateWindow();
		}
	}
	CWnd::OnMouseMove(nFlags, point);
}

void CAnsiWnd::OnCaptureChanged(CWnd *pWnd) 
{
	OnLButtonUp(0, CPoint(0,0));
	CWnd::OnCaptureChanged(pWnd);
}

void CAnsiWnd::OnUpdate(LPARAM lHint) 
{
    switch ( lHint ) {
    case TEXT_ARRIVED:
        // redraw etc 
        {
            CRect rect, rectSmall;

	        GetClientRect(&rect);

            if (pDoc->m_strOutputTempList[m_wndCode].GetCount() > 0) {
                m_strList.SetAt(m_strList.GetTailPosition(), pDoc->m_strOutputTempList[m_wndCode].GetHead());
                
				if ( pDoc->m_bClearOutputContents[m_wndCode] ) {
					for ( POSITION it = m_strList.GetTailPosition(); it != NULL && m_TotalLinesReceived >= 0; m_strList.GetPrev(it)) {
						m_strList.SetAt(it, "");
						m_TotalLinesReceived--;
					}
					m_TotalLinesReceived = 0;
				}

                POSITION pos = pDoc->m_strOutputTempList[m_wndCode].GetHeadPosition();
                CString last_line = pDoc->m_strOutputTempList[m_wndCode].GetNext(pos);

				int dcnt_last_line = 0;
				int cnt_last_line = 1;
				if (pDoc->m_bLineWrap && m_LineCountsList.size() > 0) {
					int old_len = m_LineCountsList[0];
					cnt_last_line = NumOfLines(LengthWithoutANSI((const wchar_t*)last_line), m_nLineWidth);
					dcnt_last_line = cnt_last_line - old_len;
				}
                
				int new_lines = 0;
                while(pos) {
                    CString str = pDoc->m_strOutputTempList[m_wndCode].GetNext(pos);

                    m_strList.AddTail(str);
                    m_strList.RemoveHead();
					m_TotalLinesReceived++;

					new_lines += pDoc->m_bLineWrap ? 
						NumOfLines(LengthWithoutANSI((const wchar_t*)str), m_nLineWidth) : 1;
                }
                rectSmall.left = 0;
                rectSmall.right = rect.right;
                rectSmall.bottom = rect.bottom; 
				rectSmall.top = rect.bottom -pDoc->m_nYsize*(new_lines+cnt_last_line);
                if ( pDoc->m_nOutputUpdateCount[m_wndCode] ) 
					ScrollWindowEx(0, -pDoc->m_nYsize*(new_lines + dcnt_last_line), NULL, &rect, NULL, /*&rectSmall*/ NULL , SW_INVALIDATE | SW_ERASE);

				if ( pDoc->m_bClearOutputContents[m_wndCode] )
					InvalidateRect(NULL, FALSE);
				else
					InvalidateRect(&rectSmall, FALSE);
				UpdateWindow();
            }
        }        

        break;
    case SCROLL_SIZE_CHANGED:
        if ( nScrollSize < m_strList.GetCount() ) { // remove some string from head of list
            while ( nScrollSize < m_strList.GetCount() ) 
                m_strList.RemoveHead();
        }
        if ( nScrollSize > m_strList.GetCount() ) {
            while ( nScrollSize > m_strList.GetCount() ) 
                m_strList.AddHead("");
        }
        SetScrollSettings();
        InvalidateRect(NULL, FALSE);
        UpdateWindow();
        break;
    default:
        break;
    }
	
}



BOOL CAnsiWnd::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt) 
{
	WPARAM wParam;
	if (GetKeyState(VK_CONTROL)&0x8000) {
		wParam = MAKELONG(zDelta < 0 ? SB_PAGEDOWN : SB_PAGEUP, 0);
	}
	else {
		wParam = MAKELONG(zDelta < 0 ? SB_LINEDOWN : SB_LINEUP, 0);
	}
	OnVScroll(wParam, 0, 0);
	
	return 0;
}

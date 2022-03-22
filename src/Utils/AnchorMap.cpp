// ******************************************************************************************
// **
// ** Original link:
// ** https://www.codeproject.com/Articles/9434/Automatic-resizing-controls
// **
// ** Helper Class / macros to implement docking / anchoring for controls within
// ** windows and dialogs. Works with all window classes in ATL, MFC and pure
// ** Win32
// ** 
// ** This source is freeware. In all cases, NO fee must be charged for this piece
// ** of code, for any reason. 
// ** You use this at your own risk !
// **
// ** greetz!, 
// ** A. Thiede / BluePearl Software aka. drice
// **
// ** ------------------------------
// ** Modification as of March-2022:
// ** ------------------------------ 
// **   - Revamped the entire idea to add modern C++ features, (yikes, vectors);
// **      . removed previous hardcoded control limits;
// **      . added scoping to methods to leave the class with a more intuitive usage;
// **      . separated header from code for faster compilations of projects using 
// **        precompiled headers and also to keep the header clean from new internal 
// **        helper functions;
// **      . moved MOST global variables that restricted the class to some fixed 
// **        behaviors (mostly dificulting support for child windowses) to individual 
// **        per-control items;
// **      . controls and windowses can now be added/removed dynamically so you may
// **        alter the anchoring for your controls on the fly.
// **   - Added support for child windowses inside containers in the same windows. 
// **         (Like windowsews inside tabbed controls that won't auto-move or 
// **          auto-resize if the original control resized and all children windows 
// **          inside children windows). This can be Rekursive (advised to not abuse :p)!
// **   - Added size restrictions capabilities to the class. Now you can specify
// **     minimum and maximum sizes for your controls (and for the main window).
// **   - Also the class now have several static methods to recalculate/reposition controls
// **     that were originally part of a subdialog that now must fit inside a new 
// **     window or container (like a tab control), all respecting the given docking /
// **     anchoring behaviors.
// **
// **   So I'll hapily call this class now ControlAnchorMaps 2.0
// **
// **   I hope you enjoy.
// **
// **   (Leonardo Silva)
// **
// ******************************************************************************************

// ==========================================================================================
// USAGE:
//
// - include DECLARE_ANCHOR_MAP() within your window/dialog class declaration
//   (sugested inside a private section of your window handling class)
//
// - include the other macros within the implementation file (.cpp), outside your 
//   class delimiters
//
//   example:   BEGIN_ANCHOR_MAP(CMyDialogClass)
//                ANCHOR_MAP_ADDGLOBALSIZERESTRICTION(RectSizer) 
//                ANCHOR_MAP_CHILDWINDOW(hWndWindow, ANF_ALL)
//                ANCHOR_MAP_ENTRY(hWndParent, IDOK,         ANF_BOTTOM)
//                ANCHOR_MAP_ENTRY(hWndParent, IDCANCEL,     ANF_BOTTOM)
//                ANCHOR_MAP_ENTRY(hWndParent, IDC_EDITCTRL, ANF_TOP | RIGHT | ANF_BOTTOM)
//                ANCHOR_MAP_ADDSIZERESTRICTION(hWndParent, IDC_EDITCTRL, RECTSIZER&)  
//              END_ANCHOR_MAP(YourMainWindowHandle)
// 
// Remark: ANCHOR_MAP_ADDSIZERESTRICTION() must be called adding the control to the 
//         ANCHOR_MAP_ENTRY list or else it won't find the in the control's list.
// 
// - Within your WM_SIZE handler, call macro ANCHOR_MAP_HANDLESIZERS() to auto-resize 
//   controls. This will auto-call InvalidateRect and UpdateWindow after to ensure screen 
//   redrawings. Also this macro will return the MessageProc immediately, hence put it at
//   the of your WM_SIZE processing handler.
// 
// - Within your WM_GETMINMAXINFO handler, put the ANCHOR_MAP_HANDLERESTRICTORS(wParam, 
//   lParam) macro if you have any GLOBAL WINDOW size restrictor active. Child windowses 
//   and other Controls are handled within ANCHOR_MAP_HANDLESIZERS() already.
//   Same rules apply here: put it in the END of WM_GETMINMAXINFO message processing 
//   section or equivalent.
//
// - If you DECLARE_ANCHOR_MAP() in your class, you can then put the macro 
//   ANCHOR_MAP_EREASEBACKGROUND() to handle WM_EREASEBKGND messages to use a
//   per-control ereasing handler (I personally don't use it, but depending on the
//   target machine you may gain performance here - Didn't test, left this here
//   because the original author included it - Leonardo Silva).
// 
// - You can #define USE_ANF_SCREEN_TO_CLIENT before including the header to replace
//   the original POINT-using ScreenToClient API to a new one supporting RECT
//   structures (this is just an alias call to ControlAnchorMap::screenToClientEx).
// 
// - You may add/remove controls from docking dynamically too, use the macros 
//   ANCHOR_MAP_DYNAMICCONTROL() and ANCHOR_MAP_ADDSIZERESTRICTIONDYNAMIC() for that.
//   Same rules of adding restrictors after controls apply.
//   [Dynamic-created child windowses are still managed by ANCHOR_MAP_CHILDWINDOW()].
//   Call macro ANCHOR_MAP_REMOVE(hWnd) to remove an item or ANCHOR_MAP_REMOVESIZERESTRICTOR(hWnd)
//   to remove an item restrictor. If you plan on clearing or rebuilding the list, use 
//   ANCHOR_MAP_RESETANCHORS() instead. 
// 
//   ===== EXTRAS =====
// 
// - There are new CLASS-level (static) overloaded function helpers to use 
//   when the target window of your controls is created externally and you need to 
//   reposition / anchor controls using the original screen design. 
//   They are listed bellow:
// 
//          - ControlAnchorMap::screenToClientEx 
//          - ControlAnchorMap::copyOffsetRect
//          - ControlAnchorMap::moveRect
//          - ControlAnchorMap::moveOffsetRect
//          - ControlAnchorMap::invertRect
//          - ControlAnchorMap::invertOffsetRect
//          - ControlAnchorMap::compareRects
//          - ControlAnchorMap::compareOffsetRects
//          - ControlAnchorMap::applyMargins
//          - ControlAnchorMap::applySizer
//          - ControlAnchorMap::calculateMargins
//          - ControlAnchorMap::calculateReverseMargins
//          - ControlAnchorMap::calculateOriginalMargins
//          - ControlAnchorMap::repositControl
// 
//   Please see the documentation for each on the method's header.
//
// And that's all.
//
// =============================================================================


// Using precompiled headers... original #include dependencies 
// are preserved bellow if needed
#include "pch.h"

/*
#include <vector>
#include <algorithm>
#include <assert.h>
#include <Windows.h>
*/

#include "AnchorMap.h"

// ============================== PUBLIC METHODS ====================================

#ifdef DEBUG_ANCHORLIB
bool ControlAnchorMap::addChildWindow(HWND window, unsigned int flags, const std::string& name)
{
    return addObject(window, flags, 0, true, name);
}
#else
bool ControlAnchorMap::addChildWindow(HWND window, unsigned int flags)
{
    return addObject(window, flags, 0, true);
}
#endif

#ifdef DEBUG_ANCHORLIB
bool ControlAnchorMap::addControl(HWND parent, unsigned int ctrlID, unsigned int flags, const std::string& name)
{
    return addObject(parent, flags, ctrlID, false, name);
}
#else
bool ControlAnchorMap::addControl(HWND parent, unsigned int ctrlID, unsigned int flags)
{
    return addObject(parent, flags, ctrlID, false);
}
#endif

void ControlAnchorMap::addGlobalSizeRestrictor(const RECTSIZER& rectSizer)
{
    m_globalSizer = rectSizer;
}

bool ControlAnchorMap::addSizeRestrictor(HWND parent, int nCtrlID, const RECTSIZER& rectSizer)
{
    HWND control = GetDlgItem(parent, nCtrlID);
    return addSizeRestrictor(control, rectSizer);
}

bool ControlAnchorMap::addSizeRestrictor(HWND windowOrControl, const RECTSIZER& rectSizer)
{
    for (TCtrlEntry& e : m_Controls)
    {
        if (e.hWnd == windowOrControl)
        {
            e.controlSizer.minSize.width = rectSizer.minSize.width;
            e.controlSizer.minSize.height = rectSizer.minSize.height;
            e.controlSizer.maxSize.width = rectSizer.maxSize.width;
            e.controlSizer.maxSize.height = rectSizer.maxSize.height;
            return true;
        }
    }

    assert(1==0); // tried to add an invalid restrictor. Did you call this BEFORE adding the control to the list?
    return false;
}

void ControlAnchorMap::useDefaultFlags(unsigned int nFlags)
{
    assert(isInitialized() == false);  // tried to call function after initializing
    if (isInitialized())
        return;

    m_bUsedDefaultEntry = true;
    m_nDefaultFlags = nFlags;
}

bool ControlAnchorMap::isInitialized()
{
    return(m_bInitialized);
}

bool ControlAnchorMap::screenToClientEx(HWND hWnd, RECT* pRect)
{
    POINT   pt1 = {};
    POINT   pt2 = {};

    pt1.x = pRect->left;
    pt1.y = pRect->top;
    pt2.x = pRect->right;
    pt2.y = pRect->bottom;
    if (::ScreenToClient(hWnd, &pt1) == false) return(false);
    if (::ScreenToClient(hWnd, &pt2) == false) return(false);
    pRect->left = pt1.x;
    pRect->top = pt1.y;
    pRect->right = pt2.x;
    pRect->bottom = pt2.y;

    return(true);
}

void ControlAnchorMap::initialize(HWND hWndGlobalParent, DWORD dwFlags)
{
    size_t     iCtrl = 0;
    HWND    hWndCtrl = NULL;
    RECT    rcMaxBR = {};
    SIZE    sz1 = {};
    DWORD   dw1 = 0;

    // do some validation
    assert(hWndGlobalParent != NULL);
    if (hWndGlobalParent == NULL) {
        m_Controls.clear();
        return;
    };

    rcMaxBR.right = 0;
    rcMaxBR.bottom = 0;

    // preserve the handle of the parent window. Also saves the window size
    // if we'll be using size restrictors later
    m_globalParent = hWndGlobalParent;
    GetWindowRect(m_globalParent, &m_previousWindowSize);

    // Add the "default control entries" to the list, if this option
    // has been used.
    if (m_bUsedDefaultEntry)
    {
        ::EnumChildWindows(hWndGlobalParent, initDefaultControl, (LPARAM)this);
    };

    // Options left to initialize IF our control list is not empty at this point
    if (!m_Controls.empty())
    {
        // add the sizing-grip to the parent-window
        if (dwFlags & ANIF_SIZEGRIP)
        {
            dw1 = m_nDefaultFlags;
            m_nDefaultFlags = ANF_RIGHT | ANF_BOTTOM;
            sz1.cx = ::GetSystemMetrics(SM_CXVSCROLL);
            sz1.cy = ::GetSystemMetrics(SM_CYHSCROLL);
            m_hWndSizeGrip = ::CreateWindow(L"ScrollBar", L"", WS_CHILD | WS_VISIBLE | SBS_SIZEGRIP,
                m_Controls[0].parentClientRect.right - sz1.cx, m_Controls[0].parentClientRect.bottom - sz1.cy, sz1.cx, sz1.cy,
                hWndGlobalParent, NULL, NULL, 0);
            ::SetWindowPos(m_hWndSizeGrip, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOSIZE);
            initDefaultControl(m_hWndSizeGrip, (LPARAM)this);
            m_nDefaultFlags = dw1;
        };

        // if bFindEdge!=false, find the bottom-right-most edge to get
        // the initial size of the window    
        if (dwFlags & ANIF_CALCSIZE)
        {
            for (iCtrl = 0; iCtrl < m_Controls.size(); iCtrl++)
            {

                if ((LONG)m_Controls[iCtrl].rect.right > rcMaxBR.right) rcMaxBR.right = (LONG)m_Controls[iCtrl].rect.right;
                if ((LONG)m_Controls[iCtrl].rect.bottom > rcMaxBR.bottom) rcMaxBR.bottom = (LONG)m_Controls[iCtrl].rect.bottom;

                m_Controls[iCtrl].parentPrevWindowRect.right = m_Controls[iCtrl].parentPrevWindowRect.left + rcMaxBR.right;
                m_Controls[iCtrl].parentPrevWindowRect.bottom = m_Controls[iCtrl].parentPrevWindowRect.top + rcMaxBR.bottom;

                ::AdjustWindowRect(&m_Controls[iCtrl].parentPrevWindowRect, ::GetWindowLong(m_Controls[iCtrl].hWndParent, GWL_STYLE),
                    ::GetMenu(m_Controls[iCtrl].hWndParent) != NULL);

                m_Controls[iCtrl].parentClientRect.right = m_Controls[iCtrl].parentClientRect.left +
                    (m_Controls[iCtrl].parentPrevWindowRect.right - m_Controls[iCtrl].parentPrevWindowRect.left);
                m_Controls[iCtrl].parentClientRect.bottom = m_Controls[iCtrl].parentClientRect.top +
                    (m_Controls[iCtrl].parentPrevWindowRect.bottom - m_Controls[iCtrl].parentPrevWindowRect.top);
            };
        };

        // calculate the real flags for the controls, which use
        // the ANF_AUTOMATIC flag
        for (iCtrl = 0; iCtrl < m_Controls.size(); iCtrl++)
        {
            SIZE szClient = { 0,0 };

            szClient.cx = (m_Controls[iCtrl].parentClientRect.right - m_Controls[iCtrl].parentClientRect.left);
            szClient.cy = (m_Controls[iCtrl].parentClientRect.bottom - m_Controls[iCtrl].parentClientRect.top);

            if (m_Controls[iCtrl].nFlags & ANF_AUTOMATIC) {

                m_Controls[iCtrl].nFlags = 0;

                // If the top-edge of the control is within the upper-half of the
                // client area, set a top-anchor. If the bottom-edge of the control
                // is within the lower-half of the client area, set a bottom-anchor
                if (m_Controls[iCtrl].rect.top < (szClient.cy / (double)2)) m_Controls[iCtrl].nFlags |= ANF_TOP;
                if (m_Controls[iCtrl].rect.bottom >= (szClient.cy / (double)2)) m_Controls[iCtrl].nFlags |= ANF_BOTTOM;

                // If the left-edge of the control is within the left-half of the
                // client area, set a left-anchor. If the right-edge of the control
                // is within the right-half of the client area, set a right-anchor
                if (m_Controls[iCtrl].rect.left < (szClient.cx / (double)2)) m_Controls[iCtrl].nFlags |= ANF_LEFT;
                if (m_Controls[iCtrl].rect.right >= (szClient.cx / (double)2)) m_Controls[iCtrl].nFlags |= ANF_RIGHT;

            };
        };
    }

    m_bInitialized = true;
}

intptr_t ControlAnchorMap::handleSizers()
{
    int             iCtrl = 0;
    TCtrlEntry* pCtrl = nullptr;
    bool            bChanged = false;
    HDWP            hWdp = NULL;
    WINDOWPLACEMENT wpl = {};

#ifdef DEBUG_ANCHORLIB
    assert(isInitialized() == true); // tried to call handle sizer function without initializing class
    if (!isInitialized())
        return TRUE;
#endif 

    if (m_Controls.empty())
        return TRUE;

    // handle the visibility of the sizing-grip if we have one
    if (m_hWndSizeGrip != NULL)
    {
        wpl.length = sizeof(wpl);
        ::GetWindowPlacement(m_globalParent, &wpl);
        if ((wpl.showCmd == SW_MAXIMIZE) && (::IsWindowVisible(m_globalParent)))
        {
            ::ShowWindow(m_hWndSizeGrip, SW_HIDE);
        }
        else if (!::IsWindowVisible(m_hWndSizeGrip))
        {
            ::ShowWindow(m_hWndSizeGrip, SW_SHOW);
        };
    };

    // Must group controls by parent hWnd or else BeginDeferWindowPos will fail...
    // Also we are processing parent windowses first or else children will get
    // incorrect rectangles for resizing.
    if (!m_isSorted)
    {
        fullControlsSort();
        m_isSorted = true;
    }

    // Grab number of controls in all control groups...
    std::vector<int> controlGroupNum;
    HWND hCurrent = m_Controls[0].hWndParent;
    int groupCount = 0;
    for (size_t i = 0; i < m_Controls.size(); i++)
    {
        if (hCurrent == m_Controls[i].hWndParent)
            groupCount++;
        else
        {
            // push previous group
            controlGroupNum.push_back(groupCount);

            // count back to 1
            groupCount = 1;
            hCurrent = m_Controls[i].hWndParent;
        }
    }

    // Push leftover counter
    controlGroupNum.push_back(groupCount);

    // clears global invalidation flag. this gets atualized by moveObject() if a control moved or resized.
    m_invalidated = false;

    // Now processes all control groups
    size_t curPosition = 0;
    for (size_t i = 0; i < controlGroupNum.size(); i++)
    {
        int countDefer = controlGroupNum[i];
        hWdp = BeginDeferWindowPos(countDefer);
        int prevPosition = curPosition;
        for (iCtrl = curPosition; iCtrl < (prevPosition + countDefer); iCtrl++)
        {
            preProcess(m_Controls[iCtrl]);
            moveObject(m_Controls[iCtrl], hWdp);
            postProcess(m_Controls[iCtrl]);
            curPosition++;
        };
        std::ignore = ::EndDeferWindowPos(hWdp);    
    }

    // Do an immediate update
    if (m_invalidated)
    {
        ::InvalidateRect(m_globalParent, NULL, TRUE);
        ::UpdateWindow(m_globalParent);
    }

    return FALSE;     // tells the message processor we've managed the sizing
}

intptr_t ControlAnchorMap::handleRestrictors(WPARAM wParam, LPARAM lParam)
{

#ifdef DEBUG_ANCHORLIB
    assert(isInitialized() == true); // tried to call handle restrictor function without initializing class
    if (!isInitialized())
        return FALSE;
#endif

    if (m_globalSizer.empty())
        return FALSE;

    MINMAXINFO* info = reinterpret_cast<MINMAXINFO*>(lParam);
    if (m_globalSizer.maxSize.width)
        info->ptMaxTrackSize.x = m_globalSizer.maxSize.width;
    if (m_globalSizer.maxSize.height)
        info->ptMaxTrackSize.y = m_globalSizer.maxSize.height;
    if (m_globalSizer.minSize.width)
        info->ptMinTrackSize.x = m_globalSizer.minSize.width;
    if (m_globalSizer.minSize.height)
        info->ptMinTrackSize.y = m_globalSizer.minSize.height;

    return FALSE;  // tells the message processor we've handled the message
}

bool ControlAnchorMap::eraseBackground(HDC hDC)
{
    HRGN        hRgn1 = NULL;
    HRGN        hRgn2 = NULL;
    HRGN        hRgn3 = NULL;
    RECT        rc;
    HBRUSH      hBrush = NULL;
    int         iCtrl = 0;
    HWND        hWndChild = NULL;
    bool        bForceErase = false;
    bool        bVisible = false;

    assert(hDC != NULL); // Invalid hDC
    if (hDC == NULL)
        return false;

    // create a brush to fill the background with    
    hBrush = CreateSolidBrush(m_clrBackground);

    // get the coordinates of the parent-window
    // and create a region-object for the whole
    // area of the window
    ::GetClientRect(m_globalParent, &rc);
    hRgn1 = CreateRectRgn(rc.left, rc.top, rc.right, rc.bottom);
    hRgn2 = CreateRectRgn(0, 0, 0, 0);
    hRgn3 = CreateRectRgn(0, 0, 0, 0);

    // now cycle through all visible controls and 
    // remove their area from 
    hWndChild = ::GetWindow(m_globalParent, GW_CHILD);
    while (hWndChild != NULL) {

        iCtrl = findWindow(hWndChild);
        if (iCtrl != -1) if ((m_Controls[iCtrl].nFlags & ANF_ERASE) == 0) iCtrl = -1;

        bVisible = ::IsWindowVisible(hWndChild);

        if ((iCtrl == -1) && (bVisible)) {

            ::GetWindowRect(hWndChild, &rc);
            screenToClientEx(m_globalParent, &rc);

            ::SetRectRgn(hRgn2, rc.left, rc.top, rc.right, rc.bottom);

            ::CombineRgn(hRgn3, hRgn1, hRgn2, RGN_DIFF);

            HRGN hRgnTemp = hRgn1;
            hRgn1 = hRgn3;
            hRgn3 = hRgnTemp;

        };

        hWndChild = ::GetWindow(hWndChild, GW_HWNDNEXT);

    };

    ::FillRgn(hDC, hRgn1, hBrush);

    DeleteObject(hRgn3);
    DeleteObject(hRgn2);
    DeleteObject(hRgn1);

    DeleteObject(hBrush);

    return true;
}

void ControlAnchorMap::reset()
{
    m_bInitialized = false;
    m_bUsedDefaultEntry = false;
    m_globalParent = nullptr;
    m_nDefaultFlags = 0;
    m_clrBackground = GetSysColor(COLOR_BTNFACE);
    m_hWndSizeGrip = NULL;
    m_isSorted = false;
    m_Controls.clear();
    m_globalSizer = {};
    m_previousWindowSize = {};
    m_invalidated = false;
}

void ControlAnchorMap::removeGlobalSizeRestrictor()
{
    m_globalSizer = {};
}

bool ControlAnchorMap::removeWindowOrControl(HWND toRemove)
{
    for (auto i = m_Controls.begin(); i != m_Controls.end(); i++)
    {
        if (i->hWnd == toRemove)
        {
            m_Controls.erase(i);
            m_isSorted = false;
            return true;
        }
    }

#ifdef DEBUG_ANCHORLIB
    throw; // Removed an inexistent control
#endif
    return false;
}

bool ControlAnchorMap::removeRestrictor(HWND toRemove)
{
    for (auto i = m_Controls.begin(); i != m_Controls.end(); i++)
    {
        if (i->hWnd == toRemove)
        {
            i->controlSizer = { {0,0}, {0,0} };
            return true;
        }
    }

#ifdef DEBUG_ANCHORLIB
    throw; // Removed an inexistent restrictor
#endif
    return false;
}

void ControlAnchorMap::setClearBackgroundColor(COLORREF newColor)
{
    m_clrBackground = newColor;
}

UINT ControlAnchorMap::applySizer(FSIZE& target, const SIZER& sizer, int flags) {

    bool bRestrictedX = false;
    bool bRestrictedY = false;

    if (flags == SIZER_MIN)
    {
        // is target width less than minimum width?
        if (sizer.width > 0 && target.cx < sizer.width)
        {
            target.cx = sizer.width;
            bRestrictedX = true;
        }
        // is target height less than minimum height?
        if (sizer.height > 0 && target.cy < sizer.height)
        {
            target.cy = sizer.height;
            bRestrictedY = true;
        }
    }
    else
    {
        // is target width more than maximum width?
        if (sizer.width > 0 && target.cx > sizer.width)
        {
            target.cx = sizer.width;
            bRestrictedX = true;
        }
        // is target height more than maximum height?
        if (sizer.height > 0 && target.cy > sizer.height)
        {
            target.cy = sizer.height;
            bRestrictedY = true;
        }
    }

    return (bRestrictedX ? RESTRICTED_X : 0x0000) | (bRestrictedY ? RESTRICTED_Y : 0x0000);
}

// ============================== PRIVATE METHODS ====================================

int ControlAnchorMap::findWindow(HWND hWnd) {

    for (size_t i = 0; i < m_Controls.size(); i++)
        if (m_Controls[i].hWnd == hWnd) return i;
    return(-1);

}

#ifdef DEBUG_ANCHORLIB
bool ControlAnchorMap::addObject(HWND windowOrParent, unsigned int nFlags, unsigned int nIDCtrl, bool isChildWindow, const std::string& nControlName)
#else
bool ControlAnchorMap::addObject(HWND windowOrParent, unsigned int nFlags, unsigned int nIDCtrl, bool isChildWindow)
#endif
{
    // Validations!
    assert(windowOrParent != NULL); // tried to add an invalid object
    if (windowOrParent == NULL)
        return false;

    TCtrlEntry newCtrl;
#ifdef DEBUG_ANCHORLIB
    newCtrl.nControlName = nControlName;
#endif
    HWND controlWindowHwnd;

    newCtrl.nFlags = nFlags;
    newCtrl.isChildWindow = isChildWindow;

    // Fetch informations about the control.
    if (isChildWindow)
    {
        // For child window items, the "windowOrParent" field is the child window itself...
        controlWindowHwnd = windowOrParent;
        HWND grandParent = GetParent(windowOrParent);
        assert(grandParent != NULL);       // no window here should have a NULL parent
        newCtrl.hWndParent = grandParent;
    }
    else
    {
        // nIDCtrl == -1 is an override behavior... add a control without an ID, so the 
        // "windowOrParent" is now the handle for the control window instead of a handle 
        //  to the parent. See macro ANCHOR_MAP_DYNAMICCONTROL for reference.
        if (nIDCtrl == -1)
        {
            controlWindowHwnd = windowOrParent;
            HWND newParent = GetParent(windowOrParent);
            assert(newParent != NULL);
            newCtrl.hWndParent = newParent;
        }
        else
        {
            // For child window dialog items, the "windowOrParent" field is the parent, so we grab the GetDlgItem instead...
            controlWindowHwnd = ::GetDlgItem(windowOrParent, nIDCtrl);
            assert(controlWindowHwnd != NULL); // no control here should have a nul parent
            newCtrl.hWndParent = windowOrParent;
        }
    }

    // Assign the HWND determined above to the control
    newCtrl.hWnd = controlWindowHwnd;

    // Now grab information about the parent window rectangles, which contains the control
    ::GetWindowRect(newCtrl.hWndParent, &newCtrl.parentPrevWindowRect);
    CopyRect(&newCtrl.parentNewRect, &newCtrl.parentPrevWindowRect);
    ::GetClientRect(newCtrl.hWndParent, &newCtrl.parentClientRect);

    // Now adds in information about the control's rectangle. Since control rectangles
    // are double floating-point numbers, we use a temporary RECT for that...
    RECT rcCtrl;
    ::GetWindowRect(newCtrl.hWnd, &rcCtrl);
    screenToClientEx(newCtrl.hWndParent, &rcCtrl);
    newCtrl.rect.left = rcCtrl.left;
    newCtrl.rect.top = rcCtrl.top;
    newCtrl.rect.right = rcCtrl.right;
    newCtrl.rect.bottom = rcCtrl.bottom;

    // Calculate anchors for automatic placement (this overrides other flags)
    if (newCtrl.nFlags & ANF_AUTOMATIC) {
        SIZE szClient = { 0,0 };

        newCtrl.nFlags = 0;

        szClient.cx = (newCtrl.parentClientRect.right - newCtrl.parentClientRect.left);
        szClient.cy = (newCtrl.parentClientRect.bottom - newCtrl.parentClientRect.top);
        // If the top-edge of the control is within the upper-half of the
        // client area, set a top-anchor. If the bottom-edge of the control
        // is within the lower-half of the client area, set a bottom-anchor
        if (newCtrl.rect.top < (szClient.cy / (double)2)) newCtrl.nFlags |= ANF_TOP;
        if (newCtrl.rect.bottom >= (szClient.cy / (double)2)) newCtrl.nFlags |= ANF_BOTTOM;
        // If the left-edge of the control is within the left-half of the
        // client area, set a left-anchor. If the right-edge of the control
        // is within the right-half of the client area, set a right-anchor
        if (newCtrl.rect.left < (szClient.cx / (double)2)) newCtrl.nFlags |= ANF_LEFT;
        if (newCtrl.rect.right >= (szClient.cx / (double)2)) newCtrl.nFlags |= ANF_RIGHT;
    }

    m_Controls.push_back(newCtrl);
    m_isSorted = false;

    return(true);
}

void ControlAnchorMap::preProcess(TCtrlEntry& pControl)
{
    // Validation
    assert(pControl.hWndParent != nullptr); // something bad happened to the control list
    if (pControl.hWndParent == nullptr)
        return;

    RECT pWndRect = { 0, 0, 0, 0 };
    GetWindowRect(pControl.hWndParent, &pWndRect);

    if (!pControl.additionalMargins.empty())
        applyMargins(pWndRect, pControl.additionalMargins);

    // preserve the new bounds of the parent window
    pControl.parentNewRect.left = pWndRect.left;
    pControl.parentNewRect.top = pWndRect.top;
    pControl.parentNewRect.right = pWndRect.right;
    pControl.parentNewRect.bottom = pWndRect.bottom;

    // determine which sides of the border have changed
    // (we can use our defined ANF_ constants here for simplicity)
    pControl.uiSizedBorders = 0;
    if (pControl.parentNewRect.left != pControl.parentPrevWindowRect.left)        pControl.uiSizedBorders |= ANF_LEFT;
    if (pControl.parentNewRect.top != pControl.parentPrevWindowRect.top)          pControl.uiSizedBorders |= ANF_TOP;
    if (pControl.parentNewRect.right != pControl.parentPrevWindowRect.right)      pControl.uiSizedBorders |= ANF_RIGHT;
    if (pControl.parentNewRect.bottom != pControl.parentPrevWindowRect.bottom)    pControl.uiSizedBorders |= ANF_BOTTOM;

    // calculate deltas
    pControl.szDelta.cx = (pControl.parentNewRect.right - pControl.parentNewRect.left)
        - (pControl.parentPrevWindowRect.right - pControl.parentPrevWindowRect.left);
    pControl.szDelta.cy = (pControl.parentNewRect.bottom - pControl.parentNewRect.top)
        - (pControl.parentPrevWindowRect.bottom - pControl.parentPrevWindowRect.top);

    // calculate new client-rect
    pControl.parentClientRect.right += pControl.szDelta.cx;
    pControl.parentClientRect.bottom += pControl.szDelta.cy;
}

void ControlAnchorMap::postProcess(TCtrlEntry& pControl)
{
    pControl.parentPrevWindowRect.left = pControl.parentNewRect.left;
    pControl.parentPrevWindowRect.top = pControl.parentNewRect.top;
    pControl.parentPrevWindowRect.right = pControl.parentNewRect.right;
    pControl.parentPrevWindowRect.bottom = pControl.parentNewRect.bottom;
}

void ControlAnchorMap::moveObject(TCtrlEntry& pCtrl, HDWP hDeferPos)
{
    FSIZE szCtrl = { 0,0 };

    // Get the size of the control
    szCtrl.cx = pCtrl.rect.right - pCtrl.rect.left;
    szCtrl.cy = pCtrl.rect.bottom - pCtrl.rect.top;

    // we´ve nothing changed until now
    bool bChanged = false;

    // handle docking
    RECT& cliRc = pCtrl.parentClientRect;
    if ((pCtrl.nFlags & ANF_DOCK_ALL) == ANF_DOCK_ALL) {

        //setFRect(&pCtrl.rect, 0, 0, pCtrl.parentClientRect.right, pCtrl.parentClientRect.bottom);
        setFRect(&pCtrl.rect, 0, 0, cliRc.right, cliRc.bottom);
        bChanged = true;
    }
    if (pCtrl.nFlags & ANF_DOCK_TOP) {

        setFRect(&pCtrl.rect, 0, 0, (double)cliRc.right, szCtrl.cy);
        bChanged = true;

    }
    else if (pCtrl.nFlags & ANF_DOCK_BOTTOM) {

        setFRect(&pCtrl.rect, 0, cliRc.bottom - szCtrl.cy, cliRc.right, cliRc.bottom);
        bChanged = true;

    }
    else if (pCtrl.nFlags & ANF_DOCK_LEFT) {

        setFRect(&pCtrl.rect, 0, 0, szCtrl.cx, cliRc.bottom);
        bChanged = true;

    }
    else if (pCtrl.nFlags & ANF_DOCK_RIGHT) {

        setFRect(&pCtrl.rect, cliRc.right - szCtrl.cx, 0, cliRc.right, cliRc.bottom);
        bChanged = true;

    }
    else if (pCtrl.nFlags & ANF_DOCK_LEFT_EX) {

        setFRect(&pCtrl.rect, 0, pCtrl.rect.top, szCtrl.cx, pCtrl.rect.bottom);
        bChanged = true;

    }
    else if (pCtrl.nFlags & ANF_DOCK_RIGHT_EX) {

        setFRect(&pCtrl.rect, pCtrl.rect.left, pCtrl.rect.top, cliRc.right, pCtrl.rect.bottom);
        bChanged = true;

    }
    else if (pCtrl.nFlags & ANF_DOCK_TOP_EX) {

        setFRect(&pCtrl.rect, pCtrl.rect.left, 0, pCtrl.rect.right, pCtrl.rect.bottom);
        bChanged = true;

    }
    else if (pCtrl.nFlags & ANF_DOCK_BOTTOM_EX) {

        setFRect(&pCtrl.rect, pCtrl.rect.left, pCtrl.rect.top, pCtrl.rect.right, cliRc.bottom);
        bChanged = true;

    };

    // handle anchoring
    if ((pCtrl.uiSizedBorders & ANF_LEFTRIGHT) && (pCtrl.szDelta.cx != 0) && (!bChanged)) {

        switch (pCtrl.nFlags & ANF_LEFTRIGHT) {

        case ANF_LEFT: // nothing to do here, control moves automatically
                                // with the left-border of the window (client-rect)
            break;

        case ANF_RIGHT: pCtrl.rect.left += pCtrl.szDelta.cx;
            pCtrl.rect.right = (pCtrl.rect.left + szCtrl.cx);
            bChanged = true;
            break;

        case ANF_LEFTRIGHT: pCtrl.rect.right += pCtrl.szDelta.cx;
            bChanged = true;
            break;

        default: pCtrl.rect.left += ((double)pCtrl.szDelta.cx / 2.0);
            pCtrl.rect.right = (pCtrl.rect.left + szCtrl.cx);
            bChanged = true;
            break;

        };

    };

    if ((pCtrl.uiSizedBorders & ANF_TOPBOTTOM) && (pCtrl.szDelta.cy != 0)) {

        switch (pCtrl.nFlags & ANF_TOPBOTTOM) {

        case ANF_TOP: // nothing to do here, control moves automatically
                              // with the top of the window (client-rect);
            break;

        case ANF_BOTTOM: pCtrl.rect.top += pCtrl.szDelta.cy;
            pCtrl.rect.bottom = (pCtrl.rect.top + szCtrl.cy);
            bChanged = true;
            break;

        case ANF_TOPBOTTOM: pCtrl.rect.bottom += pCtrl.szDelta.cy;
            bChanged = true;
            break;

        default: pCtrl.rect.top += ((double)pCtrl.szDelta.cy / 2.0);
            pCtrl.rect.bottom = (pCtrl.rect.top + szCtrl.cy);
            bChanged = true;
            break;
        };

    };

    // now reposition the control, if its size/position has changed
    if (bChanged)
    {
        szCtrl.cx = pCtrl.rect.right - pCtrl.rect.left;
        szCtrl.cy = pCtrl.rect.bottom - pCtrl.rect.top;

        // Apply sizers
        if (!pCtrl.controlSizer.empty())
        {
            // Determine if the control resizing was restricted and invalidate
            // drawing regions if any size changed.
            UINT restrictResult = applyRectSizer(szCtrl, pCtrl.controlSizer);
            if (restrictResult == RESTRICTED_NONE)  // Nothing restricted... that means the control changed sizes.
                m_invalidated = true;
            if (restrictResult == RESTRICTED_X && pCtrl.szDelta.cy > 0) // Restricted X axis resize, but Y still changed
                m_invalidated = true;
            if (restrictResult == RESTRICTED_Y && pCtrl.szDelta.cx > 0) // Restricted Y axis resize, but X still changed
                m_invalidated = true;
            // RESTRICTED_BOTH - then nothing changed. Skip invalidate mark
        }
        else
            // No size restrictions, then mark global invalidation flag.
            m_invalidated = true;

        hDeferPos = ::DeferWindowPos(hDeferPos, pCtrl.hWnd, NULL, (int)pCtrl.rect.left,
            (int)pCtrl.rect.top, (int)szCtrl.cx, (int)szCtrl.cy, SWP_NOZORDER | SWP_NOACTIVATE);
    };
}

void ControlAnchorMap::setFRect(FRECT* pRect, double left, double top, double right, double bottom)
{
    pRect->left = left;
    pRect->top = top;
    pRect->right = right;
    pRect->bottom = bottom;
}

int CALLBACK ControlAnchorMap::initDefaultControl(HWND hWnd, LPARAM lParam) 
{

    ControlAnchorMap* pMap = (ControlAnchorMap*)(lParam);
    TCtrlEntry          newEntry;

    // do some validation
    if (hWnd == NULL)
        return static_cast<int>(false);
    assert(pMap != NULL);              // something bad happened to your class instance...

    // do not add the control if it is already within our list
    for (size_t iCtrl = 0; iCtrl < pMap->m_Controls.size(); iCtrl++)
        if (pMap->m_Controls[iCtrl].hWnd == hWnd)
            return static_cast<int>(true);

    // don´t add a child-window if it´s not an immediate child
    // of the parent window. Nested child-windows are moved
    // by their own parents. (The Report-style-ListView-control
    // is such an example)
    if (::GetParent(hWnd) != pMap->m_globalParent)
        return static_cast<int>(true);

    // Add the unassigned control to the list
    pMap->addObject(GetParent(hWnd), pMap->m_nDefaultFlags, GetDlgCtrlID(hWnd), false);

    return static_cast<int>(true);
}

bool ControlAnchorMap::calculateOriginalMargins(HINSTANCE parent, int dialogID, int controlID, OFFSETRECT& output)
{
    // create a temporary object to calculate
    HWND tempDialog = CreateDialog(parent, MAKEINTRESOURCE(dialogID), NULL, 0);
    if (!tempDialog)
        return false;

    HWND tempControl = GetDlgItem(tempDialog, controlID);
    if (tempControl)
        return false;

    bool bSuccess = calculateOriginalMargins(tempDialog, tempControl, output);
    DestroyWindow(tempDialog);

    return bSuccess;
}

bool ControlAnchorMap::calculateOriginalMargins(HWND originalParentWindow, HWND originalChildControl, OFFSETRECT& output)
{
    if (!originalParentWindow || !originalChildControl)
        return false;

    // gets both window and control rectangles. For the window we are interested
    // in the Client area rectangle. For control, in the full size of the "window" (control)
    RECT windowClientArea = { 0, 0, 0, 0 };
    RECT controlSize = { 0, 0, 0, 0 };
    GetClientRect(originalParentWindow, &windowClientArea);
    GetWindowRect(originalChildControl, &controlSize);

    output = ControlAnchorMap::calculateMargins(windowClientArea, controlSize);

    return true;
}

bool ControlAnchorMap::repositControl(HWND targetControl, HWND originalWindow, 
    HWND originalControl, int flags, const OFFSETRECT& additionalMargins)
{
    // Validations
    if (targetControl == NULL || originalWindow == NULL || originalControl == NULL || flags < 1)
        return false;

    // Now create information for object moving/resizing...
    TCtrlEntry myControl;
    myControl.hWndParent = GetParent(targetControl);
    myControl.hWnd = targetControl;
    myControl.nFlags = flags;

    ::GetWindowRect(originalWindow, &myControl.parentPrevWindowRect);
    ::GetClientRect(originalWindow, &myControl.parentClientRect);

    RECT rcCtrl;
    ::GetWindowRect(originalControl, &rcCtrl);
    screenToClientEx(originalWindow, &rcCtrl);

    myControl.rect.left = rcCtrl.left;
    myControl.rect.top = rcCtrl.top;
    myControl.rect.right = rcCtrl.right;
    myControl.rect.bottom = rcCtrl.bottom;

    copyOffsetRect(myControl.additionalMargins, additionalMargins);

    // Pass the object into the anchoring/docking pipeline...
    preProcess(myControl);
    moveObjectStatic(myControl, NULL);

    return true;
}

bool ControlAnchorMap::repositControl(HWND targetControl, HWND originalWindow,
    int originalControlID, int flags, const OFFSETRECT& additionalMargins)
{
    // Get the original control
    HWND originalControl = GetDlgItem(originalWindow, originalControlID);
    if (!originalControl)
        return false;

    return repositControl(targetControl, originalWindow, originalControl, flags, additionalMargins);
}

bool ControlAnchorMap::repositControl(HWND targetControl, HINSTANCE originalModule,
    int originalDialogID, int originalControlID, int flags, const OFFSETRECT& additionalMargins)
{
    // create a temporary object to calculate rectangles
    HWND windowParent = GetParent(targetControl);
    HWND tempDialog = CreateDialog(originalModule, MAKEINTRESOURCE(originalDialogID), windowParent, 0);
    if (!tempDialog)
        return false;

    bool bSuccess = repositControl(targetControl, tempDialog, originalControlID, flags, additionalMargins);
    DestroyWindow(tempDialog);

    return bSuccess;
}


// ============================== HELPERS ====================================

void ControlAnchorMap::moveObjectStatic(TCtrlEntry& pCtrl, HDWP hDeferPos)
{
    FSIZE szCtrl = { 0,0 };

    // Get the size of the control
    szCtrl.cx = pCtrl.rect.right - pCtrl.rect.left;
    szCtrl.cy = pCtrl.rect.bottom - pCtrl.rect.top;

    // we´ve nothing changed until now
    bool bChanged = false;

    // handle docking
    RECT& cliRc = pCtrl.parentClientRect;
    if ((pCtrl.nFlags & ANF_DOCK_ALL) == ANF_DOCK_ALL) {

        //setFRect(&pCtrl.rect, 0, 0, pCtrl.parentClientRect.right, pCtrl.parentClientRect.bottom);
        setFRect(&pCtrl.rect, 0, 0, cliRc.right, cliRc.bottom);
        bChanged = true;
    }
    if (pCtrl.nFlags & ANF_DOCK_TOP) {

        setFRect(&pCtrl.rect, 0, 0, (double)cliRc.right, szCtrl.cy);
        bChanged = true;

    }
    else if (pCtrl.nFlags & ANF_DOCK_BOTTOM) {

        setFRect(&pCtrl.rect, 0, cliRc.bottom - szCtrl.cy, cliRc.right, cliRc.bottom);
        bChanged = true;

    }
    else if (pCtrl.nFlags & ANF_DOCK_LEFT) {

        setFRect(&pCtrl.rect, 0, 0, szCtrl.cx, cliRc.bottom);
        bChanged = true;

    }
    else if (pCtrl.nFlags & ANF_DOCK_RIGHT) {

        setFRect(&pCtrl.rect, cliRc.right - szCtrl.cx, 0, cliRc.right, cliRc.bottom);
        bChanged = true;

    }
    else if (pCtrl.nFlags & ANF_DOCK_LEFT_EX) {

        setFRect(&pCtrl.rect, 0, pCtrl.rect.top, szCtrl.cx, pCtrl.rect.bottom);
        bChanged = true;

    }
    else if (pCtrl.nFlags & ANF_DOCK_RIGHT_EX) {

        setFRect(&pCtrl.rect, pCtrl.rect.left, pCtrl.rect.top, cliRc.right, pCtrl.rect.bottom);
        bChanged = true;

    }
    else if (pCtrl.nFlags & ANF_DOCK_TOP_EX) {

        setFRect(&pCtrl.rect, pCtrl.rect.left, 0, pCtrl.rect.right, pCtrl.rect.bottom);
        bChanged = true;

    }
    else if (pCtrl.nFlags & ANF_DOCK_BOTTOM_EX) {

        setFRect(&pCtrl.rect, pCtrl.rect.left, pCtrl.rect.top, pCtrl.rect.right, cliRc.bottom);
        bChanged = true;

    };

    // handle anchoring
    if ((pCtrl.uiSizedBorders & ANF_LEFTRIGHT) && (pCtrl.szDelta.cx != 0) && (!bChanged)) {

        switch (pCtrl.nFlags & ANF_LEFTRIGHT) {

        case ANF_LEFT: // nothing to do here, control moves automatically
                                // with the left-border of the window (client-rect)
            break;

        case ANF_RIGHT: pCtrl.rect.left += pCtrl.szDelta.cx;
            pCtrl.rect.right = (pCtrl.rect.left + szCtrl.cx);
            bChanged = true;
            break;

        case ANF_LEFTRIGHT: pCtrl.rect.right += pCtrl.szDelta.cx;
            bChanged = true;
            break;

        default: pCtrl.rect.left += ((double)pCtrl.szDelta.cx / 2.0);
            pCtrl.rect.right = (pCtrl.rect.left + szCtrl.cx);
            bChanged = true;
            break;

        };

    };

    if ((pCtrl.uiSizedBorders & ANF_TOPBOTTOM) && (pCtrl.szDelta.cy != 0)) {

        switch (pCtrl.nFlags & ANF_TOPBOTTOM) {

        case ANF_TOP: // nothing to do here, control moves automatically
                              // with the top of the window (client-rect);
            break;

        case ANF_BOTTOM: pCtrl.rect.top += pCtrl.szDelta.cy;
            pCtrl.rect.bottom = (pCtrl.rect.top + szCtrl.cy);
            bChanged = true;
            break;

        case ANF_TOPBOTTOM: pCtrl.rect.bottom += pCtrl.szDelta.cy;
            bChanged = true;
            break;

        default: pCtrl.rect.top += ((double)pCtrl.szDelta.cy / 2.0);
            pCtrl.rect.bottom = (pCtrl.rect.top + szCtrl.cy);
            bChanged = true;
            break;
        };

    };

    // now reposition the control, if its size/position has changed
    if (bChanged)
    {
        szCtrl.cx = pCtrl.rect.right - pCtrl.rect.left;
        szCtrl.cy = pCtrl.rect.bottom - pCtrl.rect.top;

        // Apply sizers. Here we ignore results size results since no global redraw control is being made.
        if (!pCtrl.controlSizer.empty())
            std::ignore = applyRectSizer(szCtrl, pCtrl.controlSizer);

        SetWindowPos(pCtrl.hWnd, pCtrl.hWndParent, (int)pCtrl.rect.left,
            (int)pCtrl.rect.top, (int)szCtrl.cx, (int)szCtrl.cy, SWP_NOZORDER | SWP_NOACTIVATE);
    };
}

// ======================================================================
// Helper function to return the nesting level of a window inside 
// a group of windowses (any Windows control is a window)
// ======================================================================
int findNestingLevel(const ControlAnchorMap::TCtrlEntry& control, const std::vector<ControlAnchorMap::TCtrlEntry> list, int currentLevel)
{
    // Look for a parent of the control inside the list
    for (const ControlAnchorMap::TCtrlEntry& e : list)
    {
        // If the control has a parent, find the grandparent... and so on...
        if (control.hWndParent == e.hWnd)
            return findNestingLevel(e, list, currentLevel + 1);
    }

    // When no parent found, return the current nesting level
    return currentLevel;
}

// ======================================================================
// Sorting rules for FullControlSort
// ======================================================================
bool ControlEntrySort(ControlAnchorMap::TCtrlEntry& a, ControlAnchorMap::TCtrlEntry& b)
{
    // Child windowses come first
    if (a.isChildWindow && !b.isChildWindow)
        return true;
    if (!a.isChildWindow && b.isChildWindow)
        return false;

    // Then for any child window, the nesting level comes first
    // from bigger to smaller ones (bigger has less nesting).
    if (a.isChildWindow && b.isChildWindow)
    {
        if (a.childNestLevel < b.childNestLevel)
            return true;
        if (a.childNestLevel > b.childNestLevel)
            return false;
    }

    // then for the rest of the controls and windows with equal nesting, 
    // sort by hWndParent.
    return a.hWndParent < b.hWndParent;
}

// ==================== PRIVATE METHODS (USING HELPERS) ======================


void ControlAnchorMap::fullControlsSort()
{
    // first we determine the nesting level of our objects
    for (TCtrlEntry& e : m_Controls)
    {
        if (e.isChildWindow)
        {
            e.childNestLevel = findNestingLevel(e, m_Controls, 0);
        }
    }

    // then we sort the list following ControlEntrySort rules
    sort(m_Controls.begin(), m_Controls.end(), ControlEntrySort);
}

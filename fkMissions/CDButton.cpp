typedef struct IUnknown IUnknown;

#include <afxwin.h>
#include "CDButton.h"

IMPLEMENT_DYNAMIC(CDButton, CButton)

CDButton::CDButton()
{
}

CDButton::~CDButton()
{
}


void CDButton::OnMouseMove(UINT nFlags, CPoint point)
{
    if (!m_IsMouseInside)
    {
        // Request mouse tracking
        auto tme = TRACKMOUSEEVENT{ sizeof(TRACKMOUSEEVENT), TME_LEAVE, m_hWnd, 0 };
        ::TrackMouseEvent(&tme);

        // Update state
        m_IsMouseInside = true;

        if (hintObject != NULL && hintText != NULL)
        {
            hintObject->SetWindowTextW(hintText);
        }
    }

    BaseClass::OnMouseMove(nFlags, point);
}

void CDButton::OnMouseLeave()
{
    // Update state
    m_IsMouseInside = false;

    if (hintObject != NULL && hintText != NULL)
    {
        hintObject->SetWindowTextW(NULL);
    }

    BaseClass::OnMouseLeave();
}


BEGIN_MESSAGE_MAP(CDButton, CButton)
    ON_WM_MOUSEMOVE()
    ON_WM_MOUSELEAVE()
END_MESSAGE_MAP()
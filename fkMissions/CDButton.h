#pragma once

using BaseClass = CButton;

class CDButton : public CButton
{
	DECLARE_DYNAMIC(CDButton)

public:
	CDButton();
	virtual ~CDButton();

	CWnd* hintObject;
	LPCTSTR hintText;

protected:
	DECLARE_MESSAGE_MAP()

private:
	void OnMouseMove(UINT nFlags, CPoint point);
	void OnMouseLeave();

	// State tracking
	bool m_IsMouseInside = false;
};



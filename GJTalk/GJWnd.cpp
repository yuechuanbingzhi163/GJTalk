#include "stdafx.h"
#include "GJWnd.h"

LPCTSTR CGJWnd::GetWindowClassName() const
{
	return _T("GJFrameHostWnd");
}
UINT CGJWnd::GetClassStyle() const
{
	return CS_DBLCLKS;
}
LRESULT CGJWnd::OnCreate(UINT uMsg,WPARAM wParam,LPARAM lParam,bool &bHandled)
{
	long styleValue=::GetWindowLong(*this,GWL_STYLE);
	styleValue&=~WS_CAPTION;
	::SetWindowLong(*this,GWL_STYLE,styleValue|WS_CLIPSIBLINGS|WS_CLIPCHILDREN);
	
	m_pm.Init(m_hWnd);
	CDialogBuilder builder; 	turn 0;
}


CGJWnd::CGJWnd(GJContext &context)
	:context(context)
{
}
GJContext &CGJWnd::GetContext()
{
	return context;
}

CGJWnd::~CGJWnd(void)
{
}

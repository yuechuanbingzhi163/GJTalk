
// GJTalk.cpp : 定义应用程序的类行为。
//

#include "stdafx.h"
#include "GJTalk.h" 
#include "LoginFrame.h"
#include "WebFrame.h"
#include "custsite.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif
#include "MessageFilterHelper.h"
#include "InputBox.h"

// CGJTalkApp

BEGIN_MESSAGE_MAP(CGJTalkApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
	ON_THREAD_MESSAGE(DM_CROSSTHREAD_NOTIFY,&CGJTalkApp::OnCrossThreadNotify)
END_MESSAGE_MAP()


// CGJTalkApp 构造

CGJTalkApp::CGJTalkApp() 
{

	// 支持重新启动管理器
	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;

	// TODO: 在此处添加构造代码，
	// 将所有重要的初始化放置在 InitInstance 中
}


// 唯一的一个 CGJTalkApp 对象

CGJTalkApp theApp;


// CGJTalkApp 初始化

BOOL CGJTalkApp::InitInstance()
{
	// 如果一个运行在 Windows XP 上的应用程序清单指定要
	// 使用 ComCtl32.dll 版本 6 或更高版本来启用可视化方式，
	//则需要 InitCommonControlsEx()。否则，将无法创建窗口。
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// 将它设置为包括所有要在应用程序中使用的
	// 公共控件类。
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();


	AfxEnableControlContainer();

	// 创建 shell 管理器，以防对话框包含
	// 任何 shell 树视图控件或 shell 列表视图控件。
	CShellManager *pShellManager = new CShellManager;

	// 标准初始化
	// 如果未使用这些功能并希望减小
	// 最终可执行文件的大小，则应移除下列
	// 不需要的特定初始化例程
	// 更改用于存储设置的注册表项
	// TODO: 应适当修改该字符串，
	// 例如修改为公司或组织名
	// SetRegistryKey(_T("应用程序向导生成的本地应用程序"));

	//CTrayIcon trayIcon;

	CPaintManagerUI::SetInstance(AfxGetInstanceHandle());
	CPaintManagerUI::SetResourcePath(_T("C:\\Users\\suppo_000\\Desktop\\GJTalkSkin\\GJTalk"));
	m_pContext=new GJContext();		

	m_pContext->init("localhost");

	HRESULT hr=::CoInitialize(NULL);

	CCustomOccManager *theManager=new CCustomOccManager();

	AfxEnableControlContainer(theManager);


	MessageFilterHelper *msgFilter=new MessageFilterHelper(); 
	CPaintManagerUI::AddGlobalMessageFilter(msgFilter);

	m_pContext->GetLoginFrame().CenterWindow();
	m_pContext->GetLoginFrame().ShowWindow();

	CPaintManagerUI::MessageLoop();

	//if(m_pContext)
	//	delete m_pContext;

	if (pShellManager != NULL)
	{
		delete pShellManager;
	}

	return FALSE;
}

void CGJTalkApp::OnCrossThreadNotify(WPARAM wParam,LPARAM lParam)
{
	if(!m_pContext)
		return;
	switch (wParam)
	{
	case CT_CONNECT:
		m_pContext->onConnect();
		break;
	case CT_DISCONNECT:
		m_pContext->onDisconnect((gloox::ConnectionError)lParam);
		break;
	case CT_RECEIVE_SELF_VCARD:
		m_pContext->onReceiveSelfVCard();
		break;
	case CT_HANDLE_SUBSCRIPTION:
		m_pContext->OnSubscriptionRequest(*((SubscriptionRequest*)lParam));
		delete (SubscriptionRequest*)lParam;
	default:
		break;
	}

}
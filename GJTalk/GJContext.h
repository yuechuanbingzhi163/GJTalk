#pragma once
#include "../xmpp/gloox.h"
#include "GJTalk.h"
#include "GJUser.h"
#include "MainFrame.h"
#include "LoginFrame.h"
#include <string>
#include "../xmpp/client.h"
#include "../xmpp/connectionlistener.h"
#include "../xmpp/loghandler.h" 
#include "../xmpp/tlshandler.h"
#include "../xmpp/messageeventhandler.h"
#include "../xmpp/messagehandler.h"
#include "../xmpp/messagesessionhandler.h"
#include "../xmpp/vcardmanager.h"
#include "../xmpp/vcardhandler.h"
#include "TrayIcon.h"
#include "UIBuddyList.h"
#include "SessionManager.h"
#include "HeaderManager.h"
using namespace std;
using namespace gloox;


namespace  MainFrameCloseReasons
{
	enum
	{
		SignOut,
		Exit,
		SwitchUser
	};
};

class GJContext:
	public ConnectionListener,
	public LogHandler,
	public MessageSessionHandler,
	public MessageHandler,
	public TLSHandler,
	public ITrayIconListener,
	public IqHandler,
	public VCardHandler

{
public:
	CTrayIcon* m_pTrayIcon;
	int MainFrameCloseReason;
	CMainFrame *m_pMainFrame;
	CLoginFrame *m_pLoginFrame;
	CSessionManager m_SessionManager;


private:

	JID *m_pSelf; 
	Client *m_pClient; 
	gloox::VCardManager *m_pVCardMgr;

private:

	bool m_bRecvData;

	bool m_bConnected;

	VCard m_vcSelf;
	string m_Server;
	int m_Port;  
	CWinThread *m_pRecvThread;
	CHeaderManager m_headerMgr;

public:  

	CBuddyListUI *GetBuddyList() const;
	CLoginFrame& GetLoginFrame();
	CMainFrame& GetMainFrame();

	CBuddyListItem *GetBuddyItemByJid(const JID& jid) const;

	CString &GetAppName() const;
	JID *GetSelf() const;
	const VCard *GetSelfVCard() const; 
	Client *GetClient() const; 
	VCardManager *GetVCardManager() const; 
	CSessionManager &GetSessionManager();
	CHeaderManager &GetHeaderManager();

	void HandleUnreadMessageChanged(); 

	bool IsReceiving() const;
	void StartRecv();
	void StopRecv();
	bool init(const string& sever,int port=-1);
	bool SignIn(const string& username,const string& password,CLoginFrame *loginFrame);
	bool IsSignedIn() const;

	virtual void handleMessage( const Message& msg, MessageSession* session = 0 );
	void OnWindowDestroyed(const CGJContextWnd* pWnd);

	void onConnect() ;
	void onDisconnect( ConnectionError e );
	bool onTLSConnect( const CertInfo& info );
	void onReceiveSelfVCard();

	void handleEncryptedData( const TLSBase* /*base*/, const std::string& data );
	void handleDecryptedData( const TLSBase* /*base*/, const std::string& data );
	void handleHandshakeResult( const TLSBase* /*base*/, bool success, CertInfo& /*certinfo*/ );
	void handleMessageSession( MessageSession *session );
	void handleLog( LogLevel level, LogArea area, const std::string& message );

	virtual bool handleIq( const IQ& iq );
	virtual void handleIqID( const IQ& iq, int context );
	virtual void  OnTrayIconMessage(CTrayIconMessage &msg);

	GJContext(void);
	~GJContext(void);

	virtual void handleVCard( const JID& jid, const VCard* vcard );

	virtual void handleVCardResult( VCardContext context, const JID& jid, StanzaError se = StanzaErrorUndefined );

};


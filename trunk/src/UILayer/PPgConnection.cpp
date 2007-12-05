//this file is part of eMule
//Copyright (C)2002-2006 Merkur ( strEmail.Format("%s@%s", "devteam", "emule-project.net") / http://www.emule-project.net )
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#include "stdafx.h"
#include <math.h>
#include "emule.h"
#include "PPgConnection.h"
#include "wizard.h"
#include "Scheduler.h"
#include "OtherFunctions.h"
#include "emuledlg.h"
#include "Preferences.h"
#include "Opcodes.h"
#include "StatisticsDlg.h"
#include "Kademlia/Kademlia/Kademlia.h"
#include "HelpIDs.h"
#include "Statistics.h"
#include "Firewallopener.h"
#include "ListenSocket.h"
#include "ClientUDPSocket.h"
#include "LastCommonRouteFinder.h"
#include "UserMsgs.h"
#include ".\ppgconnection.h"
#include "Util.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNAMIC(CPPgConnection, CPropertyPage)

BEGIN_MESSAGE_MAP(CPPgConnection, CPropertyPage)
	//ON_BN_CLICKED(IDC_STARTTEST, OnStartPortTest)
	//ON_BN_CLICKED(IDC_UDPDISABLE, OnEnChangeUDPDisable)
	ON_EN_CHANGE(IDC_UDPPORT, OnEnChangeUDP)
	ON_EN_CHANGE(IDC_PORT, OnEnChangeTCP)
	ON_EN_CHANGE(IDC_MAXCON, OnSettingsChange)
	ON_EN_CHANGE(IDC_MAXHALFCON, OnSettingsChange)
	ON_EN_CHANGE(IDC_MAXSOURCEPERFILE, OnSettingsChange)
	//ON_BN_CLICKED(IDC_AUTOCONNECT, OnSettingsChange)
	ON_BN_CLICKED(IDC_RECONN, OnSettingsChange)
	//ON_BN_CLICKED(IDC_WIZARD, OnBnClickedWizard)
	//ON_BN_CLICKED(IDC_NETWORK_ED2K, OnSettingsChange)
	ON_BN_CLICKED(IDC_SHOWOVERHEAD, OnSettingsChange)
	ON_BN_CLICKED(IDC_ULIMIT_LBL, OnLimiterChange)
	ON_BN_CLICKED(IDC_DLIMIT_LBL, OnLimiterChange)
	ON_WM_HSCROLL()
	//ON_BN_CLICKED(IDC_NETWORK_KADEMLIA, OnSettingsChange)
	ON_WM_HELPINFO()
	//ON_BN_CLICKED(IDC_OPENPORTS, OnBnClickedOpenports)
	ON_BN_CLICKED(IDC_RANDOM_PORT, OnBnClickedRandomPort)
	ON_NOTIFY(UDN_DELTAPOS, IDC_MAXSOURCEPERFILESPIN, OnChangeSpin)
	ON_NOTIFY(UDN_DELTAPOS, IDC_MAXCONSPIN, OnChangeSpin)
	ON_NOTIFY(UDN_DELTAPOS, IDC_RETRYSPIN, OnChangeSpin)
	ON_NOTIFY(UDN_DELTAPOS, IDC_DELAYSPIN, OnChangeSpin)
	ON_BN_CLICKED(IDC_UPNP, OnSettingsChange)
	ON_BN_CLICKED(IDC_UPNPRPORT, OnSettingsChange)
	ON_EN_CHANGE(IDC_DOWNLOAD_CAP, OnEnChangeDownloadCap)
	ON_EN_CHANGE(IDC_UPLOAD_CAP, OnEnChangeUploadCap)
	ON_CBN_SELCHANGE(IDC_CONNECTIONTYPE, OnCbnSelchangeConnectiontype)
END_MESSAGE_MAP()


#define DFLT_MAXHALFOPEN	9

CPPgConnection::CPPgConnection()
	: CPropertyPage(CPPgConnection::IDD)
{
	guardian = false;

	m_MaxSourcePerFileEdit.SetMaxWholeDigits(4);
	m_MaxConEdit.SetMaxWholeDigits(4);
	m_MaxHalfConEdit.SetMaxWholeDigits(3);

	m_isXP			= 0;	//added by thilon on 2006.08.07
	m_iTCPIPInit	= 0;	//added by thilon on 2006.08.07
	m_iMaxHalfOpen	= 0;    //added by thilon on 2007.10.10

}

CPPgConnection::~CPPgConnection()
{
}

void CPPgConnection::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_MAXDOWN_SLIDER, m_ctlMaxDown);
	DDX_Control(pDX, IDC_MAXUP_SLIDER, m_ctlMaxUp);

	// VC-kernel[2007-03-02]:
	DDX_Control(pDX, IDC_CONNECTIONTYPE, m_ctlConnectionType);
	DDX_Control(pDX, IDC_MAXHALFCON, m_MaxHalfConEdit);
	DDX_Control(pDX, IDC_MAXCON, m_MaxConEdit);
	DDX_Control(pDX, IDC_MAXSOURCEPERFILE, m_MaxSourcePerFileEdit);
	DDX_Control(pDX, IDC_DOWNLOAD_CAP, m_DownloadEdit);
	DDX_Control(pDX, IDC_UPLOAD_CAP, m_UploadEdit);
	DDX_Control(pDX, IDC_PORT, m_PortEdit);
	DDX_Control(pDX, IDC_UDPPORT, m_UDPPortEdit);
}

void CPPgConnection::OnEnChangeTCP()
{
	OnEnChangePorts(true);
}

void CPPgConnection::OnEnChangeUDP()
{
	OnEnChangePorts(false);
}

void CPPgConnection::OnEnChangePorts(uint8 istcpport)
{
	// ports unchanged?
	CString buffer;
	GetDlgItem(IDC_PORT)->GetWindowText(buffer);
	//uint16 tcp = (uint16)_tstoi(buffer);
	GetDlgItem(IDC_UDPPORT)->GetWindowText(buffer);
	//uint16 udp = (uint16)_tstoi(buffer);

	//GetDlgItem(IDC_STARTTEST)->EnableWindow( 
	//	tcp == CGlobalVariable::listensocket->GetConnectedPort() && 
	//	udp == CGlobalVariable::clientudp->GetConnectedPort() 
	//);

	if (istcpport == 0)
		OnEnChangeUDPDisable();
	else if (istcpport == 1)
		OnSettingsChange();
}

void CPPgConnection::OnEnChangeUDPDisable()
{
	if (guardian)
		return;

	uint16 tempVal = 0;
	CString strBuffer;
	TCHAR buffer[510];
	
	guardian = true;
	SetModified();

	GetDlgItem(IDC_UDPPORT)->EnableWindow(!IsDlgButtonChecked(IDC_UDPDISABLE));

	if (GetDlgItem(IDC_UDPPORT)->GetWindowTextLength())
	{
		GetDlgItem(IDC_UDPPORT)->GetWindowText(buffer, 20);
		tempVal = (uint16)_tstoi(buffer);
	}
	
	if (IsDlgButtonChecked(IDC_UDPDISABLE) || (!IsDlgButtonChecked(IDC_UDPDISABLE) && tempVal == 0))
	{
		tempVal = (uint16)_tstoi(buffer) ? (uint16)(_tstoi(buffer)+10) : (uint16)(thePrefs.port+10);
		if (IsDlgButtonChecked(IDC_UDPDISABLE))
			tempVal = 0;
		strBuffer.Format(_T("%d"), tempVal);
		GetDlgItem(IDC_UDPPORT)->SetWindowText(strBuffer);
	}
	
	guardian = false;
}

BOOL CPPgConnection::OnInitDialog()
{
	CPropertyPage::OnInitDialog();
	InitWindowStyles(this);

	// VC-kernel[2007-02-26]:
	UDACCEL struAccel;
	struAccel.nInc = 10;
	struAccel.nSec = 0;
	((CSpinButtonCtrl*)GetDlgItem(IDC_MAXCONSPIN))->SetBuddy(GetDlgItem(IDC_MAXCON));
	((CSpinButtonCtrl*)GetDlgItem(IDC_MAXCONSPIN))->SetAccel(1,&struAccel);
	((CSpinButtonCtrl*)GetDlgItem(IDC_MAXSOURCEPERFILESPIN))->SetBuddy(GetDlgItem(IDC_MAXSOURCEPERFILE));
	((CSpinButtonCtrl*)GetDlgItem(IDC_MAXSOURCEPERFILESPIN))->SetAccel(1,&struAccel);
	((CSpinButtonCtrl*)GetDlgItem(IDC_MAXHALFCONSPIN))->SetBuddy(GetDlgItem(IDC_MAXHALFCON));
	((CSpinButtonCtrl*)GetDlgItem(IDC_MAXHALFCONSPIN))->SetAccel(1,&struAccel);
	((CSpinButtonCtrl*)GetDlgItem(IDC_RETRYSPIN))->SetBuddy(GetDlgItem(IDC_RETRY));
	((CSpinButtonCtrl*)GetDlgItem(IDC_DELAYSPIN))->SetBuddy(GetDlgItem(IDC_DELAY));

	// VC-kernel[2007-03-02]:
	m_ctlConnectionType.AddString(GetResString(IDS_WIZARD_UNKNOWN));
	m_ctlConnectionType.AddString(GetResString(IDS_WIZARD_CUSTOM));
	m_ctlConnectionType.AddString(_T("ADSL 512K"));
	m_ctlConnectionType.AddString(_T("ADSL 1MB"));
	m_ctlConnectionType.AddString(_T("ADSL 2MB"));
	m_ctlConnectionType.AddString(_T("ADSL 8MB"));
	m_ctlConnectionType.AddString(_T("LAN"));

	m_ttc.Create(this);
	m_ttc.AddTool(GetDlgItem(IDC_UPNP), GetResString(IDS_TIP_UPNP));
	m_ttc.AddTool(GetDlgItem(IDC_UPNPRPORT), GetResString(IDS_TIP_UPNPRAND));
	m_ttc.AddTool(GetDlgItem(IDC_MAXSRCHARD_LBL), GetResString(IDS_TIP_HARDCONN));
	m_ttc.AddTool(GetDlgItem(IDC_MAXSOURCEPERFILE), GetResString(IDS_TIP_HARDCONN));
	m_ttc.AddTool(GetDlgItem(IDC_MAXCONLABEL), GetResString(IDS_TIP_MAXCONN));
	m_ttc.AddTool(GetDlgItem(IDC_MAXCON), GetResString(IDS_TIP_MAXCONN));
	m_ttc.AddTool(GetDlgItem(IDC_MAXHALFCONLABEL), GetResString(IDS_TIP_HALFCONN));
	m_ttc.AddTool(GetDlgItem(IDC_MAXHALFCON), GetResString(IDS_TIP_HALFCONN));
	m_ttc.AddTool(GetDlgItem(IDC_CONNECTIONTYPE), GetResString(IDS_TIP_CONN_TYPE));

	LoadSettings();
	Localize();

	OnEnChangePorts(2);


	return TRUE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX Property Pages should return FALSE
}

void CPPgConnection::LoadSettings(void)
{
	if (m_hWnd)
	{
		if (thePrefs.maxupload != 0)
			thePrefs.maxdownload = thePrefs.GetMaxDownload();

		CString strBuffer;
		
		strBuffer.Format(_T("%d"), thePrefs.udpport);
		GetDlgItem(IDC_UDPPORT)->SetWindowText(strBuffer);
		CheckDlgButton(IDC_UDPDISABLE, (thePrefs.udpport == 0));

		GetDlgItem(IDC_UDPPORT)->EnableWindow(thePrefs.udpport > 0);
	
		strBuffer.Format(_T("%d"), thePrefs.maxGraphDownloadRate);
		GetDlgItem(IDC_DOWNLOAD_CAP)->SetWindowText(strBuffer);

		m_ctlMaxDown.SetRange(1, thePrefs.maxGraphDownloadRate);
		SetRateSliderTicks(m_ctlMaxDown);

		if (thePrefs.maxGraphUploadRate != UNLIMITED)
			strBuffer.Format(_T("%d"), thePrefs.maxGraphUploadRate);
		else
			strBuffer = _T("128");
		GetDlgItem(IDC_UPLOAD_CAP)->SetWindowText(strBuffer);

		m_ctlMaxUp.SetRange(1, thePrefs.GetMaxGraphUploadRate(true));
		SetRateSliderTicks(m_ctlMaxUp);

		// VC-kernel[2007-03-02]:
		//if(thePrefs.maxGraphUploadRate == 128 && thePrefs.maxGraphDownloadRate == 256 && !IsDlgButtonChecked(IDC_ULIMIT_LBL) && !IsDlgButtonChecked(IDC_DLIMIT_LBL))
		//{
		//	m_ctlConnectionType.SetCurSel(0);
		//	GetDlgItem(IDC_UPLOAD_CAP)->EnableWindow(False);
		//	GetDlgItem(IDC_DOWNLOAD_CAP)->EnableWindow(False);
		//}
		//else if(thePrefs.maxGraphUploadRate == 64 && thePrefs.maxGraphDownloadRate == 64)//512
		//{
		//	m_ctlConnectionType.SetCurSel(2);
		//	GetDlgItem(IDC_UPLOAD_CAP)->EnableWindow(False);
		//	GetDlgItem(IDC_DOWNLOAD_CAP)->EnableWindow(False);
		//}
		//else if(thePrefs.maxGraphUploadRate == 64 && thePrefs.maxGraphDownloadRate == 128)//1M
		//{
		//	m_ctlConnectionType.SetCurSel(3);
		//	GetDlgItem(IDC_UPLOAD_CAP)->EnableWindow(False);
		//	GetDlgItem(IDC_DOWNLOAD_CAP)->EnableWindow(False);
		//}
		//else if(thePrefs.maxGraphUploadRate == 64 && thePrefs.maxGraphDownloadRate == 256)//2M
		//{
		//		m_ctlConnectionType.SetCurSel(4);
		//		GetDlgItem(IDC_UPLOAD_CAP)->EnableWindow(False);
		//		GetDlgItem(IDC_DOWNLOAD_CAP)->EnableWindow(False);
		//}
		//else if(thePrefs.maxGraphUploadRate == 64 && thePrefs.maxGraphDownloadRate == 1024)//8M
		//{
		//	m_ctlConnectionType.SetCurSel(5);
		//	GetDlgItem(IDC_UPLOAD_CAP)->EnableWindow(False);
		//	GetDlgItem(IDC_DOWNLOAD_CAP)->EnableWindow(False);
		//}
		//else if(thePrefs.maxGraphUploadRate == 250 && thePrefs.maxGraphDownloadRate == 250 && !IsDlgButtonChecked(IDC_ULIMIT_LBL) && !IsDlgButtonChecked(IDC_DLIMIT_LBL))//LAN
		//{
		//	m_ctlConnectionType.SetCurSel(6);
		//	GetDlgItem(IDC_UPLOAD_CAP)->EnableWindow(False);
		//	GetDlgItem(IDC_DOWNLOAD_CAP)->EnableWindow(False);
		//}
		//else
		//{
		//	m_ctlConnectionType.SetCurSel(1);
		//	GetDlgItem(IDC_UPLOAD_CAP)->EnableWindow();
		//	GetDlgItem(IDC_DOWNLOAD_CAP)->EnableWindow();
		//}
		switch(thePrefs.GetConnectionType())
		{
		case 0:
			m_ctlConnectionType.SetCurSel(0);
			GetDlgItem(IDC_UPLOAD_CAP)->EnableWindow(False);
			GetDlgItem(IDC_DOWNLOAD_CAP)->EnableWindow(False);
			break;
		case 1:
			m_ctlConnectionType.SetCurSel(1);
			GetDlgItem(IDC_UPLOAD_CAP)->EnableWindow();
			GetDlgItem(IDC_DOWNLOAD_CAP)->EnableWindow();
			break;
		case 2:
			m_ctlConnectionType.SetCurSel(2);
			GetDlgItem(IDC_UPLOAD_CAP)->EnableWindow(False);
			GetDlgItem(IDC_DOWNLOAD_CAP)->EnableWindow(False);
			break;
		case 3:
			m_ctlConnectionType.SetCurSel(3);
			GetDlgItem(IDC_UPLOAD_CAP)->EnableWindow(False);
			GetDlgItem(IDC_DOWNLOAD_CAP)->EnableWindow(False);
			break;
		case 4:
			m_ctlConnectionType.SetCurSel(4);
			GetDlgItem(IDC_UPLOAD_CAP)->EnableWindow(False);
			GetDlgItem(IDC_DOWNLOAD_CAP)->EnableWindow(False);
			break;
		case 5:
			m_ctlConnectionType.SetCurSel(5);
			GetDlgItem(IDC_UPLOAD_CAP)->EnableWindow(False);
			GetDlgItem(IDC_DOWNLOAD_CAP)->EnableWindow(False);
			break;
		case 6:
			m_ctlConnectionType.SetCurSel(6);
			GetDlgItem(IDC_UPLOAD_CAP)->EnableWindow(False);
			GetDlgItem(IDC_DOWNLOAD_CAP)->EnableWindow(False);
			break;
		default:
			break;
		}

		CheckDlgButton( IDC_DLIMIT_LBL, (thePrefs.maxdownload != UNLIMITED));
		CheckDlgButton( IDC_ULIMIT_LBL, (thePrefs.maxupload != UNLIMITED));

		m_ctlMaxDown.SetPos((thePrefs.maxdownload != UNLIMITED) ? thePrefs.maxdownload : thePrefs.maxGraphDownloadRate);
		m_ctlMaxUp.SetPos((thePrefs.maxupload != UNLIMITED) ? thePrefs.maxupload : thePrefs.GetMaxGraphUploadRate(true));

		strBuffer.Format(_T("%d"), thePrefs.port);
		GetDlgItem(IDC_PORT)->SetWindowText(strBuffer);


		/*说明：Tweaks对话框初始化时，检测系统是不是XPSP2，同时检测tcpip.sys的大小和版本，来确定是否修改
		当条件满足时，eMule的最大半连接数，可以修改，并且该数字就为tcpip的最大连接数
		*/
		m_isXP = m_betterSP2.DetectSystemInformation();   //added by thilon on 2006.04.20 检测系统信息

		if(m_isXP)
		{
			m_iTCPIPInit = CGlobalVariable::GetTCPIPVaule();		//added by thilon on 2006.04.20 条件满足，直接赋值，这里通过启动eMule时，获得tcpip.sys文件里的连接数
			m_iMaxHalfOpen = m_iTCPIPInit;				//将获得的连接数直接赋值给最大半开连接数
		}
		else
		{
			m_iMaxHalfOpen = thePrefs.GetMaxHalfConnections();
		}


		// VC-kernel[2007-02-28]:
		((CSpinButtonCtrl*)GetDlgItem(IDC_MAXCONSPIN))->SetRange(10,2000);
		((CSpinButtonCtrl*)GetDlgItem(IDC_MAXSOURCEPERFILESPIN))->SetRange(10,2000);
		((CSpinButtonCtrl*)GetDlgItem(IDC_MAXHALFCONSPIN))->SetRange(0,300);

		strBuffer.Format(_T("%d"), thePrefs.maxconnections);
		GetDlgItem(IDC_MAXCON)->SetWindowText(strBuffer);

		((CSpinButtonCtrl*)GetDlgItem(IDC_MAXCONSPIN))->SetPos(thePrefs.maxconnections);

		if (thePrefs.maxsourceperfile == 0xFFFF)
		{
			//	GetDlgItem(IDC_MAXSOURCEPERFILE)->SetWindowText(_T("0"));
			((CSpinButtonCtrl*)GetDlgItem(IDC_MAXSOURCEPERFILESPIN))->SetPos(0);
		}
		else
		{
		//	strBuffer.Format(_T("%d"), thePrefs.maxsourceperfile);
		//	GetDlgItem(IDC_MAXSOURCEPERFILE)->SetWindowText(strBuffer);
			((CSpinButtonCtrl*)GetDlgItem(IDC_MAXSOURCEPERFILESPIN))->SetPos(thePrefs.maxsourceperfile);
		}

		((CSpinButtonCtrl*)GetDlgItem(IDC_MAXHALFCONSPIN))->SetPos(m_iMaxHalfOpen);

		//if (thePrefs.reconnect)
		//	CheckDlgButton(IDC_RECONN, 1);
		//else
		//	CheckDlgButton(IDC_RECONN, 0);
		
		if (thePrefs.m_bshowoverhead)
			CheckDlgButton(IDC_SHOWOVERHEAD, 1);
		else
			CheckDlgButton(IDC_SHOWOVERHEAD, 0);

		//if (thePrefs.autoconnect)
		//	CheckDlgButton(IDC_AUTOCONNECT, 1);
		//else
		//	CheckDlgButton(IDC_AUTOCONNECT, 0);

		//if (thePrefs.networkkademlia)
		//	CheckDlgButton(IDC_NETWORK_KADEMLIA, 1);
		//else
		//	CheckDlgButton(IDC_NETWORK_KADEMLIA, 0);

		//if (thePrefs.networked2k)
		//	CheckDlgButton(IDC_NETWORK_ED2K, 1);
		//else
		//	CheckDlgButton(IDC_NETWORK_ED2K, 0);

		// don't try on XP SP2 or higher, not needed there anymore
		//if (IsRunningXPSP2() == 0 && theApp.m_pFirewallOpener->DoesFWConnectionExist())
		//	GetDlgItem(IDC_OPENPORTS)->EnableWindow(true);
		//else
		//	GetDlgItem(IDC_OPENPORTS)->EnableWindow(false);

		// VC-kernel[2007-02-27]:upnp
		m_iUPnPNat = thePrefs.GetUPnPNat();
		if(m_iUPnPNat)
			CheckDlgButton(IDC_UPNP,1);
		else
			CheckDlgButton(IDC_UPNP,0);
		
		m_iUPnPTryRandom = thePrefs.GetUPnPNatTryRandom();
		if(m_iUPnPTryRandom)
			CheckDlgButton(IDC_UPNPRPORT,1);
		else
			CheckDlgButton(IDC_UPNPRPORT,0);

		ShowLimitValues();
		OnLimiterChange();

		// VC-kernel[2007-02-28]:
		((CSpinButtonCtrl*)GetDlgItem(IDC_RETRYSPIN))->SetRange(10,100);
		((CSpinButtonCtrl*)GetDlgItem(IDC_DELAYSPIN))->SetRange(10,100);
	}
}

BOOL CPPgConnection::OnApply()
{
	TCHAR buffer[510];
	int lastmaxgu = thePrefs.maxGraphUploadRate;
	int lastmaxgd = thePrefs.maxGraphDownloadRate;
	bool bRestartApp = false;

	if (GetDlgItem(IDC_DOWNLOAD_CAP)->GetWindowTextLength())
	{ 
		GetDlgItem(IDC_DOWNLOAD_CAP)->GetWindowText(buffer, 20);
		thePrefs.SetMaxGraphDownloadRate(_tstoi(buffer));
	}

	m_ctlMaxDown.SetRange(1, thePrefs.GetMaxGraphDownloadRate(), TRUE);
	SetRateSliderTicks(m_ctlMaxDown);

	if (GetDlgItem(IDC_UPLOAD_CAP)->GetWindowTextLength())
	{
		GetDlgItem(IDC_UPLOAD_CAP)->GetWindowText(buffer, 20);
		thePrefs.SetMaxGraphUploadRate(_tstoi(buffer));
	}

	m_ctlMaxUp.SetRange(1, thePrefs.GetMaxGraphUploadRate(true), TRUE);
	SetRateSliderTicks(m_ctlMaxUp);

    {
        uint16 ulSpeed;

	    if (!IsDlgButtonChecked(IDC_ULIMIT_LBL))
		    ulSpeed = UNLIMITED;
	    else
		    ulSpeed = (uint16)m_ctlMaxUp.GetPos();

	    if (thePrefs.GetMaxGraphUploadRate(true) < ulSpeed && ulSpeed != UNLIMITED)
		    ulSpeed = (uint16)(thePrefs.GetMaxGraphUploadRate(true) * 0.8);

        if(ulSpeed > thePrefs.GetMaxUpload()) {
            // make USS go up to higher ul limit faster
			CGlobalVariable::lastCommonRouteFinder->InitiateFastReactionPeriod();
        }

        thePrefs.SetMaxUpload(ulSpeed);
    }

	if (thePrefs.GetMaxUpload() != UNLIMITED)
		m_ctlMaxUp.SetPos(thePrefs.GetMaxUpload());
	
	if (!IsDlgButtonChecked(IDC_DLIMIT_LBL))
		thePrefs.SetMaxDownload(UNLIMITED);
	else
		thePrefs.SetMaxDownload(m_ctlMaxDown.GetPos());

	if (thePrefs.GetMaxGraphDownloadRate() < thePrefs.GetMaxDownload() && thePrefs.GetMaxDownload() != UNLIMITED)
		thePrefs.SetMaxDownload((uint16)(thePrefs.GetMaxGraphDownloadRate() * 0.8));

	if (thePrefs.GetMaxDownload() != UNLIMITED)
		m_ctlMaxDown.SetPos(thePrefs.GetMaxDownload());

	BOOL	bRedoUpnp = FALSE; //ADDED by fengwen on 2007/01/10 : redo upnp when ports have been changed.
	

	if (GetDlgItem(IDC_PORT)->GetWindowTextLength())
	{
		GetDlgItem(IDC_PORT)->GetWindowText(buffer, 20);
		uint16 nNewPort = ((uint16)_tstoi(buffer)) ? (uint16)_tstoi(buffer) : (uint16)thePrefs.port;
		if (nNewPort != thePrefs.port){
			thePrefs.port = nNewPort;
			if (theApp.IsPortchangeAllowed())
			{
				CGlobalVariable::listensocket->Rebind();
				bRedoUpnp = TRUE;	//ADDED by fengwen on 2007/01/10 : redo upnp when ports have been changed.
			}
			else
				bRestartApp = true;

		}
	}
	
	if (GetDlgItem(IDC_MAXSOURCEPERFILE)->GetWindowTextLength())
	{
		GetDlgItem(IDC_MAXSOURCEPERFILE)->GetWindowText(buffer, 20);
		thePrefs.maxsourceperfile = (_tstoi(buffer)) ? _tstoi(buffer) : 1;
	}

	if (GetDlgItem(IDC_UDPPORT)->GetWindowTextLength())
	{
		GetDlgItem(IDC_UDPPORT)->GetWindowText(buffer, 20);
		uint16 nNewPort = ((uint16)_tstoi(buffer) && !IsDlgButtonChecked(IDC_UDPDISABLE)) ? (uint16)_tstoi(buffer) : (uint16)0;
		if (nNewPort != thePrefs.udpport){
			thePrefs.udpport = nNewPort;
			if (theApp.IsPortchangeAllowed())
			{
				CGlobalVariable::clientudp->Rebind();
				bRedoUpnp = TRUE;	//ADDED by fengwen on 2007/01/10 : redo upnp when ports have been changed.
			}
			else 
				bRestartApp = true;

		}
	}

	//ADDED by fengwen on 2007/01/10 <begin> : redo upnp when ports have been changed.
	if (bRedoUpnp)
	{
		if (NULL != theApp.emuledlg)
			theApp.emuledlg->PostMessage(UM_PORT_CHANGED);
	}
	//ADDED by fengwen on 2007/01/10 <end> : redo upnp when ports have been changed.


	if (IsDlgButtonChecked(IDC_SHOWOVERHEAD)){
		if (!thePrefs.m_bshowoverhead){
			// reset overhead data counters before starting to meassure!
			theStats.ResetDownDatarateOverhead();
			theStats.ResetUpDatarateOverhead();
		}
		thePrefs.m_bshowoverhead = true;
	}
	else{
		if (thePrefs.m_bshowoverhead){
			// free memory used by overhead computations
			theStats.ResetDownDatarateOverhead();
			theStats.ResetUpDatarateOverhead();
		}
		thePrefs.m_bshowoverhead = false;
	}

	if (IsDlgButtonChecked(IDC_NETWORK_KADEMLIA))
		thePrefs.SetNetworkKademlia(true);
	else
		thePrefs.SetNetworkKademlia(false);

	if (IsDlgButtonChecked(IDC_NETWORK_ED2K))
		thePrefs.SetNetworkED2K(true);
	else
		thePrefs.SetNetworkED2K(false);

	GetDlgItem(IDC_UDPPORT)->EnableWindow(!IsDlgButtonChecked(IDC_UDPDISABLE));

	//thePrefs.autoconnect = IsDlgButtonChecked(IDC_AUTOCONNECT)!=0;// VC-kernel[2007-02-28]:
	//thePrefs.reconnect = IsDlgButtonChecked(IDC_RECONN)!=0;// VC-kernel[2007-02-28]:
	
		
	if (lastmaxgu != thePrefs.maxGraphUploadRate) 
		theApp.emuledlg->statisticswnd->SetARange(false, thePrefs.GetMaxGraphUploadRate(true));
	if (lastmaxgd!=thePrefs.maxGraphDownloadRate)
		theApp.emuledlg->statisticswnd->SetARange(true, thePrefs.maxGraphDownloadRate);

	UINT tempcon = thePrefs.maxconnections;
	if (GetDlgItem(IDC_MAXCON)->GetWindowTextLength())
	{
		GetDlgItem(IDC_MAXCON)->GetWindowText(buffer, 20);
		tempcon = (_tstoi(buffer)) ? _tstoi(buffer) : CPreferences::GetRecommendedMaxConnections();
	}

	if (tempcon > (unsigned)::GetMaxWindowsTCPConnections())
	{
		CString strMessage;
		strMessage.Format(GetResString(IDS_PW_WARNING), GetResString(IDS_PW_MAXC), ::GetMaxWindowsTCPConnections());
		int iResult = AfxMessageBox(strMessage, MB_ICONWARNING | MB_YESNO);
		if (iResult != IDYES)
		{
			//TODO: set focus to max connection?
			strMessage.Format(_T("%d"), thePrefs.maxconnections);
			GetDlgItem(IDC_MAXCON)->SetWindowText(strMessage);
			tempcon = ::GetMaxWindowsTCPConnections();
		}
	}
	thePrefs.maxconnections = tempcon;
	theApp.scheduler->SaveOriginals();

	// VC-kernel[2007-02-26]:upnp
	m_iUPnPNat = BOOL2bool(IsDlgButtonChecked(IDC_UPNP));
	m_iUPnPTryRandom = BOOL2bool(IsDlgButtonChecked(IDC_UPNPRPORT));
	thePrefs.SetUPnPNat( m_iUPnPNat );
	thePrefs.SetUPnPNatTryRandom( m_iUPnPTryRandom );

	if (GetDlgItem(IDC_MAXHALFCON)->GetWindowTextLength())
	{
		GetDlgItem(IDC_MAXHALFCON)->GetWindowText(buffer, 20);
		tempcon = (_tstoi(buffer)) ? _tstoi(buffer) : DFLT_MAXHALFOPEN;
		m_iMaxHalfOpen = tempcon;

		thePrefs.SetMaxHalfConnections(m_iMaxHalfOpen ? m_iMaxHalfOpen : DFLT_MAXHALFOPEN);

		// 添加修改XP连接数 added by thilon at 2006.4.20
		if(m_isXP)
		{
			if(m_iTCPIPInit != m_iMaxHalfOpen && m_iTCPIPInit > 0 && m_iMaxHalfOpen > 0)
			{
				if(m_betterSP2.ChangeTCPIPValue(m_iMaxHalfOpen))
				{
					::MessageBox(this->m_hWnd, GetResString(IDS_TCPIPCHANGED), GetResString(IDS_CAPTION), MB_OK);
					m_iTCPIPInit = m_iMaxHalfOpen;
				}
			}
		}
	}

	SetModified(FALSE);
	LoadSettings();

	theApp.emuledlg->ShowConnectionState();

	if (bRestartApp)
		AfxMessageBox(GetResString(IDS_NOPORTCHANGEPOSSIBLE));

	OnEnChangePorts(2);

	return CPropertyPage::OnApply();
}

void CPPgConnection::Localize(void)
{	
	if (m_hWnd)
	{
		SetWindowText(GetResString(IDS_PW_CONNECTION));
		GetDlgItem(IDC_CAPACITIES_FRM)->SetWindowText(GetResString(IDS_WIZ_CTFRAME));
		GetDlgItem(IDC_DCAP_LBL)->SetWindowText(GetResString(IDS_PW_CON_DOWNLBL));
		GetDlgItem(IDC_UCAP_LBL)->SetWindowText(GetResString(IDS_PW_CON_UPLBL));
		GetDlgItem(IDC_LIMITS_FRM)->SetWindowText(GetResString(IDS_PW_CON_LIMITFRM));
		GetDlgItem(IDC_DLIMIT_LBL)->SetWindowText(GetResString(IDS_PW_DOWNL));
		GetDlgItem(IDC_ULIMIT_LBL)->SetWindowText(GetResString(IDS_PW_UPL));
		//GetDlgItem(IDC_CONNECTION_NETWORK)->SetWindowText(GetResString(IDS_NETWORK));
		//GetDlgItem(IDC_KBS2)->SetWindowText(GetResString(IDS_KBYTESPERSEC));
		//GetDlgItem(IDC_KBS3)->SetWindowText(GetResString(IDS_KBYTESPERSEC));
		ShowLimitValues();
		//GetDlgItem(IDC_MAXCONN_FRM)->SetWindowText(GetResString(IDS_PW_CONLIMITS));
		GetDlgItem(IDC_MAXCONLABEL)->SetWindowText(GetResString(IDS_PW_MAXC));
		//GetDlgItem(IDC_SHOWOVERHEAD)->SetWindowText(GetResString(IDS_SHOWOVERHEAD));
		GetDlgItem(IDC_CLIENTPORT_FRM)->SetWindowText(GetResString(IDS_PW_CLIENTPORT));
		//GetDlgItem(IDC_MAXSRC_FRM)->SetWindowText(GetResString(IDS_PW_MAXSOURCES));
		//GetDlgItem(IDC_AUTOCONNECT)->SetWindowText(GetResString(IDS_PW_AUTOCON));
		//GetDlgItem(IDC_RECONN)->SetWindowText(GetResString(IDS_PW_RECON));
		GetDlgItem(IDC_MAXSRCHARD_LBL)->SetWindowText(GetResString(IDS_HARDLIMIT));
		//GetDlgItem(IDC_WIZARD)->SetWindowText(GetResString(IDS_WIZARD));
		//GetDlgItem(IDC_UDPDISABLE)->SetWindowText(GetResString(IDS_UDPDISABLED));
		//GetDlgItem(IDC_OPENPORTS)->SetWindowText(GetResString(IDS_FO_PREFBUTTON));
		//SetDlgItemText(IDC_STARTTEST, GetResString(IDS_STARTTEST) );
		//VeryCD Start
		//连接选项中的随机端口按钮，added by Chocobo on 2006.08.16
		SetDlgItemText(IDC_RANDOM_PORT, GetResString(IDS_RANDOMPORT));
		//VeryCD End
		SetDlgItemText(IDC_MAXHALFCONLABEL, GetResString(IDS_MAXHALFCONLABEL));

		SetDlgItemText(IDC_UPNP, GetResString(IDS_UPNP) );
		SetDlgItemText(IDC_UPNPRPORT, GetResString(IDS_UPNPRPORT) );
		GetDlgItem(IDC_MISC)->SetWindowText(GetResString(IDS_PW_MISC));// VC-kernel[2007-03-13]:

		m_ttc.UpdateTipText(GetResString(IDS_TIP_UPNP), GetDlgItem(IDC_UPNP));
		m_ttc.UpdateTipText(GetResString(IDS_TIP_UPNPRAND), GetDlgItem(IDC_UPNPRPORT));
		m_ttc.UpdateTipText(GetResString(IDS_TIP_HARDCONN), GetDlgItem(IDC_MAXSRCHARD_LBL));
		m_ttc.UpdateTipText(GetResString(IDS_TIP_HARDCONN), GetDlgItem(IDC_MAXSOURCEPERFILE));
		m_ttc.UpdateTipText(GetResString(IDS_TIP_MAXCONN), GetDlgItem(IDC_MAXCONLABEL));
		m_ttc.UpdateTipText(GetResString(IDS_TIP_MAXCONN), GetDlgItem(IDC_MAXCON));
		m_ttc.UpdateTipText(GetResString(IDS_TIP_HALFCONN), GetDlgItem(IDC_MAXHALFCONLABEL));
		m_ttc.UpdateTipText(GetResString(IDS_TIP_HALFCONN), GetDlgItem(IDC_MAXHALFCON));
		m_ttc.UpdateTipText(GetResString(IDS_TIP_CONN_TYPE), GetDlgItem(IDC_CONNECTIONTYPE));

	}
}

void CPPgConnection::OnBnClickedWizard()
{
	//COMMENTED by VC-fengwen on 2007/07/17 <begin>	:	CConnectionWizardDlg已变成 CPropertyPage
	//CConnectionWizardDlg conWizard;
	//conWizard.DoModal();
	//COMMENTED by VC-fengwen on 2007/07/17 <end>	:	CConnectionWizardDlg已变成 CPropertyPage
}

void CPPgConnection::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	SetModified(TRUE);

	if (pScrollBar->GetSafeHwnd() == m_ctlMaxUp.m_hWnd)
	{
		uint32 maxup = m_ctlMaxUp.GetPos();
		uint32 maxdown = m_ctlMaxDown.GetPos();
		if (maxup < 4 && maxup*3 < maxdown)
		{
			m_ctlMaxDown.SetPos(maxup*3);
		}
		if (maxup < 30 && maxup*4 < maxdown) // VC-Huby[2007-02-10]: chang to 30KB upload limit
		{
			m_ctlMaxDown.SetPos(maxup*4);
		}
	}
	else if (pScrollBar->GetSafeHwnd() == m_ctlMaxDown.m_hWnd) 
	{

	
		uint32 maxup = m_ctlMaxUp.GetPos();
		uint32 maxdown = m_ctlMaxDown.GetPos();
		if (maxdown < 13 && maxup*3 < maxdown)
		{
			m_ctlMaxUp.SetPos((int)ceil((double)maxdown/3));
		}
		if (maxdown < 41 && maxup*4 < maxdown)
		{
			m_ctlMaxUp.SetPos((int)ceil((double)maxdown/4));
		}
		else if(maxdown>=41 && maxup<30 && maxup*4< maxdown) // VC-Huby[2007-02-10]: want more download,u should more upload to others
		{
			m_ctlMaxUp.SetPos((int)ceil((double)maxdown/4));
		}
	}

	ShowLimitValues();

	UpdateData(false); 
	CPropertyPage::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CPPgConnection::ShowLimitValues()
{
	CString buffer;

	if (!IsDlgButtonChecked(IDC_ULIMIT_LBL))
		buffer = _T("");
	else
		buffer.Format(_T("%u %s"), m_ctlMaxUp.GetPos(), GetResString(IDS_KBYTESPERSEC));
	GetDlgItem(IDC_KBS4)->SetWindowText(buffer);
	
	if (!IsDlgButtonChecked(IDC_DLIMIT_LBL))
		buffer = _T("");
	else
		buffer.Format(_T("%u %s"), m_ctlMaxDown.GetPos(), GetResString(IDS_KBYTESPERSEC));
	GetDlgItem(IDC_KBS1)->SetWindowText(buffer);
}

void CPPgConnection::OnLimiterChange()
{
	m_ctlMaxDown.ShowWindow(IsDlgButtonChecked(IDC_DLIMIT_LBL) ? SW_SHOW : SW_HIDE);
	m_ctlMaxUp.ShowWindow(IsDlgButtonChecked(IDC_ULIMIT_LBL) ? SW_SHOW : SW_HIDE);

	ShowLimitValues();
	SetModified(TRUE);	
}

void CPPgConnection::OnHelp()
{
	theApp.ShowHelp(eMule_FAQ_Preferences_Connection);
}

BOOL CPPgConnection::OnCommand(WPARAM wParam, LPARAM lParam)
{
	if (wParam == ID_HELP)
	{
		OnHelp();
		return TRUE;
	}
	return __super::OnCommand(wParam, lParam);
}

BOOL CPPgConnection::OnHelpInfo(HELPINFO* /*pHelpInfo*/)
{
	OnHelp();
	return TRUE;
}

void CPPgConnection::OnBnClickedOpenports()
{
	OnApply();
	theApp.m_pFirewallOpener->RemoveRule(EMULE_DEFAULTRULENAME_UDP);
	theApp.m_pFirewallOpener->RemoveRule(EMULE_DEFAULTRULENAME_TCP);
	bool bAlreadyExisted = false;
	if (theApp.m_pFirewallOpener->DoesRuleExist(thePrefs.GetPort(), NAT_PROTOCOL_TCP) || theApp.m_pFirewallOpener->DoesRuleExist(thePrefs.GetUDPPort(), NAT_PROTOCOL_UDP)){
		bAlreadyExisted = true;
	}
	bool bResult = theApp.m_pFirewallOpener->OpenPort(thePrefs.GetPort(), NAT_PROTOCOL_TCP, EMULE_DEFAULTRULENAME_TCP, false);
	if (thePrefs.GetUDPPort() != 0)
		bResult = bResult && theApp.m_pFirewallOpener->OpenPort(thePrefs.GetUDPPort(), NAT_PROTOCOL_UDP, EMULE_DEFAULTRULENAME_UDP, false);
	if (bResult){
		if (!bAlreadyExisted)
			AfxMessageBox(GetResString(IDS_FO_PREF_SUCCCEEDED), MB_ICONINFORMATION | MB_OK);
		else
			// TODO: actually we could offer the user to remove existing rules
			AfxMessageBox(GetResString(IDS_FO_PREF_EXISTED), MB_ICONINFORMATION | MB_OK);
	}
	else
		AfxMessageBox(GetResString(IDS_FO_PREF_FAILED), MB_ICONSTOP | MB_OK);
}

void CPPgConnection::OnStartPortTest()
{
	CString buffer;

	GetDlgItem(IDC_PORT)->GetWindowText(buffer);
	uint16 tcp = (uint16)_tstoi(buffer);

	GetDlgItem(IDC_UDPPORT)->GetWindowText(buffer);
	uint16 udp = (uint16)_tstoi(buffer);

	TriggerPortTest(tcp, udp);
}

void CPPgConnection::SetRateSliderTicks(CSliderCtrl& rRate)
{
	rRate.ClearTics();
	int iMin = 0, iMax = 0;
	rRate.GetRange(iMin, iMax);
	int iDiff = iMax - iMin;
	if (iDiff > 0)
	{
		CRect rc;
		rRate.GetWindowRect(&rc);
		if (rc.Width() > 0)
		{
			int iTic;
			int iPixels = rc.Width() / iDiff;
			if (iPixels >= 6)
				iTic = 1;
			else
			{
				iTic = 10;
				while (rc.Width() / (iDiff / iTic) < 8)
					iTic *= 10;
			}
			if (iTic)
			{
				for (int i = ((iMin+(iTic-1))/iTic)*iTic; i < iMax; /**/)
				{
					rRate.SetTic(i);
					i += iTic;
				}
			}
			rRate.SetPageSize(iTic);
		}
	}
}

//VeryCD Start
//连接选项中的随机端口按钮，added by Chocobo on 2006.08.16
void CPPgConnection::OnBnClickedRandomPort()
{
	uint16 port = SafeRandomPort();
	CString szText;
	szText.Format(_T("%d"),port);
	GetDlgItem(IDC_PORT)->SetWindowText(szText);
	szText.Format(_T("%d"),port+10);
	GetDlgItem(IDC_UDPPORT)->SetWindowText(szText);
}
//VeryCD End

void CPPgConnection::OnChangeSpin(NMHDR * /*pNMHDR*/, LRESULT *pResult)
{
	//LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
	SetModified();// VC-kernel[2007-02-26]:
	*pResult = 0;
}

// VC-kernel[2007-02-28]:
void CPPgConnection::OnEnChangeDownloadCap()
{
	SetModified();
	TCHAR buffer[510];

	if (GetDlgItem(IDC_DOWNLOAD_CAP)->GetWindowTextLength())
	{ 
		GetDlgItem(IDC_DOWNLOAD_CAP)->GetWindowText(buffer, 20);
		//thePrefs.SetMaxGraphDownloadRate(_tstoi(buffer));
		m_ctlMaxDown.SetRange(1,_tstoi(buffer),TRUE);
		//m_ctlMaxDown.SetPos(m_ctlMaxDown.GetPos());
		SetRateSliderTicks(m_ctlMaxDown);
		//ShowLimitValues();
		OnLimiterChange();
	}
}

// VC-kernel[2007-02-28]:
void CPPgConnection::OnEnChangeUploadCap()
{
	SetModified();
	TCHAR buffer[510];

	if (GetDlgItem(IDC_UPLOAD_CAP)->GetWindowTextLength())
	{
		GetDlgItem(IDC_UPLOAD_CAP)->GetWindowText(buffer, 20);
		//thePrefs.SetMaxGraphDownloadRate(_tstoi(buffer));
		m_ctlMaxUp.SetRange(1,_tstoi(buffer),TRUE);
		//m_ctlMaxUp.SetPos(m_ctlMaxUp.GetPos());
		SetRateSliderTicks(m_ctlMaxUp);
		//ShowLimitValues();
		OnLimiterChange();
	}
}

// VC-kernel[2007-03-02]:
void CPPgConnection::OnCbnSelchangeConnectiontype()
{
	SetModified();

	switch(m_ctlConnectionType.GetCurSel())
	{
		case 0:
			CheckDlgButton(IDC_ULIMIT_LBL,false);
			CheckDlgButton(IDC_DLIMIT_LBL,false);
			OnLimiterChange();
			GetDlgItem(IDC_UPLOAD_CAP)->SetWindowText(_T("128"));
			GetDlgItem(IDC_DOWNLOAD_CAP)->SetWindowText(_T("256"));
			GetDlgItem(IDC_UPLOAD_CAP)->EnableWindow(False);
			GetDlgItem(IDC_DOWNLOAD_CAP)->EnableWindow(False);
			thePrefs.SetConnectionType(0);
			break;
		case 1:
			CheckDlgButton(IDC_ULIMIT_LBL,true);
			CheckDlgButton(IDC_DLIMIT_LBL,true);
			OnLimiterChange();
			GetDlgItem(IDC_UPLOAD_CAP)->SetWindowText(_T("64"));
			GetDlgItem(IDC_DOWNLOAD_CAP)->SetWindowText(_T("256"));
			GetDlgItem(IDC_UPLOAD_CAP)->EnableWindow();
			GetDlgItem(IDC_DOWNLOAD_CAP)->EnableWindow();
			thePrefs.SetConnectionType(1);
			break;
		case 2:
			CheckDlgButton(IDC_ULIMIT_LBL,true);
			CheckDlgButton(IDC_DLIMIT_LBL,true);
			OnLimiterChange();
			GetDlgItem(IDC_UPLOAD_CAP)->SetWindowText(_T("64"));
			GetDlgItem(IDC_DOWNLOAD_CAP)->SetWindowText(_T("64"));
			GetDlgItem(IDC_UPLOAD_CAP)->EnableWindow(False);
			GetDlgItem(IDC_DOWNLOAD_CAP)->EnableWindow(False);
			thePrefs.SetConnectionType(2);
			break;
		case 3:
			CheckDlgButton(IDC_ULIMIT_LBL,true);
			CheckDlgButton(IDC_DLIMIT_LBL,true);
			OnLimiterChange();
			GetDlgItem(IDC_UPLOAD_CAP)->SetWindowText(_T("64"));
			GetDlgItem(IDC_DOWNLOAD_CAP)->SetWindowText(_T("128"));
			GetDlgItem(IDC_UPLOAD_CAP)->EnableWindow(False);
			GetDlgItem(IDC_DOWNLOAD_CAP)->EnableWindow(False);
			thePrefs.SetConnectionType(3);
			break;
		case 4:
			CheckDlgButton(IDC_ULIMIT_LBL,true);
			CheckDlgButton(IDC_DLIMIT_LBL,true);
			OnLimiterChange();
			GetDlgItem(IDC_UPLOAD_CAP)->SetWindowText(_T("64"));
			GetDlgItem(IDC_DOWNLOAD_CAP)->SetWindowText(_T("256"));
			GetDlgItem(IDC_UPLOAD_CAP)->EnableWindow(False);
			GetDlgItem(IDC_DOWNLOAD_CAP)->EnableWindow(False);
			thePrefs.SetConnectionType(4);
			break;
		case 5:
			CheckDlgButton(IDC_ULIMIT_LBL,true);
			CheckDlgButton(IDC_DLIMIT_LBL,true);
			OnLimiterChange();
			GetDlgItem(IDC_UPLOAD_CAP)->SetWindowText(_T("64"));
			GetDlgItem(IDC_DOWNLOAD_CAP)->SetWindowText(_T("1024"));
			GetDlgItem(IDC_UPLOAD_CAP)->EnableWindow(False);
			GetDlgItem(IDC_DOWNLOAD_CAP)->EnableWindow(False);
			thePrefs.SetConnectionType(5);
			break;
		case 6:
			CheckDlgButton(IDC_ULIMIT_LBL,false);
			CheckDlgButton(IDC_DLIMIT_LBL,false);
			OnLimiterChange();
			GetDlgItem(IDC_UPLOAD_CAP)->SetWindowText(_T("250"));
			GetDlgItem(IDC_DOWNLOAD_CAP)->SetWindowText(_T("250"));
			GetDlgItem(IDC_UPLOAD_CAP)->EnableWindow(False);
			GetDlgItem(IDC_DOWNLOAD_CAP)->EnableWindow(False);
			thePrefs.SetConnectionType(6);
			break;
	}
}

BOOL CPPgConnection::PreTranslateMessage(MSG* pMsg)
{
	// TODO: 在此添加专用代码和/或调用基类
	m_ttc.RelayEvent(pMsg);

	return CPropertyPage::PreTranslateMessage(pMsg);
}

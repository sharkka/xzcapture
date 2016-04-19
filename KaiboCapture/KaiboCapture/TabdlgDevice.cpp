// TabdlgDevice.cpp : implementation file
//

#include "stdafx.h"
#include "KaiboCapture.h"
#include "TabdlgDevice.h"
#include "afxdialogex.h"


// CTabdlgDevice dialog

IMPLEMENT_DYNAMIC(CTabdlgDevice, CDialogEx)

CTabdlgDevice::CTabdlgDevice(CWnd* pParent /*=NULL*/)
	: CDialogEx(CTabdlgDevice::IDD, pParent)
{

}

CTabdlgDevice::~CTabdlgDevice()
{
}

void CTabdlgDevice::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CTabdlgDevice, CDialogEx)
END_MESSAGE_MAP()


// CTabdlgDevice message handlers

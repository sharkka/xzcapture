// TabdlgAbout.cpp : implementation file
//

#include "stdafx.h"
#include "KaiboCapture.h"
#include "TabdlgAbout.h"
#include "afxdialogex.h"


// CTabdlgAbout dialog

IMPLEMENT_DYNAMIC(CTabdlgAbout, CDialogEx)

CTabdlgAbout::CTabdlgAbout(CWnd* pParent /*=NULL*/)
	: CDialogEx(CTabdlgAbout::IDD, pParent)
{

}

CTabdlgAbout::~CTabdlgAbout()
{
}

void CTabdlgAbout::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CTabdlgAbout, CDialogEx)
END_MESSAGE_MAP()


// CTabdlgAbout message handlers

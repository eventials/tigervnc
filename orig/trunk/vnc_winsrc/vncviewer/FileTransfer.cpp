//  Copyright (C) 2003 Dennis Syrovatsky. All Rights Reserved.
//
//  This file is part of the VNC system.
//
//  The VNC system is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
//  USA.
//
// TightVNC distribution homepage on the Web: http://www.tightvnc.com/
//
// If the source code for the VNC system is not available from the place 
// whence you received this file, check http://www.uk.research.att.com/vnc or contact
// the authors on vnc@uk.research.att.com for information on obtaining it.

#include "vncviewer.h"
#include "FileTransfer.h"
#include "FileTransferItemInfo.h"

const char FileTransfer::delimeter[] = "...";
const char FileTransfer::uploadText[] = ">>>";
const char FileTransfer::downloadText[] = "<<<";
const char FileTransfer::noactionText[] = "<--->";
const char FileTransfer::myComputerText[] = "My Computer";
const char FileTransfer::myDocumentsText[] = "My Documents";
const char FileTransfer::myPicturesText[] = "My Pictures";
const char FileTransfer::myMusicText[] = "My Music";
const char FileTransfer::myDesktopText[] = "Desktop";

#define WM_CHECKUPLOADQUEUE WM_USER+130


FileTransfer::FileTransfer(ClientConnection * pCC, VNCviewerApp * pApp)
{
	m_clientconn = pCC;
	m_pApp = pApp;
	m_bFirstStart = TRUE;
    m_bUploadStarted = FALSE;
    m_bDownloadStarted = FALSE;
	m_bTransferEnable = FALSE;
	m_bServerBrowseRequest = FALSE;
	m_hTreeItem = NULL;
	m_ClientPath[0] = '\0';
	m_ClientPathTmp[0] = '\0';
	m_ServerPath[0] = '\0';
	m_ServerPathTmp[0] = '\0';
	m_dwNumItemsSel = 0;
}

FileTransfer::~FileTransfer()
{
	m_FTClientItemInfo.Free();
	m_FTServerItemInfo.Free();
	m_TransferInfo.Free();
}
void 
FileTransfer::CreateFileTransferDialog()
{
	m_hwndFileTransfer = CreateDialog(m_pApp->m_instance, 
									  MAKEINTRESOURCE(IDD_FILETRANSFER_DLG),
									  NULL, 
									  (DLGPROC) FileTransferDlgProc);
	
	SetWindowLong(m_hwndFileTransfer, GWL_USERDATA, (LONG) this);

	m_hwndFTProgress = GetDlgItem(m_hwndFileTransfer, IDC_FTPROGRESS);
	m_hwndProgress = GetDlgItem(m_hwndFileTransfer, IDC_PROGRESS);
	m_hwndFTClientList = GetDlgItem(m_hwndFileTransfer, IDC_FTCLIENTLIST);
	m_hwndFTServerList = GetDlgItem(m_hwndFileTransfer, IDC_FTSERVERLIST);
	m_hwndFTClientPath = GetDlgItem(m_hwndFileTransfer, IDC_CLIENTPATH);
	m_hwndFTServerPath = GetDlgItem(m_hwndFileTransfer, IDC_SERVERPATH);
	m_hwndFTStatus = GetDlgItem(m_hwndFileTransfer, IDC_FTSTATUS);
	m_hwndFTCanceling = NULL;

	ListView_SetExtendedListViewStyleEx(m_hwndFTClientList, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);
	ListView_SetExtendedListViewStyleEx(m_hwndFTServerList, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);

	RECT Rect;
	GetClientRect(m_hwndFTClientList, &Rect);
	Rect.right -= GetSystemMetrics(SM_CXHSCROLL);
	int xwidth0 = (int) (0.39 * Rect.right);
	int xwidth1 = (int) (0.22 * Rect.right);
	int xwidth2 = (int) (0.39 * Rect.right);

	FTInsertColumn(m_hwndFTClientList, "Name", 0, xwidth0, LVCFMT_LEFT);
	FTInsertColumn(m_hwndFTClientList, "Size", 1, xwidth1, LVCFMT_RIGHT);
	FTInsertColumn(m_hwndFTClientList, "Modified", 2, xwidth2, LVCFMT_RIGHT);
	FTInsertColumn(m_hwndFTServerList, "Name", 0, xwidth0, LVCFMT_LEFT);
	FTInsertColumn(m_hwndFTServerList, "Size", 1, xwidth1, LVCFMT_RIGHT);
	FTInsertColumn(m_hwndFTServerList, "Modified", 2, xwidth2, LVCFMT_RIGHT);

	SendDlgItemMessage(m_hwndFileTransfer, IDC_CLIENTPATH, CB_INSERTSTRING, (WPARAM) FT_ID_MYCOMPUTER, (LPARAM) myComputerText);
	SendDlgItemMessage(m_hwndFileTransfer, IDC_CLIENTPATH, CB_INSERTSTRING, (WPARAM) FT_ID_MYDOCUMENTS, (LPARAM) myDocumentsText);
	SendDlgItemMessage(m_hwndFileTransfer, IDC_CLIENTPATH, CB_INSERTSTRING, (WPARAM) FT_ID_MYPICTURES, (LPARAM) myPicturesText);
	SendDlgItemMessage(m_hwndFileTransfer, IDC_CLIENTPATH, CB_INSERTSTRING, (WPARAM) FT_ID_MYMUSIC, (LPARAM) myMusicText);
	SendDlgItemMessage(m_hwndFileTransfer, IDC_CLIENTPATH, CB_INSERTSTRING, (WPARAM) FT_ID_MYDESKTOP, (LPARAM) myDesktopText);

	SendDlgItemMessage(m_hwndFileTransfer, IDC_SERVERPATH, CB_INSERTSTRING, (WPARAM) FT_ID_MYCOMPUTER, (LPARAM) myComputerText);
	SendDlgItemMessage(m_hwndFileTransfer, IDC_SERVERPATH, CB_INSERTSTRING, (WPARAM) FT_ID_MYDOCUMENTS, (LPARAM) myDocumentsText);
	SendDlgItemMessage(m_hwndFileTransfer, IDC_SERVERPATH, CB_INSERTSTRING, (WPARAM) FT_ID_MYPICTURES, (LPARAM) myPicturesText);
	SendDlgItemMessage(m_hwndFileTransfer, IDC_SERVERPATH, CB_INSERTSTRING, (WPARAM) FT_ID_MYMUSIC, (LPARAM) myMusicText);
	SendDlgItemMessage(m_hwndFileTransfer, IDC_SERVERPATH, CB_INSERTSTRING, (WPARAM) FT_ID_MYDESKTOP, (LPARAM) myDesktopText);

	SendDlgItemMessage(m_hwndFileTransfer, IDC_CLIENTPATH, CB_LIMITTEXT, (WPARAM) MAX_PATH, (LPARAM) 0);
	SendDlgItemMessage(m_hwndFileTransfer, IDC_SERVERPATH, CB_LIMITTEXT, (WPARAM) MAX_PATH, (LPARAM) 0);

	InitFTIcons();

	m_TransferInfo.Free();

	m_bClientRefresh = FALSE;
	m_bServerRefresh = FALSE;
	m_bEndFTDlgOnYes = FALSE;
	m_bOverwriteAll = FALSE;

	char buf[MAX_HOST_NAME_LEN + 18];
	char localHostName[MAX_HOST_NAME_LEN];
	if (gethostname(localHostName, MAX_HOST_NAME_LEN) != SOCKET_ERROR) {
		sprintf(buf, "Local Computer: %s", localHostName);
	} else {
		sprintf(buf, "Local Computer: - Unknown -");
	}
	SetDlgItemText(m_hwndFileTransfer, IDC_LOCAL_COMP_LABEL, buf);

	sprintf(buf, "Remote Computer: %s", m_clientconn->m_desktopName);
	SetDlgItemText(m_hwndFileTransfer, IDC_TVNC_SERV_LABEL, buf);

	ShowWindow(m_hwndFileTransfer, SW_SHOW);
	UpdateWindow(m_hwndFileTransfer);

	if (m_bFirstStart) {
		m_bFirstStart = FALSE;
		ShowClientSpecFolder(CSIDL_PERSONAL);
		SendDlgItemMessage(m_hwndFileTransfer, IDC_CLIENTPATH, CB_SETCURSEL, (WPARAM) FT_ID_MYDOCUMENTS, (LPARAM) 0);
		SendDlgItemMessage(m_hwndFileTransfer, IDC_SERVERPATH, CB_SETCURSEL, (WPARAM) FT_ID_MYDOCUMENTS, (LPARAM) 0);
		CheckClientLV();
		SendFileSpecDirRequestMessage(0, rfbSpecDirMyDocuments);
	} else {
		ShowClientItems(m_ClientPathTmp);
		CheckClientLV();
		SendFileListRequestMessage(m_ServerPathTmp, 0, FT_FLR_DEST_MAIN);
	}
}

LRESULT CALLBACK 
FileTransfer::FileTransferDlgProc(HWND hwnd, 
								  UINT uMsg, 
								  WPARAM wParam, 
								  LPARAM lParam)
{
	FileTransfer *_this = (FileTransfer *) GetWindowLong(hwnd, GWL_USERDATA);
	int i;
	switch (uMsg)
	{
	case WM_INITDIALOG:
		{
			SetForegroundWindow(hwnd);
			CentreWindow(hwnd);
			return TRUE;
		}
	break;
	case WM_HELP:	
		help.Popup(lParam);
		return 0;
	case WM_COMMAND:
		{
		switch (LOWORD(wParam))
		{
			case IDC_CLIENTPATH:
				switch (HIWORD (wParam))
				{
					case EN_SETFOCUS:
						_this->ClearFTButtons();
						return TRUE;
					case CBN_SELCHANGE:
						SetWindowText(_this->m_hwndFTStatus, "");
						int curSel = SendDlgItemMessage(hwnd, IDC_CLIENTPATH, CB_GETCURSEL, (WPARAM) 0, (LPARAM) 0);
						switch (curSel) 
						{ 
						case FT_ID_MYCOMPUTER:
							strcpy(_this->m_ClientPathTmp, "");
							_this->ShowClientItems(_this->m_ClientPathTmp);
							break;
						case FT_ID_MYDOCUMENTS:
							_this->ShowClientSpecFolder(CSIDL_PERSONAL);
							break;
						case FT_ID_MYPICTURES:
//							_this->ShowClientSpecFolder(CSIDL_MYPICTURES);
							_this->ShowClientSpecFolder(0x0027);
							break;
						case FT_ID_MYMUSIC:
//							_this->ShowClientSpecFolder(CSIDL_MYMUSIC);
							_this->ShowClientSpecFolder(0x000d);
							break;
						case FT_ID_MYDESKTOP:
							_this->ShowClientSpecFolder(CSIDL_DESKTOP);
							break;
						}
						return TRUE;
				}
			break;
			case IDC_SERVERPATH:
				switch (HIWORD (wParam))
				{
					case EN_SETFOCUS:
						_this->ClearFTButtons();
						return TRUE;
					case CBN_SELCHANGE:
						SetWindowText(_this->m_hwndFTStatus, "");
						int curSel = SendDlgItemMessage(hwnd, IDC_SERVERPATH, CB_GETCURSEL, (WPARAM) 0, (LPARAM) 0);
						switch (curSel) 
						{ 
						case FT_ID_MYCOMPUTER:
							strcpy(_this->m_ServerPathTmp, "");
							_this->SendFileListRequestMessage(_this->m_ServerPathTmp, 0, FT_FLR_DEST_MAIN);
							break;
						case FT_ID_MYDOCUMENTS:
							_this->SendFileSpecDirRequestMessage(0, rfbSpecDirMyDocuments);
							break;
						case FT_ID_MYPICTURES:
							_this->SendFileSpecDirRequestMessage(0, rfbSpecDirMyPictures);
							break;
						case FT_ID_MYMUSIC:
							_this->SendFileSpecDirRequestMessage(0, rfbSpecDirMyMusic);
							break;
						case FT_ID_MYDESKTOP:
							_this->SendFileSpecDirRequestMessage(0, rfbSpecDirDesktop);
							break;
						}
						return TRUE;
				}
			break;
			case IDC_EXIT:
				_this->CloseFileTransferDialog();
				return TRUE;
			case IDC_CLIENTUP:
				_this->ClearFTControls();
				if (strcmp(_this->m_ClientPathTmp, "") == 0) return TRUE;
				for (i=(strlen(_this->m_ClientPathTmp)-2); i>=0; i--) {
					if (_this->m_ClientPathTmp[i] == '\\') {
						_this->m_ClientPathTmp[i] = '\0';
						break;
					}
					if (i == 0) _this->m_ClientPathTmp[0] = '\0';
				}
				_this->ShowClientItems(_this->m_ClientPathTmp);
				return TRUE;
			case IDC_SERVERUP:
				_this->ClearFTControls();
				if (strcmp(_this->m_ServerPathTmp, "") == 0) return TRUE;
				for (i=(strlen(_this->m_ServerPathTmp)-2); i>=0; i--) {
					if (_this->m_ServerPathTmp[i] == '\\') {
						_this->m_ServerPathTmp[i] = '\0';
						break;
					}
					if (i == 0) _this->m_ServerPathTmp[0] = '\0';
				}
				_this->SendFileListRequestMessage(_this->m_ServerPathTmp, 0, FT_FLR_DEST_MAIN);
				return TRUE;
			case IDC_CLIENTRELOAD:
				_this->ClearFTControls();
				_this->m_bClientRefresh = TRUE;
				_this->ShowClientItems(_this->m_ClientPath);
				return TRUE;
			case IDC_SERVERRELOAD:
				_this->ClearFTControls();
				_this->m_bServerRefresh = TRUE;
				_this->SendFileListRequestMessage(_this->m_ServerPathTmp, 0, FT_FLR_DEST_MAIN);
				return TRUE;
			case IDC_FTCOPY:
				{
				HANDLE hIcon = (HANDLE) SendDlgItemMessage(_this->m_hwndFileTransfer, IDC_FTCOPY, STM_GETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) 0);
				if ((HIWORD(wParam) == STN_CLICKED) && (hIcon != _this->m_hIconCopyGray)) {
					_this->ClearFTButtons();
    				if ((strcmp(_this->m_ClientPath, "") == 0) | (strcmp(_this->m_ServerPath, "") == 0)) {
						char buffer[MAX_PATH];
						sprintf(buffer, "Illegal Local Computer or TightVNC Server path. File Transfer impossible.");
						SetWindowText(_this->m_hwndFTStatus, buffer);
						return TRUE;
						}
					_this->m_bTransferEnable = TRUE;
					SendDlgItemMessage(_this->m_hwndFileTransfer, IDC_FTCANCEL, STM_SETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) _this->m_hIconCancel);
					if (_this->m_bFTCOPY == FALSE) {
						_this->FileTransferUpload();			
					} else {
						_this->FileTransferDownload();			
					}
				}
				}
				return TRUE;
			case IDC_FTCANCEL:
				{
					HANDLE hIcon = (HANDLE) SendDlgItemMessage(_this->m_hwndFileTransfer, IDC_FTCANCEL, STM_GETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) 0);
					if ((HIWORD(wParam) == STN_CLICKED) && (hIcon != _this->m_hIconCancelGray) && _this->IsTransferEnable()) {
						_this->CreateFTCancelingDlg();
					}
				}
				return TRUE;
			case IDC_CLIENTBROWSE_BUT:
				_this->ClearFTControls();
				_this->CreateFTBrowseDialog(FALSE);
				return TRUE;
			case IDC_SERVERBROWSE_BUT:
				_this->ClearFTControls();
				_this->CreateFTBrowseDialog(TRUE);
				return TRUE;
			case IDC_CREATELOCFLD:
				{
					HANDLE hIcon = (HANDLE) SendDlgItemMessage(_this->m_hwndFileTransfer, IDC_CREATELOCFLD, STM_GETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) 0);
					if ((HIWORD(wParam) == STN_CLICKED) && (hIcon != _this->m_hIconCreateLocFolderGray)) {
						_this->ClearFTControls();
						_this->ClientCreateDir();
					}
				}
				return TRUE;
			case IDC_CREATEREMFLD:
				{
					HANDLE hIcon = (HANDLE) SendDlgItemMessage(_this->m_hwndFileTransfer, IDC_CREATEREMFLD, STM_GETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) 0);
					if ((HIWORD(wParam) == STN_CLICKED) && (hIcon != _this->m_hIconCreateRemFolderGray)) {
						_this->ClearFTControls();
						_this->ServerCreateDir();
					}
				}
				return TRUE;
			case IDC_FTDELETE:
				{
					HANDLE hIcon = (HANDLE) SendDlgItemMessage(_this->m_hwndFileTransfer, IDC_FTDELETE, STM_GETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) 0);
					if ((HIWORD(wParam) == STN_CLICKED) && (hIcon != _this->m_hIconDeleteGray)) {
						_this->ClearFTControls();
						if (_this->m_bFTCOPY == FALSE) {
							_this->ClientDeleteDir();
						} else {
							_this->ServerDeleteDir();
						}
					}
				}
				return TRUE;
			case IDC_FTRENAME:
				{
				HANDLE hIcon = (HANDLE) SendDlgItemMessage(_this->m_hwndFileTransfer, IDC_FTRENAME, STM_GETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) 0);
				if ((HIWORD(wParam) == STN_CLICKED) && (hIcon != _this->m_hIconRenameGray)) {
					_this->ClearFTControls();
					if (_this->m_bFTCOPY == FALSE) {
						_this->ClientRenameDir();
					} else {
						_this->ServerRenameDir();
					}
				}
				}
				return TRUE;
		}
		}
	break;

	case WM_NOTIFY:
		switch (LOWORD(wParam))
		{
		case IDC_FTCLIENTLIST:
			switch (((LPNMHDR) lParam)->code)
			{
				case NM_CLICK:
				case NM_RCLICK:
					_this->CheckClientLV();
					return TRUE;
				case NM_SETFOCUS:
					SendDlgItemMessage(_this->m_hwndFileTransfer, IDC_FTCOPY, STM_SETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) _this->m_hIconUpload);
					_this->CheckClientLV();
					_this->m_bFTCOPY = FALSE;
					return TRUE;
				case LVN_ITEMCHANGED:
					SendDlgItemMessage(_this->m_hwndFileTransfer, IDC_FTCOPY, STM_SETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) _this->m_hIconUpload);
					_this->CheckClientLV();
					return TRUE;
				case LVN_GETDISPINFO:
					_this->OnGetDispClientInfo((NMLVDISPINFO *) lParam); 
					return TRUE;
				case LVN_ITEMACTIVATE:
					LPNMITEMACTIVATE lpnmia = (LPNMITEMACTIVATE)lParam;
					_this->ProcessListViewDBLCLK(_this->m_hwndFTClientList, _this->m_ClientPath, _this->m_ClientPathTmp, lpnmia->iItem);
					_this->CheckClientLV();
					return TRUE;
			}
		break;
		case IDC_FTSERVERLIST:
			switch (((LPNMHDR) lParam)->code)
			{
				case NM_CLICK:
				case NM_RCLICK:
					_this->CheckServerLV();
					return TRUE;
				case NM_SETFOCUS:
					SendDlgItemMessage(_this->m_hwndFileTransfer, IDC_FTCOPY, STM_SETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) _this->m_hIconDownload);
					_this->CheckServerLV();
					_this->m_bFTCOPY = TRUE;
					return TRUE;
				case LVN_ITEMCHANGED:
					SendDlgItemMessage(_this->m_hwndFileTransfer, IDC_FTCOPY, STM_SETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) _this->m_hIconDownload);
					_this->CheckServerLV();
					return TRUE;
				case LVN_GETDISPINFO: 
					_this->OnGetDispServerInfo((NMLVDISPINFO *) lParam); 
					return TRUE;
				case LVN_ITEMACTIVATE:
					LPNMITEMACTIVATE lpnmia = (LPNMITEMACTIVATE)lParam;
					_this->ProcessListViewDBLCLK(_this->m_hwndFTServerList, _this->m_ServerPath, _this->m_ServerPathTmp, lpnmia->iItem);
					return TRUE;
			}
		break;
		}
		break;
	case WM_CHECKUPLOADQUEUE:
		_this->CheckUploadQueue();
		return TRUE;
	case WM_CLOSE:
	case WM_DESTROY:
		_this->CloseFileTransferDialog();
		return TRUE;
	}
	return 0;
}

void
FileTransfer::CloseFileTransferDialog()
{
	if (IsTransferEnable()) {
		m_bEndFTDlgOnYes = TRUE;
		CreateFTCancelingDlg();
		return;
	}
	m_clientconn->m_fileTransferDialogShown = false;
	m_FTClientItemInfo.Free();
	m_FTServerItemInfo.Free();
	DestroyFTIcons();
	EndDialog(m_hwndFileTransfer, TRUE);
	return;
}

void 
FileTransfer::OnGetDispClientInfo(NMLVDISPINFO *plvdi) 
{
	if (m_FTClientItemInfo.GetIntSizeAt(plvdi->item.iItem) < 0) {
		plvdi->item.iImage = 0;
	} else {
		plvdi->item.iImage = 1;
	}
	switch (plvdi->item.iSubItem)
    {
    case 0:
		plvdi->item.pszText = m_FTClientItemInfo.GetNameAt(plvdi->item.iItem);
		break;
    case 1:
		plvdi->item.pszText = m_FTClientItemInfo.GetSizeAt(plvdi->item.iItem);
		break;
	case 2:
		if (strlen(m_ClientPath) == 0) {
			plvdi->item.pszText = "Unspecified";
		} else {
			char szData[32];
			FILETIME ft;
			Time70ToFiletime(m_FTClientItemInfo.GetDataAt(plvdi->item.iItem), &ft);
			SYSTEMTIME st;
			FileTimeToSystemTime(&ft, &st);
			if (st.wHour > 12) {
				sprintf(szData, "%d/%d/%d %d:%d PM", st.wDay, st.wMonth, st.wYear, st.wHour - 12, st.wMinute);
			} else {
				sprintf(szData, "%d/%d/%d %d:%d AM", st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute);
			}
			plvdi->item.pszText = szData;
		}
		break;
    default:
		break;
    }
} 

void 
FileTransfer::OnGetDispServerInfo(NMLVDISPINFO *plvdi) 
{
	if (m_FTServerItemInfo.GetIntSizeAt(plvdi->item.iItem) < 0) {
		plvdi->item.iImage = 0;
	} else {
		plvdi->item.iImage = 1;
	}
	switch (plvdi->item.iSubItem)
	{
    case 0:
		plvdi->item.pszText = m_FTServerItemInfo.GetNameAt(plvdi->item.iItem);
		break;
    case 1:
		plvdi->item.pszText = m_FTServerItemInfo.GetSizeAt(plvdi->item.iItem);
		break;
	case 2:
		if (strlen(m_ServerPath) == 0) {
			plvdi->item.pszText = "Unspecified";
		} else {
			char szData[32];
			FILETIME ft;
			Time70ToFiletime(m_FTServerItemInfo.GetDataAt(plvdi->item.iItem), &ft);
			SYSTEMTIME st;
			FileTimeToSystemTime(&ft, &st);
			if (st.wHour > 12) {
				sprintf(szData, "%d/%d/%d %d:%d PM", st.wDay, st.wMonth, st.wYear, st.wHour - 12, st.wMinute);
			} else {
				sprintf(szData, "%d/%d/%d %d:%d AM", st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute);
			}
			plvdi->item.pszText = szData;
		}
		break;
    default:
		break;
    }
} 

void 
FileTransfer::FileTransferUpload()
{
	strcpy(m_szLocalTransPath, m_ClientPath);
	strcpy(m_szRemoteTransPath, m_ServerPath);
	strcpy(m_szLastRelTransPath, "");
	if (m_TransferInfo.GetNumEntries() != 0) return;
	m_CantTransferInfo.Free();
	m_dwNumItemsSel = GetSelectedItems(m_hwndFTClientList, &m_TransferInfo);
	m_dwSelFileSize = GetSelectedFileSize(m_ClientPath, &m_TransferInfo);
	InitFTProgressBar(0);
	InitProgressBar(0);
	m_bUploadStarted = TRUE;
	if (!CreateTransferConfDlg()) return;
	CheckUploadQueue();
}

void
FileTransfer::CheckUploadQueue()
{
	int numEntries = m_TransferInfo.GetNumEntries() - 1;
	if ((numEntries < 0) || (!m_bUploadStarted)) {
		CheckCantTransferInfo();
		SendFileListRequestMessage(m_ServerPath, 0, FT_FLR_DEST_MAIN);
		return;
	}
/*
	if ((m_dwNumItemsSel > 0) && (strcmp(m_ServerPath, m_szRemoteTransPath) == 0)) {
		m_dwNumItemsSel--;
		SendFileListRequestMessage(m_ServerPath, 0, FT_FLR_DEST_MAIN);
	}
*/
	char path[MAX_PATH];
	if (!m_bOverwriteAll) {
		sprintf(path, "%s\\%s", m_szRemoteTransPath, m_TransferInfo.GetNameAt(0));
		char lastName[MAX_PATH];
		CutLastName(path, lastName);
		SendFileListRequestMessage(path, 0, FT_FLR_DEST_UPLOAD);
	} else {
		while (m_TransferInfo.GetIntSizeAt(0) < 0) {
			sprintf(path, "%s\\%s", m_szRemoteTransPath, m_TransferInfo.GetNameAt(0));
			SendFileCreateDirRequestMessage(strlen(path), path);
			MakeUploadQueue();
			m_TransferInfo.DeleteAt(0);
			if (m_TransferInfo.GetNumEntries() == 0) {
				CheckCantTransferInfo();
				SendFileListRequestMessage(m_ServerPath, 0, FT_FLR_DEST_MAIN);
				return;
			}
		}
		if (!UploadFile(0)) PostMessage(m_hwndFileTransfer, WM_CHECKUPLOADQUEUE, (WPARAM) 0, (LPARAM) 0);
	}
}
		
void
FileTransfer::MakeUploadQueue()		
{
	char path[MAX_PATH];
	sprintf(path, "%s\\%s\\*", m_szLocalTransPath, m_TransferInfo.GetNameAt(0));

	WIN32_FIND_DATA FindFileData;
	SetErrorMode(SEM_FAILCRITICALERRORS);
	HANDLE hFile = FindFirstFile(path, &FindFileData);
	SetErrorMode(0);

	char buff[MAX_PATH];
	if (hFile != INVALID_HANDLE_VALUE) {
		do {
			if (strcmp(FindFileData.cFileName, ".") != 0 &&
				strcmp(FindFileData.cFileName, "..") != 0) {
				LARGE_INTEGER li;
				li.LowPart = FindFileData.ftLastWriteTime.dwLowDateTime;
				li.HighPart = FindFileData.ftLastWriteTime.dwHighDateTime;							
				li.QuadPart = (li.QuadPart - 1164444736000000000) / 10000000;
				sprintf(buff, "%s\\%s", m_TransferInfo.GetNameAt(0), FindFileData.cFileName);
				if ((FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {	
					m_TransferInfo.Add(buff, -1, 0);
				} else {
					m_TransferInfo.Add(buff, FindFileData.nFileSizeLow, li.HighPart);
				}
			}
		} while (FindNextFile(hFile, &FindFileData));
		FindClose(hFile);
	}			
}

BOOL
FileTransfer::UploadFile(int num)
{
	char filename[MAX_PATH];
	strcpy(filename, m_TransferInfo.GetNameAt(num));

	char path[MAX_PATH];
	sprintf(path, "%s\\%s", m_szLocalTransPath, filename);

	WIN32_FIND_DATA FindFileData;
	SetErrorMode(SEM_FAILCRITICALERRORS);
	HANDLE hFile = FindFirstFile(path, &FindFileData);
	SetErrorMode(0);
	
	char remPath[MAX_PATH];
	sprintf(remPath, "%s\\%s", m_szRemoteTransPath, filename);

	if (hFile != INVALID_HANDLE_VALUE) {
		m_dwFileSize = FindFileData.nFileSizeLow;
		m_dwModTime = FiletimeToTime70(FindFileData.ftLastWriteTime);
	} else {
		SetStatusText("File %s missing.", path);
		FindClose(hFile);
		m_CantTransferInfo.Add(path,m_TransferInfo.GetSizeAt(num), m_TransferInfo.GetDataAt(num));
		m_TransferInfo.DeleteAt(num);
		return FALSE;
	}
	FindClose(hFile);

	SetDefaultBlockSize();

	if (m_dwFileSize <= m_dwFileBlockSize) m_dwFileBlockSize = m_dwFileSize;
	m_hFiletoRead = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if (m_hFiletoRead == INVALID_HANDLE_VALUE) {
		SetStatusText("Access Denied. Cannot Copy %s.", path);
		SendDlgItemMessage(m_hwndFileTransfer, IDC_FTCANCEL, STM_SETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) m_hIconCancelGray);
		m_CantTransferInfo.Add(path, m_TransferInfo.GetSizeAt(num), m_TransferInfo.GetDataAt(num));
		m_TransferInfo.DeleteAt(num);
		return FALSE;
	}
	MakeStatusText("UPLOAD:", m_szLocalTransPath, m_szRemoteTransPath, filename);
	ConvertPath(remPath);
	int pathLen = strlen(remPath);

	char *pAllFURMessage = new char[sz_rfbFileUploadRequestMsg + pathLen];
	rfbFileUploadRequestMsg *pFUR = (rfbFileUploadRequestMsg *) pAllFURMessage;
	char *pFollowMsg = &pAllFURMessage[sz_rfbFileUploadRequestMsg];
	pFUR->type = rfbFileUploadRequest;
	pFUR->compressedLevel = 0;
	pFUR->fNameSize = Swap16IfLE(pathLen);
	pFUR->position = Swap32IfLE(0);
	memcpy(pFollowMsg, remPath, pathLen);
	m_clientconn->WriteExact(pAllFURMessage, sz_rfbFileUploadRequestMsg + pathLen); 
	delete [] pAllFURMessage;
	
	InitProgressBar(0);
	m_TransferInfo.DeleteAt(num);
	return UploadFilePortion();
}

BOOL
FileTransfer::UploadFilePortion()
{
	DWORD dwPortionRead = 0;
	char *pBuff = new char [m_dwFileBlockSize];
	
	if (m_hwndFTCanceling != NULL) ProcessDlgMessage(m_hwndFTCanceling);
	ProcessDlgMessage(m_hwndFileTransfer);
	if (m_bTransferEnable == FALSE) {
		SetStatusText("File transfer canceled");
		SendDlgItemMessage(m_hwndFileTransfer, IDC_FTCANCEL, STM_SETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) m_hIconCancelGray);
		char reason[] = "User stop transfer file";
		int reasonLen = strlen(reason);
		char *pFUFMessage = new char[sz_rfbFileUploadFailedMsg + reasonLen];
		rfbFileUploadFailedMsg *pFUF = (rfbFileUploadFailedMsg *) pFUFMessage;
		char *pReason = &pFUFMessage[sz_rfbFileUploadFailedMsg];
		pFUF->type = rfbFileUploadFailed;
		pFUF->reasonLen = Swap16IfLE(reasonLen);
		memcpy(pReason, reason, reasonLen);
		m_clientconn->WriteExact(pFUFMessage, sz_rfbFileUploadFailedMsg + reasonLen);
		delete [] pFUFMessage;
		delete [] pBuff;
		CloseHandle(m_hFiletoRead);
		m_TransferInfo.Free();
		m_bUploadStarted = FALSE;
		m_dwNumItemsSel = 0;
		return FALSE;
	}
	DWORD dwNumOfBytesRead = 0;
	BOOL bResult = ReadFile(m_hFiletoRead, pBuff, m_dwFileBlockSize, &dwNumOfBytesRead, NULL);
	if (bResult && dwNumOfBytesRead == 0) {
		/* This is the end of the file. */
		SendFileUploadDataMessage(m_dwModTime);
		SendMessage(m_hwndProgress, PBM_SETPOS, (WPARAM) 65535, (LPARAM) 0);
		SetDlgItemText(m_hwndFileTransfer, IDC_CURRENTFILEPERCENT, "100%");
		CloseHandle(m_hFiletoRead);
		/*
		if ((m_dwNumItemsSel > 0) && (strcmp(m_ServerPath, m_szRemoteTransPath) == 0)) {
			m_dwNumItemsSel--;
			SendFileListRequestMessage(m_ServerPath, 0, FT_FLR_DEST_MAIN);
		}
		*/
		delete [] pBuff;
		PostMessage(m_hwndFileTransfer, WM_CHECKUPLOADQUEUE, (WPARAM) 0, (LPARAM) 0);
		return TRUE;
	}
	SendFileUploadDataMessage(dwNumOfBytesRead, pBuff);
	delete [] pBuff;
	IncreaseProgBarPos(dwNumOfBytesRead);
	PostMessage(m_clientconn->m_hwnd1, fileTransferUploadMessage, (WPARAM) 0, (LPARAM) 0);
	return TRUE;
}

void 
FileTransfer::SetEndTransfer(char *statusText)
{
	m_bTransferEnable = FALSE;
	m_bUploadStarted = FALSE;
	m_bDownloadStarted = FALSE;
	m_bOverwriteAll = FALSE;
	SetStatusText(statusText);
	if (m_hwndFTCanceling != NULL) EndFTCancelDlg(FALSE);
	SendDlgItemMessage(m_hwndFileTransfer, IDC_FTCANCEL, STM_SETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) m_hIconCancelGray);
	SendMessage(m_hwndProgress, PBM_SETPOS, (WPARAM) 65535, (LPARAM) 0);
	SetDlgItemText(m_hwndFileTransfer, IDC_CURRENTFILEPERCENT, "100%");
	SendMessage(m_hwndFTProgress, PBM_SETPOS, (WPARAM) 65535, (LPARAM) 0);
	SetDlgItemText(m_hwndFileTransfer, IDC_FILETRANSFERPERCENT, "100%");
}

void 
FileTransfer::ProcessFDSDMessage()
{
	rfbFileDirSizeDataMsg fdsd;
	m_clientconn->ReadExact((char *) &fdsd, sz_rfbFileDirSizeDataMsg);
	CARD32 dSize = Swap32IfLE(fdsd.dSize);

	SetFTDlgCursor(IDC_ARROW);

	switch (m_FDSRDest)
	{
	case FT_FDSR_DEST_MAIN:
		ProcessFDSDMain(dSize);
		break;
	case FT_FDSR_DEST_DOWNLOAD:
		m_dwSelFileSize -= dSize;
		CheckDownloadQueue();
		break;
	case FT_FDSR_DEST_UPLOAD:
		m_dwSelFileSize -= dSize;
		m_TransferInfo.DeleteAt(0);
		if (m_TransferInfo.GetNumEntries() == 0) {
			CheckCantTransferInfo();
			SendFileListRequestMessage(m_ServerPath, 0, FT_FLR_DEST_MAIN);
			return;
		}
		CheckUploadQueue();
		break;
	}
}

void
FileTransfer::ProcessFDSDMain(DWORD dSize)
{
	m_dwSelFileSize += dSize;

	if (m_NumReqDirSize == 0) {
		m_bDownloadStarted = TRUE;
		InitFTProgressBar(0);
		InitProgressBar(0);
		CheckDownloadQueue();
	} else {
		m_NumReqDirSize--;
		char path[MAX_PATH];
		sprintf(path, "%s\\%s", m_szRemoteTransPath, m_TransferInfo.GetNameAt(m_NumReqDirSize));
		SendFileDirSizeRequestMessage(strlen(path),path, FT_FDSR_DEST_MAIN);
	}
}

void 
FileTransfer::FileTransferDownload()
{
	strcpy(m_szLocalTransPath, m_ClientPath);
	strcpy(m_szRemoteTransPath, m_ServerPath);
	strcpy(m_szLastRelTransPath, "");
	if (m_TransferInfo.GetNumEntries() != 0) return;
	m_CantTransferInfo.Free();
	m_dwNumItemsSel = GetSelectedItems(m_hwndFTServerList, &m_TransferInfo);
	m_bDownloadStarted = TRUE;
	if (!CreateTransferConfDlg()) return;
	m_NumReqDirSize = m_dwNumItemsSel;
	m_dwSelFileSize = 0;
	m_TransferInfo.Sort();
	for (int i = m_TransferInfo.GetNumEntries() - 1; i >= 0; i--) {
		m_NumReqDirSize--;
		int size = m_TransferInfo.GetIntSizeAt(i);
		if (size >= 0) {
			m_dwSelFileSize += size;
		} else {
			char path[MAX_PATH];
			sprintf(path, "%s\\%s", m_szRemoteTransPath, m_TransferInfo.GetNameAt(i));
			SendFileDirSizeRequestMessage(strlen(path), path, FT_FDSR_DEST_MAIN);
			return;
		}
	}
	InitFTProgressBar(0);
	InitProgressBar(0);
	CheckDownloadQueue();
}

void FileTransfer::CheckDownloadQueue()
{
	char path[MAX_PATH];
	char name[MAX_PATH];
	do {
		int numEntries = m_TransferInfo.GetNumEntries() - 1;
		if ((numEntries < 0) || (!m_bDownloadStarted)) {
			m_bTransferEnable = FALSE;
			m_bDownloadStarted = FALSE;
			CheckCantTransferInfo();
			if (m_hwndFTCanceling != NULL) EndFTCancelDlg(FALSE);
			m_bOverwriteAll = FALSE;
			return;
		}
		strcpy(name, m_TransferInfo.GetNameAt(0));
		sprintf(path, "%s\\%s", m_szLocalTransPath, name);
		if (!m_bOverwriteAll) {
			WIN32_FIND_DATA FindFileData;
			SetErrorMode(SEM_FAILCRITICALERRORS);
			HANDLE hFile = FindFirstFile(path, &FindFileData);
			SetErrorMode(0);
			if (hFile != INVALID_HANDLE_VALUE) {
				SYSTEMTIME st;
				FileTimeToSystemTime(&FindFileData.ftLastWriteTime, &st);
				char existFileData[MAX_PATH + 36];
				if (st.wHour > 12) {
					sprintf(existFileData, "%s, modified on %d/%d/%d %d:%d PM", name, 
						st.wDay, st.wMonth, st.wYear, st.wHour - 12, st.wMinute);
				} else {
					sprintf(existFileData, "%s, modified on %d/%d/%d %d:%d AM", name, 
						st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute);
				}
				char transFileData[MAX_PATH + 36];
				FILETIME ft;
				Time70ToFiletime(m_TransferInfo.GetDataAt(0), &ft);
				FileTimeToSystemTime(&ft, &st);
				if (st.wHour > 12) {
					sprintf(transFileData, "%s, modified on %d/%d/%d %d:%d PM", name, 
						st.wDay, st.wMonth, st.wYear, st.wHour - 12, st.wMinute);
				} else {
					sprintf(transFileData, "%s, modified on %d/%d/%d %d:%d AM", name, 
						st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute);
				}
				char buf[2 * MAX_PATH + 255];
				if (m_TransferInfo.GetIntSizeAt(0) < 0) {
					sprintf(buf, "This folder already exists.\n\nWould you like to replace the existing file\n\n%s,\n\nWith this one?\n\n%s\n",
						existFileData, transFileData);
				} else {
					sprintf(buf, "This file already exists.\n\nWould you like to replace the existing file\n\n%s,\n\nWith this one?\n\n%s\n",
						existFileData, transFileData);
				}
				FindClose(hFile);
				if (!CreateFTConfirmDlg(buf)) {
					m_TransferInfo.DeleteAt(0);
					sprintf(path, "%s\\%s", m_szRemoteTransPath, name);
					SendFileDirSizeRequestMessage(strlen(path), path, FT_FDSR_DEST_DOWNLOAD);
					return;
				} else {
					break;
				}
			} else {
				break;
			}
		} else {
			break;
		}
	} while (1);
	if (m_TransferInfo.GetIntSizeAt(0) < 0) {
		if (!CreateDirectory(path, NULL)) {
			//FIXME
		}
		strcpy(m_szLastRelTransPath, name);
		m_TransferInfo.DeleteAt(0);
		sprintf(path, "%s\\%s", m_szRemoteTransPath, name);
		SendFileListRequestMessage(path, 0, FT_FLR_DEST_DOWNLOAD);
		if ((m_dwNumItemsSel > 0) && (strcmp(m_ClientPath, m_szLocalTransPath) == 0)) {
			m_dwNumItemsSel--;
			ShowClientItems(m_ClientPath);
		}
	} else {
		DownloadFile(0);
	}
}

void 
FileTransfer::DownloadFile(int num)
{
	char name[MAX_PATH];
	strcpy(name, m_TransferInfo.GetNameAt(0));

	char locPath[MAX_PATH];
	sprintf(locPath, "%s\\%s", m_szLocalTransPath, name);
	strcpy(m_DownloadFilename, locPath);

	char remPath[MAX_PATH];
	sprintf(remPath, "%s\\%s", m_szRemoteTransPath, name);

	MakeStatusText("DOWNLOAD:", m_szRemoteTransPath, m_szLocalTransPath, name);

	m_dwFileSize = m_TransferInfo.GetIntSizeAt(0);

	SetDefaultBlockSize();

	m_hFiletoWrite = CreateFile(locPath, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if (m_hFiletoWrite == INVALID_HANDLE_VALUE) {
		//FIXME
	}	
	InitProgressBar(0);
	m_bDownloadStarted = TRUE;
	m_CantTransferInfo.Add(remPath, m_TransferInfo.GetSizeAt(0), m_TransferInfo.GetDataAt(0));
	m_TransferInfo.DeleteAt(0);

	SendFileDownloadRequestMessage(strlen(remPath), remPath);
}


void 
FileTransfer::DownloadFilePortion()
{
	rfbFileDownloadDataMsg fdd;
	m_clientconn->ReadExact((char *)&fdd, sz_rfbFileDownloadDataMsg);
	fdd.realSize = Swap16IfLE(fdd.realSize);
	fdd.compressedSize = Swap16IfLE(fdd.compressedSize);

	if ((fdd.realSize == 0) && (fdd.compressedSize == 0)) {
		unsigned int mTime;
		m_clientconn->ReadExact((char *) &mTime, sizeof(CARD32));
		FILETIME Filetime;
		Time70ToFiletime(mTime, &Filetime);
		SetFileTime(m_hFiletoWrite, &Filetime, &Filetime, &Filetime);
		CloseHandle(m_hFiletoWrite);
		SendMessage(m_hwndProgress, PBM_SETPOS, (WPARAM) 65535, (LPARAM) 0);
		SetDlgItemText(m_hwndFileTransfer, IDC_CURRENTFILEPERCENT, "100%");
		if ((m_dwNumItemsSel > 0) && (strcmp(m_ClientPath, m_szLocalTransPath) == 0)) {
			m_dwNumItemsSel--;
			ShowClientItems(m_ClientPath);
		}
		m_CantTransferInfo.DeleteAt(m_CantTransferInfo.GetNumEntries() - 1);
		CheckDownloadQueue();
		return;
	}
	char * pBuff = new char [fdd.compressedSize];
	DWORD dwNumberOfBytesWritten;
	m_clientconn->ReadExact(pBuff, fdd.compressedSize);
	if (m_hwndFTCanceling != NULL) ProcessDlgMessage(m_hwndFTCanceling);
	ProcessDlgMessage(m_hwndFileTransfer);
	if (m_bTransferEnable == FALSE) {
		char reason [] = "User cancel download";
		unsigned short reasonLen = strlen(reason);
		SendFileDownloadCancelMessage(reasonLen, reason);
		SetStatusText(reason);
		CloseUndoneFileTransfers();		
		SendDlgItemMessage(m_hwndFileTransfer, IDC_FTCANCEL, STM_SETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) m_hIconCancelGray);
		m_bDownloadStarted = FALSE;
		m_dwNumItemsSel = 0;
		return;
	}
	WriteFile(m_hFiletoWrite, pBuff, fdd.compressedSize, &dwNumberOfBytesWritten, NULL);
	IncreaseProgBarPos(dwNumberOfBytesWritten);
	delete [] pBuff;
}

void
FileTransfer::CloseUndoneFileTransfers()
{
  if (m_bUploadStarted) {
    m_bUploadStarted = FALSE;
    CloseHandle(m_hFiletoRead);
  }
  if (m_bDownloadStarted) {
    m_bDownloadStarted = FALSE;
    CloseHandle(m_hFiletoWrite);
    DeleteFile(m_DownloadFilename);
  }
	m_dwNumItemsSel = 0;
	m_TransferInfo.Free();
	m_bTransferEnable = FALSE;
	m_bOverwriteAll = FALSE;
}

void 
FileTransfer::ShowClientItems(char *path)
{
	if (strlen(path) == 0) {
		//Show Logical Drives
		ListView_DeleteAllItems(m_hwndFTClientList);
		m_FTClientItemInfo.Free();
		int LengthDrivesString = 0;
		char DrivesString[256];
		char DriveName[MAX_PATH] = "?:";
		LengthDrivesString = GetLogicalDriveStrings(256, DrivesString);
		if ((LengthDrivesString == 0) || (LengthDrivesString > 256)) {
			strcpy(m_ClientPathTmp, m_ClientPath);
			SetFTDlgCursor(IDC_ARROW);
			return;
		} else {
			strcpy(m_ClientPath, m_ClientPathTmp);
			SetWindowText(m_hwndFTClientPath, m_ClientPath);
			for (int i=0; i<256; i++) {
				DriveName[0] = DrivesString[i];
				switch (GetDriveType(DriveName))
				{
					case DRIVE_REMOVABLE:
					case DRIVE_FIXED:
					case DRIVE_REMOTE:
					case DRIVE_CDROM:
					{
						char txt[16];
						strcpy(txt, m_FTClientItemInfo.folderText);
						m_FTClientItemInfo.Add(DriveName, txt, 0);
						break;
					}
				}
				DriveName[0] = '\0';
				i+=3;
				if ((DrivesString[i] == '\0') && (DrivesString[i+1] == '\0')) break;
			}
			m_FTClientItemInfo.Sort();
			SendDlgItemMessage(m_hwndFileTransfer, IDC_CLIENTPATH, CB_SETCURSEL, (WPARAM) FT_ID_MYCOMPUTER, (LPARAM) 0);
			ShowListViewItems(m_hwndFTClientList, &m_FTClientItemInfo);
			SendDlgItemMessage(m_hwndFileTransfer, IDC_CREATELOCFLD, STM_SETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) m_hIconCreateLocFolderGray);
		}
	} else {
		//Show Files
		HANDLE m_handle;
		int n = 0;
		WIN32_FIND_DATA m_FindFileData;
		strcat(path, "\\*");
		SetErrorMode(SEM_FAILCRITICALERRORS);
		m_handle = FindFirstFile(path, &m_FindFileData);
		DWORD LastError = GetLastError();
		SetErrorMode(0);
		if (m_handle == INVALID_HANDLE_VALUE) {
			if (LastError != ERROR_SUCCESS && LastError != ERROR_FILE_NOT_FOUND) {
				strcpy(m_ClientPathTmp, m_ClientPath);
				FindClose(m_handle);
				SetFTDlgCursor(IDC_ARROW);
				return;
			}
			path[strlen(path) - 2] = '\0';
			strcpy(m_ClientPath, m_ClientPathTmp);
			SetWindowText(m_hwndFTClientPath, m_ClientPath);
			FindClose(m_handle);
			ListView_DeleteAllItems(m_hwndFTClientList);
			SetFTDlgCursor(IDC_ARROW);
			return;
		}
		ListView_DeleteAllItems(m_hwndFTClientList);
		m_FTClientItemInfo.Free();
		char buffer[16];
		while(1) {
			if ((strcmp(m_FindFileData.cFileName, ".") != 0) &&
		       (strcmp(m_FindFileData.cFileName, "..") != 0)) {
				LARGE_INTEGER li;
				li.LowPart = m_FindFileData.ftLastWriteTime.dwLowDateTime;
				li.HighPart = m_FindFileData.ftLastWriteTime.dwHighDateTime;
				li.QuadPart = (li.QuadPart - 116444736000000000) / 10000000;
				if (!(m_FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
					sprintf(buffer, "%d", m_FindFileData.nFileSizeLow);
					m_FTClientItemInfo.Add(m_FindFileData.cFileName, buffer, li.LowPart);
				} else {
					strcpy(buffer, m_FTClientItemInfo.folderText);
					m_FTClientItemInfo.Add(m_FindFileData.cFileName, buffer, li.LowPart);
				}
			}
			if (!FindNextFile(m_handle, &m_FindFileData)) break;
		}
		FindClose(m_handle);
		m_FTClientItemInfo.Sort();
		ShowListViewItems(m_hwndFTClientList, &m_FTClientItemInfo);
		path[strlen(path) - 2] = '\0';
		strcpy(m_ClientPath, m_ClientPathTmp);
		if (!m_bClientRefresh) {
			SetWindowText(m_hwndFTClientPath, m_ClientPath);
		} else {
			m_bClientRefresh = FALSE;
		}
		SendDlgItemMessage(m_hwndFileTransfer, IDC_CREATELOCFLD, STM_SETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) m_hIconCreateLocFolder);
	}
}

void 
FileTransfer::ShowClientSpecFolder(int idFolder)
{
	BOOL bCreate = FALSE;
	char buf[MAX_PATH];
	if (SHGetSpecialFolderPath(NULL, buf, idFolder, bCreate)) {
		strcpy(m_ClientPathTmp, buf);
		ShowClientItems(m_ClientPathTmp);
	} else {
		strcpy(m_ClientPathTmp, m_ClientPath);
		ShowClientItems(m_ClientPathTmp);
	}
}

BOOL CALLBACK 
FileTransfer::FTBrowseDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	FileTransfer *_this = (FileTransfer *) GetWindowLong(hwnd, GWL_USERDATA);
	switch (uMsg)
	{
	case WM_INITDIALOG:
		{
			SetWindowLong(hwnd, GWL_USERDATA, lParam);
			_this = (FileTransfer *) lParam;
			CentreWindow(hwnd);
			_this->m_hwndFTBrowse = hwnd;
			if (_this->m_bServerBrowseRequest) {
				_this->SendFileListRequestMessage("", 0x10, FT_FLR_DEST_BROWSE);
				return TRUE;
			} else {
				TVITEM TVItem;
				TVINSERTSTRUCT tvins; 
				char DrivesString[256];
				char drive[] = "?:";
				int LengthDriveString = GetLogicalDriveStrings(256, DrivesString);
				TVItem.mask = TVIF_CHILDREN | TVIF_TEXT | TVIF_HANDLE;
				for (int i=0; i<LengthDriveString; i++) {
					drive[0] = DrivesString[i];
					switch (GetDriveType(drive))
						case DRIVE_REMOVABLE:
						case DRIVE_FIXED:
						case DRIVE_REMOTE:
						case DRIVE_CDROM:
						{
							TVItem.pszText = drive;
							TVItem.cChildren = 1;
							tvins.item = TVItem;
							tvins.hParent = TreeView_InsertItem(GetDlgItem(hwnd, IDC_FTBROWSETREE), &tvins);
							tvins.item = TVItem;
							TreeView_InsertItem(GetDlgItem(hwnd, IDC_FTBROWSETREE), &tvins);
							tvins.hParent = NULL;
						}
					i += 3;
				}
			}
			return TRUE;
		}
	break;
	case WM_COMMAND:
		{
		switch (LOWORD(wParam))
		{
			case IDC_FTBROWSECANCEL:
				EndDialog(hwnd, TRUE);
				_this->m_bServerBrowseRequest = FALSE;
				return TRUE;
			case IDC_FTBROWSEOK:
				char path[MAX_PATH];
				if (GetWindowText(GetDlgItem(hwnd, IDC_FTBROWSEEDIT), path, MAX_PATH) == 0) {
					EndDialog(hwnd, TRUE);
					_this->m_bServerBrowseRequest = FALSE;
					return TRUE;
				}
				if (_this->m_bServerBrowseRequest) {
					strcpy(_this->m_ServerPathTmp, path);
					EndDialog(hwnd,TRUE);
					_this->m_bServerBrowseRequest = FALSE;
					_this->SendFileListRequestMessage(_this->m_ServerPathTmp, 0, FT_FLR_DEST_MAIN);
					return TRUE;
				} else {
					strcpy(_this->m_ClientPathTmp, path);
					EndDialog(hwnd,TRUE);
					_this->ShowClientItems(_this->m_ClientPathTmp);
				}
				return TRUE;
		}
		}
	break;
	case WM_NOTIFY:
		switch (LOWORD(wParam))
		{
		case IDC_FTBROWSETREE:
			switch (((LPNMHDR) lParam)->code)
			{
			case TVN_SELCHANGED:
				{
					NMTREEVIEW *m_lParam = (NMTREEVIEW *) lParam;
					char path[MAX_PATH];
					_this->GetTVPath(GetDlgItem(hwnd, IDC_FTBROWSETREE), m_lParam->itemNew.hItem, path);
					SetWindowText(GetDlgItem(hwnd, IDC_FTBROWSEEDIT), path);
					return TRUE;
				}
				break;
			case TVN_ITEMEXPANDING:
				{
				NMTREEVIEW *m_lParam = (NMTREEVIEW *) lParam;
				char Path[MAX_PATH];
				if (m_lParam -> action == 2) {
					if (_this->m_bServerBrowseRequest) {
						_this->m_hTreeItem = m_lParam->itemNew.hItem;
						_this->GetTVPath(GetDlgItem(hwnd, IDC_FTBROWSETREE), m_lParam->itemNew.hItem, Path);
						_this->SendFileListRequestMessage(Path, 0x10, FT_FLR_DEST_BROWSE);
						return TRUE;
					} else {
						_this->ShowTreeViewItems(hwnd, m_lParam);
					}
				}
				return TRUE;
				}
			}
			break;
		}
	break;
	case WM_CLOSE:
	case WM_DESTROY:
		EndDialog(hwnd, FALSE);
		_this->m_bServerBrowseRequest = FALSE;
		return TRUE;
	}
	return 0;
}

void 
FileTransfer::CreateFTBrowseDialog(BOOL status)
{
	m_bServerBrowseRequest = status;
	DialogBoxParam(m_pApp->m_instance, MAKEINTRESOURCE(IDD_FTBROWSE_DLG), m_hwndFileTransfer, (DLGPROC) FTBrowseDlgProc, (LONG) this);
}

void 
FileTransfer::GetTVPath(HWND hwnd, HTREEITEM hTItem, char *path)
{
	char szText[MAX_PATH];
	TVITEM _tvi;
	path[0] = '\0';
	do {
		_tvi.mask = TVIF_TEXT | TVIF_HANDLE;
		_tvi.hItem = hTItem;
		_tvi.pszText = szText;
		_tvi.cchTextMax = MAX_PATH;
		TreeView_GetItem(hwnd, &_tvi);
		strcat(path, "\\");
		strcat(path, _tvi.pszText);
		hTItem = TreeView_GetParent(hwnd, hTItem);
	}
	while(hTItem != NULL);
	char path_tmp[MAX_PATH], path_out[MAX_PATH];
	path_tmp[0] = '\0';
	path_out[0] = '\0';
	int len = strlen(path);
	int ii = 0;
	for (int i = (len-1); i>=0; i--) {
		if (path[i] == '\\') {
			StrInvert(path_tmp);
			strcat(path_out, path_tmp);
			strcat(path_out, "\\");
			path_tmp[0] = '\0';
			ii = 0;
		} else {
			path_tmp[ii] = path[i];
			path_tmp[ii+1] = '\0';
			ii++;
		}
	}
	if (path_out[strlen(path_out)-1] == '\\') path_out[strlen(path_out)-1] = '\0';
	strcpy(path, path_out);
}

void
FileTransfer::StrInvert(char str[MAX_PATH])
{
	int len = strlen(str), i;
	char str_out[MAX_PATH];
	str_out[0] = '\0';
	for (i = (len-1); i>=0; i--) str_out[len-i-1] = str[i];
	str_out[len] = '\0';
	strcpy(str, str_out);
}

void 
FileTransfer::ShowTreeViewItems(HWND hwnd, LPNMTREEVIEW m_lParam)
{
	HANDLE m_handle;
	WIN32_FIND_DATA m_FindFileData;
	TVITEM tvi;
	TVINSERTSTRUCT tvins;
	char path[MAX_PATH];
	GetTVPath(GetDlgItem(hwnd, IDC_FTBROWSETREE), m_lParam->itemNew.hItem, path);
	strcat(path, "\\*");
	while (TreeView_GetChild(GetDlgItem(hwnd, IDC_FTBROWSETREE), m_lParam->itemNew.hItem) != NULL) {
		TreeView_DeleteItem(GetDlgItem(hwnd, IDC_FTBROWSETREE), TreeView_GetChild(GetDlgItem(hwnd, IDC_FTBROWSETREE), m_lParam->itemNew.hItem));
	}
	SetErrorMode(SEM_FAILCRITICALERRORS);
	m_handle = FindFirstFile(path, &m_FindFileData);
	SetErrorMode(0);
	if (m_handle == INVALID_HANDLE_VALUE) return;
	while(1) {
		if ((strcmp(m_FindFileData.cFileName, ".") != 0) && 
			(strcmp(m_FindFileData.cFileName, "..") != 0)) {
			if (m_FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {	
				tvi.mask = TVIF_TEXT;
				tvi.pszText = m_FindFileData.cFileName;
				tvins.hParent = m_lParam->itemNew.hItem;
				tvins.item = tvi;
				tvins.hParent = TreeView_InsertItem(GetDlgItem(hwnd, IDC_FTBROWSETREE), &tvins);
				TreeView_InsertItem(GetDlgItem(hwnd, IDC_FTBROWSETREE), &tvins);
			}
		}
		if (!FindNextFile(m_handle, &m_FindFileData)) break;
	}
	FindClose(m_handle);
}

void 
FileTransfer::ProcessDlgMessage(HWND hwnd)
{
	MSG msg;
	while(PeekMessage(&msg, hwnd, 0, 0, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

void
FileTransfer::ProcessFLRFMessage()
{
	rfbFileLastRequestFailedMsg flrf;
	m_clientconn->ReadExact((char *) &flrf, sz_rfbFileLastRequestFailedMsg);
	flrf.reasonLen = Swap16IfLE(flrf.reasonLen);
	flrf.sysError = Swap32IfLE(flrf.sysError);
	char *pReason = new char [flrf.reasonLen + 1];
	m_clientconn->ReadExact(pReason, flrf.reasonLen);
	pReason[flrf.reasonLen] = '\0';
	switch (flrf.typeOfRequest) 
	{
	case rfbFileDownloadRequest:
		CloseHandle(m_hFiletoWrite);
	    DeleteFile(m_DownloadFilename);
		CheckDownloadQueue();
		break;
	case rfbFileListRequest:
	case rfbFileRenameRequest:
	case rfbFileDeleteRequest:
		SetStatusText(pReason);
		strcpy(m_ServerPathTmp, m_ServerPath);
		SendFileListRequestMessage(m_ServerPathTmp, 0, FT_FLR_DEST_MAIN);
		break;
	}
	delete [] pReason;
}

void
FileTransfer::ProcessFSDDMessage()
{
	rfbFileSpecDirDataMsg fsdd;
	m_clientconn->ReadExact((char *)&fsdd, sz_rfbFileSpecDirDataMsg);
	fsdd.specFlags = Swap16IfLE(fsdd.specFlags);
	fsdd.dirNameSize = Swap16IfLE(fsdd.dirNameSize);
	if (fsdd.dirNameSize >= MAX_PATH) return;
	m_clientconn->ReadExact(m_ServerPathTmp, fsdd.dirNameSize);
	m_ServerPathTmp[fsdd.dirNameSize] = '\0';
	ConvertFromNetPath(m_ServerPathTmp);
	SendFileListRequestMessage(m_ServerPathTmp, 0x40, FT_FLR_DEST_MAIN);
}

void
FileTransfer::ProcessFLRMessage()
{
	switch(m_FLRDest) 
	{
	case FT_FLR_DEST_MAIN:
	case FT_FLR_DEST_BROWSE:
		ShowServerItems();
		break;
	case FT_FLR_DEST_UPLOAD:
		ProcessFLRUpload();
		break;
	case FT_FLR_DEST_DOWNLOAD:
		ProcessFLRDownload();
		break;
	case FT_FLR_DEST_DELETE:
		ProcessFLRDelete();
		break;
	}
}

void
FileTransfer::ProcessFLRDelete()
{
	rfbFileListDataMsg fld;
	m_clientconn->ReadExact((char *) &fld, sz_rfbFileListDataMsg);
	if (fld.flags & 0x80) {
		SetFTDlgCursor(IDC_ARROW);
		return;
	}
	fld.numFiles = Swap16IfLE(fld.numFiles);
	fld.dataSize = Swap16IfLE(fld.dataSize);
	fld.compressedSize = Swap16IfLE(fld.compressedSize);
	FTSIZEDATA *pftSD = new FTSIZEDATA[fld.numFiles];
	char *pFilenames = new char[fld.dataSize];
	m_clientconn->ReadExact((char *)pftSD, fld.numFiles * 8);
	m_clientconn->ReadExact(pFilenames, fld.dataSize);
	FileTransferItemInfo ftii;
	CreateItemInfoList(&ftii, pftSD, fld.numFiles, pFilenames, fld.dataSize);
	int num = m_DeleteInfo.GetNumEntries() - 1;
	int numDel = m_dwNumDelFiles + m_dwNumDelFolders + m_dwNumUndel;
	if ((num < 0) || ((numDel) > num)) return;
	if (IsExistName(&ftii, m_DeleteInfo.GetNameAt(numDel)) < 0) {
		SetStatusText("%s\\%s deleted", m_ServerPath, m_DeleteInfo.GetNameAt(numDel));
		if (m_DeleteInfo.GetIntSizeAt(numDel) < 0) {
			m_dwNumDelFolders++;
		} else {
			m_dwNumDelFiles++;
		}
	} else {
		if (m_DeleteInfo.GetIntSizeAt(numDel) < 0) {
			SetStatusText("Folder %s\\%s cannot be deleted.", m_ServerPath, m_DeleteInfo.GetNameAt(numDel));
		} else {
			SetStatusText("File %s\\%s cannot be deleted.", m_ServerPath, m_DeleteInfo.GetNameAt(numDel));
		}
		m_dwNumUndel++;
	}
	if ((numDel + 1) == m_dwNumDelItems) {
		if (m_dwNumUndel == 0) {
			SetStatusText("Delete Operation Successfully Completed. %d folder(s), %d file(s) deleted.", 
						  m_dwNumDelFolders, m_dwNumDelFiles);
		}
		m_dwNumDelFolders = 0; m_dwNumDelFiles = 0; m_dwNumUndel = 0;
		m_DeleteInfo.Free();
		SetFTDlgCursor(IDC_ARROW);
	} 
	m_FTServerItemInfo.Free();
	ListView_DeleteAllItems(m_hwndFTServerList); 
	CreateItemInfoList(&m_FTServerItemInfo, pftSD, fld.numFiles, pFilenames, fld.dataSize);
	delete [] pftSD;
	delete [] pFilenames;
	m_FTServerItemInfo.Sort();
	ShowListViewItems(m_hwndFTServerList, &m_FTServerItemInfo);
}

void
FileTransfer::ProcessFLRUpload()
{
	rfbFileListDataMsg fld;
	m_clientconn->ReadExact((char *) &fld, sz_rfbFileListDataMsg);
	if (fld.flags & 0x80) {
		SetFTDlgCursor(IDC_ARROW);
		return;
	}
	fld.numFiles = Swap16IfLE(fld.numFiles);
	fld.dataSize = Swap16IfLE(fld.dataSize);
	fld.compressedSize = Swap16IfLE(fld.compressedSize);
	FTSIZEDATA *pftSD = new FTSIZEDATA[fld.numFiles];
	char *pFilenames = new char[fld.dataSize];
	m_clientconn->ReadExact((char *)pftSD, fld.numFiles * 8);
	m_clientconn->ReadExact(pFilenames, fld.dataSize);
	FileTransferItemInfo ftii;
	CreateItemInfoList(&ftii, pftSD, fld.numFiles, pFilenames, fld.dataSize);
	delete [] pftSD;
	delete [] pFilenames;
	SetFTDlgCursor(IDC_ARROW);
	char path[MAX_PATH];
	strcpy(path, m_TransferInfo.GetNameAt(0));
	char lastName[MAX_PATH];
	CutLastName(path, lastName);
	int num = IsExistName(&ftii, lastName);
	if (num >= 0) {
		FILETIME ft;
		SYSTEMTIME st;
		Time70ToFiletime(m_TransferInfo.GetDataAt(0), &ft);
		FileTimeToSystemTime(&ft, &st);
		char transFileData[MAX_PATH + 36];
		if (st.wHour > 12) {
			sprintf(transFileData, "%s, modified on %d/%d/%d %d:%d PM", path, 
				st.wDay, st.wMonth, st.wYear, st.wHour - 12, st.wMinute);
		} else {
			sprintf(transFileData, "%s, modified on %d/%d/%d %d:%d AM", path, 
				st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute);
		}
		Time70ToFiletime(ftii.GetDataAt(num), &ft);
		FileTimeToSystemTime(&ft, &st);
		char existFileData[MAX_PATH + 36];
		if (st.wHour > 12) {
			sprintf(existFileData, "%s, modified on %d/%d/%d %d:%d PM", ftii.GetNameAt(num), 
				st.wDay, st.wMonth, st.wYear, st.wHour - 12, st.wMinute);
		} else {
			sprintf(existFileData, "%s, modified on %d/%d/%d %d:%d AM", ftii.GetNameAt(num), 
				st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute);
		}
		char buf[2 * MAX_PATH + 255];
		if (m_TransferInfo.GetIntSizeAt(0) < 0) {
			sprintf(buf, "This folder already exists.\n\nWould you like to replace the existing file\n\n%s,\n\nWith this one?\n\n%s\n",
				existFileData, transFileData);
		} else {
			sprintf(buf, "This file already exists.\n\nWould you like to replace the existing file\n\n%s,\n\nWith this one?\n\n%s\n",
				existFileData, transFileData);
		}
		if (!CreateFTConfirmDlg(buf)) {
			int size = m_TransferInfo.GetIntSizeAt(0);
			if (size < 0) {
				sprintf(path, "%s\\%s", m_szRemoteTransPath, m_TransferInfo.GetNameAt(0));
				SendFileDirSizeRequestMessage(strlen(path), path, FT_FDSR_DEST_UPLOAD);
				return;
			} else {
				m_dwSelFileSize -= size;
			}
			m_TransferInfo.DeleteAt(0);
			if (m_TransferInfo.GetNumEntries() == 0) {
				SendFileListRequestMessage(m_ServerPath, 0, FT_FLR_DEST_MAIN);
				CheckCantTransferInfo();
				return;
			}
			CheckUploadQueue();
			return;
		}
	}
	if (m_TransferInfo.GetIntSizeAt(0) < 0) {
		MakeUploadQueue();
		sprintf(path, "%s\\%s", m_szRemoteTransPath, m_TransferInfo.GetNameAt(0));
		SendFileCreateDirRequestMessage(strlen(path), path);
		m_TransferInfo.DeleteAt(0);
		if (m_TransferInfo.GetNumEntries() == 0) {
			CheckCantTransferInfo();
			SendFileListRequestMessage(m_ServerPath, 0, FT_FLR_DEST_MAIN);
			return;
		}
		CheckUploadQueue();
	} else {
		if (!UploadFile(0)) PostMessage(m_hwndFileTransfer, WM_CHECKUPLOADQUEUE, (WPARAM) 0, (LPARAM) 0);
	}
}

void
FileTransfer::ProcessFLRDownload()
{
	rfbFileListDataMsg fld;
	m_clientconn->ReadExact((char *) &fld, sz_rfbFileListDataMsg);
	if (fld.flags & 0x80) {
		SetFTDlgCursor(IDC_ARROW);
		return;
	}
	fld.numFiles = Swap16IfLE(fld.numFiles);
	fld.dataSize = Swap16IfLE(fld.dataSize);
	fld.compressedSize = Swap16IfLE(fld.compressedSize);
	FTSIZEDATA *pftSD = new FTSIZEDATA[fld.numFiles];
	char *pFilenames = new char[fld.dataSize];
	m_clientconn->ReadExact((char *)pftSD, fld.numFiles * 8);
	m_clientconn->ReadExact(pFilenames, fld.dataSize);
	CreateItemInfoList(&m_TransferInfo, pftSD, fld.numFiles, pFilenames, fld.dataSize);
	delete [] pftSD;
	delete [] pFilenames;
	SetFTDlgCursor(IDC_ARROW);
	CheckDownloadQueue();
}

void 
FileTransfer::ShowServerItems()
{
	rfbFileListDataMsg fld;
	m_clientconn->ReadExact((char *) &fld, sz_rfbFileListDataMsg);
	if ((fld.flags & 0x80) && !m_bServerBrowseRequest) {
		SetFTDlgCursor(IDC_ARROW);
		return;
	}
	fld.numFiles = Swap16IfLE(fld.numFiles);
	fld.dataSize = Swap16IfLE(fld.dataSize);
	fld.compressedSize = Swap16IfLE(fld.compressedSize);
	FTSIZEDATA *pftSD = new FTSIZEDATA[fld.numFiles];
	char *pFilenames = new char[fld.dataSize];
	m_clientconn->ReadExact((char *)pftSD, fld.numFiles * 8);
	m_clientconn->ReadExact(pFilenames, fld.dataSize);
	if (!m_bServerBrowseRequest) {
		m_FTServerItemInfo.Free();
		ListView_DeleteAllItems(m_hwndFTServerList); 
		if (fld.numFiles != 0) {
			CreateItemInfoList(&m_FTServerItemInfo, pftSD, fld.numFiles, pFilenames, fld.dataSize);
			m_FTServerItemInfo.Sort();
			ShowListViewItems(m_hwndFTServerList, &m_FTServerItemInfo);
		} else {
			SendDlgItemMessage(m_hwndFileTransfer, IDC_CREATEREMFLD, STM_SETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) m_hIconCreateRemFolderGray);
		}
		strcpy(m_ServerPath, m_ServerPathTmp);
		if (!m_bServerRefresh) {
			if (strlen(m_ServerPath) == 0) {
				SetWindowText(m_hwndFTServerPath, myComputerText);
			} else {
				if (!(fld.flags & 0x40))
					SetWindowText(m_hwndFTServerPath, m_ServerPath);
			}
		} else {
			m_bServerRefresh = FALSE;
		}
		CheckServerLV();
	} else {
		while (TreeView_GetChild(GetDlgItem(m_hwndFTBrowse, IDC_FTBROWSETREE), m_hTreeItem) != NULL) {
			TreeView_DeleteItem(GetDlgItem(m_hwndFTBrowse, IDC_FTBROWSETREE), TreeView_GetChild(GetDlgItem(m_hwndFTBrowse, IDC_FTBROWSETREE), m_hTreeItem));
		}
		TVITEM TVItem;
		TVINSERTSTRUCT tvins; 
		TVItem.mask = TVIF_CHILDREN | TVIF_TEXT | TVIF_HANDLE;
		TVItem.cChildren = 1;
		int pos = 0;
		for (int i = 0; i < fld.numFiles; i++) {
			if (pftSD[i].size == -1) {
				TVItem.pszText = pFilenames + pos;
				TVItem.cChildren = 1;
				tvins.item = TVItem;
				tvins.hParent = m_hTreeItem;
				tvins.hParent = TreeView_InsertItem(GetDlgItem(m_hwndFTBrowse, IDC_FTBROWSETREE), &tvins);
				tvins.item = TVItem;
				TreeView_InsertItem(GetDlgItem(m_hwndFTBrowse, IDC_FTBROWSETREE), &tvins);
				tvins.hParent = m_hTreeItem;;
			}
			pos += strlen(pFilenames + pos) + 1;
		}
	}
	delete [] pftSD;
	delete [] pFilenames;
	SetFTDlgCursor(IDC_ARROW);
}

void 
FileTransfer::SendFileListRequestMessage(char *filename, unsigned char flags, int dest)
{
	SetFTDlgCursor(IDC_WAIT);
	m_FLRDest = dest;
	char _filename[MAX_PATH];
	strcpy(_filename, filename);
	int len = strlen(_filename);
	if (_filename[len-1] == '\\') _filename[len-1] = '\0';
	ConvertPath(_filename);
	len = strlen(_filename);
	rfbFileListRequestMsg flr;
	flr.type = rfbFileListRequest;
	flr.dirNameSize = Swap16IfLE(len);
	flr.flags = flags;
	m_clientconn->WriteExact((char *)&flr, sz_rfbFileListRequestMsg);
	m_clientconn->WriteExact(_filename, len);
}

void 
FileTransfer::SendFileDirSizeRequestMessage(unsigned short pathLen, char *path, int dest)
{
	m_FDSRDest = dest;
	char _path[MAX_PATH];
	strcpy(_path, path);
	int len = strlen(_path);
	if (_path[len-1] == '\\') _path[len-1] = '\0';
	ConvertPath(_path);
	len = strlen(_path);
	rfbFileDirSizeRequestMsg fdsr;
	fdsr.type = rfbFileDirSizeRequest;
	fdsr.dNameLen = Swap16IfLE(len);
	m_clientconn->WriteExact((char *)&fdsr, sz_rfbFileDirSizeRequestMsg);
	m_clientconn->WriteExact(_path, len);
}

void
FileTransfer::SendFileRenameRequestMessage(char *pOldName, char *pNewName)
{
	char *_pOldName = strdup(pOldName);
	char *_pNewName = strdup(pNewName);
	ConvertPath(_pOldName);
	ConvertPath(_pNewName);
	CARD16 oldNameLen = strlen(_pOldName);
	CARD16 newNameLen = strlen(_pNewName);
	int msgLen = sz_rfbFileRenameRequestMsg + oldNameLen + newNameLen;
	char *pAllFRRMessage = new char[msgLen];
	rfbFileRenameRequestMsg *pFRR = (rfbFileRenameRequestMsg *) pAllFRRMessage;
	char *pFollow1 = &pAllFRRMessage[sz_rfbFileRenameRequestMsg];
	char *pFollow2 = &pAllFRRMessage[sz_rfbFileRenameRequestMsg + oldNameLen];
	pFRR->type = rfbFileRenameRequest;
	pFRR->oldNameLen = Swap16IfLE(oldNameLen);
	pFRR->newNameLen = Swap16IfLE(newNameLen);
	memcpy(pFollow1, _pOldName, oldNameLen);
	memcpy(pFollow2, _pNewName, newNameLen);
	m_clientconn->WriteExact(pAllFRRMessage, msgLen);
	delete [] pAllFRRMessage;
}

void 
FileTransfer::SendFileSpecDirRequestMessage(unsigned char flags, unsigned short specFlags)
{
	rfbFileSpecDirRequestMsg fsdr;
	fsdr.type = rfbFileSpecDirRequest;
	fsdr.flags = flags;
	fsdr.specFlags = Swap16IfLE(specFlags);
	m_clientconn->WriteExact((char *)&fsdr, sz_rfbFileSpecDirRequestMsg);
}

void 
FileTransfer::ProcessListViewDBLCLK(HWND hwnd, char *Path, char *PathTmp, int iItem)
{
	strcpy(PathTmp, Path);
	char buffer[MAX_PATH];
	char buffer_tmp[16];
	ListView_GetItemText(hwnd, iItem, 0, buffer, MAX_PATH);
	ListView_GetItemText(hwnd, iItem, 1, buffer_tmp, 16);
	if (strcmp(buffer_tmp, m_FTClientItemInfo.folderText) == 0) { 
		if (strlen(PathTmp) >= 2) {
			strcat(PathTmp, "\\");
		}
		strcat(PathTmp, buffer);
		if (strcmp(buffer, myDocumentsText) == 0) {
			if (hwnd == m_hwndFTClientList) strcpy(PathTmp, m_szLocalMyDocPath);
			if (hwnd == m_hwndFTServerList) strcpy(PathTmp, m_szRemoteMyDocPath);
			strcat(PathTmp, "\\");
		}
		if (hwnd == m_hwndFTClientList) ShowClientItems(PathTmp);
		if (hwnd == m_hwndFTServerList) SendFileListRequestMessage(PathTmp, 0, FT_FLR_DEST_MAIN);
	}
}

void
FileTransfer::ConvertPath(char *path)
{
	int len = strlen(path);
	if (len >= MAX_PATH) return;
	if (strcmp(path, "") == 0) {strcpy(path, "/"); return;}
	for (int i = (len - 1); i >= 0; i--) {
		if (path[i] == '\\') path[i] = '/';
		path[i+1] = path[i];
	}
	path[len + 1] = '\0';
	path[0] = '/';
	return;
}

void 
FileTransfer::ConvertFromNetPath(char *path)
{
	int len = strlen(path);
	if(len >= MAX_PATH) return;
	if((path[0] == '/') && (len == 1)) {path[0] = '\0'; return;}
	for(int i = 0; i < (len - 1); i++) {
		if(path[i+1] == '/') path[i+1] = '\\';
		path[i] = path[i+1];
	}
	path[len-1] = '\0';
}

void 
FileTransfer::ShowListViewItems(HWND hwnd, FileTransferItemInfo *ftii)
{
	LVITEM LVItem;
	LVItem.mask = LVIF_TEXT | LVIF_STATE | LVIF_IMAGE; 
	LVItem.state = 0; 
	LVItem.stateMask = 0; 
	for (int i=0; i<ftii->GetNumEntries(); i++) {
		LVItem.iItem = i;
		LVItem.iSubItem = 0;
		LVItem.iImage = I_IMAGECALLBACK;
		LVItem.pszText = LPSTR_TEXTCALLBACK;
		ListView_InsertItem(hwnd, &LVItem);
	}
}

void
FileTransfer::FTInsertColumn(HWND hwnd, char *iText, int iOrder, int xWidth, int alignFmt)
{
	LVCOLUMN lvc; 
	lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM | LVCF_ORDER;
	lvc.fmt = alignFmt;
	lvc.iSubItem = iOrder;
	lvc.pszText = iText;	
	lvc.cchTextMax = 10;
	lvc.cx = xWidth;
	lvc.iOrder = iOrder;
	ListView_InsertColumn(hwnd, iOrder, &lvc);
}

void 
FileTransfer::InitProgressBar(int nPosition)
{
	m_dwProgBarValue = 0;
	m_dwProgBarPercent = 0;
	SendMessage(m_hwndProgress, PBM_SETPOS, (WPARAM) nPosition, (LPARAM) 0);
	SendMessage(m_hwndProgress, PBM_SETRANGE, (WPARAM) 0, MAKELPARAM(0, 65535)); 
	SetDlgItemText(m_hwndFileTransfer, IDC_CURRENTFILEPERCENT, "0%");
}

void 
FileTransfer::InitFTProgressBar(int nPosition)
{
	m_dwFTProgBarValue = 0;
	m_dwFTProgBarPercent = 0;
	SendMessage(m_hwndFTProgress, PBM_SETPOS, (WPARAM) nPosition, (LPARAM) 0);
	SendMessage(m_hwndFTProgress, PBM_SETRANGE, (WPARAM) 0, MAKELPARAM(0, 65535)); 
	SetDlgItemText(m_hwndFileTransfer, IDC_FILETRANSFERPERCENT, "0%");
}

void
FileTransfer::IncreaseProgBarPos(int pos)
{
	DWORD oldPosition = (DWORD) ((((float) m_dwProgBarValue) / m_dwFileSize) * 65535);
	DWORD oldFTPosition = (DWORD) ((((float) m_dwFTProgBarValue) / m_dwSelFileSize) * 65535);
	m_dwProgBarValue += pos;
	m_dwFTProgBarValue += pos;
	if (m_dwProgBarValue > m_dwFileSize) m_dwProgBarValue = m_dwFileSize;
	if (m_dwFTProgBarValue > m_dwSelFileSize) m_dwFTProgBarValue = m_dwSelFileSize;
	DWORD newPosition = (DWORD) ((((float) m_dwProgBarValue) / m_dwFileSize) * 65535);
	DWORD newFTPosition = (DWORD) ((((float) m_dwFTProgBarValue) / m_dwSelFileSize) * 65535);
	if (newPosition != oldPosition)	SendMessage(m_hwndProgress, PBM_SETPOS, (WPARAM) newPosition, (LPARAM) 0);
	if (newFTPosition != oldFTPosition) SendMessage(m_hwndFTProgress, PBM_SETPOS, (WPARAM) newFTPosition, (LPARAM) 0);
	DWORD percent = (DWORD) (((float) m_dwProgBarValue / m_dwFileSize) * 100);
	if (percent != m_dwProgBarPercent) {
		char buf[5];
		sprintf(buf, "%d%%", percent);
		SetDlgItemText(m_hwndFileTransfer, IDC_CURRENTFILEPERCENT, buf);
		m_dwProgBarPercent = percent;
	}
	percent = (DWORD) (((float) m_dwFTProgBarValue / m_dwSelFileSize) * 100);
	if (percent != m_dwFTProgBarPercent) {
		char buf[5];
		sprintf(buf, "%d%%", percent);
		SetDlgItemText(m_hwndFileTransfer, IDC_FILETRANSFERPERCENT, buf);
		m_dwFTProgBarPercent = percent;
	}
}

void 
FileTransfer::CreateItemInfoList(FileTransferItemInfo *pftii, 
								 FTSIZEDATA *ftsd, int ftsdNum,
								 char *pfnames, int fnamesSize)
{
	int pos = 0;
	int size = 0;
	for (int i = 0; i < ftsdNum; i++) {
		char buf[16];
		ftsd[i].size = Swap32IfLE(ftsd[i].size);
		ftsd[i].data = Swap32IfLE(ftsd[i].data);
		size = ftsd[i].size;
		if (size < 0) {
			strcpy(buf, FileTransferItemInfo::folderText);
		} else {
			sprintf(buf, "%d", size);
		}
		if (m_FLRDest == FT_FLR_DEST_DOWNLOAD) {
			char fullName[MAX_PATH];
			sprintf(fullName, "%s\\%s", m_szLastRelTransPath, pfnames + pos);
			pftii->Add(fullName, buf, ftsd[i].data);
		} else {
			pftii->Add(pfnames + pos, buf, ftsd[i].data);
		}
		pos += strlen(pfnames + pos) + 1;
	}
}

void 
FileTransfer::SendFileUploadDataMessage(unsigned int mTime)
{
	rfbFileUploadDataMsg msg;
	msg.type = rfbFileUploadData;
	msg.compressedLevel = 0;
	msg.realSize = Swap16IfLE(0);
	msg.compressedSize = Swap16IfLE(0);
	
	CARD32 time32 = Swap32IfLE((CARD32)mTime);
	
	char data[sz_rfbFileUploadDataMsg + sizeof(CARD32)];
	memcpy(data, &msg, sz_rfbFileUploadDataMsg);
	memcpy(&data[sz_rfbFileUploadDataMsg], &time32, sizeof(CARD32));
	
	m_clientconn->WriteExact(data, sz_rfbFileUploadDataMsg + sizeof(CARD32));
}

void 
FileTransfer::SendFileUploadDataMessage(unsigned short size, char *pFile)
{
	int msgLen = sz_rfbFileUploadDataMsg + size;
	char *pAllFUDMessage = new char[msgLen];
	rfbFileUploadDataMsg *pFUD = (rfbFileUploadDataMsg *) pAllFUDMessage;
	char *pFollow = &pAllFUDMessage[sz_rfbFileUploadDataMsg];
	pFUD->type = rfbFileUploadData;
	pFUD->compressedLevel = 0;
	pFUD->realSize = Swap16IfLE(size);
	pFUD->compressedSize = Swap16IfLE(size);
	memcpy(pFollow, pFile, size);
	m_clientconn->WriteExact(pAllFUDMessage, msgLen);
	delete [] pAllFUDMessage;
}

void
FileTransfer::SendFileDownloadRequestMessage(unsigned short dNameLen, char *dName)
{
	char dirName[MAX_PATH];
	strcpy(dirName, dName);
	int len = strlen(dirName);
	if (dirName[len-1] == '\\') dirName[len-1] = '\0';
	ConvertPath(dirName);
	len = strlen(dirName);
	
	int msgLen = sz_rfbFileDownloadRequestMsg + len;
	char *pAllFDRMessage = new char[msgLen];
	rfbFileDownloadRequestMsg *pFDR = (rfbFileDownloadRequestMsg *) pAllFDRMessage;
	char *pFollow = &pAllFDRMessage[sz_rfbFileDownloadRequestMsg];
	pFDR->type = rfbFileDownloadRequest;
	pFDR->compressedLevel = 0;
	pFDR->fNameSize = Swap16IfLE(len);
	pFDR->position = 0;
	memcpy(pFollow, dirName, len);
	m_clientconn->WriteExact(pAllFDRMessage, msgLen);
	delete [] pAllFDRMessage;
}

void 
FileTransfer::SendFileDownloadCancelMessage(unsigned short reasonLen, char *reason)
{
	int msgLen = sz_rfbFileDownloadCancelMsg + reasonLen;
	char *pAllFDCMessage = new char[msgLen];
	rfbFileDownloadCancelMsg *pFDC = (rfbFileDownloadCancelMsg *) pAllFDCMessage;
	char *pFollow = &pAllFDCMessage[sz_rfbFileDownloadCancelMsg];
	pFDC->type = rfbFileDownloadCancel;
	pFDC->reasonLen = Swap16IfLE(reasonLen);
	memcpy(pFollow, reason, reasonLen);
	m_clientconn->WriteExact(pAllFDCMessage, msgLen);
	delete [] pAllFDCMessage;
}

void 
FileTransfer::SendFileCreateDirRequestMessage(unsigned short dNameLen, char *dName)
{
	char _filename[MAX_PATH];
	strcpy(_filename, dName);
	int len = strlen(_filename);
	if (_filename[len-1] == '\\') _filename[len-1] = '\0';
	ConvertPath(_filename);
	len = strlen(_filename);
	
	int msgLen = sz_rfbFileCreateDirRequestMsg + len;
	char *pAllCDRMessage = new char[msgLen];
	rfbFileCreateDirRequestMsg *pCDR = (rfbFileCreateDirRequestMsg *) pAllCDRMessage;
	char *pFollow = &pAllCDRMessage[sz_rfbFileCreateDirRequestMsg];
	pCDR->type = rfbFileCreateDirRequest;
	pCDR->dNameLen = Swap16IfLE(len);
	memcpy(pFollow, _filename, len);
	m_clientconn->WriteExact(pAllCDRMessage, msgLen);
	delete [] pAllCDRMessage;
}

unsigned int 
FileTransfer::FiletimeToTime70(FILETIME ftime)
{
	LARGE_INTEGER uli;
	uli.LowPart = ftime.dwLowDateTime;
	uli.HighPart = ftime.dwHighDateTime;
	uli.QuadPart = (uli.QuadPart - 116444736000000000) / 10000000;
	return uli.LowPart;
}

void 
FileTransfer::Time70ToFiletime(unsigned int time70, FILETIME *pftime)
{
    LONGLONG ll = Int32x32To64(time70, 10000000) + 116444736000000000;
    pftime->dwLowDateTime = (DWORD) ll;
    pftime->dwHighDateTime = ll >> 32;
}

void 
FileTransfer::SetIcon(HWND hwnd, int dest, int idIcon)
{
	HANDLE hIcon = LoadImage(m_pApp->m_instance, MAKEINTRESOURCE(idIcon), IMAGE_ICON, 16, 16, LR_SHARED);
	SendMessage(GetDlgItem(hwnd, dest), BM_SETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) hIcon);
	DestroyIcon((HICON) hIcon);
}

void 
FileTransfer::ClientCreateDir()
{
	if (strlen(m_ClientPath) == 0) {
		SetStatusText("Illegal local path. Directory is not created.");
	} else {
		if (DialogBoxParam(m_pApp->m_instance, MAKEINTRESOURCE(IDD_FTDIRNAME), m_hwndFileTransfer, (DLGPROC) FTCreateDirDlgProc, (LONG) this)) {
			char dirName[MAX_PATH];
			sprintf(dirName, "%s\\%s", m_ClientPath, m_szCreateDirName);
			CreateDirectory(dirName, NULL);
			m_bClientRefresh = TRUE;
			ShowClientItems(m_ClientPath);
		}
	}
}
void 
FileTransfer::ServerCreateDir()
{
	if (strlen(m_ServerPath) == 0) {
		SetStatusText("Illegal remote path. Directory is not created.");
	} else {
		if (DialogBoxParam(m_pApp->m_instance, MAKEINTRESOURCE(IDD_FTDIRNAME), m_hwndFileTransfer, (DLGPROC) FTCreateDirDlgProc, (LONG) this)) {
			char dirName[MAX_PATH];
			sprintf(dirName, "%s\\%s", m_ServerPath, m_szCreateDirName);
			m_bServerRefresh = TRUE;
			SendFileCreateDirRequestMessage(strlen(dirName), (char *) dirName);
			SendFileListRequestMessage(m_ServerPath, 0, FT_FLR_DEST_MAIN);
		}
	}
}

void
FileTransfer::ClientDeleteDir()
{
	FileTransferItemInfo ftfi;
	int numSel = GetSelectedItems(m_hwndFTClientList, &ftfi);
	if (numSel == 1) {
		if (ftfi.GetIntSizeAt(0) < 0) {
			char buf[MAX_PATH + 38];
			sprintf(buf, "Are you sure you want to delete '%s' and all its contents?", ftfi.GetNameAt(0));
			if (MessageBox(m_hwndFileTransfer, 
				_T(buf),
				_T("Confirm Folder Delete"),
				MB_YESNO | MB_ICONQUESTION) == IDYES) {
				FTClientDelete(&ftfi);
			}
		} else {
			char buf[MAX_PATH + 38];
			sprintf(buf, "Are you sure you want to delete file '%s'?", ftfi.GetNameAt(0));
			if (MessageBox(m_hwndFileTransfer, 
				_T(buf),
				_T("Confirm File Delete"),
				MB_YESNO | MB_ICONQUESTION) == IDYES) {
				FTClientDelete(&ftfi);
			}
		}
		return;
	} else {
		if (MessageBox(m_hwndFileTransfer, 
			_T("Are you sure you want to delete all selected files and folders?"),
			_T("Confirm Multiple Delete"),
			MB_YESNO | MB_ICONQUESTION) == IDYES) {
			FTClientDelete(&ftfi);
		}
	}
}

void
FileTransfer::FTClientDelete(FileTransferItemInfo *ftfi)
{
	char buff[MAX_PATH];
	ftfi->Sort();
	int numDir = ftfi->GetNumEntries();
	int cantDel = 0;
	int i = numDir - 1;
	while (ftfi->GetIntSizeAt(i) >= 0) {
			sprintf(buff, "%s\\%s", m_ClientPath, ftfi->GetNameAt(i));
			if (!DeleteFile(buff)) {
				SetStatusText("File %s cannot be deleted.", ftfi->GetNameAt(i));
				cantDel++;
			}
			ftfi->DeleteAt(i);
			i -= 1;
			if (i < 0) {
				m_bClientRefresh = TRUE;
				ShowClientItems(m_ClientPath);
				return;
			}
	}
	int numFiles = numDir - i - 1 - cantDel;
	numDir -= numFiles;
	if (ftfi->GetNumEntries() == 0) return;
	char fullPath[MAX_PATH];
	FileTransferItemInfo delDirInfo;
	while (ftfi->GetNumEntries() > 0) {
		sprintf(fullPath, "%s\\%s", m_ClientPath, ftfi->GetNameAt(0));
		delDirInfo.Add(fullPath, -1, 0);
		strcat(fullPath, "\\*");
		WIN32_FIND_DATA FindFileData;
		SetErrorMode(SEM_FAILCRITICALERRORS);
		HANDLE hFile = FindFirstFile(fullPath, &FindFileData);
		SetErrorMode(0);
		if (hFile != INVALID_HANDLE_VALUE) {
			do {
				if (strcmp(FindFileData.cFileName, ".") != 0 &&
					strcmp(FindFileData.cFileName, "..") != 0) {
					char buff[MAX_PATH];
					if ((FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {	
						sprintf(buff, "%s\\%s", ftfi->GetNameAt(0), FindFileData.cFileName);
						ftfi->Add(buff, -1, 0);
					} else {
						sprintf(buff, "%s\\%s\\%s", m_ClientPath, ftfi->GetNameAt(0), FindFileData.cFileName);
						if (!DeleteFile(buff)) {
							SetStatusText("File %s cannot be deleted.", buff);
							cantDel++;
						}

					}
				}
			} while (FindNextFile(hFile, &FindFileData));
		}			
		FindClose(hFile);
		ftfi->DeleteAt(0);
	}
	for (i = delDirInfo.GetNumEntries() - 1; i >= 0; i--) {
		if (!RemoveDirectory(delDirInfo.GetNameAt(i))) {
			SetStatusText("Folder %s cannot be deleted.", delDirInfo.GetNameAt(i));
			cantDel++;
		}
	}
	m_bClientRefresh = TRUE;
	ShowClientItems(m_ClientPath);
	CheckClientLV();
	if (cantDel == 0) {
		SetStatusText("Delete Operation Successfully Completed. %d folder(s), %d file(s) deleted.", numDir, numFiles);
	}
}

void
FileTransfer::ServerDeleteDir()
{
	int result = IDNO;
	m_dwNumDelItems = GetSelectedItems(m_hwndFTServerList, &m_DeleteInfo);
	if (m_dwNumDelItems == 1) {
		if (m_DeleteInfo.GetIntSizeAt(0) < 0) {
			char buf[MAX_PATH + 57];
			sprintf(buf, "Are you sure you want to delete '%s' and all its contents?", m_DeleteInfo.GetNameAt(0));
			result = MessageBox(m_hwndFileTransfer, 
								_T("Are you sure you want to delete all selected files and folders?"),
								_T("Confirm Folder Delete"),
								MB_YESNO | MB_ICONQUESTION);
		} else {
			char buf[MAX_PATH + 38];
			sprintf(buf, "Are you sure you want to delete file '%s'?", m_DeleteInfo.GetNameAt(0));
			result = MessageBox(m_hwndFileTransfer, 
								_T(buf),
								_T("Confirm File Delete"),
								MB_YESNO | MB_ICONQUESTION);
		}
	} else {
		result = MessageBox(m_hwndFileTransfer, 
							_T("Are you sure you want to delete all selected files and folders?"),
							_T("Confirm Multiple Delete"),
							MB_YESNO | MB_ICONQUESTION);
	}
	if (result == IDYES) {
		char delPath[MAX_PATH];
		m_dwNumDelFolders = 0; m_dwNumDelFiles = 0; m_dwNumUndel = 0;
		SetFTDlgCursor(IDC_WAIT);
		for (int i = 0; i < m_DeleteInfo.GetNumEntries(); i++) {
			sprintf(delPath, "%s\\%s", m_ServerPath, m_DeleteInfo.GetNameAt(i));
			SendFileDeleteRequestMessage(delPath);
			SendFileListRequestMessage(m_ServerPath, 0, FT_FLR_DEST_DELETE);
		}
	}
}

void
FileTransfer::SendFileDeleteRequestMessage(char *path)
{
	char _path[MAX_PATH];
	strcpy(_path, path);
	int len = strlen(_path);
	if (_path[len-1] == '\\') _path[len-1] = '\0';
	ConvertPath(_path);
	len = strlen(_path);
	rfbFileDeleteRequestMsg fdr;
	fdr.type = rfbFileDeleteRequest;
	fdr.nameLen = Swap16IfLE(len);
	m_clientconn->WriteExact((char *)&fdr, sz_rfbFileDeleteRequestMsg);
	m_clientconn->WriteExact(_path, len);
}

void
FileTransfer::ClientRenameDir()
{
	if (CreateRenameDirDlg(m_hwndFTClientList)) {
		char selItemText[MAX_PATH];
		int selItem = ListView_GetSelectionMark(m_hwndFTClientList);
		ListView_GetItemText(m_hwndFTClientList, selItem, 0, selItemText, MAX_PATH);
		char oldName[MAX_PATH];
		char newName[MAX_PATH];
		sprintf(oldName, "%s\\%s", m_ClientPath, selItemText);
		sprintf(newName, "%s\\%s", m_ClientPath, m_szCreateDirName);
		if (MoveFile(oldName, newName)) {
			m_bClientRefresh = TRUE;
			ShowClientItems(m_ClientPath);
		} else {
			SetStatusText("Access Denied. File Cannot be renamed");
		}
	}
}

void
FileTransfer::ServerRenameDir()
{
	if (CreateRenameDirDlg(m_hwndFTServerList)) {
		char selItemText[MAX_PATH];
		int selItem = ListView_GetSelectionMark(m_hwndFTServerList);
		ListView_GetItemText(m_hwndFTServerList, selItem, 0, selItemText, MAX_PATH);
		char oldName[MAX_PATH];
		char newName[MAX_PATH];
		sprintf(oldName, "%s\\%s", m_ServerPath, selItemText);
		sprintf(newName, "%s\\%s", m_ServerPath, m_szCreateDirName);
		m_bServerRefresh = TRUE;
		SendFileRenameRequestMessage(oldName, newName);
		SendFileListRequestMessage(m_ServerPath, 0, FT_FLR_DEST_MAIN);
	}
}

BOOL
FileTransfer::CreateRenameDirDlg(HWND hwnd)
{
	char selItemText0[MAX_PATH];
	char selItemText1[MAX_PATH];
	int selItem = ListView_GetSelectionMark(hwnd);
	ListView_GetItemText(hwnd, selItem, 0, selItemText0, MAX_PATH);
	ListView_GetItemText(hwnd, selItem, 1, selItemText1, MAX_PATH);
	if (strcmp(selItemText1, FileTransferItemInfo::folderText) == 0) {
		strcpy(m_szRenameDlgText1, "Rename Folder");
		sprintf(m_szRenameDlgText2, "Rename Folder '%s'", selItemText0);
	} else {
		strcpy(m_szRenameDlgText1, "Rename File");
		sprintf(m_szRenameDlgText2, "Rename File '%s'", selItemText0);
	}
	strcpy(m_szRenameDlgText3, selItemText0);
	if (DialogBoxParam(m_pApp->m_instance, 
					   MAKEINTRESOURCE(IDD_FTDIRNAME), 
		               m_hwndFileTransfer, 
					   (DLGPROC) FTRenameDirDlgProc, 
					   (LONG) this)) {
		return TRUE;
	} else {
		return FALSE;
	}
}

int 
FileTransfer::GetSelectedItems(HWND hwnd, FileTransferItemInfo *pFTII)
{
	int selCount = ListView_GetSelectedCount(hwnd);
	int selItem = ListView_GetSelectionMark(hwnd);
	if ((selCount < 1) || (selItem < 0)) return -1;
	
	selItem = -1;
	selItem = ListView_GetNextItem(hwnd, selItem, LVNI_SELECTED);
	do {
		if (hwnd == m_hwndFTClientList) pFTII->Add(m_FTClientItemInfo.GetNameAt(selItem),
			m_FTClientItemInfo.GetSizeAt(selItem),
			m_FTClientItemInfo.GetDataAt(selItem));
		if (hwnd == m_hwndFTServerList) pFTII->Add(m_FTServerItemInfo.GetNameAt(selItem),
			m_FTServerItemInfo.GetSizeAt(selItem),
			m_FTServerItemInfo.GetDataAt(selItem));
		selItem = ListView_GetNextItem(hwnd, selItem, LVNI_SELECTED);
	} while (selItem >= 0);
	
	return selCount;
}


BOOL CALLBACK 
FileTransfer::FTCreateDirDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	FileTransfer *_this = (FileTransfer *) GetWindowLong(hwnd, GWL_USERDATA);
	switch (uMsg)
	{
	case WM_INITDIALOG:
		SetWindowLong(hwnd, GWL_USERDATA, lParam);
		SetFocus(GetDlgItem(hwnd, IDC_EDITDIRNAME));
		return TRUE;
		break;
	case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
			case IDOK:
				GetWindowText(GetDlgItem(hwnd, IDC_EDITDIRNAME), _this->m_szCreateDirName, MAX_PATH);
				EndDialog(hwnd, TRUE);
				return TRUE;
			case IDCANCEL:
				EndDialog(hwnd, FALSE);
				return TRUE;
			}
		}
		break;
	case WM_CLOSE:
	case WM_DESTROY:
		EndDialog(hwnd, FALSE);
		return TRUE;
	}
	return FALSE;
}

BOOL CALLBACK 
FileTransfer::FTRenameDirDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	FileTransfer *_this = (FileTransfer *) GetWindowLong(hwnd, GWL_USERDATA);
	switch (uMsg)
	{
	case WM_INITDIALOG:
		SetWindowLong(hwnd, GWL_USERDATA, lParam);
		SetWindowText(hwnd, (LPCTSTR)((FileTransfer *)lParam)->m_szRenameDlgText1);
		SetDlgItemText(hwnd, IDC_MAINTEXT, (LPCTSTR)((FileTransfer *)lParam)->m_szRenameDlgText2);
		SetDlgItemText(hwnd, IDC_EDITDIRNAME, (LPCTSTR)((FileTransfer *)lParam)->m_szRenameDlgText3);
		SendMessage(GetDlgItem(hwnd, IDC_EDITDIRNAME), EM_SETSEL, (WPARAM) 0, (LPARAM) -1);
		SetFocus(GetDlgItem(hwnd, IDC_EDITDIRNAME));
		CentreWindow(hwnd);
		((FileTransfer *)lParam)->m_szRenameDlgText1[0] = '\0';
		((FileTransfer *)lParam)->m_szRenameDlgText2[0] = '\0';
		((FileTransfer *)lParam)->m_szRenameDlgText3[0] = '\0';
		return TRUE;
		break;
	case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
			case IDOK:
				GetWindowText(GetDlgItem(hwnd, IDC_EDITDIRNAME), _this->m_szCreateDirName, MAX_PATH);
				EndDialog(hwnd, TRUE);
				return TRUE;
			case IDCANCEL:
				EndDialog(hwnd, FALSE);
				return TRUE;
			}
		}
		break;
	case WM_CLOSE:
	case WM_DESTROY:
		EndDialog(hwnd, FALSE);
		return TRUE;
	}
	return FALSE;
}

void 
FileTransfer::SetStatusText(LPCSTR format,...)
{
	char text[1024];
	va_list args;
	va_start(args, format);
	int nSize = _vsnprintf(text, sizeof(text), format, args);
	SetWindowText(m_hwndFTStatus, text);
}

void 
FileTransfer::MakeStatusText(char *prefix, char *path1, char *path2, char *name)
{
	int length = strlen(prefix) + strlen(path1) + strlen(path2) + 2 * strlen(name) + 7;
	if (length > 125) {
		char buf[MAX_PATH];
		char _name[MAX_PATH]; strcpy(_name, name);
		char _path1[MAX_PATH]; strcpy(_path1, path1);
		char _path2[MAX_PATH]; strcpy(_path2, path2);
		strcpy(buf, name);
		for (int i = strlen(buf) - 2; i > 0; i--) {
			if (buf[i] == '\\') {
				strcpy(_name, &buf[i + 1]);
				buf[i] = '\0';
				sprintf(_path1, "%s\\%s", _path1, buf);
				sprintf(_path2, "%s\\%s", _path2, buf);
				break;
			}
		}
		char *path;
		do {
			if (strlen(_path1) > strlen(_path2)) {
				path = _path1;
			} else {
				path = _path2;
			}
			buf[0] = '\0';
			for (int i = strlen(path) - 1, j = 0; i > 0; i--, j++) {
				if ((path[i] == '\\') && (strcmp(buf, delimeter) != 0)) {
					path[i + 1] = '\0';
					strcat(path, delimeter);
					break;
				}
				buf[j] = path[i];
				buf[j + 1] = '\0';
			}
			length = strlen(prefix) + strlen(_path1) + strlen(_path2) + 2 * strlen(_name) + 13;
			if ((strlen(_path1) < 7) && (strlen(_path2) < 7)) break;
		} while(length > 125);
		SetStatusText("%s %s\\%s to %s\\%s", prefix, _path1, _name, _path2, _name);
	} else {
		SetStatusText("%s %s\\%s to %s\\%s", prefix, path1, name, path2, name);
	}
}

DWORD
FileTransfer::GetSelectedFileSize(char *path, FileTransferItemInfo *pFTFI)
{
	DWORD dwSelFileSize = 0;
	FileTransferItemInfo ftfi;
	for (int i = 0; i < pFTFI->GetNumEntries(); i++) {
		int size = pFTFI->GetIntSizeAt(i);
		if (size >= 0) {
			dwSelFileSize += size;
		} else {
			ftfi.Add(pFTFI->GetNameAt(i), size, 0);
		}
	}
	char fullPath[MAX_PATH];
	while (ftfi.GetNumEntries() > 0) {
		sprintf(fullPath, "%s\\%s\\*", path, ftfi.GetNameAt(0));
		WIN32_FIND_DATA FindFileData;
		SetErrorMode(SEM_FAILCRITICALERRORS);
		HANDLE hFile = FindFirstFile(fullPath, &FindFileData);
		SetErrorMode(0);

		if (hFile != INVALID_HANDLE_VALUE) {
			do {
				if (strcmp(FindFileData.cFileName, ".") != 0 &&
					strcmp(FindFileData.cFileName, "..") != 0) {
					char buff[MAX_PATH];
					if ((FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {	
						sprintf(buff, "%s\\%s", ftfi.GetNameAt(0), FindFileData.cFileName);
						ftfi.Add(buff, -1, 0);
					} else {
						dwSelFileSize += FindFileData.nFileSizeLow;
					}
				}
			} while (FindNextFile(hFile, &FindFileData));
		}			
		FindClose(hFile);
		ftfi.DeleteAt(0);
	}
	return dwSelFileSize;
}

void
FileTransfer::ClearFTButtons()
{
	SendDlgItemMessage(m_hwndFileTransfer, IDC_FTCOPY, STM_SETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) m_hIconCopyGray);
	SendDlgItemMessage(m_hwndFileTransfer, IDC_FTDELETE, STM_SETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) m_hIconDeleteGray);
	SendDlgItemMessage(m_hwndFileTransfer, IDC_FTRENAME, STM_SETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) m_hIconRenameGray);
}

void
FileTransfer::ClearFTControls()
{
	if (strcmp(m_ServerPath, "") == 0) 
		SendDlgItemMessage(m_hwndFileTransfer, IDC_CREATEREMFLD, STM_SETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) m_hIconCreateRemFolderGray);
	ClearFTButtons();
	SetDlgItemText(m_hwndFileTransfer, IDC_FILETRANSFERPERCENT, "0%");
	SetDlgItemText(m_hwndFileTransfer, IDC_CURRENTFILEPERCENT, "0%");
	SendMessage(m_hwndProgress, PBM_SETPOS, 0, 0);
	SendMessage(m_hwndFTProgress, PBM_SETPOS, 0, 0);
	ClearStatusText();
}

void
FileTransfer::CheckClientLV()
{
	HANDLE hIcon = (HANDLE) SendDlgItemMessage(m_hwndFileTransfer, IDC_FTCOPY, STM_GETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) 0);
	int selCount = ListView_GetSelectedCount(m_hwndFTClientList);
	if (strlen(m_ClientPath) != 0) {
		SendDlgItemMessage(m_hwndFileTransfer, IDC_CREATELOCFLD, STM_SETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) m_hIconCreateLocFolder);
		if ((GetFocus() == m_hwndFTClientList) && (selCount > 0)) {
			SendDlgItemMessage(m_hwndFileTransfer, IDC_FTDELETE, STM_SETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) m_hIconDelete);
			if (selCount == 1) {
				SendDlgItemMessage(m_hwndFileTransfer, IDC_FTRENAME, STM_SETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) m_hIconRename);
			} else {
				SendDlgItemMessage(m_hwndFileTransfer, IDC_FTRENAME, STM_SETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) m_hIconRenameGray);
			}
		}
	} else {
		SendDlgItemMessage(m_hwndFileTransfer, IDC_CREATELOCFLD, STM_SETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) m_hIconCreateLocFolderGray);
		SendDlgItemMessage(m_hwndFileTransfer, IDC_FTDELETE, STM_SETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) m_hIconDeleteGray);
		SendDlgItemMessage(m_hwndFileTransfer, IDC_FTRENAME, STM_SETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) m_hIconRenameGray);
	}
	if (selCount <= 0) {
		if (hIcon != m_hIconCopyGray) {
			SendDlgItemMessage(m_hwndFileTransfer, IDC_FTCOPY, STM_SETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) m_hIconCopyGray);
		}
		SendDlgItemMessage(m_hwndFileTransfer, IDC_FTDELETE, STM_SETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) m_hIconDeleteGray);
		SendDlgItemMessage(m_hwndFileTransfer, IDC_FTRENAME, STM_SETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) m_hIconRenameGray);
	}
}

void
FileTransfer::CheckServerLV()
{
	HANDLE hIcon = (HANDLE) SendDlgItemMessage(m_hwndFileTransfer, IDC_FTCOPY, STM_GETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) 0);
	int selCount = ListView_GetSelectedCount(m_hwndFTServerList);
	if (strlen(m_ServerPath) != 0) {
		SendDlgItemMessage(m_hwndFileTransfer, IDC_CREATEREMFLD, STM_SETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) m_hIconCreateRemFolder);
		if ((GetFocus() == m_hwndFTServerList) && (selCount > 0)) {
			SendDlgItemMessage(m_hwndFileTransfer, IDC_FTDELETE, STM_SETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) m_hIconDelete);
			if (selCount == 1) {
				SendDlgItemMessage(m_hwndFileTransfer, IDC_FTRENAME, STM_SETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) m_hIconRename);
			} else {
				SendDlgItemMessage(m_hwndFileTransfer, IDC_FTRENAME, STM_SETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) m_hIconRenameGray);
			}
		}
	} else {
		SendDlgItemMessage(m_hwndFileTransfer, IDC_CREATEREMFLD, STM_SETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) m_hIconCreateRemFolderGray);
		SendDlgItemMessage(m_hwndFileTransfer, IDC_FTDELETE, STM_SETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) m_hIconDeleteGray);
		SendDlgItemMessage(m_hwndFileTransfer, IDC_FTRENAME, STM_SETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) m_hIconRenameGray);
	}
	if (selCount <= 0) {
		if (hIcon != m_hIconCopyGray) {
			SendDlgItemMessage(m_hwndFileTransfer, IDC_FTCOPY, STM_SETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) m_hIconCopyGray);
		}
		SendDlgItemMessage(m_hwndFileTransfer, IDC_FTDELETE, STM_SETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) m_hIconDeleteGray);
		SendDlgItemMessage(m_hwndFileTransfer, IDC_FTRENAME, STM_SETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) m_hIconRenameGray);
	}
}

BOOL
FileTransfer::CreateTransferConfDlg()
{
	char *pBuf = new char [(m_dwNumItemsSel + 3) * MAX_PATH];
	if (m_bUploadStarted) {
		sprintf(pBuf, "From: Local Computer %s\n\n", m_ClientPath);
	} else {
		if (m_bDownloadStarted) {
			sprintf(pBuf, "From: Remote Computer %s\n\n", m_ServerPath);
		} else {
			delete [] pBuf;
			return FALSE;
		}
	}
	for (int i = 0; i < m_TransferInfo.GetNumEntries(); i++) {
		sprintf(pBuf, "%s  %s,", pBuf, m_TransferInfo.GetNameAt(i));
	}
	if (m_bUploadStarted) {
		sprintf(pBuf, "%s \n\nTo: Remote Computer %s", pBuf, m_ServerPath);
	} else {
		if (m_bDownloadStarted) {
			sprintf(pBuf, "%s \n\nTo: Local Computer %s", pBuf, m_ClientPath);
		} else {
			delete [] pBuf;
			return FALSE;
		}
	}
	if (MessageBox(m_hwndFileTransfer, pBuf, "Copy Selected Files and Folders", MB_OKCANCEL) == IDCANCEL) {
		m_bTransferEnable = FALSE;
		m_bUploadStarted = FALSE;
		m_bDownloadStarted = FALSE;
		SendDlgItemMessage(m_hwndFileTransfer, IDC_FTCANCEL, STM_SETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) m_hIconCancelGray);
		m_TransferInfo.Free();
		delete [] pBuf;
		return FALSE;
	}
	delete [] pBuf;
	return TRUE;
}

BOOL 
FileTransfer::CreateFTCancelingDlg()
{
	SendDlgItemMessage(m_hwndFileTransfer, IDC_FTCANCEL, STM_SETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) m_hIconCancelGray);
	m_hwndFTCanceling = CreateDialogParam(m_pApp->m_instance, 
										  MAKEINTRESOURCE(IDD_FTCANCELING),
										  NULL, 
										  (DLGPROC) FTCancelingDlgProc,
										  (LONG) this);
	ShowWindow(m_hwndFTCanceling, SW_SHOW);
	DrawIcon(GetDC(m_hwndFTCanceling), 15, 22, LoadIcon(NULL, IDI_QUESTION));
	UpdateWindow(m_hwndFTCanceling);
	return TRUE;
}

BOOL CALLBACK 
FileTransfer::FTCancelingDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	FileTransfer *_this = (FileTransfer *) GetWindowLong(hwnd, GWL_USERDATA);
	switch (uMsg)
	{
	case WM_INITDIALOG:
		{
			SetWindowLong(hwnd, GWL_USERDATA, (LONG) lParam);
			SetForegroundWindow(hwnd);
			CentreWindow(hwnd);
		}
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
			case IDOK:
				_this->ClearFTButtons();
				_this->m_bTransferEnable = FALSE;
				SendDlgItemMessage(_this->m_hwndFileTransfer, IDC_FTCANCEL, STM_SETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) _this->m_hIconCancelGray);
				_this->EndFTCancelDlg(TRUE);
				return TRUE;
			case IDCANCEL:
				_this->EndFTCancelDlg(FALSE);
				return TRUE;
		}
	break;
	case WM_CLOSE:
	case WM_DESTROY:
		_this->EndFTCancelDlg(FALSE);
		return TRUE;
	}
	return FALSE;
}

void
FileTransfer::EndFTCancelDlg(BOOL result)
{
	EndDialog(m_hwndFTCanceling, result);
	m_hwndFTCanceling = NULL;
	if (result == FALSE)
		SendDlgItemMessage(m_hwndFileTransfer, IDC_FTCANCEL, STM_SETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) m_hIconCancel);
	if ((result) && (m_bEndFTDlgOnYes)) {
		CloseUndoneFileTransfers();
		m_clientconn->m_fileTransferDialogShown = false;
		m_FTClientItemInfo.Free();
		m_FTServerItemInfo.Free();
		EndDialog(m_hwndFileTransfer, TRUE);
	}
}

void
FileTransfer::SetFTDlgCursor(LPCTSTR cursorType)
{
	HCURSOR hCurs = LoadCursor(NULL, cursorType);
	SetClassLong(m_hwndFileTransfer, GCL_HCURSOR, (LONG) hCurs);   
	SetClassLong(m_hwndFTClientList,GCL_HCURSOR, (LONG) hCurs);   
	SetClassLong(m_hwndFTClientPath, GCL_HCURSOR, (LONG) hCurs);   
	SetClassLong(m_hwndProgress, GCL_HCURSOR, (LONG) hCurs);   
//	SetClassLong(m_hwndFTProgress, GCL_HCURSOR, (LONG) hCurs);   
	SetClassLong(m_hwndFTStatus, GCL_HCURSOR, (LONG) hCurs);   
//	SetClassLong(m_hwndFTServerList, GCL_HCURSOR, (LONG) hCurs);   
//	SetClassLong(m_hwndFTServerPath, GCL_HCURSOR, (LONG) hCurs);   
	SetClassLong(GetDlgItem(m_hwndFileTransfer, IDC_CLIENTUP), GCL_HCURSOR, (LONG) hCurs);   
}

BOOL 
FileTransfer::CreateFTConfirmDlg(char *pText)
{
	INT_PTR result = DialogBoxParam(m_pApp->m_instance, 
									MAKEINTRESOURCE(IDD_FTCONFIRM),
									m_hwndFileTransfer, 
									(DLGPROC) FTConfirmDlgProc,
									(LONG) pText);
	switch (result)
	{
	case IDOK:
		return TRUE;
	case IDCANCEL:
		return FALSE;
	case ID_FTYESTOALL:
		m_bOverwriteAll = TRUE;
		return TRUE;
	}
	return FALSE;
}

INT_PTR CALLBACK 
FileTransfer::FTConfirmDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		{
			SetForegroundWindow(hwnd);
			CentreWindow(hwnd);
			SetWindowText(GetDlgItem(hwnd, IDC_FTCONFIRMTEXT), (LPCTSTR) lParam);
		}
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
			case ID_FTYESTOALL:
				EndDialog(hwnd, ID_FTYESTOALL);
				return TRUE;
			case IDOK:
				EndDialog(hwnd, IDOK);
				return TRUE;
			case IDCANCEL:
				EndDialog(hwnd, IDCANCEL);
				return TRUE;
		}
	break;
	case WM_CLOSE:
	case WM_DESTROY:
		EndDialog(hwnd, IDCANCEL);
		return TRUE;
	}
	return FALSE;
}

int
FileTransfer::IsExistName(FileTransferItemInfo *ftii, char *name)
{
	for (int i = 0; i < ftii->GetNumEntries(); i++) {
		if (strcmp(ftii->GetNameAt(i), name) == 0) {
			return i;
		}
	}
	return -1;
}

void
FileTransfer::CutLastName(char *path, char *lastName)
{
	char buf[MAX_PATH];
	int _len = strlen(path) - 1;
	for (int i = _len; i >= 0; i--)
	{
		if (path[i] == '\\') {
			path[i] = '\0';
			buf[_len - i] = '\0';
			break;
		} else {
			buf[_len - i] = path[i];
		}
		buf[_len + 1] = '\0';
	}
	_len = strlen(buf) - 1;
	for (i = _len; i >= 0; i--)
	{
		lastName[_len - i] = buf[i];
	}
	lastName[_len + 1] = '\0';
}

void
FileTransfer::CheckCantTransferInfo()
{
	int numEntries = m_CantTransferInfo.GetNumEntries();
	if (numEntries != 0) {
		char *pText = new char [numEntries * (MAX_PATH + 28) + 48];
		sprintf(pText, "Access denied. Cannot copy:");
		for (int i = 0; i < numEntries; i++) {
			sprintf(pText, "%s\n%s;", pText, m_CantTransferInfo.GetNameAt(i));
		}
		SetEndTransfer("Copy Operation Ended");
		MessageBox(NULL, pText, "Access denied. Cannot copy files.", MB_OK);
		delete [] pText;
	} else {
		SetEndTransfer("Copy Operation Successfully Completed");
	}
}

void
FileTransfer::InitFTIcons()
{
	SetIcon(m_hwndFileTransfer, IDC_CLIENTUP, IDI_FILEUP);
	SetIcon(m_hwndFileTransfer, IDC_SERVERUP, IDI_FILEUP);
	SetIcon(m_hwndFileTransfer, IDC_CLIENTRELOAD, IDI_FILERELOAD);
	SetIcon(m_hwndFileTransfer, IDC_SERVERRELOAD, IDI_FILERELOAD);

    m_hImageList = ImageList_Create(16, 16, ILC_MASK, 2, 2); 

    HICON hiconItem = LoadIcon(m_pApp->m_instance, MAKEINTRESOURCE(IDI_FTDIR)); 
    ImageList_AddIcon(m_hImageList, hiconItem);  
    DestroyIcon(hiconItem); 

    hiconItem = LoadIcon(m_pApp->m_instance, MAKEINTRESOURCE(IDI_FTFILE)); 
    ImageList_AddIcon(m_hImageList, hiconItem); 
    DestroyIcon(hiconItem); 

    ListView_SetImageList(m_hwndFTClientList, m_hImageList, LVSIL_SMALL); 
    ListView_SetImageList(m_hwndFTServerList, m_hImageList, LVSIL_SMALL); 

	m_hIconUpload   = LoadImage(m_pApp->m_instance, MAKEINTRESOURCE(IDI_UPLOAD), IMAGE_ICON, 150, 13, LR_DEFAULTCOLOR);
	m_hIconDownload = LoadImage(m_pApp->m_instance, MAKEINTRESOURCE(IDI_DOWNLOAD), IMAGE_ICON, 150, 13, LR_DEFAULTCOLOR);
	m_hIconCopyGray = LoadImage(m_pApp->m_instance, MAKEINTRESOURCE(IDI_COPYGRAY), IMAGE_ICON, 150, 13, LR_DEFAULTCOLOR);

	m_hIconDelete     = LoadImage(m_pApp->m_instance, MAKEINTRESOURCE(IDI_DELETE), IMAGE_ICON, 150, 13, LR_DEFAULTCOLOR);
	m_hIconDeleteGray = LoadImage(m_pApp->m_instance, MAKEINTRESOURCE(IDI_DELETEGRAY), IMAGE_ICON, 150, 13, LR_DEFAULTCOLOR);

	m_hIconRename     = LoadImage(m_pApp->m_instance, MAKEINTRESOURCE(IDI_RENAME), IMAGE_ICON, 150, 13, LR_DEFAULTCOLOR);
	m_hIconRenameGray = LoadImage(m_pApp->m_instance, MAKEINTRESOURCE(IDI_RENAMEGRAY), IMAGE_ICON, 150, 13, LR_DEFAULTCOLOR);

	m_hIconCancel     = LoadImage(m_pApp->m_instance, MAKEINTRESOURCE(IDI_CANCELFT), IMAGE_ICON, 150, 13, LR_DEFAULTCOLOR);
	m_hIconCancelGray = LoadImage(m_pApp->m_instance, MAKEINTRESOURCE(IDI_CANCELFTGRAY), IMAGE_ICON, 150, 13, LR_DEFAULTCOLOR);

	m_hIconCreateLocFolder     = LoadImage(m_pApp->m_instance, MAKEINTRESOURCE(IDI_CREATELOCALFOLDER), IMAGE_ICON, 150, 13, LR_DEFAULTCOLOR);
	m_hIconCreateLocFolderGray = LoadImage(m_pApp->m_instance, MAKEINTRESOURCE(IDI_CREATELOCFOLDERGRAY), IMAGE_ICON, 150, 13, LR_DEFAULTCOLOR);
	m_hIconCreateRemFolder     = LoadImage(m_pApp->m_instance, MAKEINTRESOURCE(IDI_CREATEREMOTEFOLDER), IMAGE_ICON, 150, 13, LR_DEFAULTCOLOR);
	m_hIconCreateRemFolderGray = LoadImage(m_pApp->m_instance, MAKEINTRESOURCE(IDI_CREATEREMFOLDERGRAY), IMAGE_ICON, 150, 13, LR_DEFAULTCOLOR);

	SendDlgItemMessage(m_hwndFileTransfer, IDC_FTCOPY, STM_SETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) m_hIconCopyGray);
	SendDlgItemMessage(m_hwndFileTransfer, IDC_FTDELETE, STM_SETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) m_hIconDeleteGray);
	SendDlgItemMessage(m_hwndFileTransfer, IDC_FTRENAME, STM_SETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) m_hIconRenameGray);
	SendDlgItemMessage(m_hwndFileTransfer, IDC_FTCANCEL, STM_SETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) m_hIconCancelGray);

	SendDlgItemMessage(m_hwndFileTransfer, IDC_CREATELOCFLD, STM_SETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) m_hIconCreateLocFolder);
	SendDlgItemMessage(m_hwndFileTransfer, IDC_CREATEREMFLD, STM_SETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) m_hIconCreateRemFolder);
}

void
FileTransfer::DestroyFTIcons()
{
	DestroyIcon((HICON) m_hIconCopyGray);
	DestroyIcon((HICON) m_hIconUpload);
	DestroyIcon((HICON) m_hIconDownload);
	DestroyIcon((HICON) m_hIconDelete);
	DestroyIcon((HICON) m_hIconDeleteGray);
	DestroyIcon((HICON) m_hIconRename);
	DestroyIcon((HICON) m_hIconRenameGray);
	DestroyIcon((HICON) m_hIconCreateLocFolder);
	DestroyIcon((HICON) m_hIconCreateLocFolderGray);
	DestroyIcon((HICON) m_hIconCreateRemFolder);
	DestroyIcon((HICON) m_hIconCreateRemFolderGray);
}

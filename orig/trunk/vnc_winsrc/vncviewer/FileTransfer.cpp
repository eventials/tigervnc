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

const char FileTransfer::uploadText[] = ">>>";
const char FileTransfer::downloadText[] = "<<<";
const char FileTransfer::noactionText[] = "<--->";

FileTransfer::FileTransfer(ClientConnection * pCC, VNCviewerApp * pApp)
{
	m_clientconn = pCC;
	m_pApp = pApp;
	m_TransferEnable = FALSE;
	m_bServerBrowseRequest = FALSE;
	m_bFirstFileDownloadMsg = TRUE;
	m_hTreeItem = NULL;
	m_ClientPath[0] = '\0';
	m_ClientPathTmp[0] = '\0';
	m_ServerPath[0] = '\0';
	m_ServerPathTmp[0] = '\0';
}

FileTransfer::~FileTransfer()
{
	m_FTClientItemInfo.Free();
	m_FTServerItemInfo.Free();
}
void 
FileTransfer::CreateFileTransferDialog()
{
	m_hwndFileTransfer = CreateDialog(m_pApp->m_instance, 
									  MAKEINTRESOURCE(IDD_FILETRANSFER_DLG),
									  NULL, 
									  (DLGPROC) FileTransferDlgProc); 
	ShowWindow(m_hwndFileTransfer, SW_SHOW);
	UpdateWindow(m_hwndFileTransfer);
	SetWindowLong(m_hwndFileTransfer, GWL_USERDATA, (LONG) this);

	m_hwndFTProgress = GetDlgItem(m_hwndFileTransfer, IDC_FTPROGRESS);
	m_hwndFTClientList = GetDlgItem(m_hwndFileTransfer, IDC_FTCLIENTLIST);
	m_hwndFTServerList = GetDlgItem(m_hwndFileTransfer, IDC_FTSERVERLIST);
	m_hwndFTClientPath = GetDlgItem(m_hwndFileTransfer, IDC_CLIENTPATH);
	m_hwndFTServerPath = GetDlgItem(m_hwndFileTransfer, IDC_SERVERPATH);
	m_hwndFTStatus = GetDlgItem(m_hwndFileTransfer, IDC_FTSTATUS);

	ListView_SetExtendedListViewStyleEx(m_hwndFTClientList, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);
	ListView_SetExtendedListViewStyleEx(m_hwndFTServerList, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);

	HANDLE hIcon = LoadImage(m_pApp->m_instance, MAKEINTRESOURCE(IDI_FILEUP), IMAGE_ICON, 16, 16, LR_SHARED);
	SendMessage(GetDlgItem(m_hwndFileTransfer, IDC_CLIENTUP), BM_SETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) hIcon);
	SendMessage(GetDlgItem(m_hwndFileTransfer, IDC_SERVERUP), BM_SETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) hIcon);
	DestroyIcon((HICON) hIcon);
	hIcon = LoadImage(m_pApp->m_instance, MAKEINTRESOURCE(IDI_FILERELOAD), IMAGE_ICON, 16, 16, LR_SHARED);
	SendMessage(GetDlgItem(m_hwndFileTransfer, IDC_CLIENTRELOAD), BM_SETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) hIcon);
	SendMessage(GetDlgItem(m_hwndFileTransfer, IDC_SERVERRELOAD), BM_SETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) hIcon);
	DestroyIcon((HICON) hIcon);

	RECT Rect;
	GetClientRect(m_hwndFTClientList, &Rect);
	int xwidth = (int) (0.7 * Rect.right);
	int xwidth_ = (int) (0.25 * Rect.right);

	FTInsertColumn(m_hwndFTClientList, "Name", 0, xwidth);
	FTInsertColumn(m_hwndFTClientList, "Size", 1, xwidth_);
	FTInsertColumn(m_hwndFTServerList, "Name", 0, xwidth);
	FTInsertColumn(m_hwndFTServerList, "Size", 1, xwidth_);

	ShowClientItems(m_ClientPathTmp);
	SendFileListRequestMessage(m_ServerPathTmp, 0);
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

	case WM_COMMAND:
		{
		switch (LOWORD(wParam))
		{
			case IDC_CLIENTPATH:
				switch (HIWORD (wParam))
				{
					case EN_SETFOCUS:
						SetWindowText(GetDlgItem(hwnd, IDC_FTCOPY), noactionText);
						EnableWindow(GetDlgItem(hwnd, IDC_FTCOPY), FALSE);
						return TRUE;
				}
			break;
			case IDC_SERVERPATH:
				switch (HIWORD (wParam))
				{
					case EN_SETFOCUS:
						SetWindowText(GetDlgItem(hwnd, IDC_FTCOPY), noactionText);
						EnableWindow(GetDlgItem(hwnd, IDC_FTCOPY), FALSE);
						return TRUE;
				}
			break;
			case IDC_EXIT:
				_this->m_clientconn->m_fileTransferDialogShown = false;
				_this->m_FTClientItemInfo.Free();
				_this->m_FTServerItemInfo.Free();
				EndDialog(hwnd, TRUE);
				return TRUE;
			case IDC_CLIENTUP:
				SetWindowText(GetDlgItem(hwnd, IDC_FTCOPY), noactionText);
				EnableWindow(GetDlgItem(hwnd, IDC_FTCOPY), FALSE);
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
				SetWindowText(GetDlgItem(hwnd, IDC_FTCOPY), noactionText);
				EnableWindow(GetDlgItem(hwnd, IDC_FTCOPY), FALSE);
				if (strcmp(_this->m_ServerPathTmp, "") == 0) return TRUE;
				for (i=(strlen(_this->m_ServerPathTmp)-2); i>=0; i--) {
					if (_this->m_ServerPathTmp[i] == '\\') {
						_this->m_ServerPathTmp[i] = '\0';
						break;
					}
					if (i == 0) _this->m_ServerPathTmp[0] = '\0';
				}
				_this->SendFileListRequestMessage(_this->m_ServerPathTmp, 0);
				return TRUE;
			case IDC_CLIENTRELOAD:
				SetWindowText(GetDlgItem(hwnd, IDC_FTCOPY), noactionText);
				EnableWindow(GetDlgItem(hwnd, IDC_FTCOPY), FALSE);
				_this->ShowClientItems(_this->m_ClientPath);
				return TRUE;
			case IDC_SERVERRELOAD:
				SetWindowText(GetDlgItem(hwnd, IDC_FTCOPY), noactionText);
				EnableWindow(GetDlgItem(hwnd, IDC_FTCOPY), FALSE);
				_this->SendFileListRequestMessage(_this->m_ServerPathTmp, 0);
				return TRUE;
			case IDC_FTCOPY:
				SetWindowText(GetDlgItem(hwnd, IDC_FTCOPY), noactionText);
				EnableWindow(GetDlgItem(hwnd, IDC_FTCOPY), FALSE);
				if (_this->m_bFTCOPY == FALSE) {
					if (strcmp(_this->m_ServerPath, "") == 0) {
						char buffer[rfbMAX_PATH];
						sprintf(buffer, "Unacceptable TightVNC Server path. Upload is missing");
						SetWindowText(_this->m_hwndFTStatus, buffer);
						return TRUE;
					}
					_this->m_TransferEnable = TRUE;
					EnableWindow(GetDlgItem(hwnd, IDC_FTCANCEL), TRUE);
					_this->FileTransferUpload();			
				} else {
					char buffer[rfbMAX_PATH + rfbMAX_PATH + rfbMAX_PATH];
					if (strcmp(_this->m_ClientPath, "") == 0) {
						sprintf(buffer, "Unacceptable Local Computer path. Download is missing");
						SetWindowText(_this->m_hwndFTStatus, buffer);
						return TRUE;
					}
					char path[rfbMAX_PATH + rfbMAX_PATH];
					_this->m_TransferEnable = TRUE;
					EnableWindow(GetDlgItem(hwnd, IDC_FTCANCEL), TRUE);
					_this->BlockingFileTransferDialog(FALSE);
					ListView_GetItemText(_this->m_hwndFTServerList, ListView_GetSelectionMark(_this->m_hwndFTServerList), 0, _this->m_ServerFilename, rfbMAX_PATH);
					strcpy(_this->m_ClientFilename, _this->m_ServerFilename);
					sprintf(buffer, "DOWNLOAD: %s\\%s to %s\\%s", _this->m_ServerPath, _this->m_ServerFilename, _this->m_ClientPath, _this->m_ClientFilename);
					SetWindowText(_this->m_hwndFTStatus, buffer);
					_this->m_sizeDownloadFile = _this->m_FTServerItemInfo.GetIntSizeAt(ListView_GetSelectionMark(_this->m_hwndFTServerList));
					rfbFileDownloadRequestMsg fdr;
					fdr.type = rfbFileDownloadRequest;
					fdr.compressedLevel = 0;
					fdr.position = Swap32IfLE(0);
					sprintf(path, "%s\\%s", _this->m_ServerPath, _this->m_ServerFilename);
					_this->ConvertPath(path);
					int len = strlen(path);
					fdr.fNameSize = Swap16IfLE(len);
					_this->m_clientconn->WriteExact((char *)&fdr, sz_rfbFileDownloadRequestMsg);
					_this->m_clientconn->WriteExact(path, len);
				}
				return TRUE;
			case IDC_FTCANCEL:
				SetWindowText(GetDlgItem(hwnd, IDC_FTCOPY), noactionText);
				EnableWindow(GetDlgItem(hwnd, IDC_FTCOPY), FALSE);
				_this->m_TransferEnable = FALSE;
				EnableWindow(GetDlgItem(hwnd, IDC_FTCANCEL), FALSE);
				return TRUE;
			case IDC_CLIENTBROWSE_BUT:
				SetWindowText(GetDlgItem(hwnd, IDC_FTCOPY), noactionText);
				EnableWindow(GetDlgItem(hwnd, IDC_FTCOPY), FALSE);
				_this->CreateFTBrowseDialog(FALSE);
				return TRUE;
			case IDC_SERVERBROWSE_BUT:
				SetWindowText(GetDlgItem(hwnd, IDC_FTCOPY), noactionText);
				EnableWindow(GetDlgItem(hwnd, IDC_FTCOPY), FALSE);
				_this->CreateFTBrowseDialog(TRUE);
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
				case NM_SETFOCUS:
					SetWindowText(GetDlgItem(hwnd, IDC_FTCOPY), uploadText);
					EnableWindow(GetDlgItem(hwnd, IDC_FTCOPY), TRUE);
					_this->m_bFTCOPY = FALSE;
					return TRUE;
				case LVN_GETDISPINFO:
					_this->OnGetDispClientInfo((NMLVDISPINFO *) lParam); 
					return TRUE;
				case LVN_ITEMACTIVATE:
					LPNMITEMACTIVATE lpnmia = (LPNMITEMACTIVATE)lParam;
					_this->ProcessListViewDBLCLK(_this->m_hwndFTClientList, _this->m_ClientPath, _this->m_ClientPathTmp, lpnmia->iItem);
					return TRUE;
			}
		break;
		case IDC_FTSERVERLIST:
			switch (((LPNMHDR) lParam)->code)
			{
				case NM_SETFOCUS:
					SetWindowText(GetDlgItem(hwnd, IDC_FTCOPY), downloadText);
					EnableWindow(GetDlgItem(hwnd, IDC_FTCOPY), TRUE);
					_this->m_bFTCOPY = TRUE;
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
	case WM_CLOSE:
	case WM_DESTROY:
		_this->m_clientconn->m_fileTransferDialogShown = false;
		_this->m_FTClientItemInfo.Free();
		_this->m_FTServerItemInfo.Free();
		EndDialog(hwnd, TRUE);
		return TRUE;
	}
	return 0;
}


void 
FileTransfer::OnGetDispClientInfo(NMLVDISPINFO *plvdi) 
{
  switch (plvdi->item.iSubItem)
    {
    case 0:
		plvdi->item.pszText = m_FTClientItemInfo.GetNameAt(plvdi->item.iItem);
      break;
    case 1:
		plvdi->item.pszText = m_FTClientItemInfo.GetSizeAt(plvdi->item.iItem);
      break;
    default:
      break;
    }
 } 

void 
FileTransfer::OnGetDispServerInfo(NMLVDISPINFO *plvdi) 
{
  switch (plvdi->item.iSubItem)
  {
    case 0:
		plvdi->item.pszText = m_FTServerItemInfo.GetNameAt(plvdi->item.iItem);
      break;
    case 1:
		plvdi->item.pszText = m_FTServerItemInfo.GetSizeAt(plvdi->item.iItem);
      break;
    default:
      break;
    }
 } 

void 
FileTransfer::FileTransferUpload()
{
	DWORD sz_rfbFileSize;
	DWORD sz_rfbBlockSize= 8192;
	DWORD dwNumberOfBytesRead = 0;
	unsigned int mTime = 0;
	char path[rfbMAX_PATH + rfbMAX_PATH + 2];
	BOOL bResult;
	UINT nSelItem = ListView_GetSelectionMark(m_hwndFTClientList);
	if (nSelItem == -1) {
		SetWindowText(m_hwndFTStatus, "Select file for copy to server side");
		BlockingFileTransferDialog(TRUE);
		EnableWindow(GetDlgItem(m_hwndFileTransfer, IDC_FTCANCEL), FALSE);
		return;
	}
	BlockingFileTransferDialog(FALSE);
	ListView_GetItemText(m_hwndFTClientList, nSelItem, 0, m_ClientFilename, rfbMAX_PATH);
	sprintf(path, "%s\\%s", m_ClientPath, m_ClientFilename);
	WIN32_FIND_DATA FindFileData;
	SetErrorMode(SEM_FAILCRITICALERRORS);
	HANDLE hFile = FindFirstFile(path, &FindFileData);
	SetErrorMode(0);
	if ( hFile != INVALID_HANDLE_VALUE) {
		sz_rfbFileSize = FindFileData.nFileSizeLow;
		mTime = FiletimeToTime70(FindFileData.ftLastWriteTime);
		strcpy(m_ServerFilename, FindFileData.cFileName);
	} else {
		SetWindowText(m_hwndFTStatus, "This is not a file. Select a file");
		BlockingFileTransferDialog(TRUE);
		EnableWindow(GetDlgItem(m_hwndFileTransfer, IDC_FTCANCEL), FALSE);
		return;
	}
	FindClose(hFile);
	if (sz_rfbFileSize <= sz_rfbBlockSize) sz_rfbBlockSize = sz_rfbFileSize;
	HANDLE hFiletoRead = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if (hFiletoRead == INVALID_HANDLE_VALUE) {
		SetWindowText(m_hwndFTStatus, "This is not file. Select a file");
		BlockingFileTransferDialog(TRUE);
		return;
	}
	char buffer[rfbMAX_PATH + rfbMAX_PATH + rfbMAX_PATH + rfbMAX_PATH + 20];
	sprintf(buffer, "UPLOAD: %s\\%s to %s\\%s", m_ClientPath, m_ClientFilename, m_ServerPath, m_ServerFilename);
	SetWindowText(m_hwndFTStatus, buffer);
	sprintf(path, "%s\\%s", m_ServerPath, m_ClientFilename);
	ConvertPath(path);
	int pathLen = strlen(path);

	char *pAllFURMessage = new char[sz_rfbFileUploadRequestMsg + pathLen];
	rfbFileUploadRequestMsg *pFUR = (rfbFileUploadRequestMsg *) pAllFURMessage;
	char *pFollowMsg = &pAllFURMessage[sz_rfbFileUploadRequestMsg];
	pFUR->type = rfbFileUploadRequest;
	pFUR->compressedLevel = 0;
	pFUR->fNameSize = Swap16IfLE(pathLen);
	pFUR->position = Swap32IfLE(0);
	memcpy(pFollowMsg, path, pathLen);
	m_clientconn->WriteExact(pAllFURMessage, sz_rfbFileUploadRequestMsg + pathLen); 
	delete [] pAllFURMessage;

	if (sz_rfbFileSize == 0) {
		SendFileUploadDataMessage(mTime);
	} else {
		int amount = sz_rfbFileSize / (sz_rfbBlockSize * 10);

		InitProgressBar(0, 0, amount, 1);

		DWORD dwPortionRead = 0;
		char *pBuff = new char [sz_rfbBlockSize];
		while(1) {
			ProcessDlgMessage(m_hwndFileTransfer);
			if (m_TransferEnable == FALSE) {
				SetWindowText(m_hwndFTStatus, "File transfer canceled");
				EnableWindow(GetDlgItem(m_hwndFileTransfer, IDC_FTCANCEL), FALSE);
				BlockingFileTransferDialog(TRUE);
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
				break;
			}
			bResult = ReadFile(hFiletoRead, pBuff, sz_rfbBlockSize, &dwNumberOfBytesRead, NULL);
			if (bResult && dwNumberOfBytesRead == 0) {
				/* This is the end of the file. */
				SendFileUploadDataMessage(mTime);
				break;
			}
			SendFileUploadDataMessage(dwNumberOfBytesRead, pBuff);
			dwPortionRead += dwNumberOfBytesRead;
			if (dwPortionRead >= (10 * sz_rfbBlockSize)) {
				dwPortionRead = 0;
				SendMessage(m_hwndFTProgress, PBM_STEPIT, 0, 0);
			}
		}
		delete [] pBuff;
	}
	SendMessage(m_hwndFTProgress, PBM_SETPOS, 0, 0);
	CloseHandle(hFiletoRead);
	EnableWindow(GetDlgItem(m_hwndFileTransfer, IDC_FTCANCEL), FALSE);
	SendFileListRequestMessage(m_ServerPath, 0);
}

void 
FileTransfer::FileTransferDownload()
{

	rfbFileDownloadDataMsg fdd;
	m_clientconn->ReadExact((char *)&fdd, sz_rfbFileDownloadDataMsg);
	fdd.realSize = Swap16IfLE(fdd.realSize);
	fdd.compressedSize = Swap16IfLE(fdd.compressedSize);

	char path[rfbMAX_PATH + rfbMAX_PATH + 3];
	
	if (m_bFirstFileDownloadMsg) {
		m_dwDownloadBlockSize = fdd.compressedSize;
		sprintf(path, "%s\\%s", m_ClientPath, m_ServerFilename);
		m_hFiletoWrite = CreateFile(path, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
		int amount = m_sizeDownloadFile / ((m_dwDownloadBlockSize + 1) * 10);
		InitProgressBar(0, 0, amount, 1);
		m_bFirstFileDownloadMsg = FALSE;
	}
	if ((fdd.realSize == 0) && (fdd.compressedSize == 0)) {
		unsigned int mTime;
		m_clientconn->ReadExact((char *) &mTime, sizeof(unsigned int));
		FILETIME Filetime;
		Time70ToFiletime(mTime, &Filetime);
		SetFileTime(m_hFiletoWrite, &Filetime, &Filetime, &Filetime);
		SendMessage(m_hwndFTProgress, PBM_SETPOS, 0, 0);
		CloseHandle(m_hFiletoWrite);
		ShowClientItems(m_ClientPath);
		EnableWindow(GetDlgItem(m_hwndFileTransfer, IDC_FTCANCEL), FALSE);
		BlockingFileTransferDialog(TRUE);
		m_bFirstFileDownloadMsg = TRUE;
		return;
	}
	char * pBuff = new char [fdd.compressedSize];
	DWORD dwNumberOfBytesWritten;
	m_clientconn->ReadExact(pBuff, fdd.compressedSize);
	ProcessDlgMessage(m_hwndFileTransfer);
	if (m_TransferEnable == FALSE) {
		SetWindowText(m_hwndFTStatus, "File transfer canceled");
		CloseHandle(m_hFiletoWrite);
		sprintf(path, "%s\\%s", m_ClientPath, m_ServerFilename);
		DeleteFile(path);
		EnableWindow(GetDlgItem(m_hwndFileTransfer, IDC_FTCANCEL), FALSE);
		BlockingFileTransferDialog(TRUE);
		return;
	}
/*
	if (fdd.amount == 0) {
		char tmpBuffer[rfbMAX_PATH + rfbMAX_PATH + 20];
		sprintf(tmpBuffer, "Can't download %s\\%s", m_ServerPath, m_ServerFilename);
		SetWindowText(m_hwndFTStatus, tmpBuffer);
		EnableWindow(GetDlgItem(m_hwndFileTransfer, IDC_FTCANCEL), FALSE);
		BlockingFileTransferDialog(TRUE);
		return;
	}
*/

	WriteFile(m_hFiletoWrite, pBuff, fdd.compressedSize, &dwNumberOfBytesWritten, NULL);
	m_dwDownloadRead += dwNumberOfBytesWritten;
	if (m_dwDownloadRead >= (10 * m_dwDownloadBlockSize)) {
		m_dwDownloadRead = 0;
		SendMessage(m_hwndFTProgress, PBM_STEPIT, 0, 0); 
	}
	delete [] pBuff;
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
		char DriveName[rfbMAX_PATH] = "?:";
		LengthDrivesString = GetLogicalDriveStrings(256, DrivesString);
		if ((LengthDrivesString == 0) || (LengthDrivesString > 256)) {
			BlockingFileTransferDialog(TRUE);
			strcpy(m_ClientPathTmp, m_ClientPath);
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
			ShowListViewItems(m_hwndFTClientList, &m_FTClientItemInfo);
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
				BlockingFileTransferDialog(TRUE);
				return;
			}
			path[strlen(path) - 2] = '\0';
			strcpy(m_ClientPath, m_ClientPathTmp);
			SetWindowText(m_hwndFTClientPath, m_ClientPath);
			FindClose(m_handle);
			ListView_DeleteAllItems(m_hwndFTClientList);
			BlockingFileTransferDialog(TRUE);
			return;
		}
		ListView_DeleteAllItems(m_hwndFTClientList);
		m_FTClientItemInfo.Free();
		char buffer[16];
		while(1) {
			if ((strcmp(m_FindFileData.cFileName, ".") != 0) &&
		       (strcmp(m_FindFileData.cFileName, "..") != 0)) {
				if (!(m_FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
					sprintf(buffer, "%d", m_FindFileData.nFileSizeLow);
					LARGE_INTEGER li;
					li.LowPart = m_FindFileData.ftLastWriteTime.dwLowDateTime;
					li.HighPart = m_FindFileData.ftLastWriteTime.dwHighDateTime;
					li.QuadPart = (li.QuadPart - 116444736000000000) / 10000000;
					m_FTClientItemInfo.Add(m_FindFileData.cFileName, buffer, li.LowPart);
				} else {
					strcpy(buffer, m_FTClientItemInfo.folderText);
					m_FTClientItemInfo.Add(m_FindFileData.cFileName, buffer, 0);
				}
			}
			if (!FindNextFile(m_handle, &m_FindFileData)) break;
		}
		FindClose(m_handle);
		m_FTClientItemInfo.Sort();
		ShowListViewItems(m_hwndFTClientList, &m_FTClientItemInfo);
		path[strlen(path) - 2] = '\0';
		strcpy(m_ClientPath, m_ClientPathTmp);
		SetWindowText(m_hwndFTClientPath, m_ClientPath);
	}
	BlockingFileTransferDialog(TRUE);
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
				_this->SendFileListRequestMessage("", 0x10);
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
				char path[rfbMAX_PATH];
				if (GetWindowText(GetDlgItem(hwnd, IDC_FTBROWSEEDIT), path, rfbMAX_PATH) == 0) {
					EndDialog(hwnd, TRUE);
					_this->m_bServerBrowseRequest = FALSE;
					return TRUE;
				}
				if (_this->m_bServerBrowseRequest) {
					strcpy(_this->m_ServerPathTmp, path);
					EndDialog(hwnd,TRUE);
					_this->m_bServerBrowseRequest = FALSE;
					_this->SendFileListRequestMessage(_this->m_ServerPathTmp, 0);
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
					char path[rfbMAX_PATH];
					_this->GetTVPath(GetDlgItem(hwnd, IDC_FTBROWSETREE), m_lParam->itemNew.hItem, path);
					SetWindowText(GetDlgItem(hwnd, IDC_FTBROWSEEDIT), path);
					return TRUE;
				}
				break;
			case TVN_ITEMEXPANDING:
				{
				NMTREEVIEW *m_lParam = (NMTREEVIEW *) lParam;
				char Path[rfbMAX_PATH];
				if (m_lParam -> action == 2) {
					if (_this->m_bServerBrowseRequest) {
						_this->m_hTreeItem = m_lParam->itemNew.hItem;
						_this->GetTVPath(GetDlgItem(hwnd, IDC_FTBROWSETREE), m_lParam->itemNew.hItem, Path);
						_this->SendFileListRequestMessage(Path, 0x10);
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
	char szText[rfbMAX_PATH];
	TVITEM _tvi;
	path[0] = '\0';
	do {
		_tvi.mask = TVIF_TEXT | TVIF_HANDLE;
		_tvi.hItem = hTItem;
		_tvi.pszText = szText;
		_tvi.cchTextMax = rfbMAX_PATH;
		TreeView_GetItem(hwnd, &_tvi);
		strcat(path, "\\");
		strcat(path, _tvi.pszText);
		hTItem = TreeView_GetParent(hwnd, hTItem);
	}
	while(hTItem != NULL);
	char path_tmp[rfbMAX_PATH], path_out[rfbMAX_PATH];
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
FileTransfer::StrInvert(char str[rfbMAX_PATH])
{
	int len = strlen(str), i;
	char str_out[rfbMAX_PATH];
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
	char path[rfbMAX_PATH];
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
FileTransfer::BlockingFileTransferDialog(BOOL status)
{
	EnableWindow(m_hwndFTClientList, status);
	EnableWindow(m_hwndFTServerList, status);
	EnableWindow(m_hwndFTClientPath, status);
	EnableWindow(m_hwndFTServerPath, status);
	EnableWindow(GetDlgItem(m_hwndFileTransfer, IDC_UPLOAD), status);
	EnableWindow(GetDlgItem(m_hwndFileTransfer, IDC_DOWNLOAD), status);
	EnableWindow(GetDlgItem(m_hwndFileTransfer, IDC_CLIENTUP), status);
	EnableWindow(GetDlgItem(m_hwndFileTransfer, IDC_SERVERUP), status);
	EnableWindow(GetDlgItem(m_hwndFileTransfer, IDC_CLIENTRELOAD), status);
	EnableWindow(GetDlgItem(m_hwndFileTransfer, IDC_SERVERRELOAD), status);
	EnableWindow(GetDlgItem(m_hwndFileTransfer, IDC_CLIENTBROWSE_BUT), status);
	EnableWindow(GetDlgItem(m_hwndFileTransfer, IDC_SERVERBROWSE_BUT), status);
	EnableWindow(GetDlgItem(m_hwndFileTransfer, IDC_EXIT), status);
}

void 
FileTransfer::ShowServerItems()
{
	rfbFileListDataMsg fld;
	m_clientconn->ReadExact((char *) &fld, sz_rfbFileListDataMsg);
	if ((fld.flags & 0x80) && !m_bServerBrowseRequest) {
		BlockingFileTransferDialog(TRUE);
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
		if (fld.numFiles == 0) {
			BlockingFileTransferDialog(TRUE);
			strcpy(m_ServerPath, m_ServerPathTmp);
			SetWindowText(m_hwndFTServerPath, m_ServerPath);
			ListView_DeleteAllItems(m_hwndFTServerList); 
			delete [] pftSD;
			delete [] pFilenames;
			return;
		} else {
			m_FTServerItemInfo.Free();
			ListView_DeleteAllItems(m_hwndFTServerList); 
			strcpy(m_ServerPath, m_ServerPathTmp);
			SetWindowText(m_hwndFTServerPath, m_ServerPath);
			CreateServerItemInfoList(&m_FTServerItemInfo, pftSD, fld.numFiles, pFilenames, fld.dataSize);
			m_FTServerItemInfo.Sort();
			ShowListViewItems(m_hwndFTServerList, &m_FTServerItemInfo);
		}
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
	BlockingFileTransferDialog(TRUE);
}

void 
FileTransfer::SendFileListRequestMessage(char *filename, unsigned char flags)
{
	char _filename[rfbMAX_PATH];
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
FileTransfer::ProcessListViewDBLCLK(HWND hwnd, char *Path, char *PathTmp, int iItem)
{
	SendMessage(m_hwndFTProgress, PBM_SETPOS, 0, 0);
	SetWindowText(m_hwndFTStatus, "");
	strcpy(PathTmp, Path);
	char buffer[rfbMAX_PATH];
	char buffer_tmp[16];
	ListView_GetItemText(hwnd, iItem, 0, buffer, rfbMAX_PATH);
	ListView_GetItemText(hwnd, iItem, 1, buffer_tmp, 16);
	if (strcmp(buffer_tmp, m_FTClientItemInfo.folderText) == 0) {
			BlockingFileTransferDialog(FALSE);
			if (strlen(PathTmp) >= 2) strcat(PathTmp, "\\");
			strcat(PathTmp, buffer);
			if (hwnd == m_hwndFTClientList) ShowClientItems(PathTmp);
			if (hwnd == m_hwndFTServerList) SendFileListRequestMessage(PathTmp, 0);
	}
}

void
FileTransfer::ConvertPath(char *path)
{
	int len = strlen(path);
	if (len >= rfbMAX_PATH) return;
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
FileTransfer::ShowListViewItems(HWND hwnd, FileTransferItemInfo *ftii)
{
	LVITEM LVItem;
	LVItem.mask = LVIF_TEXT | LVIF_STATE; 
	LVItem.state = 0; 
	LVItem.stateMask = 0; 
	for (int i=0; i<ftii->GetNumEntries(); i++) {
		LVItem.iItem = i;
		LVItem.iSubItem = 0;
		LVItem.pszText = LPSTR_TEXTCALLBACK;
		ListView_InsertItem(hwnd, &LVItem);
	}
}

void
FileTransfer::FTInsertColumn(HWND hwnd, char *iText, int iOrder, int xWidth)
{
    LVCOLUMN lvc; 
    lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM | LVCF_ORDER;
	lvc.fmt = LVCFMT_RIGHT;
	lvc.iSubItem = iOrder;
    lvc.pszText = iText;	
	lvc.cchTextMax = 10;
    lvc.cx = xWidth;
	lvc.iOrder = iOrder;
    ListView_InsertColumn(hwnd, iOrder, &lvc);
}

void FileTransfer::InitProgressBar(int nPosition, int nMinRange, int nMaxRange, int nStep)
{
	SendMessage(m_hwndFTProgress, PBM_SETPOS, (WPARAM) nPosition, (LPARAM) 0);
    SendMessage(m_hwndFTProgress, PBM_SETRANGE, (WPARAM) 0, MAKELPARAM(nMinRange, nMaxRange)); 
	SendMessage(m_hwndFTProgress, PBM_SETSTEP, (WPARAM) nStep, 0); 
}

void FileTransfer::CreateServerItemInfoList(FileTransferItemInfo *pftii, 
											FTSIZEDATA *ftsd, int ftsdNum,
											char *pfnames, int fnamesSize)
{
	int pos = 0;
	for (int i = 0; i < ftsdNum; i++) {
		char buf[16];
		ftsd[i].size = Swap32IfLE(ftsd[i].size);
		ftsd[i].data = Swap32IfLE(ftsd[i].data);
		if (ftsd[i].size == -1) {
			strcpy(buf, FileTransferItemInfo::folderText);
		} else {
			sprintf(buf, "%d", ftsd[i].size);
		}
		pftii->Add(pfnames + pos, buf, ftsd[i].data);
		pos += strlen(pfnames + pos) + 1;
	}
}

void FileTransfer::SendFileUploadDataMessage(unsigned int mTime)
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

void FileTransfer::SendFileUploadDataMessage(unsigned short size, char *pFile)
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

unsigned int FileTransfer::FiletimeToTime70(FILETIME ftime)
{
	LARGE_INTEGER uli;
	uli.LowPart = ftime.dwLowDateTime;
	uli.HighPart = ftime.dwHighDateTime;
	uli.QuadPart = (uli.QuadPart - 116444736000000000) / 10000000;
	return uli.LowPart;
}

void FileTransfer::Time70ToFiletime(unsigned int time70, FILETIME *pftime)
{
    LONGLONG ll = Int32x32To64(time70, 10000000) + 116444736000000000;
    pftime->dwLowDateTime = (DWORD) ll;
    pftime->dwHighDateTime = ll >> 32;
}

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

#if !defined(FILETRANSFER)
#define FILETRANSFER

#include "windows.h"
#include "shlobj.h"
#include "commctrl.h"
#include "ClientConnection.h"
#include "FileTransferItemInfo.h"

#define FT_FLR_DEST_MAIN     100
#define FT_FLR_DEST_BROWSE   101
#define FT_FLR_DEST_DOWNLOAD 102
#define FT_FLR_DEST_UPLOAD   103
#define FT_FLR_DEST_DELETE   104

#define FT_FDSR_DEST_MAIN     200
#define FT_FDSR_DEST_DOWNLOAD 201
#define FT_FDSR_DEST_UPLOAD   202


#define FT_ID_MYCOMPUTER 0
#define FT_ID_MYDOCUMENTS 1
#define FT_ID_MYPICTURES 2
#define FT_ID_MYMUSIC 3
#define FT_ID_MYDESKTOP 4 


class ClientConnection;

class FileTransfer  
{
private:
	static const char delimeter[];
	static const char uploadText[];
	static const char downloadText[];
	static const char noactionText[];
	static const char myComputerText[];
	static const char myDocumentsText[];
	static const char myPicturesText[];
	static const char myMusicText[];
	static const char myDesktopText[];

public:
	FileTransfer(ClientConnection * pCC, VNCviewerApp * pApp);
	~FileTransfer();

	void FTInsertColumn(HWND hwnd, char *iText, int iOrder, int xWidth, int alignFmt);
	void CreateFileTransferDialog();
	void ShowListViewItems(HWND hwnd, FileTransferItemInfo *ftii);
	void ConvertPath(char *path);
	void ConvertFromNetPath(char *path);
	void ProcessListViewDBLCLK(HWND hwnd, char *Path, char *PathTmp, int iItem);
	void ProcessFLRMessage();
	void ProcessFDSDMessage();
	void ProcessFLRFMessage();
	void ProcessFSDDMessage();
	void SendFileListRequestMessage(char *filename, unsigned char flags, int dest);
	void ShowServerItems();
	void ShowClientItems(char *path);
	void ShowClientSpecFolder(int idFolder);
	void ProcessDlgMessage(HWND hwnd);
	void ShowTreeViewItems(HWND hwnd, LPNMTREEVIEW m_lParam);
	void CreateFTBrowseDialog(BOOL status);
	void StrInvert(char *str);
	void GetTVPath(HWND hwnd, HTREEITEM hTItem, char *path);
	char m_ServerPath[MAX_PATH];
	char m_ClientPath[MAX_PATH];
	char m_ServerPathTmp[MAX_PATH];
	char m_ClientPathTmp[MAX_PATH];
	char m_ServerFilename[MAX_PATH];
	char m_ClientFilename[MAX_PATH];
	char m_UploadFilename[MAX_PATH];
	char m_DownloadFilename[MAX_PATH];
	void OnGetDispClientInfo(NMLVDISPINFO *plvdi); 
	void OnGetDispServerInfo(NMLVDISPINFO *plvdi); 
	static LRESULT CALLBACK FileTransferDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static BOOL CALLBACK FTBrowseDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static BOOL CALLBACK FTCreateDirDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static BOOL CALLBACK FTRenameDirDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static BOOL CALLBACK FTCancelingDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static INT_PTR CALLBACK FTConfirmDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	void CloseUndoneFileTransfers();
	void UploadFilePortion();
	void DownloadFilePortion();
	BOOL IsTransferEnable() { return m_bTransferEnable; };
	ClientConnection * m_clientconn;
	VNCviewerApp * m_pApp; 
	
private:
//	int m_sizeDownloadFile;
	int m_FLRDest;
	int m_FDSRDest;
	int m_NumReqDirSize;

	DWORD m_dwFileSize;
	DWORD m_dwFileBlockSize;
	DWORD m_dwModTime;
	DWORD m_dwNumItemsSel;
	DWORD m_dwSelFileSize;

	unsigned int FiletimeToTime70(FILETIME ftime);
	void Time70ToFiletime(unsigned int time70, FILETIME *pftime);
	void SendFileUploadDataMessage(unsigned short size, char *pFile);
	void SendFileUploadDataMessage(unsigned int mTime);
	void SendFileDownloadCancelMessage(unsigned short reasonLen, char *reason);
	void SendFileCreateDirRequestMessage(unsigned short dNameLen, char *dName);
	void SendFileDownloadRequestMessage(unsigned short dNameLen, char *dName);
	void SendFileDirSizeRequestMessage(unsigned short pathLen, char *path, int dest);
	void SendFileRenameRequestMessage(char *pOldName, char *pNewName);
	void SendFileDeleteRequestMessage(char *path);
	void SendFileSpecDirRequestMessage(unsigned char flags, unsigned short specFlags);
	void CreateItemInfoList(FileTransferItemInfo *pftii, FTSIZEDATA *ftsd, int ftsdNum, char *pfnames, int fnamesSize);
	void InitProgressBar(int nPosition);
	void InitFTProgressBar(int nPosition);
	void IncreaseProgBarPos(int pos);
	void SetIcon(HWND hwnd, int dest, int idIcon);
	void ClientCreateDir();
	void ServerCreateDir();
	void ClientDeleteDir();
	void ServerDeleteDir();
	void ClientRenameDir();
	void ServerRenameDir();
	BOOL CreateRenameDirDlg(HWND hwnd);
	BOOL CreateTransferConfDlg();
	BOOL CreateFTCancelingDlg();
	BOOL CreateFTConfirmDlg(char *pText);

	void SetDefaultBlockSize() { m_dwFileBlockSize = 8192; };
	void FTClientDelete(FileTransferItemInfo *ftfi);


	DWORD GetSelectedFileSize(char *path, FileTransferItemInfo *pFTFI);

	void SetStatusText(LPCSTR format,...);
	void ClearStatusText() { SetWindowText(m_hwndFTStatus, ""); };
	void MakeStatusText(char *prefix, char *path1, char *path2, char *name);
	
	void FileTransferUpload();
	void CheckUploadQueue();
	void MakeUploadQueue();
	void UploadFile(int num);

	void ClearFTControls();
	void ClearFTButtons();
	void CheckClientLV();
	void CheckServerLV();

	void FileTransferDownload();
	void CheckDownloadQueue();
	void DownloadFile(int num);

	void ProcessFLRUpload();
	void ProcessFLRDownload();
	void ProcessFLRDelete();
	void ProcessFDSDMain(DWORD dSize);

	void SetEndTransfer(char *statusText);
	void EndFTCancelDlg(BOOL result);
	void SetFTDlgCursor(LPCTSTR cursorType);

	void CutLastName(char *path, char *lastName);
	int IsExistName(FileTransferItemInfo *ftii, char *name);

	int GetSelectedItems(HWND hwnd, FileTransferItemInfo *pFTII);

	DWORD m_dwProgBarValue;
	DWORD m_dwProgBarPercent;
	DWORD m_dwFTProgBarValue;
	DWORD m_dwFTProgBarPercent;
	DWORD m_dwNumDelItems;
	DWORD m_dwNumDelFiles;
	DWORD m_dwNumDelFolders;

	HWND m_hwndFileTransfer;
	HWND m_hwndFTClientList;
	HWND m_hwndFTServerList;
	HWND m_hwndFTClientPath;
	HWND m_hwndFTServerPath;
	HWND m_hwndFTCanceling;
	HWND m_hwndFTProgress;
	HWND m_hwndProgress;
	HWND m_hwndFTStatus;
	HWND m_hwndFTBrowse;
	
	BOOL m_bFTCOPY;
    BOOL m_bUploadStarted;
    BOOL m_bDownloadStarted;
	BOOL m_bTransferEnable;
	BOOL m_bServerBrowseRequest;
	BOOL m_bClientRefresh;
	BOOL m_bServerRefresh;
	BOOL m_bEndFTDlgOnYes;
	BOOL m_bOverwriteAll;

	HANDLE m_hFiletoWrite;
    HANDLE m_hFiletoRead;
	HTREEITEM m_hTreeItem;
	HINSTANCE m_FTInstance;

	FileTransferItemInfo m_FTClientItemInfo;
	FileTransferItemInfo m_FTServerItemInfo;
	FileTransferItemInfo m_TransferInfo;
	FileTransferItemInfo m_DeleteInfo;

	char m_szLocalMyDocPath[MAX_PATH];
	char m_szRemoteMyDocPath[MAX_PATH];
	char m_szCreateDirName[MAX_PATH];
	char m_szLastRelTransPath[MAX_PATH];
	char m_szLocalTransPath[MAX_PATH];
	char m_szRemoteTransPath[MAX_PATH];

	char m_szRenameDlgText1[MAX_PATH];
	char m_szRenameDlgText2[MAX_PATH];
	char m_szRenameDlgText3[MAX_PATH];
};

#endif // !defined(FILETRANSFER)

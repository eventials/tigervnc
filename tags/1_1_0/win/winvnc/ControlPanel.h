// ControlPanel.h: interface for the ControlPanel class.
//
//////////////////////////////////////////////////////////////////////

#ifndef AFX_CONTROLPANEL_H__
#define AFX_CONTROLPANEL_H__


#pragma once


#include <list>
#include <winvnc/resource.h>
#include <rfb_win32/Dialog.h>
#include <rfb_win32/ListViewControl.h>
#include <rfb_win32/Win32Util.h>
#include <rfb/ListConnInfo.h>

namespace winvnc {
  
  class ControlPanel : rfb::win32::Dialog, rfb::win32::ListViewControl {
  public:
    ControlPanel(HWND hSTIcon) : Dialog(GetModuleHandle(0)), ListViewControl(){
      m_hSTIcon = hSTIcon;
      stop_updating = false;
    };
    virtual bool showDialog();
    virtual void initDialog();
    virtual bool onCommand(int cmd);
    void UpdateListView(rfb::ListConnInfo* LCInfo);
    HWND GetHandle() {return handle;};
    void SendCommand(DWORD command, int data);
    ~ControlPanel();
    rfb::ListConnInfo ListConnStatus;
  protected: 
    virtual BOOL dialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void getSelConnInfo();
    HWND m_hSTIcon;
    rfb::ListConnInfo ListConn;
    rfb::ListConnInfo ListSelConn;
    bool stop_updating;
  };
};

#endif  
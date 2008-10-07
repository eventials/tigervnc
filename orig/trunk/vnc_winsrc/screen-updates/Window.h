//  Copyright (C) 2008 GlavSoft LLC. All Rights Reserved.
//
//  This file is part of the TightVNC software.
//
//  TightVNC is free software; you can redistribute it and/or modify
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
// TightVNC homepage on the Web: http://www.tightvnc.com/

#ifndef __WINDOW_H__
#define __WINDOW_H__

#include "windows.h"

class Window
{
public:
  Window(HINSTANCE hinst, TCHAR *windowClassName);
  virtual ~Window(void);

  HWND getHWND() const { return m_hwnd; }

protected:
  // Function must return true value if the message has been processed.
  virtual bool wndProc(UINT message, WPARAM wParam, LPARAM lParam) { return true; }

	HWND m_hwnd;

	HINSTANCE m_hinst;
  TCHAR *m_windowClassName;

private:
	ATOM regClass(HINSTANCE hinst, TCHAR *windowClassName);

  static LRESULT CALLBACK staticWndProc(HWND hwnd,
                                        UINT message,
                                        WPARAM wParam,
                                        LPARAM lParam);
};

#endif // __WINDOW_H__
/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 * USA.
 */
//
// AboutDialog.h
//

#ifndef __ABOUTDIALOG_H__
#define __ABOUTDIALOG_H__

#include "TXMsgBox.h"
#include "parameters.h"

#include "gettext.h"
#define _(String) gettext (String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)

extern char buildtime[];

class AboutDialog : public TXMsgBox {
public:
  AboutDialog(Display* dpy)
    : TXMsgBox(dpy, aboutText, MB_OK, _("About TigerVNC Viewer")) {
  }
};

#endif

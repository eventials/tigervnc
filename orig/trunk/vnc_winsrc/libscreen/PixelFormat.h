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

#ifndef __PIXELFORMAT_H__
#define __PIXELFORMAT_H__

#include "inttypes.h"

struct PixelFormat
{
  UINT16 bitsPerPixel;
  UINT8 colorDepth;

  UINT32 redMax;
  UINT32 greenMax;
  UINT32 blueMax;

  UINT8 redShift;
  UINT8 greenShift;
  UINT8 blueShift;
};

#endif // __PIXELFORMAT_H__
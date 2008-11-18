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

#ifndef __MOUSEGRABBER_H__
#define __MOUSEGRABBER_H__

#include "FrameBuffer.h"

class MouseGrabber
{
public:
  MouseGrabber(void);
  virtual ~MouseGrabber(void);

  virtual bool grab(PixelFormat *pixelFormat) = 0;

  // Returns true if the cursor shape has been changed, false otherwise.
  // Calling this function resets the state back to unchanged.
  virtual bool isCursorShapeChanged() = 0;

  const FrameBuffer &getPixels() const { return m_pixels; }
  const FrameBuffer &getMask() const { return m_mask; }

protected:
  FrameBuffer m_pixels;
  FrameBuffer m_mask;
};

#endif // __MOUSEGRABBER_H__

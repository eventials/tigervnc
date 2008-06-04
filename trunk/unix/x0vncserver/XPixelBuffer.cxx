/* Copyright (C) 2007-2008 Constantin Kaplinsky.  All Rights Reserved.
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
// XPixelBuffer.cxx
//

#include <vector>
#include <rfb/Region.h>
#include <X11/Xlib.h>
#include <x0vncserver/XPixelBuffer.h>

using namespace rfb;

XPixelBuffer::XPixelBuffer(Display *dpy, ImageFactory &factory,
                           const Rect &rect, ColourMap* cm)
  : FullFramePixelBuffer(),
    m_dpy(dpy),
    m_image(factory.newImage(dpy, rect.width(), rect.height())),
    m_offsetLeft(rect.tl.x),
    m_offsetTop(rect.tl.y),
    m_stride(0)
{
  // Fill in the PixelFormat structure of the parent class.
  format.bpp        = m_image->xim->bits_per_pixel;
  format.depth      = m_image->xim->depth;
  format.bigEndian  = (m_image->xim->byte_order == MSBFirst);
  format.trueColour = m_image->isTrueColor();
  format.redShift   = ffs(m_image->xim->red_mask) - 1;
  format.greenShift = ffs(m_image->xim->green_mask) - 1;
  format.blueShift  = ffs(m_image->xim->blue_mask) - 1;
  format.redMax     = m_image->xim->red_mask   >> format.redShift;
  format.greenMax   = m_image->xim->green_mask >> format.greenShift;
  format.blueMax    = m_image->xim->blue_mask  >> format.blueShift;

  // Set up the remaining data of the parent class.
  width_ = rect.width();
  height_ = rect.height();
  data = (rdr::U8 *)m_image->xim->data;
  colourmap = cm;

  // Calculate the distance in pixels between two subsequent scan
  // lines of the framebuffer. This may differ from image width.
  m_stride = m_image->xim->bytes_per_line * 8 / m_image->xim->bits_per_pixel;

  // Get initial screen image from the X display.
  m_image->get(DefaultRootWindow(m_dpy), m_offsetLeft, m_offsetTop);
}

XPixelBuffer::~XPixelBuffer()
{
  delete m_image;
}

void
XPixelBuffer::grabRegion(const rfb::Region& region)
{
  std::vector<Rect> rects;
  std::vector<Rect>::const_iterator i;
  region.get_rects(&rects);
  for (i = rects.begin(); i != rects.end(); i++) {
    grabRect(*i);
  }
}


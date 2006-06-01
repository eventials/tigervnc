/* Copyright (C) 2002-2004 RealVNC Ltd.  All Rights Reserved.
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

package vncviewer;

import java.awt.*;

class InfoDialog extends vncviewer.Dialog {

  public InfoDialog() { 
    super(false);
    setTitle("VNC connection info");
    Panel p1 = new Panel();
    infoLabel = new TextArea(12,60);
    infoLabel.setEditable(false);
    p1.add(infoLabel);
    add("Center", p1);

    Panel p2 = new Panel();
    okButton = new Button("OK");
    p2.add(okButton);
    add("South", p2);

    pack();
  }

  public boolean action(Event event, Object arg) {
    if (event.target == okButton) {
      ok = true;
      endDialog();
    }
    return true;
  }

  TextArea infoLabel;
  Button okButton;
}

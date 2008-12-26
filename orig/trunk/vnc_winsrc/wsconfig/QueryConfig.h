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

#ifndef _QUERY_CONFIG_H_
#define _QUERY_CONFIG_H_

class QueryConfig
{
public:

  enum QueryAction {
    QA_ACCEPT = 0,
    QA_REFUSE = 1
  };

public:
  QueryConfig();
  ~QueryConfig();

  bool isQueryConsoleOnIncomingConnectionsEnabled() {
    return m_queryConsoleOnIncomingConnections;
  }
  void enableQueryConsoleOnIncomingConnections(bool enabled) {
    m_queryConsoleOnIncomingConnections = enabled;
  }

  bool isAllowedOptionsToAcceptWithoutAuth() {
    return m_allowOptionsToAcceptWithoutAuth;
  }
  void allowOptionsToAcceptWithoutAuth(bool enabled) {
    m_allowOptionsToAcceptWithoutAuth = enabled;
  }

  unsigned int getQueryTimeout() { return m_queryTimeout; }
  void setQueryTimeout(unsigned int timeout) { m_queryTimeout = timeout; }

  QueryAction getDefaultAction() { return m_defaultAction; }
  void setDefaultAction(QueryAction defaultAction) { m_defaultAction = defaultAction; }
protected:
  bool m_queryConsoleOnIncomingConnections;
  bool m_allowOptionsToAcceptWithoutAuth;
  unsigned int m_queryTimeout;
  QueryAction m_defaultAction;
};

#endif

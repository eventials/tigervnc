/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2002 Ultr@VNC Team Members. All Rights Reserved.
//
//  This program is free software; you can redistribute it and/or modify
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
// If the source code for the program is not available from the place from
// which you received this file, check 
// http://ultravnc.sourceforge.net/
//
/////////////////////////////////////////////////////////////////////////////

#include "VideoDriver.h"

char* MIRROR_driverName = "Mirage Driver";
#define	MIRROR_MiniportName "dfmirage"

vncVideoDriver::vncVideoDriver()
{
	bufdata.buffer=NULL;
	bufdata.Userbuffer=NULL;
	driver = false;
}

vncVideoDriver::~vncVideoDriver()
{
	UnMapSharedbuffers();
	DesActivate_video_driver();
	driver = false;
}
BOOL
vncVideoDriver::MapSharedbuffers()
{
	gdc = GetDC(NULL);
	oldaantal=0;
	if ( ExtEscape(gdc, MAP1, 0, NULL, sizeof(GETCHANGESBUF), (LPSTR) &bufdata) >0)
		driver = true;
	return driver;	
}

BOOL
vncVideoDriver::testdriver()
{
	gdc = GetDC(NULL);
	return ExtEscape(gdc, TESTDRIVER, 0, NULL, sizeof(GETCHANGESBUF), (LPSTR) &bufdata);
		
}

BOOL
vncVideoDriver::testmapped()
{
	gdc = GetDC(NULL);
	return ExtEscape(gdc, TESTMAPPED, 0, NULL, sizeof(GETCHANGESBUF), (LPSTR) &bufdata);	
}

void
vncVideoDriver::UnMapSharedbuffers()
{
	ExtEscape(gdc, UNMAP1, 0, NULL, sizeof(GETCHANGESBUF), (LPSTR) &bufdata);
	ReleaseDC(NULL, gdc);
	driver = false;
}


BOOL
vncVideoDriver::Activate_video_driver()
   {
	HDESK   hdeskInput;
    HDESK   hdeskCurrent;
	HINSTANCE  hInstUser32;
 
    int j=0;
	pEnumDisplayDevices pd;
	DEVMODE devmode;
    FillMemory(&devmode, sizeof(DEVMODE), 0);
    devmode.dmSize = sizeof(DEVMODE);
    devmode.dmDriverExtra = 0;
    BOOL change = EnumDisplaySettings(NULL,ENUM_CURRENT_SETTINGS,&devmode);
	devmode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

	hInstUser32 = LoadLibrary("User32.DLL");
    if (!hInstUser32) return FALSE;  

	pd = (pEnumDisplayDevices)GetProcAddress(hInstUser32,"EnumDisplayDevicesA");
    if (!pd) {
        FreeLibrary(hInstUser32);
        return FALSE;
    }
	DISPLAY_DEVICE dd;
	ZeroMemory(&dd, sizeof(dd));
	dd.cb = sizeof(dd);
	LPSTR deviceName = NULL;
	devmode.dmDeviceName[0] = '\0';
	INT devNum = 0;
	BOOL result;
	while (result = (*pd)(NULL,devNum, &dd,0))
	{
		if (strcmp((const char *)&dd.DeviceString[0], MIRROR_driverName) == 0)
			break;
		devNum++;
	}
	if (!result) {
		vnclog.Print(LL_INTERR, VNCLOG("No '%s' found.\n"), MIRROR_driverName);
		return FALSE;
	}
    vnclog.Print(LL_INTERR, VNCLOG("DevNum:%d\nName:%s\nString:%s\n\n"),devNum,&dd.DeviceName[0],&dd.DeviceString[0]);
	vnclog.Print(LL_INTERR, VNCLOG("Sreen Settings'%i %i %i'\n"),devmode.dmPelsWidth,devmode.dmPelsHeight,devmode.dmBitsPerPel);
	CHAR deviceNum[MAX_PATH];
	strcpy(&deviceNum[0], "DEVICE0");
	HKEY hKeyProfileMirror = (HKEY)0;
	if (RegCreateKey(HKEY_LOCAL_MACHINE,
		("SYSTEM\\CurrentControlSet\\Hardware Profiles\\Current\\System\\CurrentControlSet\\Services\\" MIRROR_MiniportName),
                         &hKeyProfileMirror) != ERROR_SUCCESS) {
		vnclog.Print(LL_INTERR, VNCLOG("Can't access registry.\n"));
		return FALSE;
	}
	HKEY hKeyDevice = (HKEY)0;
	if (RegCreateKey(hKeyProfileMirror, (&deviceNum[0]), &hKeyDevice) != ERROR_SUCCESS) {
		vnclog.Print(LL_INTERR, VNCLOG("Can't access DEVICE# hardware profiles key.\n"));
		return FALSE;
	}
	DWORD one = 1;
	if (RegSetValueEx(hKeyDevice,("Attach.ToDesktop"), 0, REG_DWORD, (unsigned char *)&one, 4) != ERROR_SUCCESS) {
		vnclog.Print(LL_INTERR, VNCLOG("Can't set Attach.ToDesktop to 0x1\n"));
		return FALSE;
	}

    strcpy((LPSTR)&devmode.dmDeviceName[0], MIRROR_MiniportName);
	deviceName = (LPSTR)&dd.DeviceName[0];
	// Save the current desktop
	hdeskCurrent = GetThreadDesktop(GetCurrentThreadId());
	if (hdeskCurrent != NULL) {
		hdeskInput = OpenInputDesktop(0, FALSE, MAXIMUM_ALLOWED);
		if (hdeskInput != NULL) 
			SetThreadDesktop(hdeskInput);
	}
	if (devmode.dmBitsPerPel==24) devmode.dmBitsPerPel=16;
	INT code =ChangeDisplaySettingsEx(deviceName, &devmode, NULL, CDS_UPDATEREGISTRY,NULL);
    
   // Reset desktop
   SetThreadDesktop(hdeskCurrent);


   // Close the input desktop
   CloseDesktop(hdeskInput);
   FreeLibrary(hInstUser32);
   RegCloseKey(hKeyProfileMirror);
   RegCloseKey(hKeyDevice);

   return TRUE;
}



void
vncVideoDriver::DesActivate_video_driver()
   {
	HDESK   hdeskInput;
	HDESK   hdeskCurrent;
	HINSTANCE  hInstUser32;
 
    int j=0;
	pEnumDisplayDevices pd;
	DEVMODE devmode;
    FillMemory(&devmode, sizeof(DEVMODE), 0);
    devmode.dmSize = sizeof(DEVMODE);
    devmode.dmDriverExtra = 0;
    BOOL change = EnumDisplaySettings(NULL,ENUM_CURRENT_SETTINGS,&devmode);
    devmode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
	pd = (pEnumDisplayDevices)GetProcAddress( LoadLibrary("USER32"), "EnumDisplayDevicesA");
   	hInstUser32 = LoadLibrary("User32.DLL");
    if (!hInstUser32) return ;  
	pd = (pEnumDisplayDevices)GetProcAddress(hInstUser32,"EnumDisplayDevicesA");
    if (!pd) {
        FreeLibrary(hInstUser32);
        return ;
    }
	DISPLAY_DEVICE dd;
	ZeroMemory(&dd, sizeof(dd));
	dd.cb = sizeof(dd);
	LPSTR deviceName = NULL;
	devmode.dmDeviceName[0] = '\0';
	INT devNum = 0;
	BOOL result;
	while (result = (*pd)(NULL,devNum, &dd,0))
	{
		if (strcmp((const char *)&dd.DeviceString[0], MIRROR_driverName) == 0)
			break;
		devNum++;
	}
	if (!result) {
		vnclog.Print(LL_INTERR, VNCLOG("No '%s' found.\n"), MIRROR_driverName);
		return;
	}
	
	CHAR deviceNum[MAX_PATH];
	strcpy(&deviceNum[0], "DEVICE0");
	HKEY hKeyProfileMirror = (HKEY)0;
	if (RegCreateKey(HKEY_LOCAL_MACHINE,
                        ("SYSTEM\\CurrentControlSet\\Hardware Profiles\\Current\\System\\CurrentControlSet\\Services\\" MIRROR_MiniportName),
                         &hKeyProfileMirror) != ERROR_SUCCESS){
		vnclog.Print(LL_INTERR, VNCLOG("Can't access registry.\n"));
		return;
	}
	HKEY hKeyDevice = (HKEY)0;
	if (RegCreateKey(hKeyProfileMirror, (&deviceNum[0]), &hKeyDevice) != ERROR_SUCCESS){
		vnclog.Print(LL_INTERR, VNCLOG("Can't access DEVICE# hardware profiles key.\n"));
		return;
	}

    DWORD one = 0;
	if (RegSetValueEx(hKeyDevice,("Attach.ToDesktop"), 0, REG_DWORD, (unsigned char *)&one,4) != ERROR_SUCCESS) {
		vnclog.Print(LL_INTERR, VNCLOG("Can't set Attach.ToDesktop to 0x1\n"));
		return;
	}
	strcpy((LPSTR)&devmode.dmDeviceName[0], MIRROR_MiniportName);
	deviceName = (LPSTR)&dd.DeviceName[0];
	// Save the current desktop
	hdeskCurrent = GetThreadDesktop(GetCurrentThreadId());
	if (hdeskCurrent != NULL) {
		hdeskInput = OpenInputDesktop(0, FALSE, MAXIMUM_ALLOWED);
		if (hdeskInput != NULL)
			SetThreadDesktop(hdeskInput);
	}
	if (devmode.dmBitsPerPel==24) devmode.dmBitsPerPel=16;

    // Add 'Default.*' settings to the registry under above hKeyProfile\mirror\device
	INT code = ChangeDisplaySettingsEx(deviceName, &devmode, NULL, CDS_UPDATEREGISTRY, NULL);
    
	// Reset desktop
	SetThreadDesktop(hdeskCurrent);

	// Close the input desktop
	CloseDesktop(hdeskInput);

	FreeLibrary(hInstUser32);
	RegCloseKey(hKeyProfileMirror);
	RegCloseKey(hKeyDevice);
	return;
}



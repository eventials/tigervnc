; Script generated by the Inno Setup Script Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

[Setup]
AppName=TightVNC
AppVerName=TightVNC 1.2.1
AppVersion=1.2.1
AppPublisher=Const Kaplinsky
AppPublisherURL=http://www.tightvnc.com
AppSupportURL=http://www.tightvnc.com
AppUpdatesURL=http://www.tightvnc.com
DefaultDirName={pf}\TightVNC
DefaultGroupName=TightVNC
AlwaysCreateUninstallIcon=no
LicenseFile=LICENCE.txt

WindowVisible=No
CompressLevel=9
BackColor=clBlack
BackColor2=clBlue

; uncomment the following line if you want your installation to run on NT 3.51 too.
; MinVersion=4,3.51

[Files]
Source: "WinVNC.exe"; DestDir: "{app}"; CopyMode: alwaysoverwrite
Source: "README.txt"; DestDir: "{app}"; CopyMode: alwaysoverwrite
Source: "VNCHooks.dll"; DestDir: "{app}"; CopyMode: alwaysoverwrite
Source: "vncviewer.exe"; DestDir: "{app}"; CopyMode: alwaysoverwrite
Source: "WhatsNew.txt"; DestDir: "{app}"; CopyMode: alwaysoverwrite
Source: "LICENCE.txt"; DestDir: "{app}"; CopyMode: alwaysoverwrite
Source: "VNCHooks_Settings.reg"; DestDir: "{app}"; CopyMode: alwaysoverwrite

[Icons]
Name: "{group}\Launch TightVNC Server";               FileName: "{app}\WinVNC.exe";                                    WorkingDir: "{app}"
Name: "{group}\Show About Box";                       FileName: "{app}\WinVNC.exe";    Parameters: "-about";           WorkingDir: "{app}"
Name: "{group}\Show User Settings";                   FileName: "{app}\WinVNC.exe";    Parameters: "-settings";        WorkingDir: "{app}"
Name: "{group}\TightVNC Viewer (Best Compression)";   FileName: "{app}\vncviewer.exe"; Parameters: "-compresslevel 9 -quality 0"; WorkingDir: "{app}"
Name: "{group}\TightVNC Viewer (Fast Compression)";   FileName: "{app}\vncviewer.exe"; Parameters: "-encoding hextile"; WorkingDir: "{app}"
Name: "{group}\TightVNC Viewer (Listen Mode)";        FileName: "{app}\vncviewer.exe"; Parameters: "-listen";          WorkingDir: "{app}"
Name: "{group}\Administration\Install Default Registry Settings"; FileName: "{app}\VNCHooks_Settings.reg"; WorkingDir: "{app}"
Name: "{group}\Administration\Install VNC Service";   FileName: "{app}\WinVNC.exe";    Parameters: "-install";         WorkingDir: "{app}"
Name: "{group}\Administration\Remove VNC Service";    FileName: "{app}\WinVNC.exe";    Parameters: "-remove";          WorkingDir: "{app}"
Name: "{group}\Administration\Run Service Helper";    FileName: "{app}\WinVNC.exe";    Parameters: "-servicehelper";   WorkingDir: "{app}"
Name: "{group}\Administration\Show Default Settings"; FileName: "{app}\WinVNC.exe";    Parameters: "-defaultsettings"; WorkingDir: "{app}"


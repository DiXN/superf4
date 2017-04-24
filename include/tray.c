/*
  Copyright (C) 2015  Stefan Sundin

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
*/

NOTIFYICONDATA tray;
HICON icon[2];
int tray_added = 0;
extern int update;

int InitTray() {
  if (!disableTray) {
    // Load icons
    icon[0] = LoadImage(g_hinst, L"tray_disabled", IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
    icon[1] = LoadImage(g_hinst, L"tray_enabled", IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
    if (icon[0] == NULL || icon[1] == NULL) {
      Error(L"LoadImage('tray_*')", L"Could not load tray icons.", GetLastError());
      return 1;
    }

    // Create icondata
    tray.cbSize = sizeof(NOTIFYICONDATA);
    tray.uID = 0;
    tray.uFlags = NIF_INFO|NIF_MESSAGE|NIF_ICON|NIF_TIP;
    tray.hWnd = g_hwnd;
    tray.uCallbackMessage = WM_TRAY;

    // Register TaskbarCreated so we can re-add the tray icon if (when) explorer.exe crashes
    WM_TASKBARCREATED = RegisterWindowMessage(L"TaskbarCreated");

  }

  return 0;
}

int UpdateTray() {
  if (!disableTray) {
    wcsncpy(tray.szTip, (ENABLED() && elevated?l10n.tray_elevated:ENABLED()?l10n.tray_enabled:l10n.tray_disabled), ARRAY_SIZE(tray.szTip));
    tray.hIcon = icon[ENABLED()?1:0];

    if (Shell_NotifyIcon((tray_added?NIM_MODIFY:NIM_ADD),&tray)) {
      tray_added = 1;
    }
  }

  return 0;
}

int RemoveTray() {
  if (!disableTray) {
    if (!tray_added) {
      // Tray not added
      return 1;
    }

    if (Shell_NotifyIcon(NIM_DELETE,&tray) == FALSE) {
      Error(L"Shell_NotifyIcon(NIM_DELETE)", L"Failed to remove tray icon.", GetLastError());
      return 1;
    }

    // Success
    tray_added = 0;
  }

  return 0;
}

void showKillMessage(HANDLE process) {
  if(!disableTray) {
    wchar_t szProcessName[128] = L"can not get process info.";
    HMODULE hMod;
    DWORD cbNeeded;
    
    if (EnumProcessModules(process, &hMod, sizeof(hMod), &cbNeeded)) {
      GetModuleBaseName(process, hMod, szProcessName, sizeof(szProcessName)/sizeof(wchar_t));
    }

    wchar_t output[256];
    wcscpy(output, L"killed: ");
    wcscat(output, szProcessName);
    tray.uFlags = NIF_INFO|NIF_MESSAGE|NIF_ICON|NIF_TIP;
    wcscpy(tray.szInfo, output);
    tray.uTimeout = 3000;
    Shell_NotifyIcon(NIM_MODIFY, &tray);
  }
}

void ShowContextMenu(HWND hwnd) {
  POINT pt;
  GetCursorPos(&pt);
  HMENU menu = CreatePopupMenu();

  // Check autostart
  int autostart=0, autostart_elevate=0;
  CheckAutostart(&autostart, &autostart_elevate);
  // Check TimerCheck
  wchar_t txt[10];
  GetPrivateProfileString(L"General", L"TimerCheck", L"0", txt, ARRAY_SIZE(txt), inipath);
  short timercheck = _wtoi(txt);

  // Menu
  InsertMenu(menu, -1, MF_BYPOSITION, SWM_TOGGLE, (ENABLED()?l10n.menu.disable:l10n.menu.enable));
  InsertMenu(menu, -1, MF_BYPOSITION|(elevated || !vista?MF_DISABLED|MF_GRAYED:0), SWM_ELEVATE, (elevated?l10n.menu.elevated:l10n.menu.elevate));

  // Options
  HMENU menu_options = CreatePopupMenu();
  InsertMenu(menu_options, -1, MF_BYPOSITION|(autostart?MF_CHECKED:0), (autostart?SWM_AUTOSTART_OFF:SWM_AUTOSTART_ON), l10n.menu.autostart);
  InsertMenu(menu_options, -1, MF_BYPOSITION|(autostart_elevate?MF_CHECKED:0)|(!vista?MF_DISABLED|MF_GRAYED:0), (autostart_elevate?SWM_AUTOSTART_ELEVATE_OFF:SWM_AUTOSTART_ELEVATE_ON), l10n.menu.autostart_elevate);
  InsertMenu(menu_options, -1, MF_BYPOSITION|MF_SEPARATOR, 0, NULL);
  InsertMenu(menu_options, -1, MF_BYPOSITION|(timercheck?MF_CHECKED:0), (timercheck?SWM_TIMERCHECK_OFF:SWM_TIMERCHECK_ON), l10n.menu.timercheck);
  InsertMenu(menu_options, -1, MF_BYPOSITION|MF_SEPARATOR, 0, NULL);
  InsertMenu(menu_options, -1, MF_BYPOSITION, SWM_WEBSITE, l10n.menu.website);
  InsertMenu(menu_options, -1, MF_BYPOSITION, SWM_INI, l10n.menu.ini);
  InsertMenu(menu_options, -1, MF_BYPOSITION|MF_DISABLED|MF_GRAYED, 0, l10n.menu.version);
  InsertMenu(menu, -1, MF_BYPOSITION|MF_POPUP, (UINT_PTR)menu_options, l10n.menu.options);

  InsertMenu(menu, -1, MF_BYPOSITION|MF_SEPARATOR, 0, NULL);
  InsertMenu(menu, -1, MF_BYPOSITION, SWM_XKILL, l10n.menu.xkill);

  InsertMenu(menu, -1, MF_BYPOSITION|MF_SEPARATOR, 0, NULL);
  InsertMenu(menu, -1, MF_BYPOSITION, SWM_EXIT, l10n.menu.exit);

  // Track menu
  SetForegroundWindow(hwnd);
  TrackPopupMenu(menu, TPM_BOTTOMALIGN, pt.x, pt.y, 0, hwnd, NULL);
  DestroyMenu(menu);
}

int OpenUrl(wchar_t *url) {
  int ret = (INT_PTR) ShellExecute(NULL, L"open", url, NULL, NULL, SW_SHOWDEFAULT);
  if (ret <= 32 && MessageBox(NULL,L"Failed to open browser. Copy url to clipboard?",APP_NAME,MB_ICONWARNING|MB_YESNO) == IDYES) {
    int size = (wcslen(url)+1)*sizeof(url[0]);
    wchar_t *data = LocalAlloc(LMEM_FIXED, size);
    memcpy(data, url, size);
    OpenClipboard(NULL);
    EmptyClipboard();
    SetClipboardData(CF_UNICODETEXT, data);
    CloseClipboard();
    LocalFree(data);
  }
  return ret;
}

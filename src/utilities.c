/**********/
/* utilities.c */
/**********/

/*
 * Utility functions and helper routines
 * 
 * Contents:
 * - Random number generation (simple modulo-based PRNG)
 * - Error message display with string resource loading
 * - Menu management (checkmarks, visibility toggling)
 * - Dialog helpers (integer retrieval with range validation)
 * - System metrics caching and initialization
 * - Multi-monitor aware positioning
 * 
 * Initialization sequence:
 * 1. Seed PRNG with GetCurrentTime()
 * 2. Load string resources (app name, default player name, time format)
 * 3. Cache system metrics (caption height, border sizes, menu height)
 * 4. Check for existing registry configuration
 * 5. If first run, migrate from legacy .ini file
 * 
 * Platform notes:
 * - Uses SM_CXVIRTUALSCREEN for multi-monitor support
 * - Falls back to SM_CXSCREEN on older Windows versions
 * - Menu wrapping detection handles variable-height font rendering
 */

#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <strsafe.h>

#include "main.h"
#include "resource.h"
#include "preferences.h"
#include "utilities.h"
#include "sound.h"
#include "game.h"
#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include "dos.h"

extern INT dypBorder;
extern INT dxpBorder;
extern INT dypCaption;
extern INT dypMenu;

extern TCHAR g_WindowClass[cchNameMax];
extern TCHAR szTime[cchNameMax];
extern TCHAR szDefaultName[cchNameMax];

extern HANDLE g_AppInstance;
extern HWND   g_MainWindow;
extern HMENU  g_MenuHandle;

extern PREF g_GameConfig;

extern  HKEY g_hReg;
extern  TCHAR * rgszPref[iszPrefMax];
TCHAR   szIniFile[] = TEXT("entpack.ini");


/****** R N D ******/

/* Return a random number between 0 and rndMax */

INT GenerateRandomNumber(INT rndMax)
{
        return (rand() % rndMax);
}



/****** R E P O R T  E R R ******/

/* Report and error and exit */

VOID DisplayErrorMessage(WORD idErr)
{
        TCHAR szMsg[cchMsgMax];
        TCHAR szMsgTitle[cchMsgMax];

		if (idErr < ID_ERR_MAX)
				LoadString(g_AppInstance, idErr, szMsg, cchMsgMax);
		else
				{
				LoadString(g_AppInstance, ID_ERR_UNKNOWN, szMsgTitle, cchMsgMax);
				StringCchPrintf(szMsg, ARRAYSIZE(szMsg), szMsgTitle, idErr);
				}

        LoadString(g_AppInstance, ID_ERR_TITLE, szMsgTitle, cchMsgMax);

        MessageBox(NULL, szMsg, szMsgTitle, MB_OK | MB_ICONHAND);
}


/****** L O A D  S Z ******/

VOID LoadStringResource(WORD id, TCHAR * sz, DWORD cch)
{
        if (LoadString(g_AppInstance, id, sz, cch) == 0)
                DisplayErrorMessage(1001);
}


// Routines to read the ini file.

INT ReadIniInteger(INT iszPref, INT valDefault, INT valMin, INT valMax)
{
	return max(valMin, min(valMax,
		(INT) GetPrivateProfileInt(g_WindowClass, rgszPref[iszPref], valDefault, szIniFile) ) );
}

#define ReadIniBoolean(iszPref, valDefault) ReadIniInteger(iszPref, valDefault, 0, 1)


VOID ReadIniString(INT iszPref, TCHAR FAR * szRet)
{
	GetPrivateProfileString(g_WindowClass, rgszPref[iszPref], szDefaultName, szRet, cchNameMax, szIniFile);
}




/****** I N I T  C O N S T ******/

VOID InitializeConstants(VOID)
{
INT     iAlreadyPlayed = 0;     // have we already updated the registry ?
DWORD   dwDisposition;
       

        srand(LOWORD(GetCurrentTime()));

        LoadStringResource(ID_GAMENAME, g_WindowClass, ARRAYSIZE(g_WindowClass));
        LoadStringResource(ID_MSG_SEC,  szTime, ARRAYSIZE(szTime));
        LoadStringResource(ID_NAME_DEFAULT, szDefaultName, ARRAYSIZE(szDefaultName));

        dypCaption = GetSystemMetrics(SM_CYCAPTION) + 1;
        dypMenu    = GetSystemMetrics(SM_CYMENU)    + 1;
        dypBorder  = GetSystemMetrics(SM_CYBORDER)  + 1;
        dxpBorder  = GetSystemMetrics(SM_CXBORDER)  + 1;

        // Open the registry key;
        if (RegCreateKeyEx(HKEY_CURRENT_USER, SZWINMINEREG, 0, NULL, 0, KEY_READ, NULL, 
                   &g_hReg, &dwDisposition) == ERROR_SUCCESS)
        {
            iAlreadyPlayed = ReadRegistryInteger(iszPrefAlreadyPlayed, 0, 0, 1);
            RegCloseKey(g_hReg);
        }


        // Read it from the .ini file and write it to registry.
        if (!iAlreadyPlayed)
        {
            g_GameConfig.Height= ReadIniInteger(iszPrefHeight,MINHEIGHT,DEFHEIGHT,25);
            g_GameConfig.Width= ReadIniInteger(iszPrefWidth,MINWIDTH,DEFWIDTH,30);

            g_GameConfig.wGameType = (WORD)ReadIniInteger(iszPrefGame,wGameBegin, wGameBegin, wGameExpert+1);
            g_GameConfig.Mines    = ReadIniInteger(iszPrefMines, 10, 10, 999);
            g_GameConfig.xWindow  = ReadIniInteger(iszPrefxWindow, 80, 0, 1024);
            g_GameConfig.yWindow  = ReadIniInteger(iszPrefyWindow, 80, 0, 1024);

            g_GameConfig.fSound = ReadIniInteger(iszPrefSound, 0, 0, fsoundOn);
            g_GameConfig.fMark  = ReadIniBoolean(iszPrefMark,  TRUE);
            g_GameConfig.fTick  = ReadIniBoolean(iszPrefTick,  FALSE);
            g_GameConfig.fMenu  = ReadIniInteger(iszPrefMenu,  fmenuAlwaysOn, fmenuAlwaysOn, fmenuOn);
	
            g_GameConfig.rgTime[wGameBegin]  = ReadIniInteger(iszPrefBeginTime, 999, 0, 999);
            g_GameConfig.rgTime[wGameInter]  = ReadIniInteger(iszPrefInterTime, 999, 0, 999);
            g_GameConfig.rgTime[wGameExpert] = ReadIniInteger(iszPrefExpertTime, 999, 0, 999);

            ReadIniString(iszPrefBeginName, g_GameConfig.szBegin);
            ReadIniString(iszPrefInterName, g_GameConfig.szInter);
            ReadIniString(iszPrefExpertName, g_GameConfig.szExpert);

            if (FSoundOn())
                g_GameConfig.fSound = InitializeAudioSystem();
            
            // Write it to registry.
            SaveConfiguration();
        }

}



/* * * * * *  M E N U S  * * * * * */

/****** C H E C K  E M ******/

VOID SetMenuCheckmark(WORD idm, BOOL fCheck)
{
        CheckMenuItem(g_MenuHandle, idm, fCheck ? MF_CHECKED : MF_UNCHECKED);
}

/****** S E T  M E N U  B A R ******/

VOID SetMenuVisibility(INT fActive)
{
        g_GameConfig.fMenu = fActive;
        UpdateMenuStates();
        SetMenu(g_MainWindow, FMenuOn() ? g_MenuHandle : NULL);
        ResizeGameWindow(fResize);
}


/****** D O  A B O U T ******/

VOID ShowAboutDialog(VOID)
{
        TCHAR szVersion[cchMsgMax];
        TCHAR szCredit[cchMsgMax];

        LoadStringResource(ID_MSG_VERSION, szVersion, ARRAYSIZE(szVersion));
        LoadStringResource(ID_MSG_CREDIT,  szCredit, ARRAYSIZE(szCredit));

        ShellAbout(g_MainWindow,
          szVersion, szCredit, LoadIcon(NULL, IDI_APPLICATION));
}


/****** G E T  D L G  I N T ******/

INT GetDialogInteger(HWND hDlg, INT dlgID, INT numLo, INT numHi)
{
        INT num;
        BOOL fFlag;

        num = GetDlgItemInt(hDlg, dlgID, &fFlag, FALSE);

        if (num < numLo)
                num = numLo;
        else if (num > numHi)
                num = numHi;

        return num;
}


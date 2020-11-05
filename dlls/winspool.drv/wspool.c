/******************************************************************************
 * Print Spooler Functions
 *
 *
 * Copyright 1999 Thuy Nguyen
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */


#include "config.h"
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winspool.h"

#include "winreg.h"
#include "ddk/winsplp.h"
#include "wine/debug.h"

#include "wspool.h"

WINE_DEFAULT_DEBUG_CHANNEL(winspool);

/* ############################### */

static CRITICAL_SECTION backend_cs;
static CRITICAL_SECTION_DEBUG backend_cs_debug =
{
    0, 0, &backend_cs,
    { &backend_cs_debug.ProcessLocksList, &backend_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": backend_cs") }
};
static CRITICAL_SECTION backend_cs = { &backend_cs_debug, -1, 0, 0, 0, 0 };

/* ############################### */

HINSTANCE WINSPOOL_hInstance = NULL;

static HMODULE hlocalspl = NULL;
static BOOL (WINAPI *pInitializePrintProvidor)(LPPRINTPROVIDOR, DWORD, LPWSTR);

PRINTPROVIDOR * backend = NULL;

/******************************************************************************
 * load_backend [internal]
 *
 * load and init our backend (the local printprovider: "localspl.dll")
 *
 * PARAMS
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE and RPC_S_SERVER_UNAVAILABLE
 *
 * NOTES
 *  In windows, winspool.drv use RPC to interact with the spooler service
 *  (spoolsv.exe with spoolss.dll) and the spooler router (spoolss.dll) interact
 *  with the correct printprovider (localspl.dll for the local system)
 *
 */
BOOL load_backend(void)
{
    static PRINTPROVIDOR mybackend;
    DWORD res;

    EnterCriticalSection(&backend_cs);
    hlocalspl = LoadLibraryA("localspl.dll");
    if (hlocalspl) {
        pInitializePrintProvidor = (void *) GetProcAddress(hlocalspl, "InitializePrintProvidor");
        if (pInitializePrintProvidor) {

            /* native localspl does not clear unused entries */
            memset(&mybackend, 0, sizeof(mybackend));
            res = pInitializePrintProvidor(&mybackend, sizeof(mybackend), NULL);
            if (res) {
                backend = &mybackend;
                LeaveCriticalSection(&backend_cs);
                TRACE("backend: %p (%p)\n", backend, hlocalspl);
                return TRUE;
            }
        }
        FreeLibrary(hlocalspl);
    }

    LeaveCriticalSection(&backend_cs);

    WARN("failed to load the backend: %u\n", GetLastError());
    SetLastError(RPC_S_SERVER_UNAVAILABLE);
    return FALSE;
}

static void create_color_profiles(void)
{
    static const WCHAR color_dir[] = {'\\','s','p','o','o','l',
                                      '\\','d','r','i','v','e','r','s',
                                      '\\','c','o','l','o','r','\\',0};
    static const WCHAR srgb_icm[] = {'s','R','G','B',' ',
                                     'C','o','l','o','r',' ',
                                     'S','p','a','c','e',' ',
                                     'P','r','o','f','i','l','e','.','i','c','m',0};
    WCHAR profile_path[MAX_PATH];
    HANDLE file;
    HRSRC res;
    DWORD size, written;
    char *data;
    BOOL ret;

    GetSystemDirectoryW(profile_path, ARRAY_SIZE(profile_path));
    lstrcatW(profile_path, color_dir);
    lstrcatW(profile_path, srgb_icm);

    file = CreateFileW(profile_path, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE)
        return;

    ret = ((res = FindResourceA(WINSPOOL_hInstance, MAKEINTRESOURCEA(IDR_SRGB_ICM), (const char *)RT_RCDATA)) &&
           (size = SizeofResource(WINSPOOL_hInstance, res)) &&
           (data = LoadResource(WINSPOOL_hInstance, res)) &&
           WriteFile(file, data, size, &written, NULL) &&
           written == size);
    CloseHandle(file);
    if (!ret)
    {
        ERR("Failed to create %s\n", wine_dbgstr_w(profile_path));
        DeleteFileW(profile_path);
    }
}

/******************************************************************************
 *  DllMain
 *
 * Winspool entry point.
 *
 */
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD reason, LPVOID lpReserved)
{
  switch (reason)
  {
    case DLL_PROCESS_ATTACH: {
      WINSPOOL_hInstance = hInstance;
      DisableThreadLibraryCalls(hInstance);
      WINSPOOL_LoadSystemPrinters();
      create_color_profiles();
      break;
    }
    case DLL_PROCESS_DETACH:
      if (lpReserved) break;
      DeleteCriticalSection(&backend_cs);
      FreeLibrary(hlocalspl);
      break;
  }

  return TRUE;
}

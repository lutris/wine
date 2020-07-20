/*
 * Copyright 2016 Austin English
 * Copyright 2016 Michael Müller
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

#include <windows.h>
#include <wine/debug.h>

#include "resources.h"

WINE_DEFAULT_DEBUG_CHANNEL(fsutil);

static void output_write(const WCHAR *str, DWORD wlen)
{
    DWORD count, ret;

    ret = WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), str, wlen, &count, NULL);
    if (!ret)
    {
        DWORD len;
        char  *msgA;

        /* On Windows WriteConsoleW() fails if the output is redirected. So fall
         * back to WriteFile(), assuming the console encoding is still the right
         * one in that case.
         */
        len = WideCharToMultiByte(GetConsoleOutputCP(), 0, str, wlen, NULL, 0, NULL, NULL);
        msgA = HeapAlloc(GetProcessHeap(), 0, len * sizeof(char));
        if (!msgA) return;

        WideCharToMultiByte(GetConsoleOutputCP(), 0, str, wlen, msgA, len, NULL, NULL);
        WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), msgA, len, &count, FALSE);
        HeapFree(GetProcessHeap(), 0, msgA);
    }
}

static int output_vprintf(const WCHAR* fmt, __ms_va_list va_args)
{
    WCHAR str[8192];
    int len;

    SetLastError(NO_ERROR);
    len = FormatMessageW(FORMAT_MESSAGE_FROM_STRING, fmt, 0, 0, str,
                         sizeof(str)/sizeof(*str), &va_args);
    if (len == 0 && GetLastError() != NO_ERROR)
        WINE_FIXME("Could not format string: le=%u, fmt=%s\n", GetLastError(), wine_dbgstr_w(fmt));
    else
        output_write(str, len);
    return 0;
}

static int WINAPIV output_string(int msg, ...)
{
    WCHAR fmt[8192];
    __ms_va_list arguments;

    LoadStringW(GetModuleHandleW(NULL), msg, fmt, sizeof(fmt)/sizeof(fmt[0]));
    __ms_va_start(arguments, msg);
    output_vprintf(fmt, arguments);
    __ms_va_end(arguments);
    return 0;
}

static BOOL output_error_string(DWORD error)
{
    LPWSTR pBuffer;
    if (FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_ALLOCATE_BUFFER,
            NULL, error, 0, (LPWSTR)&pBuffer, 0, NULL))
    {
        output_write(pBuffer, lstrlenW(pBuffer));
        LocalFree(pBuffer);
        return TRUE;
    }
    return FALSE;
}

static int create_hardlink(int argc, WCHAR *argv[])
{
    if (argc != 5)
    {
        output_string(STRING_HARDLINK_CREATE_USAGE);
        return 1;
    }

    if (CreateHardLinkW(argv[3], argv[4], NULL))
        return 0;

    output_error_string(GetLastError());
    return 1;
}

static int hardlink(int argc, WCHAR *argv[])
{
    static const WCHAR createW[]={'c','r','e','a','t','e',0};
    int ret;

    if (argc > 2)
    {
        if (!_wcsicmp(argv[2], createW))
            return create_hardlink(argc, argv);
        else
        {
            FIXME("unsupported parameter %s\n", debugstr_w(argv[2]));
            output_string(STRING_UNSUPPORTED_PARAM, argv[2]);
            ret = 1;
        }
    }

    output_string(STRING_HARDLINK_USAGE);
    return ret;
}

int __cdecl wmain(int argc, WCHAR *argv[])
{
    static const WCHAR hardlinkW[]={'h','a','r','d','l','i','n','k',0};
    int i, ret = 0;

    for (i = 0; i < argc; i++)
        WINE_TRACE(" %s", wine_dbgstr_w(argv[i]));
    WINE_TRACE("\n");

    if (argc > 1)
    {
        if (!_wcsicmp(argv[1], hardlinkW))
            return hardlink(argc, argv);
        else
        {
            FIXME("unsupported command %s\n", debugstr_w(argv[1]));
            output_string(STRING_UNSUPPORTED_CMD, argv[1]);
            ret = 1;
        }
    }

    output_string(STRING_USAGE);
    return ret;
}

/*
 * MAPISendMail implementation for xdg-email
 *
 * Copyright 2016 Jeremy White for CodeWeavers
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

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winnls.h"
#include "mapi.h"
#include "process.h"
#include "wine/heap.h"
#include "wine/debug.h"

#include "winemapi_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(winemapi);

static inline WCHAR *heap_strdupAtoW(const char *str)
{
    LPWSTR ret = NULL;

    if(str) {
        DWORD len;

        len = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
        ret = heap_alloc(len*sizeof(WCHAR));
        if(ret)
            MultiByteToWideChar(CP_ACP, 0, str, -1, ret, len);
    }

    return ret;
}

static void add_argument(char **argv, int *argc, const char *arg, const char *param)
{
    argv[(*argc)] = heap_alloc(strlen(arg) + 1);
    strcpy(argv[(*argc)++], arg);
    if (param)
    {
        argv[(*argc)] = heap_alloc(strlen(param) + 1);
        strcpy(argv[(*argc)++], param);
    }
}

static void add_target(char **argv, int *argc, ULONG class, const char *address)
{
    static const char smtp[] = "smtp:";

    if (!strncasecmp(address, smtp, sizeof(smtp) - 1))
        address += sizeof(smtp) - 1;

    switch (class)
    {
        case MAPI_ORIG:
            TRACE("From: %s\n (unused)", debugstr_a(address));
            break;

        case MAPI_TO:
            TRACE("To: %s\n", debugstr_a(address));
            add_argument(argv, argc, address, NULL);
            break;

        case MAPI_CC:
            TRACE("CC: %s\n", debugstr_a(address));
            add_argument(argv, argc, "--cc", address);
            break;

        case MAPI_BCC:
            TRACE("BCC: %s\n", debugstr_a(address));
            add_argument(argv, argc, "--bcc", address);
            break;

        default:
            TRACE("Unknown recipient class: %d\n", class);
    }
}

static void add_file(char **argv, int *argc, const char *path, const char *file)
{
    char fullname[MAX_PATH] = {0};
    WCHAR *fullnameW;
    char *unixpath;

    if (path)
    {
        strcpy(fullname, path);
        strcat(fullname, "\\");
    }
    if (file)
        strcat(fullname, file);

    fullnameW = heap_strdupAtoW(fullname);
    if(!fullnameW)
    {
        ERR("Out of memory\n");
        return;
    }

    unixpath = wine_get_unix_file_name(fullnameW);
    if (unixpath)
    {
        add_argument(argv, argc, "--attach", unixpath);
        heap_free(unixpath);
    }
    else
        ERR("Cannot find unix path of '%s'; not attaching.\n", debugstr_w(fullnameW));

    heap_free(fullnameW);
}

/**************************************************************************
 *  XDGIsAvailable
 *
 */
BOOLEAN XDGMailAvailable(void)
{
#ifdef HAVE_UNISTD_H
    char *p, *q, *test;
    int len;
    int rc;

    for (p = getenv("PATH"); p; p = q)
    {
        while (*p == ':')
            p++;

        if (!*p)
            break;

        q = strchr(p, ':');
        len = q ? q - p : strlen(p);

        test = heap_alloc(len + strlen("xdg-email") + 2); /* '/' + NULL */
        if(!test)
            break;
        memcpy(test, p, len);
        strcpy(test + len, "/");
        strcat(test, "xdg-email");

        rc = access(test, X_OK);
        heap_free(test);

        if (rc == 0)
            return TRUE;
    }
#endif
    return FALSE;
}

/**************************************************************************
 *  XDGSendMail
 *
 * Send a message using xdg-email mail client.
 *
 */
ULONG XDGSendMail(LHANDLE session, ULONG_PTR uiparam,
    lpMapiMessage message, FLAGS flags, ULONG reserved)
{
    int i;
    int argc = 0;
    int max_args;
    char **argv = NULL;
    ULONG ret = MAPI_E_FAILURE;

    TRACE("(0x%08lx 0x%08lx %p 0x%08x 0x%08x)\n", session, uiparam, message, flags, reserved);

    if (!message)
        return MAPI_E_FAILURE;

    max_args = 1 + (2 + message->nRecipCount + message->nFileCount) * 2;
    argv = heap_alloc_zero( (max_args + 1) * sizeof(*argv));

    add_argument(argv, &argc, "xdg-email", NULL);

    if (message->lpOriginator)
        TRACE("From: %s (unused)\n", debugstr_a(message->lpOriginator->lpszAddress));

    for (i = 0; i < message->nRecipCount; i++)
    {
        if (!message->lpRecips)
        {
            WARN("Recipient %d missing\n", i);
            goto exit;
        }

        if (message->lpRecips[i].lpszAddress)
            add_target(argv, &argc, message->lpRecips[i].ulRecipClass,
                        message->lpRecips[i].lpszAddress);
        else
            FIXME("Name resolution and entry identifiers not supported\n");
    }

    for (i = 0; i < message->nFileCount; i++)
    {
        TRACE("File Path: %s, name %s\n", debugstr_a(message->lpFiles[i].lpszPathName),
                debugstr_a(message->lpFiles[i].lpszFileName));
        add_file(argv, &argc, message->lpFiles[i].lpszPathName, message->lpFiles[i].lpszFileName);
    }

    if (message->lpszSubject)
    {
        TRACE("Subject: %s\n", debugstr_a(message->lpszSubject));
        add_argument(argv, &argc, "--subject", message->lpszSubject);
    }

    if (message->lpszNoteText)
    {
        TRACE("Body: %s\n", debugstr_a(message->lpszNoteText));
        add_argument(argv, &argc, "--body", message->lpszNoteText);
    }

    TRACE("Executing xdg-email; parameters:\n");
    for (i = 0; argv[i] && i <= max_args; i++)
        TRACE(" %d: [%s]\n", i, argv[i]);
    if (_spawnvp(_P_WAIT, "xdg-email", (const char **) argv) == 0)
        ret = SUCCESS_SUCCESS;

exit:
    heap_free(argv);
    return ret;
}

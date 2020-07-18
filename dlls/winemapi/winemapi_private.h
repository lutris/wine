/*
 *
 * Copyright 2016 Jeremy White
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

#ifndef __WINEMAPI_PRIVATE_INCLUDED__
#define __WINEMAPI_PRIVATE_INCLUDED__

ULONG XDGSendMail(LHANDLE session, ULONG_PTR uiparam,
    lpMapiMessage message, FLAGS flags, ULONG reserved);
BOOLEAN XDGMailAvailable(void);

#endif /* __WINEMAPI_PRIVATE_INCLUDED__ */

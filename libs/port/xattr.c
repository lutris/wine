/*
 * extended attributes functions
 *
 * Copyright 2014 Erich E. Hoover
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
#include "wine/port.h"

#if defined(HAVE_ATTR_XATTR_H)
# undef XATTR_ADDITIONAL_OPTIONS
# include <attr/xattr.h>
#elif defined(HAVE_SYS_XATTR_H)
# include <sys/xattr.h>
#elif defined(HAVE_SYS_EXTATTR_H)
# undef XATTR_ADDITIONAL_OPTIONS
# include <sys/extattr.h>
#endif

#include <ctype.h>
#include <errno.h>

#ifndef XATTR_USER_PREFIX_LEN
# define XATTR_USER_PREFIX_LEN (sizeof(XATTR_USER_PREFIX) - 1)
#endif

#ifdef HAVE_SYS_EXTATTR_H
static inline int xattr_valid_namespace( const char *name )
{
    if (strncmp( XATTR_USER_PREFIX, name, XATTR_USER_PREFIX_LEN ) != 0)
    {
        errno = EPERM;
        return 0;
    }
    return 1;
}
#endif

int xattr_fget( int filedes, const char *name, void *value, size_t size )
{
#if defined(XATTR_ADDITIONAL_OPTIONS)
    return fgetxattr( filedes, name, value, size, 0, 0 );
#elif defined(HAVE_SYS_XATTR_H) || defined(HAVE_ATTR_XATTR_H)
    return fgetxattr( filedes, name, value, size );
#elif defined(HAVE_SYS_EXTATTR_H)
    if (!xattr_valid_namespace( name )) return -1;
    return extattr_get_fd( filedes, EXTATTR_NAMESPACE_USER, &name[XATTR_USER_PREFIX_LEN],
                           value, size );
#else
    errno = ENOSYS;
    return -1;
#endif
}

int xattr_fremove( int filedes, const char *name )
{
#if defined(XATTR_ADDITIONAL_OPTIONS)
    return fremovexattr( filedes, name, 0 );
#elif defined(HAVE_SYS_XATTR_H) || defined(HAVE_ATTR_XATTR_H)
    return fremovexattr( filedes, name );
#elif defined(HAVE_SYS_EXTATTR_H)
    if (!xattr_valid_namespace( name )) return -1;
    return extattr_delete_fd( filedes, EXTATTR_NAMESPACE_USER, &name[XATTR_USER_PREFIX_LEN] );
#else
    errno = ENOSYS;
    return -1;
#endif
}

int xattr_fset( int filedes, const char *name, void *value, size_t size )
{
#if defined(XATTR_ADDITIONAL_OPTIONS)
    return fsetxattr( filedes, name, value, size, 0, 0 );
#elif defined(HAVE_SYS_XATTR_H) || defined(HAVE_ATTR_XATTR_H)
    return fsetxattr( filedes, name, value, size, 0 );
#elif defined(HAVE_SYS_EXTATTR_H)
    if (!xattr_valid_namespace( name )) return -1;
    return extattr_set_fd( filedes, EXTATTR_NAMESPACE_USER, &name[XATTR_USER_PREFIX_LEN],
                           value, size );
#else
    errno = ENOSYS;
    return -1;
#endif
}

int xattr_get( const char *path, const char *name, void *value, size_t size )
{
#if defined(XATTR_ADDITIONAL_OPTIONS)
    return getxattr( path, name, value, size, 0, 0 );
#elif defined(HAVE_SYS_XATTR_H) || defined(HAVE_ATTR_XATTR_H)
    return getxattr( path, name, value, size );
#elif defined(HAVE_SYS_EXTATTR_H)
    if (!xattr_valid_namespace( name )) return -1;
    return extattr_get_file( path, EXTATTR_NAMESPACE_USER, &name[XATTR_USER_PREFIX_LEN],
                             value, size );
#else
    errno = ENOSYS;
    return -1;
#endif
}

int xattr_remove( const char *path, const char *name )
{
#if defined(XATTR_ADDITIONAL_OPTIONS)
    return removexattr( path, name, 0 );
#elif defined(HAVE_SYS_XATTR_H) || defined(HAVE_ATTR_XATTR_H)
    return removexattr( path, name );
#elif defined(HAVE_SYS_EXTATTR_H)
    if (!xattr_valid_namespace( name )) return -1;
    return extattr_delete_file( path, EXTATTR_NAMESPACE_USER, &name[XATTR_USER_PREFIX_LEN] );
#else
    errno = ENOSYS;
    return -1;
#endif
}

int xattr_set( const char *path, const char *name, void *value, size_t size )
{
#if defined(XATTR_ADDITIONAL_OPTIONS)
    return setxattr( path, name, value, size, 0, 0 );
#elif defined(HAVE_SYS_XATTR_H) || defined(HAVE_ATTR_XATTR_H)
    return setxattr( path, name, value, size, 0 );
#elif defined(HAVE_SYS_EXTATTR_H)
    if (!xattr_valid_namespace( name )) return -1;
    return extattr_set_file( path, EXTATTR_NAMESPACE_USER, &name[XATTR_USER_PREFIX_LEN],
                             value, size );
#else
    errno = ENOSYS;
    return -1;
#endif
}

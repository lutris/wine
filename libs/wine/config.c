/*
 * Configuration parameters shared between Wine server and clients
 *
 * Copyright 2002 Alexandre Julliard
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

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_PWD_H
#include <pwd.h>
#endif
#ifdef __APPLE__
#include <crt_externs.h>
#include <spawn.h>
#ifndef _POSIX_SPAWN_DISABLE_ASLR
#define _POSIX_SPAWN_DISABLE_ASLR 0x0100
#endif
#endif
#include "wine/library.h"

static const char server_config_dir[] = "/.wine";        /* config dir relative to $HOME */
static const char server_root_prefix[] = "/tmp/.wine";   /* prefix for server root dir */
static const char server_dir_prefix[] = "/server-";      /* prefix for server dir */

static char *bindir;
static char *dlldir;
static char *datadir;
static char *config_dir;
static char *server_dir;
static char *build_dir;
static char *user_name;
static char *argv0_name;
static char *wineserver64;

#ifdef __GNUC__
static void fatal_error( const char *err, ... )  __attribute__((noreturn,format(printf,1,2)));
static void fatal_perror( const char *err, ... )  __attribute__((noreturn,format(printf,1,2)));
#endif

#if defined(__linux__) || defined(__FreeBSD_kernel__) || defined(__NetBSD__)
static const char exe_link[] = "/proc/self/exe";
#elif defined (__FreeBSD__) || defined(__DragonFly__)
static const char exe_link[] = "/proc/curproc/file";
#else
static const char exe_link[] = "";
#endif

/* die on a fatal error */
static void fatal_error( const char *err, ... )
{
    va_list args;

    va_start( args, err );
    fprintf( stderr, "wine: " );
    vfprintf( stderr, err, args );
    va_end( args );
    exit(1);
}

/* die on a fatal error */
static void fatal_perror( const char *err, ... )
{
    va_list args;

    va_start( args, err );
    fprintf( stderr, "wine: " );
    vfprintf( stderr, err, args );
    perror( " " );
    va_end( args );
    exit(1);
}

/* malloc wrapper */
static void *xmalloc( size_t size )
{
    void *res;

    if (!size) size = 1;
    if (!(res = malloc( size ))) fatal_error( "virtual memory exhausted\n");
    return res;
}

/* strdup wrapper */
static char *xstrdup( const char *str )
{
    size_t len = strlen(str) + 1;
    char *res = xmalloc( len );
    memcpy( res, str, len );
    return res;
}

/* check if a string ends in a given substring */
static inline int strendswith( const char* str, const char* end )
{
    size_t len = strlen( str );
    size_t tail = strlen( end );
    return len >= tail && !strcmp( str + len - tail, end );
}

/* remove all trailing slashes from a path name */
static inline void remove_trailing_slashes( char *path )
{
    int len = strlen( path );
    while (len > 1 && path[len-1] == '/') path[--len] = 0;
}

/* build a path from the specified dir and name */
static char *build_path( const char *dir, const char *name )
{
    size_t len = strlen(dir);
    char *ret = xmalloc( len + strlen(name) + 2 );

    memcpy( ret, dir, len );
    if (len && ret[len-1] != '/') ret[len++] = '/';
    strcpy( ret + len, name );
    return ret;
}

/* return the directory that contains the library at run-time */
static char *get_runtime_libdir(void)
{
#ifdef HAVE_DLADDR
    Dl_info info;
    char *libdir;

    if (dladdr( get_runtime_libdir, &info ) && info.dli_fname[0] == '/')
    {
        const char *p = strrchr( info.dli_fname, '/' );
        unsigned int len = p - info.dli_fname;
        if (!len) len++;  /* include initial slash */
        libdir = xmalloc( len + 1 );
        memcpy( libdir, info.dli_fname, len );
        libdir[len] = 0;
        return libdir;
    }
#endif /* HAVE_DLADDR */
    return NULL;
}

/* read a symlink and return its directory */
static char *symlink_dirname( const char *name )
{
    char *p, *fullpath = realpath( name, NULL );

    if (fullpath)
    {
        p = strrchr( fullpath, '/' );
        if (p == fullpath) p++;
        if (p) *p = 0;
    }
    return fullpath;
}

/* return the directory that contains the main exe at run-time */
static char *get_runtime_exedir(void)
{
    if (exe_link[0]) return symlink_dirname( exe_link );
    return NULL;
}

/* return the base directory from argv0 */
static char *get_runtime_argvdir( const char *argv0 )
{
    char *p, *bindir, *cwd;
    int len, size;

    if (!(p = strrchr( argv0, '/' ))) return NULL;

    len = p - argv0;
    if (!len) len++;  /* include leading slash */

    if (argv0[0] == '/')  /* absolute path */
    {
        bindir = xmalloc( len + 1 );
        memcpy( bindir, argv0, len );
        bindir[len] = 0;
    }
    else
    {
        /* relative path, make it absolute */
        for (size = 256 + len; ; size *= 2)
        {
            if (!(cwd = malloc( size ))) return NULL;
            if (getcwd( cwd, size - len ))
            {
                bindir = cwd;
                cwd += strlen(cwd);
                *cwd++ = '/';
                memcpy( cwd, argv0, len );
                cwd[len] = 0;
                break;
            }
            free( cwd );
            if (errno != ERANGE) return NULL;
        }
    }
    return bindir;
}

/* initialize the server directory value */
static void init_server_dir( dev_t dev, ino_t ino )
{
    char *p, *root;

#ifdef __ANDROID__  /* there's no /tmp dir on Android */
    root = build_path( config_dir, ".wineserver" );
#else
    root = xmalloc( sizeof(server_root_prefix) + 12 );
    sprintf( root, "%s-%u", server_root_prefix, getuid() );
#endif

    server_dir = xmalloc( strlen(root) + sizeof(server_dir_prefix) + 2*sizeof(dev) + 2*sizeof(ino) + 2 );
    strcpy( server_dir, root );
    strcat( server_dir, server_dir_prefix );
    p = server_dir + strlen(server_dir);

    if (dev != (unsigned long)dev)
        p += sprintf( p, "%lx%08lx-", (unsigned long)((unsigned long long)dev >> 32), (unsigned long)dev );
    else
        p += sprintf( p, "%lx-", (unsigned long)dev );

    if (ino != (unsigned long)ino)
        sprintf( p, "%lx%08lx", (unsigned long)((unsigned long long)ino >> 32), (unsigned long)ino );
    else
        sprintf( p, "%lx", (unsigned long)ino );
    free( root );
}

/* retrieve the default dll dir */
const char *get_dlldir( const char **default_dlldir )
{
    *default_dlldir = DLLDIR;
    return dlldir;
}

/* initialize all the paths values */
static void init_paths(void)
{
    struct stat st;

    const char *home = getenv( "HOME" );
    const char *user = NULL;
    const char *prefix = getenv( "WINEPREFIX" );
    char uid_str[32];
    struct passwd *pwd = getpwuid( getuid() );

    if (pwd)
    {
        user = pwd->pw_name;
        if (!home) home = pwd->pw_dir;
    }
    if (!user)
    {
        sprintf( uid_str, "%lu", (unsigned long)getuid() );
        user = uid_str;
    }
    user_name = xstrdup( user );

    /* build config_dir */

    if (prefix)
    {
        config_dir = xstrdup( prefix );
        remove_trailing_slashes( config_dir );
        if (config_dir[0] != '/')
            fatal_error( "invalid directory %s in WINEPREFIX: not an absolute path\n", prefix );
        if (stat( config_dir, &st ) == -1)
        {
            if (errno == ENOENT) return;  /* will be created later on */
            fatal_perror( "cannot open %s as specified in WINEPREFIX", config_dir );
        }
    }
    else
    {
        if (!home) fatal_error( "could not determine your home directory\n" );
        if (home[0] != '/') fatal_error( "your home directory %s is not an absolute path\n", home );
        config_dir = xmalloc( strlen(home) + sizeof(server_config_dir) );
        strcpy( config_dir, home );
        remove_trailing_slashes( config_dir );
        strcat( config_dir, server_config_dir );
        if (stat( config_dir, &st ) == -1)
        {
            if (errno == ENOENT) return;  /* will be created later on */
            fatal_perror( "cannot open %s", config_dir );
        }
    }
    if (!S_ISDIR(st.st_mode)) fatal_error( "%s is not a directory\n", config_dir );
    if (st.st_uid != getuid()) fatal_error( "%s is not owned by you\n", config_dir );
    init_server_dir( st.st_dev, st.st_ino );
}

/* check if bindir is valid by checking for wineserver */
static int is_valid_bindir( const char *bindir )
{
    struct stat st;
    char *path = build_path( bindir, "wineserver" );
    int ret = (stat( path, &st ) != -1);
    free( path );
    return ret;
}

/* check if dlldir is valid by checking for ntdll */
static int is_valid_dlldir( const char *dlldir )
{
    struct stat st;
    char *path = build_path( dlldir, "ntdll.dll.so" );
    int ret = (stat( path, &st ) != -1);
    free( path );
    return ret;
}

/* check if basedir is a valid build dir by checking for wineserver and ntdll */
/* helper for running_from_build_dir */
static inline int is_valid_build_dir( char *basedir, int baselen )
{
    struct stat st;

    strcpy( basedir + baselen, "/server/wineserver" );
    if (stat( basedir, &st ) == -1) return 0;  /* no wineserver found */
    /* check for ntdll too to make sure */
    strcpy( basedir + baselen, "/dlls/ntdll/ntdll.dll.so" );
    if (stat( basedir, &st ) == -1) return 0;  /* no ntdll found */

    basedir[baselen] = 0;
    return 1;
}

/* check if we are running from the build directory */
static char *running_from_build_dir( const char *basedir )
{
    const char *p;
    char *path;

    /* remove last component from basedir */
    p = basedir + strlen(basedir) - 1;
    while (p > basedir && *p == '/') p--;
    while (p > basedir && *p != '/') p--;
    if (p == basedir) return NULL;
    path = xmalloc( p - basedir + sizeof("/dlls/ntdll/ntdll.dll.so") );
    memcpy( path, basedir, p - basedir );

    if (!is_valid_build_dir( path, p - basedir ))
    {
        /* remove another component */
        while (p > basedir && *p == '/') p--;
        while (p > basedir && *p != '/') p--;
        if (p == basedir || !is_valid_build_dir( path, p - basedir ))
        {
            free( path );
            return NULL;
        }
    }
    return path;
}

/* try to set the specified directory as bindir, or set build_dir if it's inside the build directory */
static int set_bindir( char *dir )
{
    if (!dir) return 0;
    if (is_valid_bindir( dir ))
    {
        bindir = dir;
        dlldir = build_path( bindir, BIN_TO_DLLDIR );
    }
    else
    {
        build_dir = running_from_build_dir( dir );
        free( dir );
    }
    return bindir || build_dir;
}

/* try to set the specified directory as dlldir, or set build_dir if it's inside the build directory */
static int set_dlldir( char *libdir )
{
    char *path;

    if (!libdir) return 0;

    path = build_path( libdir, LIB_TO_DLLDIR );
    if (is_valid_dlldir( path ))
    {
        dlldir = path;
        bindir = build_path( libdir, LIB_TO_BINDIR );
    }
    else
    {
        build_dir = running_from_build_dir( libdir );
        free( path );
    }
    free( libdir );
    return dlldir || build_dir;
}

/* initialize the argv0 path */
void wine_init_argv0_path( const char *argv0 )
{
    const char *basename, *wineloader;

    if (!(basename = strrchr( argv0, '/' ))) basename = argv0;
    else basename++;

    if (set_bindir( get_runtime_exedir() )) goto done;
    if (set_dlldir( get_runtime_libdir() )) goto done;
    if (set_bindir( get_runtime_argvdir( argv0 ))) goto done;
    if ((wineloader = getenv( "WINELOADER" ))) set_bindir( get_runtime_argvdir( wineloader ));

done:
    if (build_dir)
    {
        argv0_name = build_path( "loader/", basename );
        if (sizeof(int) == sizeof(void *))
        {
            char *loader, *linkname = build_path( build_dir, "loader/wine64" );
            if ((loader = symlink_dirname( linkname )))
            {
                wineserver64 = build_path( loader, "../server/wineserver" );
                free( loader );
            }
            free( linkname );
        }
    }
    else
    {
        if (bindir) datadir = build_path( bindir, BIN_TO_DATADIR );
        argv0_name = xstrdup( basename );
    }
}

/* return the configuration directory ($WINEPREFIX or $HOME/.wine) */
const char *wine_get_config_dir(void)
{
    if (!config_dir) init_paths();
    return config_dir;
}

/* retrieve the wine data dir */
const char *wine_get_data_dir(void)
{
    return datadir;
}

/* retrieve the wine build dir (if we are running from there) */
const char *wine_get_build_dir(void)
{
    return build_dir;
}

/* return the full name of the server directory (the one containing the socket) */
const char *wine_get_server_dir(void)
{
    if (!server_dir)
    {
        if (!config_dir) init_paths();
        else
        {
            struct stat st;

            if (stat( config_dir, &st ) == -1)
            {
                if (errno != ENOENT) fatal_error( "cannot stat %s\n", config_dir );
                return NULL;  /* will have to try again once config_dir has been created */
            }
            init_server_dir( st.st_dev, st.st_ino );
        }
    }
    return server_dir;
}

/* return the current user name */
const char *wine_get_user_name(void)
{
    if (!user_name) init_paths();
    return user_name;
}

/* return the standard version string */
const char *wine_get_version(void)
{
    return PACKAGE_VERSION;
}

static const struct
{
    const char *author;
    const char *subject;
    int revision;
}
wine_patch_data[] =
{
    { "Alexander E. Patrakov", "dsound: Add a linear resampler for use with a large number of mixing buffers.", 2 },
    { "Alex Henrie", "include: Add RtlIpv6StringToAddress(Ex)[AW].", 1 },
    { "Alex Henrie", "iphlpapi: Implement ParseNetworkString for IPv6 addresses and services.", 1 },
    { "Alex Henrie", "mountmgr.sys: Do a device check before returning a default serial port name.", 1 },
    { "Alex Henrie", "ntdll: Add semi-stub for RtlIpv6AddressToString(Ex)A.", 1 },
    { "Alex Henrie", "ntdll: Implement RtlIpv6AddressToString(Ex)W.", 1 },
    { "Alex Henrie", "ntdll: Implement RtlIpv6StringToAddress(Ex)[AW].", 1 },
    { "Alex Henrie", "winemenubuilder: Blacklist desktop integration for certain associations.", 1 },
    { "Alistair Leslie-Hughes", "d3dx9: Implement D3DXComputeTangent.", 1 },
    { "Alistair Leslie-Hughes", "ddraw: Allow writing to vtable for surface and palette.", 1 },
    { "Alistair Leslie-Hughes", "dinput: Allow mapping of controls based of Genre type.", 1 },
    { "Alistair Leslie-Hughes", "dinput: Dont allow Fixed actions to be changed.", 1 },
    { "Alistair Leslie-Hughes", "dinput: Improved tracing of Semantic value.", 1 },
    { "Alistair Leslie-Hughes", "directmanipulation: Create DirectManipulationManager/DirectManipulationSharedManager objects.", 1 },
    { "Alistair Leslie-Hughes", "directmanipulation: Fake success from IDirectManipulationViewport2 ActivateConfiguration.", 1 },
    { "Alistair Leslie-Hughes", "directmanipulation: Fake success in some functions.", 1 },
    { "Alistair Leslie-Hughes", "directmanipulation: Implement IDirectManipulationCompositor SetUpdateManager.", 1 },
    { "Alistair Leslie-Hughes", "directmanipulation: Implement IDirectManipulationManager2 CreateViewport.", 1 },
    { "Alistair Leslie-Hughes", "directmanipulation: Implement IDirectManipulationManager2 GetUpdateManager.", 1 },
    { "Alistair Leslie-Hughes", "directmanipulation: Implement IDirectManipulationViewport2 GetPrimaryContent.", 1 },
    { "Alistair Leslie-Hughes", "directmanipulation: New dll.", 1 },
    { "Alistair Leslie-Hughes", "directmanipulation: Support DCompManipulationCompositor interface.", 1 },
    { "Alistair Leslie-Hughes", "directmanipulation: Support IDirectManipulationContent in IDirectManipulationPrimaryContent interface.", 1 },
    { "Alistair Leslie-Hughes", "directmanipulation: Supprot IDirectManipulationFrameInfoProvider interface in IDirectManipulationCompositor.", 1 },
    { "Alistair Leslie-Hughes", "directmanipulation/tests: Initial tests.", 1 },
    { "Alistair Leslie-Hughes", "dsdmo: Add IDirectSoundFXChorus support.", 1 },
    { "Alistair Leslie-Hughes", "dsdmo: Add IDirectSoundFXCompressor support.", 1 },
    { "Alistair Leslie-Hughes", "dsdmo: Add IDirectSoundFXDistortion support.", 1 },
    { "Alistair Leslie-Hughes", "dsdmo: Add IDirectSoundFXEcho support.", 1 },
    { "Alistair Leslie-Hughes", "dsdmo: Add IDirectSoundFXFlanger support.", 1 },
    { "Alistair Leslie-Hughes", "dsdmo: Add IDirectSoundFXGargle support.", 1 },
    { "Alistair Leslie-Hughes", "dsdmo: Add IDirectSoundFXI3DL2Reverb support.", 1 },
    { "Alistair Leslie-Hughes", "dsdmo: Add IDirectSoundFXParamEq support.", 1 },
    { "Alistair Leslie-Hughes", "dsdmo: Add IDirectSoundFXWavesReverb support.", 1 },
    { "Alistair Leslie-Hughes", "dsdmo: Implement IDirectSoundFXChorus SetAllParameters.", 1 },
    { "Alistair Leslie-Hughes", "dsdmo: Implement IDirectSoundFXCompressor SetAllParameters.", 1 },
    { "Alistair Leslie-Hughes", "dsdmo: Implement IDirectSoundFXDistortion SetAllParameters.", 1 },
    { "Alistair Leslie-Hughes", "dsdmo: Implement IDirectSoundFXEcho GetAllParameters.", 1 },
    { "Alistair Leslie-Hughes", "dsdmo: Implement IDirectSoundFXEcho SetAllParameters.", 1 },
    { "Alistair Leslie-Hughes", "dsdmo: Implement IDirectSoundFXFlanger SetAllParameters.", 1 },
    { "Alistair Leslie-Hughes", "dsdmo: Implement IDirectSoundFXGargle GetAllParameters.", 1 },
    { "Alistair Leslie-Hughes", "dsdmo: Implement IDirectSoundFXGargle SetAllParameters.", 1 },
    { "Alistair Leslie-Hughes", "dsdmo: Implement IDirectSoundFXI3DL2Reverb SetAllParameters.", 1 },
    { "Alistair Leslie-Hughes", "dsdmo: Implement IDirectSoundFXParamEq SetAllParameters.", 1 },
    { "Alistair Leslie-Hughes", "dsdmo: Implemnet IDirectSoundFXChorus GetAllParameters.", 1 },
    { "Alistair Leslie-Hughes", "dsdmo: Implemnet IDirectSoundFXCompressor GetAllParameters.", 1 },
    { "Alistair Leslie-Hughes", "dsdmo: Implemnet IDirectSoundFXDistortion GetAllParameters.", 1 },
    { "Alistair Leslie-Hughes", "dsdmo: Implemnet IDirectSoundFXFlanger GetAllParameters.", 1 },
    { "Alistair Leslie-Hughes", "dsdmo: Implemnet IDirectSoundFXI3DL2Reverb GetAllParameters.", 1 },
    { "Alistair Leslie-Hughes", "dsdmo: Implemnet IDirectSoundFXParamEq GetAllParameters.", 1 },
    { "Alistair Leslie-Hughes", "dsound: IDirectSoundBuffer8 GetObjectInPath loops for the requested interface.", 1 },
    { "Alistair Leslie-Hughes", "include: Add XACT defines.", 1 },
    { "Alistair Leslie-Hughes", "include: Add XACTENGINE_* error codes.", 1 },
    { "Alistair Leslie-Hughes", "include: Add XACTNOTIFICATIONTYPE_* values.", 1 },
    { "Alistair Leslie-Hughes", "loader: Add Keyboard Layouts registry enteries.", 1 },
    { "Alistair Leslie-Hughes", "msctf: Added ITfActiveLanguageProfileNotifySink support in ITfSource.", 1 },
    { "Alistair Leslie-Hughes", "mshtml: Improve IOleInPlaceActiveObject TranslateAccelerator.", 1 },
    { "Alistair Leslie-Hughes", "oleaut32: Implement semi-stub for CreateTypeLib.", 1 },
    { "Alistair Leslie-Hughes", "Revert \"dxva2: Build with msvcrt.\".", 1 },
    { "Alistair Leslie-Hughes", "shlwapi: Support ./ in UrlCanonicalize.", 1 },
    { "Alistair Leslie-Hughes", "user32: Improve GetKeyboardLayoutList.", 1 },
    { "Alistair Leslie-Hughes", "user32/msgbox: Support WM_COPY Message.", 1 },
    { "Alistair Leslie-Hughes", "user32/msgbox: Use a windows hook to trap Ctrl+C.", 1 },
    { "Alistair Leslie-Hughes", "uuid: Add directmanipulation.h.", 1 },
    { "Alistair Leslie-Hughes", "windowscodecs: Avoid implicit cast of interface pointer.", 1 },
    { "Alistair Leslie-Hughes", "windowscodecs/tests: Add IWICBitmapEncoderInfo test.", 1 },
    { "Alistair Leslie-Hughes", "winex11.drv: Support multiplex categories WTI_DSCTXS and WTI_DDCTXS.", 1 },
    { "Alistair Leslie-Hughes", "winex11: Handle negative orAltitude values.", 1 },
    { "Alistair Leslie-Hughes", "winex11: Specify a default vulkan driver if one not found at build time.", 1 },
    { "Alistair Leslie-Hughes", "winex11: Support WTI_STATUS in WTInfo.", 1 },
    { "Alistair Leslie-Hughes", "wintab32: Set lcSysExtX/Y for the first index of WTI_DDCTXS.", 1 },
    { "Alistair Leslie-Hughes", "wintrust: Add parameter check in WTHelperGetProvCertFromChain.", 1 },
    { "Alistair Leslie-Hughes", "xaudio2_7: Always return S_OK in IXACT34Cue Destroy.", 1 },
    { "Alistair Leslie-Hughes", "xaudio2_7: Correct callback to windows function.", 1 },
    { "Alistair Leslie-Hughes", "xaudio2_7: Dont cast interface pointers.", 1 },
    { "Alistair Leslie-Hughes", "xaudio2_7: Initial IXACT3Engine tests.", 1 },
    { "Alistair Leslie-Hughes", "xaudio2_7: IXACT3Engine CreateSoundBank return correct HRESULT values.", 1 },
    { "Alistair Leslie-Hughes", "xaudio2_7: IXACT3Engine Initialize return valid error code.", 1 },
    { "Alistair Leslie-Hughes", "xaudio2_7: Support older XACT3Engine interfaces.", 1 },
    { "Alistair Leslie-Hughes", "xaudio2_7/tests: Add more tests.", 1 },
    { "Alistair Leslie-Hughes", "xaudio2_7: Trace FAudio version being used.", 1 },
    { "Alistair Leslie-Hughes", "xaudio2_7: Unwrap structure based of it's type.", 1 },
    { "André Hentschel", "ntdll: Support ISOLATIONAWARE_MANIFEST_RESOURCE_ID range.", 1 },
    { "André Hentschel", "wpcap: Load libpcap dynamically.", 1 },
    { "Andrej Shadura", "comctl32: Fixed rebar behaviour when there's capture and no drag.", 1 },
    { "Andrew Church", "dinput: Allow reconnecting to disconnected joysticks.", 1 },
    { "Andrew Church", "dinput: Allow remapping of joystick buttons.", 1 },
    { "Andrew D'Addesio", "ddraw: Return correct devices based off requested DirectX version.", 1 },
    { "Andrew Eikum", "winepulse: Account for PA devices that fall way behind.", 1 },
    { "Andrew Eikum", "winepulse: Don't fake being one period behind in GetPosition.", 1 },
    { "Andrew Eikum", "winepulse: Don't rely on pulseaudio callbacks for timing.", 1 },
    { "Andrew Eikum", "winepulse: Fix local buffer offset wrapping.", 1 },
    { "Andrew Eikum", "winepulse: Fix up recording.", 1 },
    { "Andrew Eikum", "winepulse: Update last time on underrun.", 1 },
    { "Andrew Wesie", "ntdll: Add stub for NtQuerySystemInformation(SystemModuleInformationEx).", 1 },
    { "Andrew Wesie", "ntdll: Refactor RtlCreateUserThread into NtCreateThreadEx.", 1 },
    { "Andrew Wesie", "ntdll: Return ntdll.dll as the first entry for SystemModuleInformation.", 1 },
    { "Andrew Wesie", "ntdll/tests: Test updating TickCount in user_shared_data.", 1 },
    { "Andrew Wesie", "ntdll: Use NtContinue to continue execution after exceptions.", 1 },
    { "Andrew Wesie", "wined3d: Use glReadPixels for RT texture download.", 1 },
    { "Andrey Gusev", "d3dx9_*: Add D3DXSHProjectCubeMap stub.", 1 },
    { "Austin English", "user32: Added LoadKeyboardLayoutEx stub.", 1 },
    { "Bernhard Reiter", "imagehlp: Implement parts of BindImageEx to make freezing Python scripts work.", 1 },
    { "Bruno Jesus", "dinput: Recalculated Axis after deadzone change.", 1 },
    { "Charles Davis", "crypt32: Skip unknown item when decoding a CMS certificate.", 1 },
    { "Christian Costa", "d3dx9_36: Add format description for X8L8V8U8 for format conversions.", 1 },
    { "Christian Costa", "d3dx9_36: Add semi-stub for D3DXOptimizeVertices.", 1 },
    { "Christian Costa", "d3dx9_36: Add support for FOURCC surface to save_dds_surface_to_memory.", 1 },
    { "Christian Costa", "d3dx9_36: Filter out D3DCompile warning messages that are not present with D3DCompileShader.", 4 },
    { "Christian Costa", "d3dx9_36: Implement D3DXDisassembleShader.", 2 },
    { "Christian Costa", "d3dx9_36: Implement ID3DXSkinInfoImpl_UpdateSkinnedMesh.", 1 },
    { "Christian Costa", "d3dx9_36: Improve D3DXSaveTextureToFile to save simple texture to dds file.", 1 },
    { "Christian Costa", "d3dx9_36: No need to fail if we don't support vertices reordering in D3DXMESHOPT_ATTRSORT.", 1 },
    { "Christian Costa", "d3dx9_36/tests: Add additional tests for special cases.", 1 },
    { "Christian Costa", "d3dx9_36/tests: Remove useless \\n within some ok messages.", 1 },
    { "Christian Costa", "d3dx9: Return D3DFMT_A8R8G8B8 in D3DXGetImageInfoFromFileInMemory for 32 bpp BMP with alpha.", 1 },
    { "Christian Costa", "ddraw: Silence noisy FIXME about unimplemented D3DPROCESSVERTICES_UPDATEEXTENTS.", 1 },
    { "Christian Costa", "ntoskrnl.exe: Implement MmMapLockedPages and MmUnmapLockedPages.", 1 },
    { "Christian Costa", "shdocvw: Check precisely ParseURLFromOutsideSourceX returned values in tests and make code clearer about that.", 3 },
    { "Christian Costa", "wined3d: Print FIXME only once in surface_cpu_blt.", 1 },
    { "Daniel Jelinski", "wine.inf: Add registry keys for Windows Performance Library.", 1 },
    { "Daniel Wendt", "gdi32: Fix for rotated Arc, ArcTo, Chord and Pie drawing problem.", 1 },
    { "Daniel Wendt", "gdi32: Fix for rotated ellipse.", 1 },
    { "David Torok", "ntdll: Stub NtQueryInformationThread(ThreadHideFromDebugger).", 1 },
    { "David Torok", "user32: AddInternalGetWindowIcon stub.", 1 },
    { "Derek Lesho", "bcrypt: Implement BCRYPT_KDF_HASH.", 1 },
    { "Derek Lesho", "bcrypt: Implement BCryptSecretAgreement with libgcrypt.", 1 },
    { "Derek Lesho", "user32: Add support for RIDEV_NOLEGACY flag in RegisterRawInputDevices.", 1 },
    { "Derek Lesho", "winex11.drv: Add support for absolute RawMotion events.", 1 },
    { "Dmitry Timoshkov", "comctl32: Bump version to 6.0.", 1 },
    { "Dmitry Timoshkov", "comdlg32: Postpone setting ofn->lpstrFileTitle to work around an application bug.", 1 },
    { "Dmitry Timoshkov", "cryptext: Implement CryptExtOpenCER.", 1 },
    { "Dmitry Timoshkov", "gdiplus: Add support for more image color formats.", 1 },
    { "Dmitry Timoshkov", "gdiplus: Change multiplications by additions in the x/y scaler loops.", 1 },
    { "Dmitry Timoshkov", "gdiplus: Change the order of x/y loops in the scaler.", 1 },
    { "Dmitry Timoshkov", "gdiplus: Prefer using pre-multiplied ARGB data in the scaler.", 1 },
    { "Dmitry Timoshkov", "gdiplus: Remove ceilf/floorf calls from bilinear scaler.", 2 },
    { "Dmitry Timoshkov", "gdiplus/tests: Add some tests for loading TIFF images in various color formats.", 1 },
    { "Dmitry Timoshkov", "include: Make stdole32.idl a public component.", 1 },
    { "Dmitry Timoshkov", "kernel32: Implement K32GetMappedFileName.", 2 },
    { "Dmitry Timoshkov", "kernel32: On process entry store PEB address in %ebx.", 1 },
    { "Dmitry Timoshkov", "kernel32/tests: Add a PE test image that resembles format of some of 8k demos.", 1 },
    { "Dmitry Timoshkov", "kernel32/tests: Add tests for NtQuerySection.", 2 },
    { "Dmitry Timoshkov", "libs/wine: Allow to modify reserved LDT entries.", 1 },
    { "Dmitry Timoshkov", "ntdll: If PE image size is larger than the backed file size then treat file as removable.", 1 },
    { "Dmitry Timoshkov", "ntdll: Implement NtQueryVirtualMemory(MemorySectionName).", 3 },
    { "Dmitry Timoshkov", "ntdll: Implement NtSetLdtEntries.", 1 },
    { "Dmitry Timoshkov", "ntdll/tests: Add tests for NtQueryVirtualMemory(MemorySectionName).", 1 },
    { "Dmitry Timoshkov", "ole32: Add a check for hglobal pointer to GetHGlobalFromStream.", 1 },
    { "Dmitry Timoshkov", "ole32: Add a wrapper for memory block managed by HGLOBAL based IStream.", 1 },
    { "Dmitry Timoshkov", "ole32: Allow moving a being reallocated block of memory managed by HGLOBAL based IStream.", 1 },
    { "Dmitry Timoshkov", "ole32/tests: Add a bunch of tests for HGLOBAL based IStream::Clone.", 1 },
    { "Dmitry Timoshkov", "oleaut32: Add support for decoding SLTG function help strings.", 1 },
    { "Dmitry Timoshkov", "oleaut32: Add support for decoding SLTG variable help strings.", 1 },
    { "Dmitry Timoshkov", "oleaut32: Add support for loading and saving EMF to IPicture interface.", 1 },
    { "Dmitry Timoshkov", "oleaut32: Do not reimplement OleLoadPicture in OleLoadPicturePath.", 1 },
    { "Dmitry Timoshkov", "oleaut32: Factor out stream creation from OleLoadPicturePath.", 1 },
    { "Dmitry Timoshkov", "oleaut32: Fix logic for deciding whether type description follows the name.", 2 },
    { "Dmitry Timoshkov", "oleaut32: Implement a better stub for IPicture::SaveAsFile.", 1 },
    { "Dmitry Timoshkov", "oleaut32: Implement decoding of SLTG help strings.", 1 },
    { "Dmitry Timoshkov", "oleaut32: Implement OleLoadPictureFile.", 2 },
    { "Dmitry Timoshkov", "oleaut32: Make OleLoadPicture load DIBs using WIC decoder.", 1 },
    { "Dmitry Timoshkov", "oleaut32: OleLoadPicture should create a DIB section for a being loaded bitmap.", 3 },
    { "Dmitry Timoshkov", "oleaut32/tests: Add some tests for loading and saving EMF using IPicture interface.", 1 },
    { "Dmitry Timoshkov", "riched20/tests: Add a test to see what richedit class flavours should be available.", 1 },
    { "Dmitry Timoshkov", "server: Add support for a layered window region.", 3 },
    { "Dmitry Timoshkov", "server: All fields up to CheckSum are mandatory regardless of SizeOfOptionalHeader value.", 1 },
    { "Dmitry Timoshkov", "setupapi: Fix parameters of SPFILENOTIFY_FILEINCABINET handler.", 1 },
    { "Dmitry Timoshkov", "setupapi/tests: Add more tests for SPFILENOTIFY_FILEINCABINET handler.", 1 },
    { "Dmitry Timoshkov", "shell32: Add more Tango icons to the IE toolbar.", 1 },
    { "Dmitry Timoshkov", "shell32: Add toolbar bitmaps compatible with IE6.", 1 },
    { "Dmitry Timoshkov", "user32: Add a workaround for Windows 3.1 apps which call LoadImage(LR_LOADFROMFILE) with a resource id.", 2 },
    { "Dmitry Timoshkov", "user32: Before asking a WM to activate a window make sure that the window is in foreground and not minimized.", 1 },
    { "Dmitry Timoshkov", "user32: Do not initialize dialog info for every window passed to DefDlgProc.", 1 },
    { "Dmitry Timoshkov", "user32: Fix return value of ScrollWindowEx for invisible windows.", 1 },
    { "Dmitry Timoshkov", "user32: MessageBox should be topmost when MB_SYSTEMMODAL style is set.", 1 },
    { "Dmitry Timoshkov", "user32/tests: Add a bunch of tests for DM_SETDEFID/DM_GETDEFID handling by a DefDlgProc.", 1 },
    { "Dmitry Timoshkov", "user32/tests: Add some tests to see when MessageBox gains WS_EX_TOPMOST style.", 1 },
    { "Dmitry Timoshkov", "user32: Try harder to find a target for mouse messages.", 1 },
    { "Dmitry Timoshkov", "user32: Use root dialog for DM_SETDEFID/DM_GETDEFID in DefDlgProc.", 1 },
    { "Dmitry Timoshkov", "uxtheme: Protect CloseThemeData() from invalid input.", 1 },
    { "Dmitry Timoshkov", "widl: Add initial implementation of SLTG typelib generator.", 1 },
    { "Dmitry Timoshkov", "widl: Add support for function parameter flags to SLTG typelib generator.", 1 },
    { "Dmitry Timoshkov", "widl: Add support for inherited interfaces to SLTG typelib generator.", 1 },
    { "Dmitry Timoshkov", "widl: Add support for interfaces to SLTG typelib generator.", 1 },
    { "Dmitry Timoshkov", "widl: Add support for recursive type references to SLTG typelib generator.", 1 },
    { "Dmitry Timoshkov", "widl: Add support for structures.", 1 },
    { "Dmitry Timoshkov", "widl: Add support for VT_USERDEFINED to SLTG typelib generator.", 1 },
    { "Dmitry Timoshkov", "widl: Add support for VT_VOID and VT_VARIANT to SLTG typelib generator.", 1 },
    { "Dmitry Timoshkov", "widl: Avoid relying on side effects when marking function index as the last one.", 1 },
    { "Dmitry Timoshkov", "widl: Calculate size of instance for structures.", 1 },
    { "Dmitry Timoshkov", "widl: Create library block index right after the CompObj one.", 1 },
    { "Dmitry Timoshkov", "widl: Factor out SLTG tail initialization.", 1 },
    { "Dmitry Timoshkov", "widl: Fix generation of resources containing an old typelib.", 1 },
    { "Dmitry Timoshkov", "widl: Make automatic dispid generation scheme better match what midl does.", 1 },
    { "Dmitry Timoshkov", "widl: Minor/cosmetic clean up.", 1 },
    { "Dmitry Timoshkov", "widl: More accurately report variable descriptions data size.", 1 },
    { "Dmitry Timoshkov", "widl: Properly align name table entries.", 1 },
    { "Dmitry Timoshkov", "widl: Set the lowest bit in the param name to indicate whether type description follows the name.", 1 },
    { "Dmitry Timoshkov", "widl: Write correct syskind by SLTG typelib generator.", 1 },
    { "Dmitry Timoshkov", "widl: Write correct typekind to the SLTG typeinfo block.", 1 },
    { "Dmitry Timoshkov", "widl: Write SLTG blocks according to the index order.", 1 },
    { "Dmitry Timoshkov", "windowscodecs: Add initial implementation of the GIF encoder.", 1 },
    { "Dmitry Timoshkov", "windowscodecs: Add registration of the GIF encoder.", 1 },
    { "Dmitry Timoshkov", "windowscodecs: Better follow the GIF spec and don't specify the local color table size if there is no local palette.", 1 },
    { "Dmitry Timoshkov", "windowscodecs: Correctly indicate that the global info was written even without the global palette.", 1 },
    { "Dmitry Timoshkov", "windowscodecs: Fix IWICBitmapDecoder::CopyPalette for a not initialized case in the GIF decoder.", 1 },
    { "Dmitry Timoshkov", "windowscodecs: Initialize empty property bag in GIF encoder's CreateNewFrame implementation.", 1 },
    { "Dmitry Timoshkov", "windowscodecs/tests: Add the tests for GIF encoder and decoder.", 1 },
    { "Dmitry Timoshkov", "windowscodecs: Tolerate partial reads in the IFD metadata loader.", 1 },
    { "Dmitry Timoshkov", "wine.inf: Add 'Counters' to the perflib key as an alias for 'Counter'.", 1 },
    { "Dmitry Timoshkov", "wineps.drv: Add support for GETFACENAME and DOWNLOADFACE escapes.", 1 },
    { "Dmitry Timoshkov", "winex11.drv: Add support for _NET_ACTIVE_WINDOW.", 2 },
    { "Dmitry Timoshkov", "winex11.drv: Don't add MWM_DECOR_BORDER to windows without a caption.", 1 },
    { "Dmitry Timoshkov", "winex11.drv: Don't use MWM_DECOR_RESIZEH, window resizing is controlled by MWM_FUNC_RESIZE.", 1 },
    { "Dmitry Timoshkov", "winex11.drv: Send WM_WINDOWPOSCHANGING/WM_WINDOWPOSCHANGED messages to a being deactivated topmost window.", 1 },
    { "Dmitry Timoshkov", "winex11: Fix handling of window attributes for WS_EX_LAYERED | WS_EX_COMPOSITED.", 1 },
    { "Enrico Horn", "winex11.drv: Handle missing thread data in X11DRV_get_ic.", 1 },
    { "Erich E. Hoover", "dsound: Add stub support for DSPROPSETID_EAX20_BufferProperties.", 1 },
    { "Erich E. Hoover", "dsound: Add stub support for DSPROPSETID_EAX20_ListenerProperties.", 1 },
    { "Erich E. Hoover", "fonts: Add WenQuanYi Micro Hei as a Microsoft Yahei replacement.", 1 },
    { "Erich E. Hoover", "kernel32: Advertise junction point support.", 1 },
    { "Erich E. Hoover", "kernel32: Implement CreateSymbolicLink[A|W] with ntdll reparse points.", 1 },
    { "Erich E. Hoover", "kernel32,ntdll: Add support for deleting junction points with RemoveDirectory.", 1 },
    { "Erich E. Hoover", "kernel32: Set error code when attempting to delete file symlinks as directories.", 1 },
    { "Erich E. Hoover", "libport: Add support for FreeBSD style extended attributes.", 1 },
    { "Erich E. Hoover", "libport: Add support for Mac OS X style extended attributes.", 1 },
    { "Erich E. Hoover", "ntdll: Add a test for junction point advertisement.", 1 },
    { "Erich E. Hoover", "ntdll: Add support for absolute symlink creation.", 1 },
    { "Erich E. Hoover", "ntdll: Add support for deleting junction points.", 1 },
    { "Erich E. Hoover", "ntdll: Add support for deleting symlinks.", 1 },
    { "Erich E. Hoover", "ntdll: Add support for file symlinks.", 1 },
    { "Erich E. Hoover", "ntdll: Add support for junction point creation.", 1 },
    { "Erich E. Hoover", "ntdll: Add support for reading absolute symlinks.", 1 },
    { "Erich E. Hoover", "ntdll: Add support for reading junction points.", 1 },
    { "Erich E. Hoover", "ntdll: Add support for reading relative symlinks.", 1 },
    { "Erich E. Hoover", "ntdll: Add support for relative symlink creation.", 1 },
    { "Erich E. Hoover", "ntdll: Allow creation of dangling reparse points to non-existent paths.", 1 },
    { "Erich E. Hoover", "ntdll: Always report symbolic links as containing zero bytes.", 1 },
    { "Erich E. Hoover", "ntdll: Correctly report file symbolic links as files.", 1 },
    { "Erich E. Hoover", "ntdll: Find dangling symlinks quickly.", 1 },
    { "Erich E. Hoover", "ntdll: Implement retrieving DOS attributes in NtQuery[Full]AttributesFile and NtQueryDirectoryFile.", 1 },
    { "Erich E. Hoover", "ntdll: Implement retrieving DOS attributes in NtQueryInformationFile.", 1 },
    { "Erich E. Hoover", "ntdll: Implement storing DOS attributes in NtCreateFile.", 1 },
    { "Erich E. Hoover", "ntdll: Implement storing DOS attributes in NtSetInformationFile.", 1 },
    { "Erich E. Hoover", "ntdll: Perform the Unix-style hidden file check within the unified file info grabbing routine.", 1 },
    { "Erich E. Hoover", "server: Convert return of file security masks with generic access mappings.", 7 },
    { "Erich E. Hoover", "server: Inherit security attributes from parent directories on creation.", 7 },
    { "Erich E. Hoover", "server: Properly handle file symlink deletion.", 1 },
    { "Erich E. Hoover", "server: Retrieve file security attributes with extended file attributes.", 7 },
    { "Erich E. Hoover", "server: Store file security attributes with extended file attributes.", 8 },
    { "Erich E. Hoover", "server: Unify the retrieval of security attributes for files and directories.", 7 },
    { "Erich E. Hoover", "server: Unify the storage of security attributes for files and directories.", 7 },
    { "Erich E. Hoover", "strmbase: Fix MediaSeekingPassThru_GetPositions return when the pins are unconnected.", 1 },
    { "Erich E. Hoover", "ws2_32: Add support for TF_DISCONNECT to TransmitFile.", 1 },
    { "Erich E. Hoover", "ws2_32: Add support for TF_REUSE_SOCKET to TransmitFile.", 1 },
    { "Erich Hoover", "pdh: Support the 'Processor' object string.", 1 },
    { "Ethan Lee", "include: Add xact3.idl.", 1 },
    { "Ethan Lee", "xaudio2: Add support for xactengine3.", 1 },
    { "Felix Yan", "winex11.drv: Update a candidate window's position with over-the-spot style.", 2 },
    { "Firerat", "winecfg: Toggle upstream CSMT implementation.", 1},
    { "Gabriel Ivăncescu", "ntdll/server: Mark drive_c as case-insensitive when created.", 1 },
    { "Gabriel Ivăncescu", "shell32/iconcache: Generate icons from available icons if some icon sizes failed to load.", 1 },
    { "Gabriel Ivăncescu", "user32/focus: Prevent a recursive loop with the activation messages.", 1 },
    { "Gabriel Ivăncescu", "user32/tests: Test a recursive activation loop on WM_ACTIVATE.", 1 },
    { "Gabriel Ivăncescu", "winex11.drv/mouse: Use root-relative coordinates for events, if possible.", 1 },
    { "Gabriel Ivăncescu", "winex11.drv/window: Query the X server for the actual rect of the window before unmapping it.", 1 },
    { "Gijs Vermeulen", "imm32: Only generate 'WM_IME_SETCONTEXT' message if window has focus.", 1 },
    { "Hao Peng", "winecfg: Double click in dlls list to edit item's overides.", 3 },
    { "Henri Verbeet", "wined3d: Dont set DDSCAPS_FLIP for gdi renderer.", 1 },
    { "Hermès BÉLUSCA-MAÏTO", "shlwapi: Fix the return value of SHAddDataBlock.", 1 },
    { "Hirofumi Katayama", "user32: Implement TileWindows.", 1 },
    { "Ivan Akulinchev", "uxtheme: Initial implementation of GTK backend.", 1 },
    { "Jactry Zeng", "riched20: Fix ME_RunOfsFromCharOfs() when nCharOfs > strlen().", 1 },
    { "Jactry Zeng", "riched20: Stub for ITextPara interface and implement ITextRange::GetPara.", 1 },
    { "James Coonradt", "user32: Improve FlashWindowEx message and return value.", 1 },
    { "Jarkko Korpi", "ntoskrnl.exe: Add IoGetDeviceAttachmentBaseRef stub.", 1 },
    { "Jarkko Korpi", "wined3d: Also check for 'Brian Paul' to detect Mesa gl_vendor.", 1 },
    { "Jason Edmeades", "cmd: Ftype failed to clear file associations.", 1 },
    { "Jason Edmeades", "cmd: Support for launching programs based on file association.", 1 },
    { "Jeremy White", "winemapi: Directly use xdg-email if available, enabling file attachments.", 1 },
    { "Jetro Jormalainen", "dinput: Allow empty Joystick mappings.", 1 },
    { "Jetro Jormalainen", "dinput: Load users Joystick mappings.", 1 },
    { "Jetro Jormalainen", "dinput: Support username in Config dialog.", 1 },
    { "Jianqiu Zhang", "ntdll: Add support for FileFsFullSizeInformation class in NtQueryVolumeInformationFile.", 2 },
    { "Joakim Hernberg", "wineserver: Draft to implement priority levels through POSIX scheduling policies on linux.", 1 },
    { "Johannes Specht", "d3d11: Implement ClearUnorderedAccessViewFloat for deferred contexts.", 1 },
    { "Johannes Specht", "d3d11: Implement ClearUnorderedAccessViewUint for deferred contexts.", 1 },
    { "Johannes Specht", "d3d11: Implement CopyResource for deferred contexts.", 1 },
    { "Johannes Specht", "d3d11: Implement CopyStructureCount for deferred contexts.", 1 },
    { "Johannes Specht", "d3d11: Implement CopySubresourceRegion for deferred contexts.", 1 },
    { "Johannes Specht", "d3d11: Implement DispatchIndirect for deferred contexts.", 1 },
    { "Johannes Specht", "d3d11: Implement DrawAuto for deferred contexts.", 1 },
    { "Johannes Specht", "d3d11: Implement DrawIndexedInstancedIndirect for deferred contexts.", 1 },
    { "Johannes Specht", "d3d11: Implement DrawInstanced for deferred contexts.", 1 },
    { "Johannes Specht", "d3d11: Implement DrawInstancedIndirect for deferred contexts.", 1 },
    { "Johannes Specht", "d3d11: Implement GenerateMips for deferred contexts.", 1 },
    { "Johannes Specht", "d3d11: Implement OMSetRenderTargetsAndUnorderedAccessViews for deferred contexts.", 1 },
    { "Johannes Specht", "d3d11: Implement ResolveSubresource for deferred contexts.", 1 },
    { "Johannes Specht", "d3d11: Implement RsSetScissorRects for deferred contexts.", 1 },
    { "Johannes Specht", "d3d11: Implement SetPredication for deferred contexts.", 1 },
    { "Johannes Specht", "d3d11: Implement SetResourceMinLOD for deferred contexts.", 1 },
    { "Johannes Specht", "d3d11: Implement SOSetTargets for deferred contexts.", 1 },
    { "Jordan Galby", "winex11.drv: Accumulate mouse movement to avoid rounding losses.", 1 },
    { "katahiromz", "user32: Implement CascadeWindows.", 1 },
    { "Ken Thomases", "winex11: Match keyboard in Unicode.", 1 },
    { "Kimmo Myllyvirta", "d3d11: Add stub deferred rendering context.", 1 },
    { "Kimmo Myllyvirta", "d3d11: Correctly align map info buffer.", 1 },
    { "Kimmo Myllyvirta", "d3d11: Implement Begin and End for deferred contexts.", 1 },
    { "Kimmo Myllyvirta", "d3d11: Implement CSSetSamplers for deferred contexts.", 1 },
    { "Kimmo Myllyvirta", "d3d11: Implement CSSetShaderResources for deferred contexts.", 1 },
    { "Kimmo Myllyvirta", "d3d11: Implement GSSetConstantBuffers for deferred contexts.", 1 },
    { "Kimmo Myllyvirta", "d3d11: Implement GSSetSamplers for deferred contexts.", 1 },
    { "Kimmo Myllyvirta", "d3d11: Implement GSSetShader for deferred contexts.", 1 },
    { "Kimmo Myllyvirta", "d3d11: Implement GSSetShaderResources for deferred contexts.", 1 },
    { "Kimmo Myllyvirta", "d3d11: Implement HSSetSamplers for deferred contexts.", 1 },
    { "Kimmo Myllyvirta", "d3d11: Implement HSSetShaderResources for deferred contexts.", 1 },
    { "Kimmo Myllyvirta", "d3d11: Implement VSSetSamplers for deferred contexts.", 1 },
    { "Kimmo Myllyvirta", "d3d11: Implement VSSetShaderResources for deferred contexts.", 1 },
    { "Kimmo Myllyvirta", "user32: ShowWindow should not send message when window is already visible.", 1 },
    { "Louis Lenders", "dwmapi: Add initial tests.", 1 },
    { "Louis Lenders", "shell32: Improve semi-stub SHGetStockIconInfo, try find existing iconhandle.", 1 },
    { "Louis Lenders", "tasklist.exe: Add minimal functionality.", 1 },
    { "Lucian Poston", "dwrite: Test GetMetrics with custom fontcollection.", 1 },
    { "Lucian Poston", "dwrite: Test IDWriteTextFormat with nonexistent font.", 1 },
    { "Lucian Poston", "dwrite: Use font fallback when mapping characters.", 1 },
    { "Lucian Poston", "dwrite: Use MapCharacters for dummy line metrics.", 1 },
    { "Lucian Poston", "dwrite: Use MapCharacters for non-visual characters.", 1 },
    { "Mark Harmstone", "dsound: Add delay line EAX functions.", 1 },
    { "Mark Harmstone", "dsound: Add EAX init and free stubs.", 1 },
    { "Mark Harmstone", "dsound: Add EAX presets.", 1 },
    { "Mark Harmstone", "dsound: Add EAX propset stubs.", 1 },
    { "Mark Harmstone", "dsound: Add EAX v1 constants and structs.", 1 },
    { "Mark Harmstone", "dsound: Add EAX VerbPass stub.", 1 },
    { "Mark Harmstone", "dsound: Allocate EAX delay lines.", 1 },
    { "Mark Harmstone", "dsound: Feed data through EAX function.", 1 },
    { "Mark Harmstone", "dsound: Implement EAX decorrelator.", 1 },
    { "Mark Harmstone", "dsound: Implement EAX early reflections.", 1 },
    { "Mark Harmstone", "dsound: Implement EAX late all-pass filter.", 1 },
    { "Mark Harmstone", "dsound: Implement EAX late reverb.", 1 },
    { "Mark Harmstone", "dsound: Implement EAX lowpass filter.", 1 },
    { "Mark Harmstone", "dsound: Report that we support EAX.", 1 },
    { "Mark Harmstone", "dsound: Support getting and setting EAX buffer properties.", 1 },
    { "Mark Harmstone", "dsound: Support getting and setting EAX properties.", 1 },
    { "Mark Harmstone", "winecfg: Add checkbox to enable/disable EAX support.", 1 },
    { "Mark Harmstone", "winepulse: Expose audio devices directly to programs.", 1 },
    { "Mark Harmstone", "winepulse: Fetch actual program name if possible.", 1 },
    { "Mark Harmstone", "winepulse: Fix segfault in pulse_rd_loop.", 1 },
    { "Mark Harmstone", "winepulse: Implement GetPropValue.", 1 },
    { "Mark Harmstone", "winepulse: Return PKEY_AudioEndpoint_PhysicalSpeakers device prop.", 1 },
    { "Mark Jansen", "kernel32/tests: Add tests for job object accounting.", 1 },
    { "Mark Jansen", "msi: Do not sign extend after multiplying.", 1 },
    { "Mark Jansen", "shlwapi: Implement AssocGetPerceivedType.", 2 },
    { "Mark Jansen", "shlwapi/tests: Add tests for AssocGetPerceivedType.", 1 },
    { "Mark Jansen", "version: Test for VerQueryValueA.", 2 },
    { "Martin Storsjo", "configure: Avoid clobbering x18 on arm64 within wine.", 1 },
    { "Martin Storsjo", "ntdll: Always restore TEB to x18 on aarch 64 on return from calls to builtins.", 1 },
    { "Mathieu Comandon", "esync: Add note about file limits not being raised when using systemd.", 1 },
    { "Matt Durgavich", "ws2_32: Proper WSACleanup implementation using wineserver function.", 2 },
    { "Michael Müller", "aclui: Add basic ACE viewer.", 1 },
    { "Michael Müller", "Add licenses for fonts as separate files.", 1 },
    { "Michael Müller", "advapi32: Fix error code when calling LsaOpenPolicy for non existing remote machine.", 1 },
    { "Michael Müller", "advapi32: Implement CreateRestrictedToken.", 1 },
    { "Michael Müller", "advapi32/tests: Add test for perflib registry key.", 1 },
    { "Michael Müller", "advapi32/tests: Extend security label / token integrity tests.", 1 },
    { "Michael Müller", "advapi32: Use TRACE for LsaOpenPolicy/LsaClose.", 1 },
    { "Michael Müller", "comctl32: Preserve custom colors between subitems.", 2 },
    { "Michael Müller", "d3d11: Implement ClearDepthStencilView for deferred contexts.", 1 },
    { "Michael Müller", "d3d11: Implement ClearRenderTargetView for deferred contexts.", 1 },
    { "Michael Müller", "d3d11: Implement CSSetConstantBuffers for deferred contexts.", 1 },
    { "Michael Müller", "d3d11: Implement CSSetShader for deferred contexts.", 1 },
    { "Michael Müller", "d3d11: Implement CSSetUnorderedAccessViews for deferred contexts.", 1 },
    { "Michael Müller", "d3d11: Implement d3d11_deferred_context_UpdateSubresource.", 1 },
    { "Michael Müller", "d3d11: Implement Dispatch for deferred contexts.", 1 },
    { "Michael Müller", "d3d11: Implement Draw for deferred contexts.", 1 },
    { "Michael Müller", "d3d11: Implement restoring of state after executing a command list.", 1 },
    { "Michael Müller", "d3d11: Initial implementation for deferred contexts.", 1 },
    { "Michael Müller", "d3d9/tests: Check MaxVertexBlendMatrixIndex capability.", 1 },
    { "Michael Müller", "d3d9/tests: Test normal calculation when indexed vertex blending is enabled.", 1 },
    { "Michael Müller", "ddraw: Allow size and format conversions in IDirect3DTexture2::Load.", 1 },
    { "Michael Müller", "ddraw: Create rendering targets in video memory if possible.", 1 },
    { "Michael Müller", "ddraw: Don't set HWTRANSFORMANDLIGHT flag on d3d7 RGB device.", 1 },
    { "Michael Müller", "ddraw: Implement DDENUMSURFACES_CANBECREATED flag in ddraw7_EnumSurfaces.", 1 },
    { "Michael Müller", "ddraw: Remove const from ddraw1_vtbl and ddraw_surface1_vtbl.", 1 },
    { "Michael Müller", "ddraw: Set dwZBufferBitDepth in ddraw7_GetCaps.", 1 },
    { "Michael Müller", "ddraw/tests: Add more tests for IDirect3DTexture2::Load.", 1 },
    { "Michael Müller", "ddraw/tests: Add more tests for IDirectDraw7::EnumSurfaces.", 1 },
    { "Michael Müller", "dxdiagn: Calling GetChildContainer with an empty string on a leaf container returns the object itself.", 1 },
    { "Michael Müller", "dxdiagn: Enumerate DirectSound devices and add some basic properties.", 1 },
    { "Michael Müller", "dxgkrnl.sys: Add stub driver.", 1 },
    { "Michael Müller", "dxgmms1.sys: Add stub driver.", 1 },
    { "Michael Müller", "dxva2: Add DRM mode for vaapi.", 1 },
    { "Michael Müller", "dxva2: Always destroy buffers when calling vaRenderPicture.", 1 },
    { "Michael Müller", "dxva2: Fill h264 luma and chroma weights / offsets with default values in case they are not specified.", 1 },
    { "Michael Müller", "dxva2: Implement h264 decoder.", 1 },
    { "Michael Müller", "dxva2: Implement stubbed DirectX Software VideoProcessor interface.", 1 },
    { "Michael Müller", "dxva2: Implement stubbed interfaces for IDirectXVideo{Acceleration,Decoder,Processor}Service.", 1 },
    { "Michael Müller", "dxva2: Initial implementation of MPEG2 decoder using vaapi backend.", 1 },
    { "Michael Müller", "dxva2: Only declare debug channels when they are actually used.", 1 },
    { "Michael Müller", "dxva2/tests: Add tests for dxva2 decoder.", 1 },
    { "Michael Müller", "explorer: Create CurrentControlSet\\Control\\Video registry key as non-volatile.", 1 },
    { "Michael Müller", "ext-ms-win-appmodel-usercontext-l1-1-0: Add dll and add stub for UserContextExtInitialize.", 1 },
    { "Michael Müller", "ext-ms-win-xaml-pal-l1-1-0: Add dll and add stub for XamlBehaviorEnabled.", 1 },
    { "Michael Müller", "ext-ms-win-xaml-pal-l1-1-0: Add stub for GetThemeServices.", 1 },
    { "Michael Müller", "fsutil: Add fsutil program with support for creating hard links.", 1 },
    { "Michael Müller", "iertutil: Add dll and add stub for ordinal 811.", 1 },
    { "Michael Müller", "inseng: Implement CIF reader and download functions.", 1 },
    { "Michael Müller", "kernel32: Add stub for SetThreadIdealProcessorEx.", 1 },
    { "Michael Müller", "kernel32: Add support for progress callback in CopyFileEx.", 1 },
    { "Michael Müller", "kernel32: Implement some processor group functions.", 1 },
    { "Michael Müller", "kernel32: Make K32GetPerformanceInfo faster.", 1 },
    { "Michael Müller", "kernel32: Strip invalid characters from mask in FindFirstFileExW.", 1 },
    { "Michael Müller", "kernel32/tests: Add basic tests for fake dlls.", 1 },
    { "Michael Müller", "kernel32/tests: Add tests for FindFirstFileA with invalid characters.", 1 },
    { "Michael Müller", "krnl386.exe16: Emulate GDT and LDT access.", 1 },
    { "Michael Müller", "krnl386.exe16: Really translate all invalid console handles into usable DOS handles.", 1 },
    { "Michael Müller", "libs/wine: Use same file alignment for fake and builtin DLLs.", 1 },
    { "Michael Müller", "libwine: Add process specific debug channels.", 1 },
    { "Michael Müller", "mmsystem.dll16: Refcount midihdr to work around buggy application which unprepares buffer during a callback.", 1 },
    { "Michael Müller", "mmsystem.dll16: Translate MidiIn messages.", 1 },
    { "Michael Müller", "mountmgr.sys: Write usable device paths into HKLM\\SYSTEM\\MountedDevices.", 1 },
    { "Michael Müller", "mscoree: Implement semi-stub for _CorValidateImage.", 1 },
    { "Michael Müller", "ntdll: Add dummy apiset to PEB.", 1 },
    { "Michael Müller", "ntdll: Add function to create new tokens for elevation purposes.", 1 },
    { "Michael Müller", "ntdll: Add stub for NtContinue.", 1 },
    { "Michael Müller", "ntdll: Allow special characters in pipe names.", 1 },
    { "Michael Müller", "ntdll: Catch windows int 0x2e syscall on i386.", 1 },
    { "Michael Müller", "ntdll: Create thread to update user_shared_data time values when necessary.", 1 },
    { "Michael Müller", "ntdll: Fill out thread times in process enumeration.", 1 },
    { "Michael Müller", "ntdll: Fill process kernel and user time.", 1 },
    { "Michael Müller", "ntdll: Fill process virtual memory counters in NtQuerySystemInformation.", 1 },
    { "Michael Müller", "ntdll: Fix holes in ELF mappings.", 2 },
    { "Michael Müller", "ntdll: Implement HashLinks field in LDR module data.", 1 },
    { "Michael Müller", "ntdll: Implement NtFilterToken.", 1 },
    { "Michael Müller", "ntdll: Implement ObjectTypesInformation in NtQueryObject.", 1 },
    { "Michael Müller", "ntdll: Implement opening files through nt device paths.", 1 },
    { "Michael Müller", "ntdll: Implement process token elevation through manifests.", 1 },
    { "Michael Müller", "ntdll: Implement SystemExtendedHandleInformation in NtQuerySystemInformation.", 1 },
    { "Michael Müller", "ntdll: Mimic object type behavior for different windows versions.", 1 },
    { "Michael Müller", "ntdll: Move NtProtectVirtualMemory and NtCreateSection to separate pages on x86.", 2 },
    { "Michael Müller", "ntdll: Properly handle PAGE_WRITECOPY protection.", 5 },
    { "Michael Müller", "ntdll: Set correct thread creation time for SystemProcessInformation in NtQuerySystemInformation.", 1 },
    { "Michael Müller", "ntdll: Set object type for System(Extended)HandleInformation in NtQuerySystemInformation.", 1 },
    { "Michael Müller", "ntdll: Set process start time.", 1 },
    { "Michael Müller", "ntdll: Set TypeIndex for ObjectTypeInformation in NtQueryObject.", 1 },
    { "Michael Müller", "ntdll: Setup a temporary signal handler during process startup to handle page faults.", 2 },
    { "Michael Müller", "ntdll/tests: Add basic tests for RtlQueryPackageIdentity.", 1 },
    { "Michael Müller", "ntdll: Use HashLinks when searching for a dll using the basename.", 1 },
    { "Michael Müller", "nvapi: Add NvAPI_GetPhysicalGPUsFromLogicalGPU.", 1 },
    { "Michael Müller", "nvapi: Add stub for EnumNvidiaDisplayHandle.", 1 },
    { "Michael Müller", "nvapi: Add stub for NvAPI_D3D9_RegisterResource.", 1 },
    { "Michael Müller", "nvapi: Add stub for NvAPI_D3D_GetCurrentSLIState.", 1 },
    { "Michael Müller", "nvapi: Add stub for NvAPI_D3D_GetObjectHandleForResource.", 1 },
    { "Michael Müller", "nvapi: Add stub for NvAPI_DISP_GetGDIPrimaryDisplayId.", 1 },
    { "Michael Müller", "nvapi: Add stub for NvAPI_EnumPhysicalGPUs.", 1 },
    { "Michael Müller", "nvapi: Add stub for NvAPI_GetLogicalGPUFromDisplay.", 1 },
    { "Michael Müller", "nvapi: Add stub for NvAPI_GPU_GetGpuCoreCount.", 1 },
    { "Michael Müller", "nvapi: Add stub for NvAPI_SYS_GetDriverAndBranchVersion.", 1 },
    { "Michael Müller", "nvapi: Add stub for NvAPI_Unload.", 1 },
    { "Michael Müller", "nvapi: Add stubs for NvAPI_EnumLogicalGPUs and undocumented equivalent.", 1 },
    { "Michael Müller", "nvapi: Add stubs for NvAPI_GPU_GetFullName.", 1 },
    { "Michael Müller", "nvapi: Explicity return NULL for 0x33c7358c and 0x593e8644.", 1 },
    { "Michael Müller", "nvapi: First implementation.", 1 },
    { "Michael Müller", "nvapi: Implement NvAPI_D3D11_CreateDevice and NvAPI_D3D11_CreateDeviceAndSwapChain.", 1 },
    { "Michael Müller", "nvapi: Implement NvAPI_D3D11_SetDepthBoundsTest.", 2 },
    { "Michael Müller", "nvapi: Implement NvAPI_GPU_Get{Physical,Virtual}FrameBufferSize.", 1 },
    { "Michael Müller", "nvapi: Improve NvAPI_D3D_GetCurrentSLIState.", 1 },
    { "Michael Müller", "nvapi/tests: Use structure to list imports.", 1 },
    { "Michael Müller", "nvcuda: Add semi stub for cuD3D10GetDevice.", 1 },
    { "Michael Müller", "nvcuda: Emulate two d3d9 initialization functions.", 1 },
    { "Michael Müller", "nvcuda: First implementation.", 2 },
    { "Michael Müller", "nvcuda: Properly wrap undocumented 'ContextStorage' interface and add tests.", 1 },
    { "Michael Müller", "nvcuda: Search for dylib library on Mac OS X.", 1 },
    { "Michael Müller", "nvcuvid: First implementation.", 2 },
    { "Michael Müller", "nvencodeapi: Add debian specific paths to native library.", 1 },
    { "Michael Müller", "nvencodeapi: Add support for version 6.0.", 1 },
    { "Michael Müller", "nvencodeapi: First implementation.", 1 },
    { "Michael Müller", "programs/runas: Basic implementation for starting processes with a different trustlevel.", 1 },
    { "Michael Müller", "programs/winedbg: Print process arguments in info threads.", 1 },
    { "Michael Müller", "programs/winedevice: Load some common drivers and fix ldr order.", 1 },
    { "Michael Müller", "server: Correctly assign security labels for tokens.", 1 },
    { "Michael Müller", "server: Correctly treat zero access mask in duplicate_token wineserver call.", 1 },
    { "Michael Müller", "server: Implement support for creating processes using a token.", 1 },
    { "Michael Müller", "server: Implement support for global and local shared memory blocks based on memfd.", 1 },
    { "Michael Müller", "server: Implement token elevation information.", 1 },
    { "Michael Müller", "server: Implement token integrity level.", 1 },
    { "Michael Müller", "server: Register types during startup.", 1 },
    { "Michael Müller", "server: Rename ObjectType to Type.", 1 },
    { "Michael Müller", "setupapi: Ignore deletion of added files in SetupAddToDiskSpaceList.", 1 },
    { "Michael Müller", "setupapi: Implement SetupAddInstallSectionToDiskSpaceList.", 1 },
    { "Michael Müller", "setupapi: ImplementSetupAddSectionToDiskSpaceList.", 1 },
    { "Michael Müller", "setupapi: Implement SetupAddToDiskSpaceList.", 1 },
    { "Michael Müller", "setupapi: Implement SetupQueryDrivesInDiskSpaceList.", 1 },
    { "Michael Müller", "setupapi: Rewrite DiskSpaceList logic using lists.", 1 },
    { "Michael Müller", "shell32: Add parameter to ISFHelper::DeleteItems to allow deleting files without confirmation.", 1 },
    { "Michael Müller", "shell32: Add security property tab.", 1 },
    { "Michael Müller", "shell32: Add support for setting/getting PREFERREDDROPEFFECT in IDataObject.", 1 },
    { "Michael Müller", "shell32: Correct indentation in shfileop.c.", 1 },
    { "Michael Müller", "shell32: Do not use unixfs for devices without mountpoint.", 1 },
    { "Michael Müller", "shell32: Fix copying of files when using a context menu.", 1 },
    { "Michael Müller", "shell32: Implement file operation progress dialog.", 1 },
    { "Michael Müller", "shell32: Implement insert/paste for item context menus.", 1 },
    { "Michael Müller", "shell32: Implement NewMenu with new folder item.", 1 },
    { "Michael Müller", "shell32: Implement process elevation using runas verb.", 1 },
    { "Michael Müller", "shell32: Pass FILE_INFORMATION into SHNotify* functions.", 1 },
    { "Michael Müller", "shell32: Recognize cut/copy/paste string verbs in item menu context menu.", 1 },
    { "Michael Müller", "shell32: Remove source files when using cut in the context menu.", 1 },
    { "Michael Müller", "shell32: Set return value correctly in DoPaste.", 1 },
    { "Michael Müller", "shell32: Set SFGAO_HASSUBFOLDER correctly for normal shellfolders.", 1 },
    { "Michael Müller", "shell32: Set SFGAO_HASSUBFOLDER correctly for unixfs.", 1 },
    { "Michael Müller", "shell32: Show animation during SHFileOperation.", 1 },
    { "Michael Müller", "tools/winebuild: Add syscall thunks for 64 bit.", 1 },
    { "Michael Müller", "uiautomationcore: Add dll and stub some functions.", 1 },
    { "Michael Müller", "user32: Allow changing the tablet / media center status via wine registry key.", 1 },
    { "Michael Müller", "user32: Decrease minimum SetTimer interval to 5 ms.", 2 },
    { "Michael Müller", "user32: Fix calculation of listbox size when horizontal scrollbar is present.", 1 },
    { "Michael Müller", "user32: Get rid of wineserver call for GetLastInputInfo.", 1 },
    { "Michael Müller", "user32: Start explorer.exe using limited rights.", 1 },
    { "Michael Müller", "uxtheme: Reset FPU flags before calling GTK3 functions.", 1 },
    { "Michael Müller", "win32k.sys: Add stub driver.", 1 },
    { "Michael Müller", "wineboot: Add some generic hardware in HKEY_DYN_DATA\\Config Manager\\Enum.", 1 },
    { "Michael Müller", "wineboot: Initialize proxy settings registry key.", 1 },
    { "Michael Müller", "winebuild: Add stub functions in fake dlls.", 1 },
    { "Michael Müller", "winebuild: Add syscall thunks in fake dlls.", 1 },
    { "Michael Müller", "winebuild: Fix size of relocation information in fake dlls.", 1 },
    { "Michael Müller", "winebuild: Generate syscall thunks for ntdll exports.", 1 },
    { "Michael Müller", "winebuild: Try to make sure RVA matches between fake and builtin DLLs.", 1 },
    { "Michael Müller", "winebuild: Use multipass label system to generate fake dlls.", 1 },
    { "Michael Müller", "winecfg: Add option to enable/disable GTK3 theming.", 1 },
    { "Michael Müller", "winecfg: Add staging tab for CSMT.", 1 },
    { "Michael Müller", "wined3d: Add wined3d_resource_map_info function.", 1 },
    { "Michael Müller", "wined3d: Improve wined3d_cs_emit_update_sub_resource.", 1 },
    { "Michael Müller", "wined3d: Use real values for memory accounting on NVIDIA cards.", 1 },
    { "Michael Müller", "wine.inf: Add 'New' context menu handler entry for directories.", 1 },
    { "Michael Müller", "wine.inf.in: Add invalid dummy certificate to CA certificate store.", 1 },
    { "Michael Müller", "winex11.drv: Allow to select default display frequency in registry key.", 1 },
    { "Michael Müller", "winex11.drv: Indicate direct rendering through OpenGL extension.", 1 },
    { "Michael Müller", "winex11.drv: Only warn about used contexts in wglShareLists.", 1 },
    { "Michael Müller", "wininet: Replacing header fields should fail if they do not exist yet.", 1 },
    { "Michael Müller", "wininet: Strip filename if no path is set in cookie.", 1 },
    { "Michael Müller", "wininet/tests: Add more tests for cookies.", 1 },
    { "Michael Müller", "wininet/tests: Check cookie behaviour when overriding host.", 1 },
    { "Michael Müller", "wininet/tests: Test auth credential reusage with host override.", 1 },
    { "Michael Müller", "winmm: Delay import ole32 msacm32 to workaround bug when loading multiple winmm versions.", 1 },
    { "Michael Müller", "winmm: Do not crash in Win 9X mode when an invalid device ptr is passed to MCI_OPEN.", 1 },
    { "Nakarin Khankham", "opencl: Add OpenCL 1.0 function pointer loader.", 1 },
    { "Nakarin Khankham", "opencl: Add OpenCL 1.1 implementation.", 1 },
    { "Nakarin Khankham", "opencl: Add OpenCL 1.2 implementation.", 1 },
    { "Nakarin Khankham", "opencl: Expose all extensions list to wine.", 1 },
    { "Nakarin Khankham", "opencl: Use function pointer instead of call the function directly.", 1 },
    { "Olivier F. R. Dierick", "kernel32: Implement GetSystemDEPPolicy().", 1 },
    { "Olivier F. R. Dierick", "kernel32: Implement SetProcessDEPPolicy().", 1 },
    { "Olivier F. R. Dierick", "kernel32: Make system DEP policy affect GetProcessDEPPolicy().", 1 },
    { "Ondrej Kraus", "winex11.drv: Fix main Russian keyboard layout.", 1 },
    { "Paul Gofman", "d3d11/tests: Add a basic test for drawing with deferred context.", 1 },
    { "Paul Gofman", "d3d9: Support SWVP vertex shader float constants limits.", 1 },
    { "Paul Gofman", "d3d9/tests: Add test for indexed vertex blending.", 1 },
    { "Paul Gofman", "ddraw: Allow setting texture without DDSCAPS_TEXTURE for software device.", 1 },
    { "Paul Gofman", "kernelbase: Call FLS callbacks from DeleteFiber().", 1 },
    { "Paul Gofman", "kernelbase: Call FLS callbacks from FlsFree().", 1 },
    { "Paul Gofman", "kernelbase: Don't use PEB lock for FLS data.", 1 },
    { "Paul Gofman", "kernelbase: Maintain FLS storage list in PEB.", 1 },
    { "Paul Gofman", "kernelbase: Zero all FLS slots instances in FlsFree().", 1 },
    { "Paul Gofman", "libs/wine: Add functions for managing free area list.", 1 },
    { "Paul Gofman", "ntdll: Call FLS callbacks on thread shutdown.", 1 },
    { "Paul Gofman", "ntdll: Call NtOpenFile through syscall thunk.", 1 },
    { "Paul Gofman", "ntdll: Force bottom up allocation order for 64 bit arch unless top down is requested.", 1 },
    { "Paul Gofman", "ntdll: Increase step after failed map attempt in try_map_free_area().", 1 },
    { "Paul Gofman", "ntdll: Permanently exclude natively mapped areas from free areas list.", 1 },
    { "Paul Gofman", "ntdll: Stop search on mmap() error in try_map_free_area().", 1 },
    { "Paul Gofman", "ntdll: Support x86_64 syscall emulation.", 1 },
    { "Paul Gofman", "ntdll: Use free area list for virtual memory allocation.", 1 },
    { "Paul Gofman", "ntdll: Use MAP_FIXED_NOREPLACE flag in try_map_free_area() if available.", 1 },
    { "Paul Gofman", "wined3d: Add a setting to workaround 0 * inf problem in shader models 1-3.", 1 },
    { "Paul Gofman", "wined3d: Allow higher world matrix states.", 1 },
    { "Paul Gofman", "wined3d: Report actual vertex shader float constants limit for SWVP device.", 1 },
    { "Paul Gofman", "wined3d: Support indexed vertex blending.", 1 },
    { "Paul Gofman", "wined3d: Support SWVP mode vertex shaders.", 1 },
    { "Paul Gofman", "wined3d: Support SWVP vertex shader constants limit in state tracking.", 1 },
    { "Paul Gofman", "wined3d: Use UBO for vertex shader float constants if supported.", 1 },
    { "Philippe Valembois", "winex11: Fix more key translation.", 1 },
    { "Qian Hong", "atl: Implement AtlAxDialogBox[A,W].", 1 },
    { "Qian Hong", "ntdll: Add fake data implementation for ProcessQuotaLimits class.", 1 },
    { "Qian Hong", "ntdll: Improve invalid paramater handling in NtAccessCheck.", 1 },
    { "Qian Hong", "ntdll: Initialize mod_name to zero.", 1 },
    { "Qian Hong", "ntdll: Set EOF on file which has a memory mapping should fail.", 1 },
    { "Qian Hong", "ntdll/tests: Added tests for open behaviour on readonly files.", 1 },
    { "Qian Hong", "ntdll/tests: Added tests to set disposition on file which is mapped to memory.", 1 },
    { "Qian Hong", "server: Do not allow to set disposition on file which has a file mapping.", 1 },
    { "Rémi Bernon", "dinput8: Add support for dinput devices that use raw input interface.", 1 },
    { "Rémi Bernon", "dinput8/tests: Add test for DISCL_EXCLUSIVE flag interaction with rawinput.", 1 },
    { "Rémi Bernon", "dinput8: Use raw input interface for dinput8 mouse device.", 1 },
    { "Rémi Bernon", "dinput: Fix rawinput events sequence number.", 1 },
    { "Rémi Bernon", "dinput: Set correct rawinput flags for DISCL_EXCLUSIVE.", 1 },
    { "Rémi Bernon", "dinput: Set RIDEV_INPUTSINK flag only when DISCL_BACKGROUND is requested.", 1 },
    { "Rémi Bernon", "hidclass.sys: Send input message to server when HID report is received.", 1 },
    { "Rémi Bernon", "server: Add HID input message type to send_hardware_message request.", 1 },
    { "Rémi Bernon", "server: Add process argument to find_rawinput_device.", 1 },
    { "Rémi Bernon", "server: Add send_hardware_message flags for rawinput translation.", 1 },
    { "Rémi Bernon", "server: Allow extra data for hardware_msg_data message.", 1 },
    { "Rémi Bernon", "server: Also update the key state if RIDEV_NOLEGACY is used.", 1 },
    { "Rémi Bernon", "server: Broadcast rawinput message if request flag is SEND_HWMSG_RAWINPUT.", 1 },
    { "Rémi Bernon", "server: Update desktop cursor pos even if RIDEV_NOLEGACY flag is set.", 1 },
    { "Rémi Bernon", "user32: Add __wine_send_input flags to hint raw input translation.", 1 },
    { "Rémi Bernon", "user32: Implement GetRegisteredRawInputDevices.", 1 },
    { "Rémi Bernon", "user32: Implement WM_INPUT/RIM_TYPEHID message handling.", 1 },
    { "Rémi Bernon", "user32/tests: Add rawinput test for ClipCursor interactions.", 1 },
    { "Rémi Bernon", "user32/tests: Add rawinput test for cross-process interactions.", 1 },
    { "Rémi Bernon", "user32/tests: Add rawinput test for cross-thread interactions.", 1 },
    { "Rémi Bernon", "winex11.drv: Advertise XInput2 version 2.1 support.", 1 },
    { "Rémi Bernon", "winex11.drv: Keep track of pointer and device button mappings.", 1 },
    { "Rémi Bernon", "winex11.drv: Listen to RawMotion and RawButton* events in the desktop thread.", 1 },
    { "Rémi Bernon", "winex11.drv: Send relative RawMotion events unprocessed.", 1 },
    { "Ryan S. Northrup (RyNo)", "user32: Semi-stub GetMouseMovePointsEx.", 2 },
    { "Sebastian Lackner", "advapi32/tests: Add ACL inheritance tests for creating subdirectories with NtCreateFile.", 1 },
    { "Sebastian Lackner", "advapi32/tests: Add tests for ACL inheritance in CreateDirectoryA.", 1 },
    { "Sebastian Lackner", "advapi: Trigger write watches before passing userdata pointer to read syscall.", 1 },
    { "Sebastian Lackner", "configure: Also add the absolute RPATH when linking against libwine.", 1 },
    { "Sebastian Lackner", "d2d1: Avoid implicit cast of interface pointer.", 1 },
    { "Sebastian Lackner", "d3d11: Avoid implicit cast of interface pointer.", 1 },
    { "Sebastian Lackner", "d3d8: Avoid implicit cast of interface pointer.", 1 },
    { "Sebastian Lackner", "d3d9: Avoid implicit cast of interface pointer.", 1 },
    { "Sebastian Lackner", "d3dx9_36: Improve stub for ID3DXEffectImpl_CloneEffect.", 1 },
    { "Sebastian Lackner", "d3dx9_36/tests: Add initial tests for D3DXDisassembleShader.", 1 },
    { "Sebastian Lackner", "dbghelp: Always check for debug symbols in BINDIR.", 1 },
    { "Sebastian Lackner", "ddraw: Avoid implicit cast of interface pointer.", 1 },
    { "Sebastian Lackner", "dsound: Allow disabling of EAX support in the registry.", 1 },
    { "Sebastian Lackner", "dsound: Apply filters before sound is multiplied to speakers.", 1 },
    { "Sebastian Lackner", "dsound: Various improvements to EAX support.", 1 },
    { "Sebastian Lackner", "dwrite: Avoid implicit cast of interface pointer.", 1 },
    { "Sebastian Lackner", "dxva2: Implement semi-stub for Direct3DDeviceManager9 interface.", 1 },
    { "Sebastian Lackner", "fonts: Add Liberation Mono as an Courier New replacement.", 1 },
    { "Sebastian Lackner", "fonts: Add Liberation Serif as an Times New Roman replacement.", 1 },
    { "Sebastian Lackner", "gdi32: Perform lazy initialization of fonts to improve startup performance.", 1 },
    { "Sebastian Lackner", "include: Add cuda.h.", 1 },
    { "Sebastian Lackner", "include: Always define hton/ntoh macros.", 1 },
    { "Sebastian Lackner", "include: Check element type in CONTAINING_RECORD and similar macros.", 1 },
    { "Sebastian Lackner", "include: Move interlocked_inc/dec to port.h.", 1 },
    { "Sebastian Lackner", "iphlpapi: Fallback to system ping when ICMP permissions are not present.", 1 },
    { "Sebastian Lackner", "kernel32: Add winediag message to show warning, that this isn't vanilla wine.", 1 },
    { "Sebastian Lackner", "kernel32: Always start debugger on WinSta0.", 1 },
    { "Sebastian Lackner", "krnl386.exe16: Do not abuse WOW32Reserved field for 16-bit stack address.", 1 },
    { "Sebastian Lackner", "loader: Add commandline option --patches to show the patch list.", 1 },
    { "Sebastian Lackner", "msvcrt: Calculate sinh/cosh/exp/pow with higher precision.", 2 },
    { "Sebastian Lackner", "msxml3: Avoid implicit cast of interface pointer.", 1 },
    { "Sebastian Lackner", "ntdll: Add back SS segment prefixes in set_full_cpu_context.", 1 },
    { "Sebastian Lackner", "ntdll: Add helper function to delete free blocks.", 1 },
    { "Sebastian Lackner", "ntdll: Add inline versions of RtlEnterCriticalSection / RtlLeaveCriticalSections.", 1 },
    { "Sebastian Lackner", "ntdll: Add semi-stub for TokenLinkedToken info class.", 1 },
    { "Sebastian Lackner", "ntdll: Add special handling for \\SystemRoot to satisfy MSYS2 case-insensitive system check.", 1 },
    { "Sebastian Lackner", "ntdll: Add support for hiding wine version information from applications.", 1 },
    { "Sebastian Lackner", "ntdll: Allow to query section names from other processes.", 2 },
    { "Sebastian Lackner", "ntdll: Always store SAMBA_XATTR_DOS_ATTRIB when path could be interpreted as hidden.", 1 },
    { "Sebastian Lackner", "ntdll: Do not allow to deallocate thread stack for current thread.", 1 },
    { "Sebastian Lackner", "ntdll: Fix race-condition when threads are killed during shutdown.", 1 },
    { "Sebastian Lackner", "ntdll: Fix return value for missing ACTIVATION_CONTEXT_SECTION_ASSEMBLY_INFORMATION key.", 1 },
    { "Sebastian Lackner", "ntdll: Implement virtual_map_shared_memory.", 1 },
    { "Sebastian Lackner", "ntdll: Improve heap allocation performance.", 2 },
    { "Sebastian Lackner", "ntdll: Improve stub of NtQueryEaFile.", 1 },
    { "Sebastian Lackner", "ntdll: Move code to update user shared data into a separate function.", 1 },
    { "Sebastian Lackner", "ntdll: Only enable wineserver shared memory communication when a special environment variable is set.", 1 },
    { "Sebastian Lackner", "ntdll: OutputDebugString should throw the exception a second time, if a debugger is attached.", 1 },
    { "Sebastian Lackner", "ntdll: Resolve drive symlinks before returning section name.", 1 },
    { "Sebastian Lackner", "ntdll: Return buffer filled with random values from SystemInterruptInformation.", 1 },
    { "Sebastian Lackner", "ntdll: Return correct values in GetThreadTimes() for all threads.", 1 },
    { "Sebastian Lackner", "ntdll: Return fake device type when systemroot is located on virtual disk.", 1 },
    { "Sebastian Lackner", "ntdll: Return STATUS_INVALID_DEVICE_REQUEST when trying to call NtReadFile on directory.", 1 },
    { "Sebastian Lackner", "ntdll: Reuse old async fileio structures if possible.", 1 },
    { "Sebastian Lackner", "ntdll/tests: Add test to ensure section name is full path.", 1 },
    { "Sebastian Lackner", "ntdll: Trigger write watches before passing userdata pointer to wait_reply.", 1 },
    { "Sebastian Lackner", "ntdll: Use fast CS functions for heap locking.", 1 },
    { "Sebastian Lackner", "ntdll: Use fast CS functions for threadpool locking.", 1 },
    { "Sebastian Lackner", "ntoskrnl: Update USER_SHARED_DATA before accessing memory.", 1 },
    { "Sebastian Lackner", "nvcuda: Add stub dll.", 1 },
    { "Sebastian Lackner", "nvcuda: Add support for CUDA 7.0.", 1 },
    { "Sebastian Lackner", "nvcuda: Implement cuModuleLoad wrapper function.", 1 },
    { "Sebastian Lackner", "nvcuda: Implement new functions added in CUDA 6.5.", 1 },
    { "Sebastian Lackner", "nvcuda: Properly wrap stream callbacks by forwarding them to a worker thread.", 1 },
    { "Sebastian Lackner", "ole32: Improve thread-safety of HGLOBALStreamImpl_Read.", 1 },
    { "Sebastian Lackner", "ole32: Improve thread-safety of HGLOBALStreamImpl_Write.", 1 },
    { "Sebastian Lackner", "ole32: Set DebugInfo->Spare[0] for handle_wrapper lock.", 1 },
    { "Sebastian Lackner", "oleaut32: Avoid implicit cast of interface pointer.", 1 },
    { "Sebastian Lackner", "oleaut32: Implement SaveAsFile for PICTYPE_ENHMETAFILE.", 1 },
    { "Sebastian Lackner", "packager: Prefer native version.", 1 },
    { "Sebastian Lackner", "riched20: Silence repeated FIXMEs triggered by Adobe Reader.", 1 },
    { "Sebastian Lackner", "rpcrt4: Avoid implicit cast of interface pointer.", 1 },
    { "Sebastian Lackner", "server: Add a helper function set_sd_from_token_internal to merge two security descriptors.", 1 },
    { "Sebastian Lackner", "server: Allow multiple registry notifications for the same key.", 1 },
    { "Sebastian Lackner", "server: Allow to open files without any permission bits.", 2 },
    { "Sebastian Lackner", "server: Assign random name when no name was passed to create_winstation.", 1 },
    { "Sebastian Lackner", "server: Do not signal violently terminated threads until they are really gone.", 1 },
    { "Sebastian Lackner", "server: FILE_WRITE_ATTRIBUTES should succeed for readonly files.", 1 },
    { "Sebastian Lackner", "server: Fix handling of GetMessage after previous PeekMessage call.", 3 },
    { "Sebastian Lackner", "server: Growing files which are mapped to memory should still work.", 1 },
    { "Sebastian Lackner", "server: Implement locking and synchronization of keystate buffer.", 3 },
    { "Sebastian Lackner", "server: Improve mapping of DACL to file permissions.", 1 },
    { "Sebastian Lackner", "server: Improve STATUS_CANNOT_DELETE checks for directory case.", 1 },
    { "Sebastian Lackner", "server: Introduce a helper function to update the thread_input key state.", 1 },
    { "Sebastian Lackner", "server: Introduce a new alloc_handle object callback.", 2 },
    { "Sebastian Lackner", "server: Introduce refcounting for registry notifications.", 1 },
    { "Sebastian Lackner", "server: Store a list of associated queues for each thread input.", 1 },
    { "Sebastian Lackner", "server: Temporarily store the full security descriptor for file objects.", 1 },
    { "Sebastian Lackner", "server: Track desktop handle count more correctly.", 1 },
    { "Sebastian Lackner", "server: Use all group attributes in create_token.", 1 },
    { "Sebastian Lackner", "server: When creating new directories temporarily give read-permissions until they are opened.", 1 },
    { "Sebastian Lackner", "setupapi: Fix CabinetName passed to SPFILENOTIFY_CABINETINFO handler.", 1 },
    { "Sebastian Lackner", "setupapi/tests: Add tests for cabinet name passed to SPFILENOTIFY_FILEEXTRACTED.", 1 },
    { "Sebastian Lackner", "shlwapi/tests: Add additional tests for UrlCombine and UrlCanonicalize.", 1 },
    { "Sebastian Lackner", "shlwapi: UrlCombineW workaround for relative paths.", 1 },
    { "Sebastian Lackner", "stdole32.tlb: Compile typelib with --oldtlb.", 1 },
    { "Sebastian Lackner", "user32: Avoid unnecessary wineserver calls in PeekMessage/GetMessage.", 1 },
    { "Sebastian Lackner", "user32: Call UpdateWindow() during DIALOG_CreateIndirect.", 1 },
    { "Sebastian Lackner", "user32: Fix handling of invert_y in DrawTextExW.", 1 },
    { "Sebastian Lackner", "user32: Get rid of wineserver call for GetActiveWindow, GetFocus, GetCapture.", 1 },
    { "Sebastian Lackner", "user32: Get rid of wineserver call for GetInputState.", 1 },
    { "Sebastian Lackner", "user32: Refresh MDI menus when DefMDIChildProc(WM_SETTEXT) is called.", 1 },
    { "Sebastian Lackner", "user32/tests: Add tests for clicking through layered window.", 1 },
    { "Sebastian Lackner", "user32/tests: Add tests for DC region.", 1 },
    { "Sebastian Lackner", "user32/tests: Add tests for window region of layered windows.", 1 },
    { "Sebastian Lackner", "uxtheme: Correctly render buttons with GTK >= 3.14.0.", 1 },
    { "Sebastian Lackner", "uxtheme: Fix some incorrect error codes.", 1 },
    { "Sebastian Lackner", "vbscript: Avoid implicit cast of interface pointer.", 1 },
    { "Sebastian Lackner", "widl: Add --oldtlb switch in usage message.", 1 },
    { "Sebastian Lackner", "wineboot: Assign a drive serial number during prefix creation/update.", 1 },
    { "Sebastian Lackner", "wineboot: Init system32/drivers/etc/{host,networks,protocol,services}.", 1 },
    { "Sebastian Lackner", "winecfg: Add checkbox to enable/disable HideWineExports registry key.", 1 },
    { "Sebastian Lackner", "winecfg: Add checkbox to enable/disable vaapi GPU decoder.", 1 },
    { "Sebastian Lackner", "winelib: Append '(Staging)' at the end of the version string.", 1 },
    { "Sebastian Lackner", "winemenubuilder: Create desktop shortcuts with absolute wine path.", 1 },
    { "Sebastian Lackner", "winepulse.drv: Use a separate mainloop and ctx for pulse_test_connect.", 1 },
    { "Sebastian Lackner", "winex11: Enable/disable windows when they are (un)mapped by foreign applications.", 1 },
    { "Sebastian Lackner", "winex11: Fix alpha blending in X11DRV_UpdateLayeredWindow.", 1 },
    { "Sebastian Lackner", "winex11: Implement X11DRV_FLUSH_GDI_DISPLAY ExtEscape command.", 1 },
    { "Sebastian Lackner", "ws2_32: Divide values returned by SO_RCVBUF and SO_SNDBUF getsockopt options by two.", 1 },
    { "Sebastian Lackner", "ws2_32: Fix handling of empty string in WS_getaddrinfo.", 1 },
    { "Sebastian Lackner", "ws2_32: Implement returning the proper time with SO_CONNECT_TIME.", 1 },
    { "Sebastian Lackner", "ws2_32: Invalidate client-side file descriptor cache in WSACleanup.", 1 },
    { "Sebastian Lackner", "ws2_32: Reuse old async ws2_async_io structures if possible.", 1 },
    { "Sebastian Lackner", "wsdapi: Avoid implicit cast of interface pointer.", 1 },
    { "Sebastian Lackner", "wtsapi32: Partial implementation of WTSEnumerateProcessesW.", 1 },
    { "Stanislav Zhukov", "wined3d: Implement WINED3DFMT_B8G8R8X8_UNORM to WINED3DFMT_L8_UNORM conversion.", 1 },
    { "Steve Melenchuk", "d3d11: Allow NULL pointer for initial count in d3d11_deferred_context_CSSetUnorderedAccessViews.", 1 },
    { "Torsten Kurbad", "fonts: Add Liberation Sans as an Arial replacement.", 2 },
    { "Zebediah Figura", "configure: Check for sys/eventfd.h, ppoll(), and shm_open().", 1 },
    { "Zebediah Figura", "esync: Add a README.", 1 },
    { "Zebediah Figura", "esync: Update README.", 1 },
    { "Zebediah Figura", "esync: Update README.", 1 },
    { "Zebediah Figura", "esync: Update README.", 1 },
    { "Zebediah Figura", "esync: Update README.", 1 },
    { "Zebediah Figura", "kernel32/tests: Add some event tests.", 1 },
    { "Zebediah Figura", "kernel32/tests: Add some mutex tests.", 1 },
    { "Zebediah Figura", "kernel32/tests: Add some semaphore tests.", 1 },
    { "Zebediah Figura", "kernel32/tests: Add some tests for wait timeouts.", 1 },
    { "Zebediah Figura", "kernel32/tests: Mark some existing tests as failing under esync.", 1 },
    { "Zebediah Figura", "kernel32/tests: Zigzag test.", 1 },
    { "Zebediah Figura", "makefiles: Only apply non-include-path EXTRAINCL flags to C sources.", 1 },
    { "Zebediah Figura", "ntdll: Add a stub implementation of Wow64Transition.", 1 },
    { "Zebediah Figura", "ntdll: Add stub for NtQuerySystemInformation(SystemExtendedProcessInformation).", 1 },
    { "Zebediah Figura", "ntdll: Avoid fstatat().", 1 },
    { "Zebediah Figura", "ntdll: Avoid server_select() when waiting for critical sections.", 1 },
    { "Zebediah Figura", "ntdll: Cache the esync struct itself instead of a pointer to it.", 1 },
    { "Zebediah Figura", "ntdll: Check the APC fd first.", 1 },
    { "Zebediah Figura", "ntdll: Close esync objects.", 1 },
    { "Zebediah Figura", "ntdll: Correctly allocate the esync handle cache.", 1 },
    { "Zebediah Figura", "ntdll: Create esync objects for events.", 1 },
    { "Zebediah Figura", "ntdll: Create esync objects for mutexes.", 1 },
    { "Zebediah Figura", "ntdll: Create eventfd-based objects for semaphores.", 1 },
    { "Zebediah Figura", "ntdll: Don't call LdrQueryProcessModuleInformation in NtQuerySystemInformation(SystemModuleInformation).", 1 },
    { "Zebediah Figura", "ntdll/esync: Lock accessing the shm_addrs array.", 1 },
    { "Zebediah Figura", "ntdll: Fix a couple of misplaced global variables.", 1 },
    { "Zebediah Figura", "ntdll: Fix a missing break statement.", 1 },
    { "Zebediah Figura", "ntdll: Fix growing the shm_addrs array.", 1 },
    { "Zebediah Figura", "ntdll: Get rid of the per-event spinlock for auto-reset events.", 1 },
    { "Zebediah Figura", "ntdll: Go through the server if necessary when performing event/semaphore/mutex ops.", 1 },
    { "Zebediah Figura", "ntdll: Ignore pseudo-handles.", 1 },
    { "Zebediah Figura", "ntdll: Implement NtOpenEvent().", 1 },
    { "Zebediah Figura", "ntdll: Implement NtOpenMutant().", 1 },
    { "Zebediah Figura", "ntdll: Implement NtPulseEvent().", 1 },
    { "Zebediah Figura", "ntdll: Implement NtQueryEvent().", 1 },
    { "Zebediah Figura", "ntdll: Implement NtQueryMutant().", 1 },
    { "Zebediah Figura", "ntdll: Implement NtQuerySemaphore().", 1 },
    { "Zebediah Figura", "ntdll: Implement NtReleaseMutant().", 1 },
    { "Zebediah Figura", "ntdll: Implement NtReleaseSemaphore().", 1 },
    { "Zebediah Figura", "ntdll: Implement NtResetEvent().", 1 },
    { "Zebediah Figura", "ntdll: Implement NtSetEvent().", 1 },
    { "Zebediah Figura", "ntdll: Implement NtSignalAndWaitForSingleObject().", 1 },
    { "Zebediah Figura", "ntdll: Implement wait-all.", 1 },
    { "Zebediah Figura", "ntdll: Implement waiting on esync objects.", 1 },
    { "Zebediah Figura", "ntdll: Implement waiting on events.", 1 },
    { "Zebediah Figura", "ntdll: Implement waiting on mutexes.", 1 },
    { "Zebediah Figura", "ntdll: Let the server know when we are doing a message wait.", 1 },
    { "Zebediah Figura", "ntdll: Lock creating and opening objects with volatile state.", 1 },
    { "Zebediah Figura", "ntdll: Record the current count of a semaphore locally.", 1 },
    { "Zebediah Figura", "ntdll: Report SegDs to be identical to SegSs on x86_64.", 1 },
    { "Zebediah Figura", "ntdll, server: Abandon esync mutexes on thread exit.", 1 },
    { "Zebediah Figura", "ntdll, server: Abort if esync is enabled for the server but not the client, and vice versa.", 1 },
    { "Zebediah Figura", "ntdll, server: Allow DuplicateHandle() to succeed by implementing esync_get_esync_fd().", 1 },
    { "Zebediah Figura", "ntdll, server: Check the value of WINEESYNC instead of just the presence.", 1 },
    { "Zebediah Figura", "ntdll, server: Implement NtOpenSemaphore().", 1 },
    { "Zebediah Figura", "ntdll, server: Implement waiting on server-bound objects.", 1 },
    { "Zebediah Figura", "ntdll, server: Initialize the shared memory portion on the server side.", 1 },
    { "Zebediah Figura", "ntdll, server: Revert to old implementation of hung queue detection.", 1 },
    { "Zebediah Figura", "ntdll, server: Specify EFD_SEMAPHORE on the server side.", 1 },
    { "Zebediah Figura", "ntdll: Store an event's signaled state internally.", 1 },
    { "Zebediah Figura", "ntdll: Store esync objects locally.", 1 },
    { "Zebediah Figura", "ntdll: Try again if poll() returns EINTR.", 1 },
    { "Zebediah Figura", "ntdll: Try to avoid poll() for uncontended objects.", 1 },
    { "Zebediah Figura", "ntdll: Use shared memory segments to store semaphore and mutex state.", 1 },
    { "Zebediah Figura", "ntdll, wineandroid.drv, winemac.drv, winex11.drv: Store the thread's queue fd in ntdll.", 1 },
    { "Zebediah Figura", "ntdll: Yield during PulseEvent().", 1 },
    { "Zebediah Figura", "Revert \"uxtheme: Build with msvcrt.\".", 1 },
    { "Zebediah Figura", "rpcrt4: Avoid closing the server thread handle while it is being waited on.", 1 },
    { "Zebediah Figura", "server: Add an object operation to grab the esync file descriptor.", 1 },
    { "Zebediah Figura", "server: Add a request to get the eventfd file descriptor associated with a waitable handle.", 1 },
    { "Zebediah Figura", "server: Allocate shared memory segments for semaphores and mutexes.", 1 },
    { "Zebediah Figura", "server: Allow (re)setting esync events on the server side.", 1 },
    { "Zebediah Figura", "server: Alter conditions in is_queue_hung().", 1 },
    { "Zebediah Figura", "server: Alter conditions in is_queue_hung(), again.", 1 },
    { "Zebediah Figura", "server: Create esync file descriptors for true file objects and use them for directory change notifications.", 1 },
    { "Zebediah Figura", "server: Create eventfd descriptors for console_input_events objects.", 1 },
    { "Zebediah Figura", "server: Create eventfd descriptors for device manager objects.", 1 },
    { "Zebediah Figura", "server: Create eventfd descriptors for pseudo-fd objects and use them for named pipes.", 1 },
    { "Zebediah Figura", "server: Create eventfd descriptors for timers.", 1 },
    { "Zebediah Figura", "server: Create eventfd file descriptors for event objects.", 1 },
    { "Zebediah Figura", "server: Create eventfd file descriptors for message queues.", 1 },
    { "Zebediah Figura", "server: Create eventfd file descriptors for process objects.", 1 },
    { "Zebediah Figura", "server: Create eventfd file descriptors for thread objects.", 1 },
    { "Zebediah Figura", "server: Create server objects for eventfd-based synchronization objects.", 1 },
    { "Zebediah Figura", "server: Don't check for a hung queue when sending low-level hooks.", 1 },
    { "Zebediah Figura", "server: Implement esync_map_access().", 1 },
    { "Zebediah Figura", "server, ntdll: Also store the esync type in the server.", 1 },
    { "Zebediah Figura", "server, ntdll: Also wait on the queue fd when waiting for driver events.", 1 },
    { "Zebediah Figura", "server, ntdll: Implement alertable waits.", 1 },
    { "Zebediah Figura", "server, ntdll: Pass the shared memory index back from get_esync_fd.", 1 },
    { "Zebediah Figura", "server: Only signal the APC fd for user APCs.", 1 },
    { "Zebediah Figura", "server: Update the shared memory state when (re)setting an event.", 1 },
    { "Zebediah Figura", "user32: Remove hooks that time out.", 1 },
    { "Zebediah Figura", "=?UTF-8?q?server:=20Try=20to=20remove=20a=20pre?= =?UTF-8?q?=C3=ABxisting=20shm=20file.?=.", 1 },
    { "Zebediah Figura", "wow64cpu: Add stub dll.", 1 },
    { "Zhenbo Li", "mshtml: Add IHTMLLocation::hash property's getter implementation.", 1 },
    { "Zhenbo Li", "shell32: Fix SHFileOperation(FO_MOVE) for creating subdirectories.", 1 },
    { "Zhiyi Zhang", "user32: Send a WM_ACTIVATE message after restoring a minimized window.", 1 },
    { "Zhiyi Zhang", "winepulse.drv: Fix getting the same timing info.", 1 },
    { NULL, NULL, 0 }
};

/* return the applied non-standard patches */
const void *wine_get_patches(void)
{
    return &wine_patch_data[0];
}

/* return the build id string */
const char *wine_get_build_id(void)
{
    extern const char wine_build[];
    return wine_build;
}

/* exec a binary using the preloader if requested; helper for wine_exec_wine_binary */
static void preloader_exec( char **argv, int use_preloader )
{
    if (use_preloader)
    {
        static const char preloader[] = "wine-preloader";
        static const char preloader64[] = "wine64-preloader";
        char *p, *full_name;
        char **last_arg = argv, **new_argv;

        if (!(p = strrchr( argv[0], '/' ))) p = argv[0];
        else p++;

        full_name = xmalloc( p - argv[0] + sizeof(preloader64) );
        memcpy( full_name, argv[0], p - argv[0] );
        if (strendswith( p, "64" ))
            memcpy( full_name + (p - argv[0]), preloader64, sizeof(preloader64) );
        else
            memcpy( full_name + (p - argv[0]), preloader, sizeof(preloader) );

        /* make a copy of argv */
        while (*last_arg) last_arg++;
        new_argv = xmalloc( (last_arg - argv + 2) * sizeof(*argv) );
        memcpy( new_argv + 1, argv, (last_arg - argv + 1) * sizeof(*argv) );
        new_argv[0] = full_name;
#ifdef __APPLE__
        {
            posix_spawnattr_t attr;
            posix_spawnattr_init( &attr );
            posix_spawnattr_setflags( &attr, POSIX_SPAWN_SETEXEC | _POSIX_SPAWN_DISABLE_ASLR );
            posix_spawn( NULL, full_name, NULL, &attr, new_argv, *_NSGetEnviron() );
            posix_spawnattr_destroy( &attr );
        }
#endif
        execv( full_name, new_argv );
        free( new_argv );
        free( full_name );
    }
    execv( argv[0], argv );
}

/* exec a wine internal binary (either the wine loader or the wine server) */
void wine_exec_wine_binary( const char *name, char **argv, const char *env_var )
{
    const char *path, *pos, *ptr;
    int use_preloader;

    if (!name) name = argv0_name;  /* no name means default loader */

#if defined(linux) || defined(__APPLE__)
    use_preloader = !strendswith( name, "wineserver" );
#else
    use_preloader = 0;
#endif

    if ((ptr = strrchr( name, '/' )))
    {
        /* if we are in build dir and name contains a path, try that */
        if (build_dir)
        {
            if (wineserver64 && !strcmp( name, "server/wineserver" ))
                argv[0] = xstrdup( wineserver64 );
            else
                argv[0] = build_path( build_dir, name );
            preloader_exec( argv, use_preloader );
            free( argv[0] );
        }
        name = ptr + 1;  /* get rid of path */
    }

    /* first, bin directory from the current libdir or argv0 */
    if (bindir)
    {
        argv[0] = build_path( bindir, name );
        preloader_exec( argv, use_preloader );
        free( argv[0] );
    }

    /* then specified environment variable */
    if (env_var)
    {
        argv[0] = (char *)env_var;
        preloader_exec( argv, use_preloader );
    }

    /* now search in the Unix path */
    if ((path = getenv( "PATH" )))
    {
        argv[0] = xmalloc( strlen(path) + strlen(name) + 2 );
        pos = path;
        for (;;)
        {
            while (*pos == ':') pos++;
            if (!*pos) break;
            if (!(ptr = strchr( pos, ':' ))) ptr = pos + strlen(pos);
            memcpy( argv[0], pos, ptr - pos );
            strcpy( argv[0] + (ptr - pos), "/" );
            strcat( argv[0] + (ptr - pos), name );
            preloader_exec( argv, use_preloader );
            pos = ptr;
        }
        free( argv[0] );
    }

    /* and finally try BINDIR */
    argv[0] = build_path( BINDIR, name );
    preloader_exec( argv, use_preloader );
    free( argv[0] );
}

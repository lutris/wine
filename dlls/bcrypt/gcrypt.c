#if 0
#pragma makedep unix
#endif

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>

#ifdef HAVE_GCRYPT_H

#include <gcrypt.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "bcrypt.h"

#include "bcrypt_internal.h"

#include "wine/debug.h"
#include "wine/heap.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(bcrypt);
WINE_DECLARE_DEBUG_CHANNEL(winediag);

static void *libgcrypt_handle;
#define MAKE_FUNCPTR(f) static typeof(f) * p##f
MAKE_FUNCPTR(gcry_check_version);
MAKE_FUNCPTR(gcry_sexp_build);
MAKE_FUNCPTR(gcry_pk_encrypt);
MAKE_FUNCPTR(gcry_mpi_new);
MAKE_FUNCPTR(gcry_mpi_print);
MAKE_FUNCPTR(gcry_sexp_release);
MAKE_FUNCPTR(gcry_mpi_release);
MAKE_FUNCPTR(gcry_strsource);
MAKE_FUNCPTR(gcry_strerror);
MAKE_FUNCPTR(gcry_sexp_find_token);
MAKE_FUNCPTR(gcry_sexp_nth_mpi);
#undef MAKE_FUNCPTR

BOOL gcrypt_initialize(void)
{
    if (!(libgcrypt_handle = dlopen( SONAME_LIBGCRYPT, RTLD_NOW)))
    {
        ERR_(winediag)( "failed to load libgcrypt, no support for diffie hellman key exchange\n" );
        return FALSE;
    }

#define LOAD_FUNCPTR(f) \
    if (!(p##f = dlsym( libgcrypt_handle, #f))) \
    { \
        ERR( "failed to load %s\n", #f ); \
        goto fail; \
    }

    LOAD_FUNCPTR(gcry_check_version);
    LOAD_FUNCPTR(gcry_sexp_build);
    LOAD_FUNCPTR(gcry_pk_encrypt);
    LOAD_FUNCPTR(gcry_mpi_new);
    LOAD_FUNCPTR(gcry_mpi_print);
    LOAD_FUNCPTR(gcry_sexp_release);
    LOAD_FUNCPTR(gcry_mpi_release);
    LOAD_FUNCPTR(gcry_strsource);
    LOAD_FUNCPTR(gcry_strerror);
    LOAD_FUNCPTR(gcry_sexp_find_token);
    LOAD_FUNCPTR(gcry_sexp_nth_mpi);
#undef LOAD_FUNCPTR

    return TRUE;

fail:
    dlclose( libgcrypt_handle);
    libgcrypt_handle = NULL;
    return FALSE;
}


static void gcrypt_uninitialize(void)
{
    if (libgcrypt_handle)
    {
        dlclose( libgcrypt_handle);
        libgcrypt_handle = NULL;
    }
}

static NTSTATUS extract_result_into_secret(gcry_sexp_t result, struct secret *secret)
{
    NTSTATUS status = STATUS_SUCCESS;
    gcry_mpi_t fullcoords = NULL;
    gcry_sexp_t fragment = NULL;
    UCHAR *tmp_buffer = NULL;
    gcry_error_t err;
    size_t size;

    fragment = pgcry_sexp_find_token(result, "s", 0);
    if (!fragment)
    {
        status = STATUS_NO_MEMORY;
        goto done;
    }

    fullcoords = pgcry_sexp_nth_mpi(fragment, 1, GCRYMPI_FMT_USG);
    if (!fullcoords)
    {
        status = STATUS_NO_MEMORY;
        goto done;
    }

    if ((err = pgcry_mpi_print(GCRYMPI_FMT_USG, NULL, 0, &size, fullcoords)))
    {
        ERR("Error = %s/%s.\n", pgcry_strsource(err), pgcry_strerror(err));
        status = STATUS_INTERNAL_ERROR;
        goto done;
    }

    tmp_buffer = malloc(size);
    if ((err = pgcry_mpi_print(GCRYMPI_FMT_STD, tmp_buffer, size, NULL, fullcoords)))
    {
        ERR("Error = %s/%s.\n", pgcry_strsource(err), pgcry_strerror(err));
        status = STATUS_INTERNAL_ERROR;
        goto done;
    }

    secret->data = RtlAllocateHeap(GetProcessHeap(), 0, size / 2);
    memcpy(secret->data, tmp_buffer + size % 2, size / 2);
    secret->len = size / 2;

done:
    free(tmp_buffer);

    pgcry_mpi_release(fullcoords);
    pgcry_sexp_release(fragment);

    return status;
}

/* this is necessary since GNUTLS doesn't support ECDH public key encryption, maybe we can replace this when it does:
   https://github.com/gnutls/gnutls/blob/cdc4fc288d87f91f974aa23b6e8595a53970ce00/lib/nettle/pk.c#L495 */
static NTSTATUS CDECL key_compute_secret_ecc(unsigned char *privkey_in, struct key *pubkey_in, struct secret *secret)
{
    const char *pubkey_format;
    DWORD key_size;
    gcry_sexp_t pubkey = NULL;
    gcry_sexp_t privkey = NULL;
    gcry_sexp_t xchg_result = NULL;
    gcry_error_t err;
    NTSTATUS status = STATUS_SUCCESS;

    if (!libgcrypt_handle)
    {
        ERR("Secrets not supported without gcrypt\n");
        return STATUS_INTERNAL_ERROR;
    }

    switch (pubkey_in->alg_id)
    {
        case ALG_ID_ECDH_P256:
            pubkey_format = "NIST P-256";
            key_size = 32;
            break;
        default:
            FIXME("Unsupported algorithm id: %u\n", pubkey_in->alg_id);
            return STATUS_INTERNAL_ERROR;
    }

    /* import public key -
       copy public key into temporary buffer so we can prepend 0x04 (to indicate it is uncompressed) */
    {
        UCHAR *public_key_raw = malloc((key_size * 2) + 1);
        public_key_raw[0] = 0x04;
        memcpy(public_key_raw + 1, pubkey_in->u.a.pubkey + sizeof(BCRYPT_ECCKEY_BLOB), key_size * 2);

        err = pgcry_sexp_build(&pubkey, NULL,
                            "(key-data(public-key(ecdh(curve %s)(q %b))))",
                            pubkey_format,
                            (key_size * 2) + 1,
                            public_key_raw);

        free(public_key_raw);
    }

    if (err)
    {
        ERR("Failed to build gcrypt public key\n");
        goto done;
    }

    /* import private key */
    /* extract private key from blob structure */
    err = pgcry_sexp_build(&privkey, NULL,
                        "(data(flags raw)(value %b))",
                        key_size,
                        privkey_in + sizeof(BCRYPT_ECCKEY_BLOB) + key_size * 2);

    if (err)
    {
        ERR("Failed to build gcrypt private key data\n");
        goto done;
    }

    if ((err = pgcry_pk_encrypt(&xchg_result, privkey, pubkey)))
    {
        ERR("Failed to perform key exchange\n");
        goto done;
    }

    status = extract_result_into_secret(xchg_result, secret);
    if (status)
    {
        ERR("Failed to extract secret key\n");
        goto done;
    }

    if (secret->len != key_size)
    {
        ERR("got secret size %u, expected %u\n", secret->len, key_size);
        status = STATUS_INTERNAL_ERROR;
        goto done;
    }

    done:
    pgcry_sexp_release(pubkey);
    pgcry_sexp_release(privkey);
    pgcry_sexp_release(xchg_result);

    if (status)
    {
        return status;
    }
    if (err)
    {
        ERR("Error = %s/%s\n", pgcry_strsource (err), pgcry_strerror (err));
        return STATUS_INTERNAL_ERROR;
    }
    return STATUS_SUCCESS;
}

static struct key_funcs key_funcs =
{
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    key_compute_secret_ecc
};

struct key_funcs * gcrypt_lib_init( DWORD reason )
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        if (!gcrypt_initialize()) return NULL;
        return &key_funcs;
    case DLL_PROCESS_DETACH:
        gcrypt_uninitialize();
    }
    return NULL;
}

#else
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"

struct key_funcs * gcrypt_lib_init( DWORD reason )
{
    return NULL;
}
#endif

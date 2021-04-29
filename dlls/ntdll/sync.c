/*
 *	Process synchronisation
 *
 * Copyright 1996, 1997, 1998 Marcus Meissner
 * Copyright 1997, 1998, 1999 Alexandre Julliard
 * Copyright 1999, 2000 Juergen Schmied
 * Copyright 2003 Eric Pouech
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

#include <limits.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#define NONAMELESSUNION
#include "windef.h"
#include "winternl.h"
#include "wine/server.h"
#include "wine/debug.h"
#include "ntdll_misc.h"

WINE_DEFAULT_DEBUG_CHANNEL(sync);
WINE_DECLARE_DEBUG_CHANNEL(relay);

static const char *debugstr_timeout( const LARGE_INTEGER *timeout )
{
    if (!timeout) return "(infinite)";
    return wine_dbgstr_longlong( timeout->QuadPart );
}

/******************************************************************
 *              RtlRunOnceInitialize (NTDLL.@)
 */
void WINAPI RtlRunOnceInitialize( RTL_RUN_ONCE *once )
{
    once->Ptr = NULL;
}

/******************************************************************
 *              RtlRunOnceBeginInitialize (NTDLL.@)
 */
DWORD WINAPI RtlRunOnceBeginInitialize( RTL_RUN_ONCE *once, ULONG flags, void **context )
{
    if (flags & RTL_RUN_ONCE_CHECK_ONLY)
    {
        ULONG_PTR val = (ULONG_PTR)once->Ptr;

        if (flags & RTL_RUN_ONCE_ASYNC) return STATUS_INVALID_PARAMETER;
        if ((val & 3) != 2) return STATUS_UNSUCCESSFUL;
        if (context) *context = (void *)(val & ~3);
        return STATUS_SUCCESS;
    }

    for (;;)
    {
        ULONG_PTR next, val = (ULONG_PTR)once->Ptr;

        switch (val & 3)
        {
        case 0:  /* first time */
            if (!InterlockedCompareExchangePointer( &once->Ptr,
                                                    (flags & RTL_RUN_ONCE_ASYNC) ? (void *)3 : (void *)1, 0 ))
                return STATUS_PENDING;
            break;

        case 1:  /* in progress, wait */
            if (flags & RTL_RUN_ONCE_ASYNC) return STATUS_INVALID_PARAMETER;
            next = val & ~3;
            if (InterlockedCompareExchangePointer( &once->Ptr, (void *)((ULONG_PTR)&next | 1),
                                                   (void *)val ) == (void *)val)
                NtWaitForKeyedEvent( 0, &next, FALSE, NULL );
            break;

        case 2:  /* done */
            if (context) *context = (void *)(val & ~3);
            return STATUS_SUCCESS;

        case 3:  /* in progress, async */
            if (!(flags & RTL_RUN_ONCE_ASYNC)) return STATUS_INVALID_PARAMETER;
            return STATUS_PENDING;
        }
    }
}

/******************************************************************
 *              RtlRunOnceComplete (NTDLL.@)
 */
DWORD WINAPI RtlRunOnceComplete( RTL_RUN_ONCE *once, ULONG flags, void *context )
{
    if ((ULONG_PTR)context & 3) return STATUS_INVALID_PARAMETER;

    if (flags & RTL_RUN_ONCE_INIT_FAILED)
    {
        if (context) return STATUS_INVALID_PARAMETER;
        if (flags & RTL_RUN_ONCE_ASYNC) return STATUS_INVALID_PARAMETER;
    }
    else context = (void *)((ULONG_PTR)context | 2);

    for (;;)
    {
        ULONG_PTR val = (ULONG_PTR)once->Ptr;

        switch (val & 3)
        {
        case 1:  /* in progress */
            if (InterlockedCompareExchangePointer( &once->Ptr, context, (void *)val ) != (void *)val) break;
            val &= ~3;
            while (val)
            {
                ULONG_PTR next = *(ULONG_PTR *)val;
                NtReleaseKeyedEvent( 0, (void *)val, FALSE, NULL );
                val = next;
            }
            return STATUS_SUCCESS;

        case 3:  /* in progress, async */
            if (!(flags & RTL_RUN_ONCE_ASYNC)) return STATUS_INVALID_PARAMETER;
            if (InterlockedCompareExchangePointer( &once->Ptr, context, (void *)val ) != (void *)val) break;
            return STATUS_SUCCESS;

        default:
            return STATUS_UNSUCCESSFUL;
        }
    }
}

/******************************************************************
 *              RtlRunOnceExecuteOnce (NTDLL.@)
 */
DWORD WINAPI RtlRunOnceExecuteOnce( RTL_RUN_ONCE *once, PRTL_RUN_ONCE_INIT_FN func,
                                    void *param, void **context )
{
    DWORD ret = RtlRunOnceBeginInitialize( once, 0, context );

    if (ret != STATUS_PENDING) return ret;

    if (!func( once, param, context ))
    {
        RtlRunOnceComplete( once, RTL_RUN_ONCE_INIT_FAILED, NULL );
        return STATUS_UNSUCCESSFUL;
    }

    return RtlRunOnceComplete( once, 0, context ? *context : NULL );
}

struct srw_lock
{
    short exclusive_waiters;

    /* Number of shared owners, or -1 if owned exclusive.
     *
     * Sadly Windows has no equivalent to FUTEX_WAIT_BITSET, so in order to wake
     * up *only* exclusive or *only* shared waiters (and thus avoid spurious
     * wakeups), we need to wait on two different addresses.
     * RtlAcquireSRWLockShared() needs to know the values of "exclusive_waiters"
     * and "owners", but RtlAcquireSRWLockExclusive() only needs to know the
     * value of "owners", so the former can wait on the entire structure, and
     * the latterwaits only on the "owners" member. Note then that "owners" must be
     * not be the first element in the structure. */
    short owners;
};
C_ASSERT( sizeof(struct srw_lock) == 4 );

/***********************************************************************
 *              RtlInitializeSRWLock (NTDLL.@)
 *
 * NOTES
 *  Please note that SRWLocks do not keep track of the owner of a lock.
 *  It doesn't make any difference which thread for example unlocks an
 *  SRWLock (see corresponding tests). This implementation uses two
 *  keyed events (one for the exclusive waiters and one for the shared
 *  waiters) and is limited to 2^15-1 waiting threads.
 */
void WINAPI RtlInitializeSRWLock( RTL_SRWLOCK *lock )
{
    lock->Ptr = NULL;
}

/***********************************************************************
 *              RtlAcquireSRWLockExclusive (NTDLL.@)
 *
 * NOTES
 *  Unlike RtlAcquireResourceExclusive this function doesn't allow
 *  nested calls from the same thread. "Upgrading" a shared access lock
 *  to an exclusive access lock also doesn't seem to be supported.
 */
void WINAPI RtlAcquireSRWLockExclusive( RTL_SRWLOCK *lock )
{
    union { RTL_SRWLOCK *rtl; struct srw_lock *s; LONG *l; } u = { lock };

    InterlockedIncrement16( &u.s->exclusive_waiters );

    for (;;)
    {
        union { struct srw_lock s; LONG l; } old, new;
        BOOL wait;

        do
        {
            old.s = *u.s;
            new.s = old.s;

            if (!old.s.owners)
            {
                /* Not locked exclusive or shared. We can try to grab it. */
                new.s.owners = -1;
                --new.s.exclusive_waiters;
                wait = FALSE;
            }
            else
            {
                wait = TRUE;
            }
        } while (InterlockedCompareExchange( u.l, new.l, old.l ) != old.l);

        if (!wait) return;
        RtlWaitOnAddress( &u.s->owners, &new.s.owners, sizeof(short), NULL );
    }
}

/***********************************************************************
 *              RtlAcquireSRWLockShared (NTDLL.@)
 *
 * NOTES
 *   Do not call this function recursively - it will only succeed when
 *   there are no threads waiting for an exclusive lock!
 */
void WINAPI RtlAcquireSRWLockShared( RTL_SRWLOCK *lock )
{
    union { RTL_SRWLOCK *rtl; struct srw_lock *s; LONG *l; } u = { lock };

    for (;;)
    {
        union { struct srw_lock s; LONG l; } old, new;
        BOOL wait;

        do
        {
            old.s = *u.s;
            new = old;

            if (old.s.owners != -1 && !old.s.exclusive_waiters)
            {
                /* Not locked exclusive, and no exclusive waiters.
                 * We can try to grab it. */
                ++new.s.owners;
                wait = FALSE;
            }
            else
            {
                wait = TRUE;
            }
        } while (InterlockedCompareExchange( u.l, new.l, old.l ) != old.l);

        if (!wait) return;
        RtlWaitOnAddress( u.s, &new.s, sizeof(struct srw_lock), NULL );
    }
}

/***********************************************************************
 *              RtlReleaseSRWLockExclusive (NTDLL.@)
 */
void WINAPI RtlReleaseSRWLockExclusive( RTL_SRWLOCK *lock )
{
    union { RTL_SRWLOCK *rtl; struct srw_lock *s; LONG *l; } u = { lock };
    union { struct srw_lock s; LONG l; } old, new;

    do
    {
        old.s = *u.s;
        new = old;

        if (old.s.owners != -1) ERR("Lock %p is not owned exclusive!\n", lock);

        new.s.owners = 0;
    } while (InterlockedCompareExchange( u.l, new.l, old.l ) != old.l);

    if (new.s.exclusive_waiters)
        RtlWakeAddressSingle( &u.s->owners );
    else
        RtlWakeAddressAll( u.s );
}

/***********************************************************************
 *              RtlReleaseSRWLockShared (NTDLL.@)
 */
void WINAPI RtlReleaseSRWLockShared( RTL_SRWLOCK *lock )
{
    union { RTL_SRWLOCK *rtl; struct srw_lock *s; LONG *l; } u = { lock };
    union { struct srw_lock s; LONG l; } old, new;

    do
    {
        old.s = *u.s;
        new = old;

        if (old.s.owners == -1) ERR("Lock %p is owned exclusive!\n", lock);
        else if (!old.s.owners) ERR("Lock %p is not owned shared!\n", lock);

        --new.s.owners;
    } while (InterlockedCompareExchange( u.l, new.l, old.l ) != old.l);

    if (!new.s.owners)
        RtlWakeAddressSingle( &u.s->owners );
}

/***********************************************************************
 *              RtlTryAcquireSRWLockExclusive (NTDLL.@)
 *
 * NOTES
 *  Similarly to AcquireSRWLockExclusive, recursive calls are not allowed
 *  and will fail with a FALSE return value.
 */
BOOLEAN WINAPI RtlTryAcquireSRWLockExclusive( RTL_SRWLOCK *lock )
{
    union { RTL_SRWLOCK *rtl; struct srw_lock *s; LONG *l; } u = { lock };
    union { struct srw_lock s; LONG l; } old, new;
    BOOLEAN ret;

    do
    {
        old.s = *u.s;
        new.s = old.s;

        if (!old.s.owners)
        {
            /* Not locked exclusive or shared. We can try to grab it. */
            new.s.owners = -1;
            ret = TRUE;
        }
        else
        {
            ret = FALSE;
        }
    } while (InterlockedCompareExchange( u.l, new.l, old.l ) != old.l);

    return ret;
}

/***********************************************************************
 *              RtlTryAcquireSRWLockShared (NTDLL.@)
 */
BOOLEAN WINAPI RtlTryAcquireSRWLockShared( RTL_SRWLOCK *lock )
{
    union { RTL_SRWLOCK *rtl; struct srw_lock *s; LONG *l; } u = { lock };
    union { struct srw_lock s; LONG l; } old, new;
    BOOLEAN ret;

    do
    {
        old.s = *u.s;
        new.s = old.s;

        if (old.s.owners != -1 && !old.s.exclusive_waiters)
        {
            /* Not locked exclusive, and no exclusive waiters.
             * We can try to grab it. */
            ++new.s.owners;
            ret = TRUE;
        }
        else
        {
            ret = FALSE;
        }
    } while (InterlockedCompareExchange( u.l, new.l, old.l ) != old.l);

    return ret;
}

/***********************************************************************
 *           RtlInitializeConditionVariable   (NTDLL.@)
 *
 * Initializes the condition variable with NULL.
 *
 * PARAMS
 *  variable [O] condition variable
 *
 * RETURNS
 *  Nothing.
 */
void WINAPI RtlInitializeConditionVariable( RTL_CONDITION_VARIABLE *variable )
{
    variable->Ptr = NULL;
}

/***********************************************************************
 *           RtlWakeConditionVariable   (NTDLL.@)
 *
 * Wakes up one thread waiting on the condition variable.
 *
 * PARAMS
 *  variable [I/O] condition variable to wake up.
 *
 * RETURNS
 *  Nothing.
 *
 * NOTES
 *  The calling thread does not have to own any lock in order to call
 *  this function.
 */
void WINAPI RtlWakeConditionVariable( RTL_CONDITION_VARIABLE *variable )
{
    InterlockedIncrement( (int *)&variable->Ptr );
    RtlWakeAddressSingle( variable );
}

/***********************************************************************
 *           RtlWakeAllConditionVariable   (NTDLL.@)
 *
 * See WakeConditionVariable, wakes up all waiting threads.
 */
void WINAPI RtlWakeAllConditionVariable( RTL_CONDITION_VARIABLE *variable )
{
    InterlockedIncrement( (int *)&variable->Ptr );
    RtlWakeAddressAll( variable );
}

/***********************************************************************
 *           RtlSleepConditionVariableCS   (NTDLL.@)
 *
 * Atomically releases the critical section and suspends the thread,
 * waiting for a Wake(All)ConditionVariable event. Afterwards it enters
 * the critical section again and returns.
 *
 * PARAMS
 *  variable  [I/O] condition variable
 *  crit      [I/O] critical section to leave temporarily
 *  timeout   [I]   timeout
 *
 * RETURNS
 *  see NtWaitForKeyedEvent for all possible return values.
 */
NTSTATUS WINAPI RtlSleepConditionVariableCS( RTL_CONDITION_VARIABLE *variable, RTL_CRITICAL_SECTION *crit,
                                             const LARGE_INTEGER *timeout )
{
    int value = *(int *)&variable->Ptr;
    NTSTATUS status;

    RtlLeaveCriticalSection( crit );
    status = RtlWaitOnAddress( &variable->Ptr, &value, sizeof(value), timeout );
    RtlEnterCriticalSection( crit );
    return status;
}

/***********************************************************************
 *           RtlSleepConditionVariableSRW   (NTDLL.@)
 *
 * Atomically releases the SRWLock and suspends the thread,
 * waiting for a Wake(All)ConditionVariable event. Afterwards it enters
 * the SRWLock again with the same access rights and returns.
 *
 * PARAMS
 *  variable  [I/O] condition variable
 *  lock      [I/O] SRWLock to leave temporarily
 *  timeout   [I]   timeout
 *  flags     [I]   type of the current lock (exclusive / shared)
 *
 * RETURNS
 *  see NtWaitForKeyedEvent for all possible return values.
 *
 * NOTES
 *  the behaviour is undefined if the thread doesn't own the lock.
 */
NTSTATUS WINAPI RtlSleepConditionVariableSRW( RTL_CONDITION_VARIABLE *variable, RTL_SRWLOCK *lock,
                                              const LARGE_INTEGER *timeout, ULONG flags )
{
    int value = *(int *)&variable->Ptr;
    NTSTATUS status;

    if (flags & RTL_CONDITION_VARIABLE_LOCKMODE_SHARED)
        RtlReleaseSRWLockShared( lock );
    else
        RtlReleaseSRWLockExclusive( lock );

    status = RtlWaitOnAddress( &variable->Ptr, &value, sizeof(value), timeout );

    if (flags & RTL_CONDITION_VARIABLE_LOCKMODE_SHARED)
        RtlAcquireSRWLockShared( lock );
    else
        RtlAcquireSRWLockExclusive( lock );
    return status;
}

/* The following functions define a lock-free array mapping thread IDs to
 * values, which can be grown but not shrunk. We do this by allocating one slice
 * of the array at a time, and storing a pointer to the next slice at the end.
 *
 * This is both for efficiency (we want this function to be as fast as possible)
 * and because locking the TEB list is hard otherwise—we need to safely access
 * the TEB list, but cannot do so using any of these synchronization primitives,
 * and we may need to access the TEB list before being inserted into it (e.g.
 * from heap locks, or the TEB list lock itself.)
 */

struct addr_wait_entry
{
    void *addr;
    HANDLE tid;
};

struct addr_wait_array
{
    struct addr_wait_entry entries[(0x1000 - sizeof(struct addr_wait_entry *)) / sizeof(struct addr_wait_entry)];
    struct addr_wait_array *next;
};

static struct addr_wait_array first_addr_wait_array;

static struct addr_wait_entry *addr_wait_allocate_entry( HANDLE tid )
{
    struct addr_wait_array *array = &first_addr_wait_array;

    for (;;)
    {
        struct addr_wait_array *new_array = NULL;
        SIZE_T size = sizeof(*new_array);
        unsigned int i;

        for (;;)
        {
            for (i = 0; i < ARRAY_SIZE(array->entries); ++i)
            {
                if (!array->entries[i].tid && !InterlockedCompareExchangePointer( &array->entries[i].tid, tid, NULL ))
                    return &array->entries[i];
            }

            if (!array->next) break;
            array = array->next;
        }

        if (NtAllocateVirtualMemory( NtCurrentProcess(), (void **)&new_array, 0, &size, MEM_COMMIT, PAGE_READWRITE ))
            return NULL;

        if (InterlockedCompareExchangePointer( (void **)&array->next, new_array, NULL ))
        {
            /* another thread beat us to it */
            NtFreeVirtualMemory( NtCurrentProcess(), (void **)&new_array, &size, MEM_RELEASE );
        }
        /* start searching again from the new array */
        array = array->next;
    }
}

void addr_wait_free_entry(void)
{
    struct addr_wait_entry *entry = NtCurrentTeb()->ReservedForPerf;
    if (entry)
        InterlockedExchangePointer( &entry->tid, NULL );
}

static BOOL compare_addr( const void *addr, const void *cmp, SIZE_T size )
{
    switch (size)
    {
        case 1:
            return (*(const UCHAR *)addr == *(const UCHAR *)cmp);
        case 2:
            return (*(const USHORT *)addr == *(const USHORT *)cmp);
        case 4:
            return (*(const ULONG *)addr == *(const ULONG *)cmp);
        case 8:
            return (*(const ULONG64 *)addr == *(const ULONG64 *)cmp);
    }

    return FALSE;
}

/***********************************************************************
 *           RtlWaitOnAddress   (NTDLL.@)
 */
NTSTATUS WINAPI RtlWaitOnAddress( const void *addr, const void *cmp, SIZE_T size,
                                  const LARGE_INTEGER *timeout )
{
    struct addr_wait_entry *entry = NtCurrentTeb()->ReservedForPerf;
    NTSTATUS ret;

    TRACE("addr %p cmp %p size %#Ix timeout %s\n", addr, cmp, size, debugstr_timeout( timeout ));

    if (size != 1 && size != 2 && size != 4 && size != 8)
        return STATUS_INVALID_PARAMETER;

    if (!entry)
    {
        if (!(entry = addr_wait_allocate_entry( NtCurrentTeb()->ClientId.UniqueThread )))
            return STATUS_NO_MEMORY;
        NtCurrentTeb()->ReservedForPerf = entry;
    }

    InterlockedExchangePointer( &entry->addr, (void *)addr );

    /* Ensure that the compare-and-swap above is ordered before the comparison
     * below. This barrier is paired with another in RtlWakeByAddress*().
     *
     * In more detail, given the following sequence:
     *
     * Thread A                                 Thread B
     * -----------------------------------------------------------------
     * RtlWaitOnAddress( &val );                val = 1;
     * queue thread                             RtlWakeByAddress( &val );
     * MemoryBarrier(); <---- paired with ----> MemoryBarrier();
     * compare_addr( &val );                    if (thread is queued)
     *
     * We must ensure that the thread is queued [through the above
     * InterlockedExchangePointer()] before reading "val", and that writes to
     * "val" by the application happen before we check for queued threads.
     * Otherwise, thread A can deadlock: "val" may appear unchanged, while
     * thread B observed that thread A was not queued.
     */
    MemoryBarrier();

    if (!compare_addr( addr, cmp, size ))
    {
        InterlockedExchangePointer( &entry->addr, NULL );
        return STATUS_SUCCESS;
    }

    ret = NtWaitForAlertByThreadId( NULL, timeout );
    InterlockedExchangePointer( &entry->addr, NULL );
    if (ret == STATUS_ALERTED) ret = STATUS_SUCCESS;
    return ret;
}

/***********************************************************************
 *           RtlWakeAddressAll    (NTDLL.@)
 */
void WINAPI RtlWakeAddressAll( const void *addr )
{
    struct addr_wait_array *array;
    unsigned int i;

    TRACE("%p\n", addr);

    if (!addr) return;

    /* Ensure that memory stores to "addr" are ordered before reading the
     * array below. Paired with another barrier in RtlWaitOnAddress() [q.v.].
     */
    MemoryBarrier();

    for (array = &first_addr_wait_array; array != NULL; array = array->next)
    {
        for (i = 0; i < ARRAY_SIZE(array->entries); ++i)
        {
            if (array->entries[i].addr == addr)
                NtAlertThreadByThreadId( array->entries[i].tid );
        }
    }
}

/***********************************************************************
 *           RtlWakeAddressSingle (NTDLL.@)
 */
void WINAPI RtlWakeAddressSingle( const void *addr )
{
    struct addr_wait_array *array;
    unsigned int i;

    TRACE("%p\n", addr);

    if (!addr) return;

    /* Ensure that memory stores to "addr" are ordered before reading the
     * array below. Paired with another barrier in RtlWaitOnAddress() [q.v.].
     */
    MemoryBarrier();

    for (array = &first_addr_wait_array; array != NULL; array = array->next)
    {
        for (i = 0; i < ARRAY_SIZE(array->entries); ++i)
        {
            if (array->entries[i].addr == addr
                    && InterlockedCompareExchangePointer( &array->entries[i].addr, NULL, (void *)addr ) == addr)
            {
                NtAlertThreadByThreadId( array->entries[i].tid );
                return;
            }
        }
    }
}


/***********************************************************************
 * Critical sections
 ***********************************************************************/


static void *no_debug_info_marker = (void *)(ULONG_PTR)-1;

static BOOL crit_section_has_debuginfo( const RTL_CRITICAL_SECTION *crit )
{
    return crit->DebugInfo != NULL && crit->DebugInfo != no_debug_info_marker;
}

static inline HANDLE get_semaphore( RTL_CRITICAL_SECTION *crit )
{
    HANDLE ret = crit->LockSemaphore;
    if (!ret)
    {
        HANDLE sem;
        if (NtCreateSemaphore( &sem, SEMAPHORE_ALL_ACCESS, NULL, 0, 1 )) return 0;
        if (!(ret = InterlockedCompareExchangePointer( &crit->LockSemaphore, sem, 0 )))
            ret = sem;
        else
            NtClose(sem);  /* somebody beat us to it */
    }
    return ret;
}

static inline NTSTATUS wait_semaphore( RTL_CRITICAL_SECTION *crit, int timeout )
{
    LARGE_INTEGER time = {.QuadPart = timeout * (LONGLONG)-10000000};

    /* debug info is cleared by MakeCriticalSectionGlobal */
    if (!crit_section_has_debuginfo( crit ))
    {
        HANDLE sem = get_semaphore( crit );
        return NtWaitForSingleObject( sem, FALSE, &time );
    }
    else
    {
        int *lock = (int *)&crit->LockSemaphore;
        while (!InterlockedCompareExchange( lock, 0, 1 ))
        {
            static const int zero;
            /* this may wait longer than specified in case of multiple wake-ups */
            if (RtlWaitOnAddress( (int *)&crit->LockSemaphore, &zero, sizeof(int), &time ) == STATUS_TIMEOUT)
                return STATUS_TIMEOUT;
        }
        return STATUS_WAIT_0;
    }
}

/******************************************************************************
 *      RtlInitializeCriticalSection   (NTDLL.@)
 */
NTSTATUS WINAPI RtlInitializeCriticalSection( RTL_CRITICAL_SECTION *crit )
{
    return RtlInitializeCriticalSectionEx( crit, 0, 0 );
}


/******************************************************************************
 *      RtlInitializeCriticalSectionAndSpinCount   (NTDLL.@)
 */
NTSTATUS WINAPI RtlInitializeCriticalSectionAndSpinCount( RTL_CRITICAL_SECTION *crit, ULONG spincount )
{
    return RtlInitializeCriticalSectionEx( crit, spincount, 0 );
}


/******************************************************************************
 *      RtlInitializeCriticalSectionEx   (NTDLL.@)
 */
NTSTATUS WINAPI RtlInitializeCriticalSectionEx( RTL_CRITICAL_SECTION *crit, ULONG spincount, ULONG flags )
{
    if (flags & (RTL_CRITICAL_SECTION_FLAG_DYNAMIC_SPIN|RTL_CRITICAL_SECTION_FLAG_STATIC_INIT))
        FIXME("(%p,%u,0x%08x) semi-stub\n", crit, spincount, flags);

    /* FIXME: if RTL_CRITICAL_SECTION_FLAG_STATIC_INIT is given, we should use
     * memory from a static pool to hold the debug info. Then heap.c could pass
     * this flag rather than initialising the process heap CS by hand. If this
     * is done, then debug info should be managed through Rtlp[Allocate|Free]DebugInfo
     * so (e.g.) MakeCriticalSectionGlobal() doesn't free it using HeapFree().
     */
    if (flags & RTL_CRITICAL_SECTION_FLAG_NO_DEBUG_INFO)
        crit->DebugInfo = no_debug_info_marker;
    else
    {
        crit->DebugInfo = RtlAllocateHeap( GetProcessHeap(), 0, sizeof(RTL_CRITICAL_SECTION_DEBUG ));
        if (crit->DebugInfo)
        {
            crit->DebugInfo->Type = 0;
            crit->DebugInfo->CreatorBackTraceIndex = 0;
            crit->DebugInfo->CriticalSection = crit;
            crit->DebugInfo->ProcessLocksList.Blink = &crit->DebugInfo->ProcessLocksList;
            crit->DebugInfo->ProcessLocksList.Flink = &crit->DebugInfo->ProcessLocksList;
            crit->DebugInfo->EntryCount = 0;
            crit->DebugInfo->ContentionCount = 0;
            memset( crit->DebugInfo->Spare, 0, sizeof(crit->DebugInfo->Spare) );
        }
    }
    crit->LockCount      = -1;
    crit->RecursionCount = 0;
    crit->OwningThread   = 0;
    crit->LockSemaphore  = 0;
    if (NtCurrentTeb()->Peb->NumberOfProcessors <= 1) spincount = 0;
    crit->SpinCount = spincount & ~0x80000000;
    return STATUS_SUCCESS;
}


/******************************************************************************
 *      RtlSetCriticalSectionSpinCount   (NTDLL.@)
 */
ULONG WINAPI RtlSetCriticalSectionSpinCount( RTL_CRITICAL_SECTION *crit, ULONG spincount )
{
    ULONG oldspincount = crit->SpinCount;
    if (NtCurrentTeb()->Peb->NumberOfProcessors <= 1) spincount = 0;
    crit->SpinCount = spincount;
    return oldspincount;
}


/******************************************************************************
 *      RtlDeleteCriticalSection   (NTDLL.@)
 */
NTSTATUS WINAPI RtlDeleteCriticalSection( RTL_CRITICAL_SECTION *crit )
{
    crit->LockCount      = -1;
    crit->RecursionCount = 0;
    crit->OwningThread   = 0;
    if (crit_section_has_debuginfo( crit ))
    {
        /* only free the ones we made in here */
        if (!crit->DebugInfo->Spare[0])
        {
            RtlFreeHeap( GetProcessHeap(), 0, crit->DebugInfo );
            crit->DebugInfo = NULL;
        }
    }
    else NtClose( crit->LockSemaphore );
    crit->LockSemaphore = 0;
    return STATUS_SUCCESS;
}


/******************************************************************************
 *      RtlpWaitForCriticalSection   (NTDLL.@)
 */
NTSTATUS WINAPI RtlpWaitForCriticalSection( RTL_CRITICAL_SECTION *crit )
{
    LONGLONG timeout = NtCurrentTeb()->Peb->CriticalSectionTimeout.QuadPart / -10000000;

    /* Don't allow blocking on a critical section during process termination */
    if (RtlDllShutdownInProgress())
    {
        WARN( "process %s is shutting down, returning STATUS_SUCCESS\n",
              debugstr_w(NtCurrentTeb()->Peb->ProcessParameters->ImagePathName.Buffer) );
        return STATUS_SUCCESS;
    }

    for (;;)
    {
        EXCEPTION_RECORD rec;
        NTSTATUS status = wait_semaphore( crit, 5 );
        timeout -= 5;

        if ( status == STATUS_TIMEOUT )
        {
            const char *name = NULL;
            if (crit_section_has_debuginfo( crit )) name = (char *)crit->DebugInfo->Spare[0];
            if (!name) name = "?";
            ERR( "section %p %s wait timed out in thread %04x, blocked by %04x, retrying (60 sec)\n",
                 crit, debugstr_a(name), GetCurrentThreadId(), HandleToULong(crit->OwningThread) );
            status = wait_semaphore( crit, 60 );
            timeout -= 60;

            if ( status == STATUS_TIMEOUT && TRACE_ON(relay) )
            {
                ERR( "section %p %s wait timed out in thread %04x, blocked by %04x, retrying (5 min)\n",
                     crit, debugstr_a(name), GetCurrentThreadId(), HandleToULong(crit->OwningThread) );
                status = wait_semaphore( crit, 300 );
                timeout -= 300;
            }
        }
        if (status == STATUS_WAIT_0) break;

        /* Throw exception only for Wine internal locks */
        if (!crit_section_has_debuginfo( crit ) || !crit->DebugInfo->Spare[0]) continue;

        /* only throw deadlock exception if configured timeout is reached */
        if (timeout > 0) continue;

        rec.ExceptionCode    = STATUS_POSSIBLE_DEADLOCK;
        rec.ExceptionFlags   = 0;
        rec.ExceptionRecord  = NULL;
        rec.ExceptionAddress = RtlRaiseException;  /* sic */
        rec.NumberParameters = 1;
        rec.ExceptionInformation[0] = (ULONG_PTR)crit;
        RtlRaiseException( &rec );
    }
    if (crit_section_has_debuginfo( crit )) crit->DebugInfo->ContentionCount++;
    return STATUS_SUCCESS;
}


/******************************************************************************
 *      RtlpUnWaitCriticalSection   (NTDLL.@)
 */
NTSTATUS WINAPI RtlpUnWaitCriticalSection( RTL_CRITICAL_SECTION *crit )
{
    NTSTATUS ret;

    /* debug info is cleared by MakeCriticalSectionGlobal */
    if (!crit_section_has_debuginfo( crit ))
    {
        HANDLE sem = get_semaphore( crit );
        ret = NtReleaseSemaphore( sem, 1, NULL );
    }
    else
    {
        int *lock = (int *)&crit->LockSemaphore;
        *lock = 1;
        RtlWakeAddressSingle( lock );
        ret = STATUS_SUCCESS;
    }
    if (ret) RtlRaiseStatus( ret );
    return ret;
}


static inline void small_pause(void)
{
#ifdef __i386__
    __asm__ __volatile__( "rep;nop" : : : "memory" );
#else
    __asm__ __volatile__( "" : : : "memory" );
#endif
}

/******************************************************************************
 *      RtlEnterCriticalSection   (NTDLL.@)
 */
NTSTATUS WINAPI RtlEnterCriticalSection( RTL_CRITICAL_SECTION *crit )
{
    if (crit->SpinCount)
    {
        ULONG count;

        if (RtlTryEnterCriticalSection( crit )) return STATUS_SUCCESS;
        for (count = crit->SpinCount; count > 0; count--)
        {
            if (crit->LockCount > 0) break;  /* more than one waiter, don't bother spinning */
            if (crit->LockCount == -1)       /* try again */
            {
                if (InterlockedCompareExchange( &crit->LockCount, 0, -1 ) == -1) goto done;
            }
            small_pause();
        }
    }

    if (InterlockedIncrement( &crit->LockCount ))
    {
        if (crit->OwningThread == ULongToHandle(GetCurrentThreadId()))
        {
            crit->RecursionCount++;
            return STATUS_SUCCESS;
        }

        /* Now wait for it */
        RtlpWaitForCriticalSection( crit );
    }
done:
    crit->OwningThread   = ULongToHandle(GetCurrentThreadId());
    crit->RecursionCount = 1;
    return STATUS_SUCCESS;
}


/******************************************************************************
 *      RtlTryEnterCriticalSection   (NTDLL.@)
 */
BOOL WINAPI RtlTryEnterCriticalSection( RTL_CRITICAL_SECTION *crit )
{
    BOOL ret = FALSE;
    if (InterlockedCompareExchange( &crit->LockCount, 0, -1 ) == -1)
    {
        crit->OwningThread   = ULongToHandle(GetCurrentThreadId());
        crit->RecursionCount = 1;
        ret = TRUE;
    }
    else if (crit->OwningThread == ULongToHandle(GetCurrentThreadId()))
    {
        InterlockedIncrement( &crit->LockCount );
        crit->RecursionCount++;
        ret = TRUE;
    }
    return ret;
}


/******************************************************************************
 *      RtlIsCriticalSectionLocked   (NTDLL.@)
 */
BOOL WINAPI RtlIsCriticalSectionLocked( RTL_CRITICAL_SECTION *crit )
{
    return crit->RecursionCount != 0;
}


/******************************************************************************
 *      RtlIsCriticalSectionLockedByThread   (NTDLL.@)
 */
BOOL WINAPI RtlIsCriticalSectionLockedByThread( RTL_CRITICAL_SECTION *crit )
{
    return crit->OwningThread == ULongToHandle(GetCurrentThreadId()) &&
           crit->RecursionCount;
}


/******************************************************************************
 *      RtlLeaveCriticalSection   (NTDLL.@)
 */
NTSTATUS WINAPI RtlLeaveCriticalSection( RTL_CRITICAL_SECTION *crit )
{
    if (--crit->RecursionCount)
    {
        if (crit->RecursionCount > 0) InterlockedDecrement( &crit->LockCount );
        else ERR( "section %p is not acquired\n", crit );
    }
    else
    {
        crit->OwningThread = 0;
        if (InterlockedDecrement( &crit->LockCount ) >= 0)
        {
            /* someone is waiting */
            RtlpUnWaitCriticalSection( crit );
        }
    }
    return STATUS_SUCCESS;
}

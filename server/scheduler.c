/*
 * Scheduler priority management
 *
 * Copyright (C) 2015 Joakim Hernberg
 * Copyright (C) 2015 Sebastian Lackner
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

#define _GNU_SOURCE  /* for SCHED_BATCH, SCHED_IDLE */
#include <assert.h>
#include <stdio.h>
#include <stdarg.h>
#ifdef HAVE_SYS_RESOURCE_H
# include <sys/resource.h>
#endif
#ifdef HAVE_SCHED_H
# include <sched.h>
#endif
#ifndef SCHED_RESET_ON_FORK
# define SCHED_RESET_ON_FORK 0x40000000
#endif
#ifndef SCHED_IDLE
 #define SCHED_IDLE 5
#endif

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "winternl.h"
#include "thread.h"

#if defined(__linux__) && defined(HAVE_SYS_RESOURCE_H) && defined(HAVE_SCHED_H)

static int thread_base_priority = -1;

/* gets the priority value from an environment variable */
static int get_priority( const char *variable, int min, int max )
{
    const char *env;
    int val;

    env = getenv( variable );
    if (!env) return -1;

    val = atoi( env );
    if (val >= min && val <= max) return val;
    fprintf( stderr, "wineserver: %s should be between %d and %d\n", variable, min, max );
    return -1;
}

/* initializes the scheduler */
void init_scheduler( void )
{
    int min, max, priority;

    min = sched_get_priority_min( SCHED_FIFO );
    max = sched_get_priority_max( SCHED_FIFO );
    if (min == -1 || max == -1)
        return;

    /* change the wineserver priority */
    if ((priority = get_priority( "STAGING_RT_PRIORITY_SERVER", min, max )) != -1)
    {
        struct sched_param param;
        memset( &param, 0, sizeof(param) );
        param.sched_priority = priority;
        if (sched_setscheduler( 0, SCHED_FIFO | SCHED_RESET_ON_FORK, &param ) == -1 &&
            sched_setscheduler( 0, SCHED_FIFO, &param ) == -1)
        {
            fprintf( stderr, "wineserver: failed to change priority to SCHED_FIFO/%d\n",
                     param.sched_priority );
            /* do not bother to check the rest */
            return;
        }

        if (debug_level) fprintf( stderr, "wineserver: changed priority to SCHED_FIFO/%d\n",
                                  param.sched_priority );
    }

    /* determine base priority which will be used for all threads */
    if ((priority = get_priority( "STAGING_RT_PRIORITY_BASE", min, max - 4 )) != -1)
    {
        thread_base_priority = priority;

        if (debug_level) fprintf( stderr, "wineserver: initialized thread base priority to %d\n",
                                  thread_base_priority );
    }
}

/* sets the scheduler priority of a windows thread */
void set_scheduler_priority( struct thread *thread )
{
    struct sched_param param;
    int policy;

    if (thread_base_priority == -1) return;
    if (thread->unix_tid == -1) return;

    memset( &param, 0, sizeof(param) );
    if (thread->priority >= THREAD_PRIORITY_TIME_CRITICAL)
    {
        policy = SCHED_FIFO;
        param.sched_priority = thread_base_priority + 4;
    }
    else if (thread->priority >= THREAD_PRIORITY_HIGHEST)
    {
        policy = SCHED_FIFO;
        param.sched_priority = thread_base_priority + 2;
    }
    else if (thread->priority >= THREAD_PRIORITY_ABOVE_NORMAL)
    {
        policy = SCHED_FIFO;
        param.sched_priority = thread_base_priority;
    }
    else if (thread->priority >= THREAD_PRIORITY_NORMAL)
    {
        policy = SCHED_OTHER;
    }
    else if (thread->priority >= THREAD_PRIORITY_LOWEST)
    {
        policy = SCHED_BATCH;
    }
    else
    {
        policy = SCHED_IDLE;
    }

    if (sched_setscheduler(thread->unix_tid, policy | SCHED_RESET_ON_FORK, &param) == -1 &&
        sched_setscheduler(thread->unix_tid, policy, &param) == -1)
    {
        static int once;
        if (debug_level || !once++)
            fprintf( stderr, "%04x: failed to change priority to %d/%d\n",
                     thread->id, policy, param.sched_priority );
        return;
    }

    if (debug_level) fprintf( stderr, "%04x: changed priority to %d/%d\n",
                              thread->id, policy, param.sched_priority );
}

#else

void init_scheduler( void )
{
}

void set_scheduler_priority( struct thread *thread )
{
}

#endif

/* -*- Mode: C++; c-basic-offset: 4 -*- */
/* 
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is the Netscape Portable Runtime (NSPR).
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are 
 * Copyright (C) 1998-2000 Netscape Communications Corporation.  All
 * Rights Reserved.
 * 
 * Contributor(s):
 * 
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 or later (the
 * "GPL"), in which case the provisions of the GPL are applicable 
 * instead of those above.  If you wish to allow use of your 
 * version of this file only under the terms of the GPL and not to
 * allow others to use your version of this file under the MPL,
 * indicate your decision by deleting the provisions above and
 * replace them with the notice and other provisions required by
 * the GPL.  If you do not delete the provisions above, a recipient
 * may use your version of this file under either the MPL or the
 * GPL.
 */

#include "primpl.h"

#include <stdlib.h>

PRLock *_connectLock = NULL;
PRUint32 connectCount = 0;
ConnectListNode connectList[64];
                                   
void
_MD_cleanup_before_exit (void)
{
}

void
_MD_exit (PRIntn status)
{
    exit(status);
}

void
_MD_early_init (void)
{
}

static PRLock *monitor = NULL;

void
_MD_final_init (void)
{
    _connectLock = PR_NewLock();
    PR_ASSERT(NULL != _connectLock); 
    connectCount = 0;
}

void
_MD_AtomicInit (void)
{
    if (monitor == NULL) {
        monitor = PR_NewLock();
    }
}

/*
** This is exceedingly messy.  atomic_add returns the last value, NSPR expects the new value.
** We just add or subtract 1 from the result.  The actual memory update is atomic.
*/

PRInt32
_MD_AtomicAdd( PRInt32 *ptr, PRInt32 val )
{
    return( ( atomic_add( (long *)ptr, val ) ) + val );
}

PRInt32
_MD_AtomicIncrement( PRInt32 *val )
{
    return( ( atomic_add( (long *)val, 1 ) ) + 1 );
}

PRInt32
_MD_AtomicDecrement( PRInt32 *val )
{
    return( ( atomic_add( (long *)val, -1 ) ) - 1 );
}

PRInt32
_MD_AtomicSet( PRInt32 *val, PRInt32 newval )
{
    PRInt32 rv;

    if (!_pr_initialized) {
        _PR_ImplicitInitialization();
    }
    PR_Lock(monitor);
    rv = *val;
    *val = newval;
    PR_Unlock(monitor);
    return rv;
}

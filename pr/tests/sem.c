/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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

/***********************************************************************
**
** Name: sem.c
**
** Description: Tests Semaphonre functions.
**
** Modification History:
** 20-May-97 AGarcia- Converted the test to accomodate the debug_mode flag.
**	         The debug mode will print all of the printfs associated with this test.
**			 The regress mode will be the default mode. Since the regress tool limits
**           the output to a one line status:PASS or FAIL,all of the printf statements
**			 have been handled with an if (debug_mode) statement.
** 04-June-97 AGarcia removed the Test_Result function. Regress tool has been updated to
**			recognize the return code from tha main program.
***********************************************************************/

/***********************************************************************
** Includes
***********************************************************************/
/* Used to get the command line option */
#include "plgetopt.h"

#include "nspr.h"
#include "prpriv.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

PRIntn failed_already=0;
PRIntn debug_mode;

/* 
	Since we don't have stdin, stdout everywhere, we will fake 
	it with our in-memory buffers called stdin and stdout.
*/

#define SBSIZE 1024

#ifdef XP_MAC
#include "prlog.h"
#include "prsem.h"
#define printf PR_LogPrint
extern void SetupMacPrintfLog(char *logFile);
#else
#include "obsolete/prsem.h"
#endif

static char stdinBuf[SBSIZE];
static char stdoutBuf[SBSIZE];

static PRUintn stdinBufIdx = 0;
static PRUintn stdoutBufIdx = 0;
static PRStatus finalResult = PR_SUCCESS;


static size_t dread (PRUintn device, char *buf, size_t bufSize)
{
	PRUintn	i;
	
	/* during first read call, initialize the stdinBuf buffer*/
	if (stdinBufIdx == 0) {
		for (i=0; i<SBSIZE; i++)
			stdinBuf[i] = i;
	}

	/* now copy data from stdinBuf to the given buffer upto bufSize */
	for (i=0; i<bufSize; i++) {
		if (stdinBufIdx == SBSIZE)
			break;
		buf[i] = stdinBuf[stdinBufIdx++];
	}

	return i;
}

static size_t dwrite (PRUintn device, char *buf, size_t bufSize)
{
	PRUintn	i, j;
	
	/* copy data from the given buffer upto bufSize to stdoutBuf */
	for (i=0; i<bufSize; i++) {
		if (stdoutBufIdx == SBSIZE)
			break;
		stdoutBuf[stdoutBufIdx++] = buf[i];
	}

	/* during last write call, compare the two buffers */
	if (stdoutBufIdx == SBSIZE)
		for (j=0; j<SBSIZE; j++)
			if (stdinBuf[j] != stdoutBuf[j]) {
				if (debug_mode) printf("data mismatch for index= %d \n", j);
				finalResult = PR_FAILURE;
			}

	return i;
}

/*------------------ Following is the real test program ---------*/
/*
	Program to copy standard input to standard output.  The program
	uses two threads.  One reads the input and puts the data in a 
	double buffer.  The other reads the buffer contents and writes 
	it to standard output.
*/

PRSemaphore	*emptyBufs;	/* number of empty buffers */
PRSemaphore *fullBufs;	/* number of buffers that are full */

#define BSIZE	100

struct {
	char data[BSIZE];
	PRUintn nbytes;		/* number of bytes in this buffer */
} buf[2];

static void PR_CALLBACK reader(void *arg)
{
	PRUintn	i = 0;
	size_t	nbytes;
	
	do {
		(void) PR_WaitSem(emptyBufs);
		nbytes = dread(0, buf[i].data, BSIZE);
		buf[i].nbytes = nbytes;
		PR_PostSem(fullBufs);
		i = (i + 1) % 2;
	} while (nbytes > 0);
}

static void writer(void)
{
	PRUintn	i = 0;
	size_t	nbytes;
	
	do {
		(void) PR_WaitSem(fullBufs);
		nbytes = buf[i].nbytes;
		if (nbytes > 0) {
			nbytes = dwrite(1, buf[i].data, nbytes);
			PR_PostSem(emptyBufs);
			i = (i + 1) % 2;
		}
	} while (nbytes > 0);
}

int main(int argc, char **argv)
{
	PRThread *r;

    PR_STDIO_INIT();
    PR_Init(PR_USER_THREAD, PR_PRIORITY_NORMAL, 0);

    {
    	/* The command line argument: -d is used to determine if the test is being run
    	in debug mode. The regress tool requires only one line output:PASS or FAIL.
    	All of the printfs associated with this test has been handled with a if (debug_mode)
    	test.
    	Usage: test_name -d
    	*/
    	PLOptStatus os;
    	PLOptState *opt = PL_CreateOptState(argc, argv, "d:");
    	while (PL_OPT_EOL != (os = PL_GetNextOpt(opt)))
        {
    		if (PL_OPT_BAD == os) continue;
            switch (opt->option)
            {
            case 'd':  /* debug mode */
    			debug_mode = 1;
                break;
             default:
                break;
            }
        }
    	PL_DestroyOptState(opt);
    }        

 /* main test */

#ifdef XP_MAC
	SetupMacPrintfLog("sem.log");
	debug_mode = 1;
#endif

    emptyBufs = PR_NewSem(2);	/* two empty buffers */

    fullBufs = PR_NewSem(0);	/* zero full buffers */

	/* create the reader thread */
	
	r = PR_CreateThread(PR_USER_THREAD,
				      reader, 0, 
				      PR_PRIORITY_NORMAL,
				      PR_LOCAL_THREAD,
    				  PR_UNJOINABLE_THREAD,
				      0);

	/* Do the writer operation in this thread */
	writer();

	PR_DestroySem(emptyBufs);
	PR_DestroySem(fullBufs);

	if (finalResult == PR_SUCCESS) {
		if (debug_mode) printf("sem Test Passed.\n");
	}
	else{
		if (debug_mode) printf("sem Test Failed.\n");
		failed_already=1;
	}
    PR_Cleanup();
	if(failed_already)	
		return 1;
	else
		return 0;
}

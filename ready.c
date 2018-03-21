/* ready.c - ready */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <systypes.h>
#include <mem.h>
#include <io.h>
#include <stdio.h>
/*------------------------------------------------------------------------
 * ready  --  make a process eligible for CPU service
 *------------------------------------------------------------------------
 */
int ready(int pid, int resch)
{
	register struct	pentry	*pptr;

	if (isbadpid(pid))
		return(SYSERR);
	pptr = &proctab[pid];
	pptr->pstate = PRREADY;
	if(pptr->stateProc==1) //real time process
	{
		insert(pid,readyQhead,1);
	}
	else
	{
		insert(pid,rdyhead,pptr->pprio);
	}
	if (resch)
		resched();
	return(OK);
}

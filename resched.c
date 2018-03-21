/* resched.c  -  resched */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <systypes.h>
#include <mem.h>
#include <q.h>
#include <lab1.h>
#include <io.h>
#include <stdio.h>
#define EPOCH_TIME_NEW 2
unsigned long currSP;	/* REAL sp of current process */
extern int ctxsw(int, int, int, int);
int sched_class=0;
void setschedclass(int);
int getschedclass();
/*-----------------------------------------------------------------------
 * resched  --  reschedule processor to highest priority ready process
 *
 * Notes:	Upon entry, currpid gives current process id.
 *		Proctab[currpid].pstate gives correct NEXT state for
 *			current process if other than PRREADY.
 *------------------------------------------------------------------------
 */
int resched()
{
	switch(getschedclass())
	{
		case LINUXSCHED:
			return linuxScheduling();
		case RANDOMSCHED:
			return randomScheduling();
		default:
			return xinuDefaultScheduling();
	}
}
void setschedclass(int shedClass)
{
	sched_class=shedClass;
}
int getschedclass()
{
	return sched_class;
}
int xinuDefaultScheduling()
{
	register struct	pentry	*optr;	/* pointer to old process entry */
	register struct	pentry	*nptr;	/* pointer to new process entry */

	/* no switch needed if current process priority higher than next*/

	if ( ( (optr= &proctab[currpid])->pstate == PRCURR) &&
	   (lastkey(rdytail)<optr->pprio)) {
		return(OK);
	}
	
	/* force context switch */

	if (optr->pstate == PRCURR) {
		optr->pstate = PRREADY;
		insert(currpid,rdyhead,optr->pprio);
	}

	/* remove highest priority process at end of ready list */

	nptr = &proctab[ (currpid = getlast(rdytail)) ];
	nptr->pstate = PRCURR;		/* mark it currently running	*/
#ifdef	RTCLOCK
	preempt = QUANTUM;		/* reset preemption counter	*/
#endif
	
	ctxsw((int)&optr->pesp, (int)optr->pirmask, (int)&nptr->pesp, (int)nptr->pirmask);
	
	/* The OLD process returns here when resumed. */
	return OK;
}
int linuxScheduling()
{
	register struct pentry *optr;
	register struct pentry *nptr;
	int epochTimeNew=FALSE;
	label: optr=&proctab[currpid];
	int oldPriority=optr->waitFactor-optr->intervalLeft;
	optr->waitFactor=oldPriority+preempt;
	optr->intervalLeft=preempt;
	if(optr->intervalLeft<=0 || currpid == NULLPROC)
	{
		optr->intervalLeft=0;
		optr->waitFactor=0;
	}
	int maxWaitTime=0, i=0, newID=0;
	for(i=q[rdytail].qprev;i!=rdyhead;i=q[i].qprev)
	{
		if(proctab[i].waitFactor>maxWaitTime)
		{
			newID=i;
			maxWaitTime=proctab[i].waitFactor;
		}
	}
	if(maxWaitTime==0 && (optr->pstate!=PRCURR || optr->intervalLeft==0))
	{
		if(epochTimeNew == FALSE)
		{
			epochTimeNew=TRUE;
			int i;
			struct pentry *p;
			for(i=0;i<NPROC;i++)
			{
				p=&proctab[i];
				if(p->pstate==PRFREE)
					continue;
				else if(p->intervalLeft==0 || p->intervalLeft==p->intervalTime)
				{
					p->intervalTime=p->pprio;
				}
				else
				{
					p->intervalTime=(p->intervalLeft)/2+p->pprio;
				}
				p->intervalLeft=p->intervalTime;
				p->waitFactor=p->intervalLeft+p->pprio;
			}
				preempt=optr->intervalLeft;
				goto label;
		}
		if(maxWaitTime==0)
		{
			if(currpid==NULLPROC)
				return OK;
			else
			{
				newID=NULLPROC;
				if(optr->pstate==PRCURR)
				{
					optr->pstate=PRREADY;
					insert(currpid,rdyhead,optr->pprio);
				}
				nptr=&proctab[newID];
				nptr->pstate=PRCURR;
				dequeue(newID);
				currpid=newID;
				#ifdef RTCLOCK
					preempt=QUANTUM;
				#endif
				ctxsw((int) &optr->pesp,(int)optr->pirmask, (int) &nptr->pesp,(int)nptr->pirmask);
				return OK;
			}
		}
	}
	else if(optr->waitFactor>=maxWaitTime && optr->waitFactor>0 && optr->pstate==PRCURR)
	{
		preempt=optr->intervalLeft;
		return OK;
	}
	else if(maxWaitTime>0 && (optr->pstate!=PRCURR || optr->intervalLeft==0 || optr->waitFactor < maxWaitTime))
	{
		if(optr->pstate==PRCURR)
		{
			optr->pstate=PRREADY;
			insert(currpid, rdyhead, optr->pprio);
		}
		nptr=&proctab[newID];
		nptr->pstate=PRCURR;
		dequeue(newID);
		currpid=newID;
		preempt=nptr->intervalLeft;
		ctxsw((int) &optr->pesp,(int)optr->pirmask, (int) &nptr->pesp,(int)nptr->pirmask);
		return OK;
	}
	else
		return SYSERR;
}
int randomScheduling()
{
        register struct pentry *optr;
        register struct pentry *nptr;
        int newID=0;
        optr=&proctab[currpid];

	int i,total_sum=0,ran=0;
	if(optr->pstate==PRCURR)
		total_sum=optr->pprio;
	struct pentry *process;
	if(isempty(rdyhead))
		return OK;
	
	for(i=q[rdyhead].qnext;i!=rdytail;i=q[i].qnext)
	{
		process=&proctab[i];
		total_sum=total_sum+process->pprio;
	}
	ran=rand()%(total_sum-1);
	if(optr->pstate==PRCURR && ran<optr->pprio)
	{
		return OK;
	}
	int k;
	struct pentry *p;
	ran=ran-optr->pprio;
	for(k=q[rdytail].qprev;k!=rdyhead && !isempty(rdyhead);k=q[k].qprev)
        {
        	p=&proctab[k];
                if(ran<p->pprio)
                {
                	newID=k;
         	        break;
                }
        	ran=ran-p->pprio;
        }
	if(optr->pstate==PRCURR)
	{
		optr->pstate=PRREADY;
		insert(currpid,rdyhead,optr->pprio);
	}
	nptr=&proctab[currpid=newID];
	nptr->pstate=PRCURR;
	dequeue(newID);
#ifdef RTCLOCK
	preempt=QUANTUM;
#endif
	ctxsw((int)&optr->pesp,(int)optr->pirmask,(int)&nptr->pesp,(int)nptr->pirmask);
	return OK;		
}

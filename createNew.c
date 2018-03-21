/*
 * FileName : CreateNew.c
 * Creates real time process 
 * author : srout@ncsu.edu
 */
#include <conf.h>
#include <i386.h>
#include <kernel.h>
#include <proc.h>
#include <sem.h>
#include <mem.h>
#include <io.h>
#include <stdio.h>

LOCAL int newpid();
SYSCALL createNew(procAddr,procName,priority,sSize,nArgs,args)
	int *procAddr;
	char *procName;
	int priority;
	int sSize;
	int nArgs;
	long args;
{
	unsigned long savsp, *pushsp;
	STATWORD ps;
	int pid;
	struct pentry *pptr;
	int i;
	unsigned long *a;
	unsigned long *saddr;
	int INITRET();
	disable(ps);
	if(sSize<MINSTK)
		sSize=MINSTK;
	sSize=(int)roundew(sSize);
	if(((saddr=(unsigned long *)getstk(sSize)) == (unsigned long *)SYSERR) || (pid =newpid())==SYSERR || priority<1)
	{
		restore(ps);
		return (SYSERR);
	}
	numproc++;
	pptr=&proctab[pid];
	pptr->fildes[0]=0;
	pptr->fildes[1]=0;
	pptr->fildes[2]=0;
	for(i=3;i<_NFILE;i++)
		pptr->fildes[i]=FDFREE;
	pptr->pstate=PRSUSP;
	for(i=0;i<PNMLEN && (int)(pptr->pname[i]=procName[i])!=0;i++);
	pptr->pprio=priority;
	pptr->pbase=(long)saddr;
	pptr->pstklen=sSize;
	pptr->psem=0;
	pptr->phasmsg=FALSE;
	pptr->plimit=pptr->pbase-sSize+sizeof(long);
	pptr->intervalLeft=0;
	pptr->intervalTime=0;
	pptr->waitFactor=0;
	pptr->stateProc=TRUE;
	pptr->pirmask[0]=0;
	pptr->pnxtkin=BADPID;
	pptr->pdevs[0]=BADDEV;
	pptr->pdevs[1]=BADDEV;
	pptr->ppagedev=BADDEV;
	*saddr=MAGIC;
	savsp=(unsigned long)saddr;
	pptr->pargs=nArgs;
	a=(unsigned long *)(&args)+(nArgs-1);
	for(;nArgs>0;nArgs--)
		*--saddr=(long)INITRET;
	*--saddr==(long)procAddr;
	pptr->paddr=(long)procAddr;
	*--saddr=savsp;
	savsp=(unsigned long)saddr;
	*--saddr=0;
	*--saddr=0;
	*--saddr=0;
	*--saddr=0;
	*--saddr=0;
	*--saddr=0;
	pushsp=saddr;
	*--saddr=savsp;
	*--saddr=0;
	*--saddr=0;
	*pushsp=(unsigned long)saddr;
	pptr->pesp=(unsigned long)saddr;
	restore(ps);
	return (pid);
}
LOCAL int newpid()
{
	int i,pid;
	for(i=0;i<NPROC;i++)
	{
		if((pid=nextproc--)<=0)
			nextproc=NPROC-1;
		if(proctab[pid].pstate==PRFREE)
			return (pid);
	}
	return (SYSERR);
}
	
	


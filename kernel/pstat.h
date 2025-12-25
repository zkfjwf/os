#define NPROC 64
// kernel/pstat.h
#ifndef _PSTAT_H_
#define _PSTAT_H_



struct pstat {
  int pid[NPROC];
  int priority[NPROC];
  int ticks[NPROC];
  int state[NPROC];
  int wait_time[NPROC];
  char name[NPROC][16];
  int inuse[NPROC];
};

#endif


struct pstat; // 前置声明

// 系统调用声明
int setpriority(int pid, int priority);
int getpinfo(struct pstat*);

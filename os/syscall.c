#include "syscall.h"
#include "console.h"
#include "defs.h"
#include "loader.h"
#include "syscall_ids.h"
#include "timer.h"
#include "trap.h"

uint64 sys_write(int fd, uint64 va, uint len)
{
	debugf("sys_write fd = %d str = %x, len = %d", fd, va, len);
	if (fd != STDOUT)
		return -1;
	struct proc *p = curr_proc();
	char str[MAX_STR_LEN];
	int size = copyinstr(p->pagetable, str, va, MIN(len, MAX_STR_LEN));
	debugf("size = %d", size);
	for (int i = 0; i < size; ++i) {
		console_putchar(str[i]);
	}
	return size;
}

uint64 sys_read(int fd, uint64 va, uint64 len)
{
	debugf("sys_read fd = %d str = %x, len = %d", fd, va, len);
	if (fd != STDIN)
		return -1;
	struct proc *p = curr_proc();
	char str[MAX_STR_LEN];
	for (int i = 0; i < len; ++i) {
		int c = consgetc();
		str[i] = c;
	}
	copyout(p->pagetable, va, str, len);
	return len;
}

__attribute__((noreturn)) void sys_exit(int code)
{
	exit(code);
	__builtin_unreachable();
}

uint64 sys_sched_yield()
{
	yield();
	return 0;
}

uint64 sys_gettimeofday(uint64 val, int _tz)
{
	struct proc *p = curr_proc();
	uint64 cycle = get_cycle();
	TimeVal t;
	t.sec = cycle / CPU_FREQ;
	t.usec = (cycle % CPU_FREQ) * 1000000 / CPU_FREQ;
	copyout(p->pagetable, val, (char *)&t, sizeof(TimeVal));
	return 0;
}

uint64 sys_getpid()
{
	return curr_proc()->pid;
}

uint64 sys_getppid()
{
	struct proc *p = curr_proc();
	return p->parent == NULL ? IDLE_PID : p->parent->pid;
}

uint64 sys_clone()
{
	debugf("fork!\n");
	return fork();
}

uint64 sys_exec(uint64 va)
{
	struct proc *p = curr_proc();
	char name[200];
	copyinstr(p->pagetable, name, va, 200);
	debugf("sys_exec %s\n", name);
	return exec(name);
}

uint64 sys_wait(int pid, uint64 va)
{
	struct proc *p = curr_proc();
	int *code = (int *)useraddr(p->pagetable, va);
	return wait(pid, code);
}

uint64 sys_spawn(uint64 va)
{
	// TODO: your job is to complete the sys call
	return -1;
}

uint64 sys_set_priority(long long prio){
    // TODO: your job is to complete the sys call
    return -1;
}


uint64 sys_sbrk(int n)
{
        uint64 addr;
        struct proc *p = curr_proc();
        addr = p->program_brk;
        if(growproc(n) < 0)
                return -1;
        return addr;
}

static uint64 get_ms_time()
{
	uint64 cycle = get_cycle();
	uint64 sec = cycle / CPU_FREQ;
	uint64 usec = (cycle % CPU_FREQ) * 1000000 / CPU_FREQ;
	uint64 time = sec * 1000 + usec / 1000 ;

	return time;
}

int sys_task_info(TaskInfo *ti)
{
	struct proc *p = curr_proc();
	TaskInfo ti_tmp;
	int mtime;
	int i;

	if (!ti)
		return -1;

	for (i = 0; i < MAX_SYSCALL_NUM; i++)
		ti_tmp.syscall_times[i] = p->ti.syscall_times[i];

	mtime = get_ms_time();
	ti_tmp.time = mtime - p->time_start;
	ti_tmp.status = Running;

	copyout(p->pagetable, (uint64)ti, (char *)&ti_tmp, sizeof(TaskInfo));

	return 0;
}

int mmap(void* start, unsigned long long len, int port, int flag, int fd)
{
	uint64 end = (uint64)start + len;
	void *page;
	uint64 va;
	int perm;

	if (len == 0 || len > (1 << 30))
		return -1;

	if ((uint64)start % PGSIZE != 0)
		return -1;

	if ((port & ~0x7) != 0)
		return -1;

	if ((port & 0x7) == 0)
		return -1;

	for (va = (uint64)start; va < end; va += PGSIZE) {
		// already mapped, overlap
		if (walkaddr(curr_proc()->pagetable, va) != 0)
			return -1;

		page = kalloc();
		if (!page)
			return -1;

		// the page should be valid and accessible to userspace
		perm = PTE_V | PTE_U;
		if (port & 0x1)
			perm |= PTE_R;

		if (port & 0x2)
			perm |= PTE_W;

		if (port & 0x4)
			perm |= PTE_X;

		if (mappages(curr_proc()->pagetable, va, PGSIZE, (uint64)page, perm) != 0) {
			kfree(page);

			return -1;
		}
	}

    return 0;
}

int munmap(void* start, unsigned long long len)
{
	uint64 end = (uint64)start + len;
	struct proc *p = curr_proc();
	pte_t *pte;
	uint64 va;

	if ((uint64)start % PGSIZE != 0)
		return -1;

	for (va = (uint64)start; va < end; va += PGSIZE) {
		pte = walk(curr_proc()->pagetable, va, 0);
		if (pte == 0 || (*pte & PTE_V) == 0)
			return -1;

		uvmunmap(p->pagetable, va, 1, 1);
	}

	return 0;
}

extern char trap_page[];

void syscall()
{
	struct trapframe *trapframe = curr_proc()->trapframe;
	int id = trapframe->a7, ret;
	struct proc *p = curr_proc();
	uint64 args[6] = { trapframe->a0, trapframe->a1, trapframe->a2,
			   trapframe->a3, trapframe->a4, trapframe->a5 };
	tracef("syscall %d args = [%x, %x, %x, %x, %x, %x]", id, args[0],
	       args[1], args[2], args[3], args[4], args[5]);
	p->ti.syscall_times[id]++;
	switch (id) {
	case SYS_write:
		ret = sys_write(args[0], args[1], args[2]);
		break;
	case SYS_read:
		ret = sys_read(args[0], args[1], args[2]);
		break;
	case SYS_exit:
		sys_exit(args[0]);
		// __builtin_unreachable();
	case SYS_sched_yield:
		ret = sys_sched_yield();
		break;
	case SYS_gettimeofday:
		ret = sys_gettimeofday(args[0], args[1]);
		break;
	case SYS_getpid:
		ret = sys_getpid();
		break;
	case SYS_getppid:
		ret = sys_getppid();
		break;
	case SYS_task_info:
		ret = sys_task_info((TaskInfo*)args[0]);
		break;
	case SYS_mmap:
		ret = mmap((void*)args[0], (unsigned long long)args[1], (int)args[2], (int)args[3], (int)args[4]);
		break;
	case SYS_munmap:
		ret = munmap((void*)args[0], (unsigned long long)args[1]);
		break;

	case SYS_clone: // SYS_fork
		ret = sys_clone();
		break;
	case SYS_execve:
		ret = sys_exec(args[0]);
		break;
	case SYS_wait4:
		ret = sys_wait(args[0], args[1]);
		break;
	case SYS_spawn:
		ret = sys_spawn(args[0]);
		break;
	case SYS_sbrk:
                ret = sys_sbrk(args[0]);
                break;
	default:
		ret = -1;
		errorf("unknown syscall %d", id);
	}
	trapframe->a0 = ret;
	tracef("syscall ret %d", ret);
}

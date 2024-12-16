#include "syscall.h"
#include "defs.h"
#include "loader.h"
#include "syscall_ids.h"
#include "timer.h"
#include "trap.h"
#include "proc.h"

uint64 sys_write(int fd, uint64 va, uint len)
{
	debugf("sys_write fd = %d va = %x, len = %d", fd, va, len);
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

uint64 sys_gettimeofday(TimeVal *val, int _tz) // TODO: implement sys_gettimeofday in pagetable. (VA to PA)
{
	struct proc *p = curr_proc();
	uint64 cycle = get_cycle();
	TimeVal tval;

	tval.sec = cycle / CPU_FREQ;
	tval.usec = (cycle % CPU_FREQ) * 1000000 / CPU_FREQ;
	copyout(p->pagetable, (uint64)val, (char *)&tval, sizeof(TimeVal));
	
	return 0;
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

int sys_getpid()
{
	struct proc *p = curr_proc();

	return p->pid;
}

// TODO: add support for mmap and munmap syscall.
// hint: read through docstrings in vm.c. Watching CH4 video may also help.
// Note the return value and PTE flags (especially U,X,W,R)
/*
* LAB1: you may need to define sys_task_info here
*/

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
	/*
	* LAB1: you may need to update syscall counter for task info here
	*/
	p->ti.syscall_times[id]++;
	switch (id) {
	case SYS_write:
		ret = sys_write(args[0], args[1], args[2]);
		break;
	case SYS_exit:
		sys_exit(args[0]);
		// __builtin_unreachable();
	case SYS_sched_yield:
		ret = sys_sched_yield();
		break;
	case SYS_gettimeofday:
		ret = sys_gettimeofday((TimeVal *)args[0], args[1]);
		break;
	case SYS_sbrk:
		ret = sys_sbrk(args[0]);
		break;
	case SYS_getpid:
		ret = sys_getpid();
		break;
	/*
	* LAB1: you may need to add SYS_taskinfo case here
	*/
	case SYS_task_info:
		ret = sys_task_info((TaskInfo*)args[0]);
		break;
	case SYS_mmap:
		ret = mmap((void*)args[0], (unsigned long long)args[1], (int)args[2], (int)args[3], (int)args[4]);
		break;
	case SYS_munmap:
		ret = munmap((void*)args[0], (unsigned long long)args[1]);
		break;
	default:
		ret = -1;
		errorf("unknown syscall %d", id);
	}
	trapframe->a0 = ret;
	tracef("syscall ret %d", ret);
}

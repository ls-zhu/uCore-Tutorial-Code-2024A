# uCore Lab1 实验报告
姓名：祝令山 学号：2024319220
## 功能总结
1. 支持系统调用SYS_getpid，实现函数sys_getpid*()，返回当前进程的pid。
3. 支持系统调用SYS_task_info:   
3.1 实现内核态的get_ms_time()，通过计算cycles返回当前时间。  
3.2 在struc proc中加入TaskInfo用来继续进程信息和time_start用于记录进程启动时间，time_start在proc_init时初始化为0，并且在任务第一次被调度时候赋值。  
3.3 在syscall()函数中将每次被执行的系统调用此处累加记录到proc->TaskInfo.syscall_times[id]中   
3.4 实现函数sys_task_info():  
3.4.1 程序运行总时长TaskInfo.time为当前时间减去程序启动时刻(proc->start_time)  
3.4.2 从proc->TaskInfo.syscall_timesp[]中获取每个系统调用的执行次数  
3.4.3 因为被查询的任务为当前进程，所以其一定处于RUNNING状态，所以TaskInfo->status = Running(enum TaskStatus.Running)。**请注意这里使用了hard code直接赋值，而不是读取proc->state。这是一个无奈的workaround，因为proc->state为 enum procstate { UNUSED, USED, SLEEPING, RUNNABLE, RUNNING, ZOMBIE }其中RUNNING == 4，而用户态测试程序校验使用的是enum TaskStatus{UnInit, Ready, Running, Exited,}其中Running == 3，返回TaskStatus.Running通不过测试。这是一个需要修复的issue**  

## 作业问答  
1. 进入U状态后，使用S态的指令会产生特权级异常报错，我的用例为在用户态通过汇编访问sstatus，会在usertrap中触发“IllegalInstruction in application, core dumped”。这和x86在ring3试图访问ring0的资源或者指令的行为的异常逻辑是一致的。sbi为RustSBI-QEMU Version 0.2.0-alpha.2  
2.1 a0：指向用户态程序的trapframe   a1：指向用户进程的页表基址  
2.2 os通过 “csrw satp, a1”切换到用户态页表，所以需要使用sfence.vma保证(1)之前的内存更改已经生效(2)flush TLB。删掉sfence.vma可能会导致内存地址映射错误或者安全性问题  
2.3 因为a0指向trapframe，在后续的语句，例如ld ra, 40(a0)中，a0被用做基地址，所以不能改动。现在的a0为用户态程序的trapframe地址。a0的值被存储在sscratch寄存器中  
2.4 sret会从S模式切换到U模式。因为usertrapret()函数将SPP设置为0 (x &= ~SSTATUS_SPP)。而spec定义为：When
an SRET instruction (see Section 3.3.2) is executed to return from the trap handler, the privilege level
is set to user mode if the SPP bit is 0, or supervisor mode if the SPP bit is 1; SPP is then set to 0. 所以sret会切换到用户态  
2.5 这里csrrw a0, sscratch, a0相当于一个swap操作，交换了a0和sscratch的值。即sscratch指向trapframe，a0为原本保存在sscratch中的用户态a0的值  
2.6 从a0(trapframe) + 40处开始保存，由struct trapframe定义可知，trapframe从偏移0到39为内核相关值，不需要保存。没有保存所有值，比如trapframe + 112 (a0)就没有保存，因为当前正在使用a0指向trapframe  
2.7 由ecall指令进入S模式  
2.8 执行ld t0, 16(a0)后，t0的值为a0 + 16，即trapframe + 16, 由struct trapfame定义可知，偏移16为kernel_trap，即usertrap()  

# 荣誉准则
1. 在完成本次实验的过程（含此前学习的过程）中，我曾分别与 以下各位 就（与本次实验相关的）以下方面做过交流，还在代码中对应的位置以注释形式记录了具体的交流对象及内容：
>无
2. 此外，我也参考了 以下资料 ，还在代码中对应的位置以注释形式记录了具体的参考来源及内容：
> 《The RISC-V Instruction Set Manual: Volume II Privileged Architecture》
3. 我独立完成了本次实验除以上方面之外的所有工作，包括代码与文档。 我清楚地知道，从以上方面获得的信息在一定程度上降低了实验难度，可能会影响起评分。
4. 我从未使用过他人的代码，不管是原封不动地复制，还是经过了某些等价转换。 我未曾也不会向他人（含此后各届同学）复制或公开我的实验代码，我有义务妥善保管好它们。 我提交至本实验的评测系统的代码，均无意于破坏或妨碍任何计算机系统的正常运转。 我清楚地知道，以上情况均为本课程纪律所禁止，若违反，对应的实验成绩将按“-100”分计。

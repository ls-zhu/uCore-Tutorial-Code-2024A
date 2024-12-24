# uCore Lab3 实验报告
姓名：祝令山 学号：2024319220
## 功能总结
1. 实现int sys_spawn(char *filename)系统调用，syscall ID: 400，实现如下：  
1.1 通过copyinstr()从父进程得到子进程的文件名file_name  
1.2 通过get_id_by_name(file_name)得到子进程的app_id  
1.3 通过allocproc()分配子进程  
1.4 设置子进程child的属性，子进程的父进程指向“当前进程”，子进程的优先级继承自父进程，stride为0；用父进程的trapframe初始化子进程的trapframe(这里使用浅拷贝，因为trapframe中都是u64),而后进一步初始化子进程在的sp为child->ustack + USTACK_SIZE, epc为BASE_ADDRESS, ra = 0  
1.5 通过loader()加载子进程  
1.6 设置子进程的状态为RUNNABLE，通过add_task()把子进程加入到进程队列中  
1.7 返回子进程的pid。  
以上有任何一步失败则返回-1  

2. 实现sys_set_priority(long long prio)系统调用，实现如下：  
2.1 在proc中增加priority参数  
2.2 校验参数prio，过大或者过小都返回-1  
2.3 将curr_proc的priority设置为prio

3. 实现stride调度算法,这里主要更改了fetch_task()函数，与原有fetch_task()函数只是pop_task，从队列头取出第一个任务不同，更改过的fetch_task会返回任务队列中stride值最小的任务，实现如下：  
3.1 从task poll中找到sride值最小，并且状态为RUNNABLE的任务min_proc，并且记录其在task_queue中的id(这是为了从task_queue中移除这个任务)  
3.2 如果存在这样一个min_proc，则根据它在task_queue中的id，把它从task_queue中移除，并且task_queue中在id后面的任务依次向前覆盖(即循环用id+1号任务覆盖id号任务，以达到移除id号任务而且不留下空洞的目的)。这一步操作的目的是为了仿照原版fetch_task的pop操作，避免出现panic("queue shouldn't be overflow")这一错误。  
3.3 计算pass值，并且加到min_proc的stride值上  
3.4 返回min_proc

## 作业问答  
1. Stride算法深入  
1.1 实际情况是轮到 p1 执行吗？为什么？  
答：不一定stride为8bit，最大值255，如果p1.stride=255，p2.stride=250，而p2运行后会增加stride，即p2.stride可能会溢出变为一个比较小的正整数，所以在下一次选择任务的时候依然会选择p2.  
1.2 为什么STRIDE_MAX – STRIDE_MIN <= BigStride / 2？  
答：因为priority最小为2，所以SRIDE_MAX = BIG_STRID / 2；STRIDE_MIN为正整数，所以有STRIDE_MAX – STRIDE_MIN <= BigStride / 2，即最大步长pass不会超过BigStride/2。也就是说，正常情况下p1.stride和p2.stride的差值不会超过BigStride / 2，这实际上就是确定了一个界，以便可以用下面1.3中的办法来解决优先级反转问题。  
1.3 设计的函数如下，解释在注释里：  
```
typedef unsigned long long Stride_t;
const Stride_t BIG_STRIDE = 65536;
int cmp(Stride_t a, Stride_t b){
  Stride_t diff;
  if (a == b)
    return 0;
//如果a>b,则返回a - b，否则返回b - a，即返回a和b之间的差
  diff = (a > b) ? (a - b) : (b - a);
//如果diff不大于BIG_STRIDE / 2，则正常。如果diff大于BIG_STRIDE / 2，则越界，发生了优先级反转(因为步长不可能大于BIG_STRIDE / 2)
  if (diff <= BIG_STRIDE / 2
    //正常返回a大或者b大
    return (a > b) ? 1 : -1;
  else
  //溢出了，反转了，则返回和正常值相反的结果
    return (a > b) ? -1 : 1;
}
```

# 问题和看法
即使在一行代码都不改的情况下，make test CHAPTER=5t 和 make run BASE=2都会报错[PANIC 0] os/loader.c:95: Cannpt find INIT_PROC usershell
# 荣誉准则
1. 在完成本次实验的过程（含此前学习的过程）中，我曾分别与 以下各位 就（与本次实验相关的）以下方面做过交流，还在代码中对应的位置以注释形式记录了具体的交流对象及内容：
>无
2. 此外，我也参考了 以下资料 ，还在代码中对应的位置以注释形式记录了具体的参考来源及内容：
> 无
3. 我独立完成了本次实验除以上方面之外的所有工作，包括代码与文档。 我清楚地知道，从以上方面获得的信息在一定程度上降低了实验难度，可能会影响起评分。
4. 我从未使用过他人的代码，不管是原封不动地复制，还是经过了某些等价转换。 我未曾也不会向他人（含此后各届同学）复制或公开我的实验代码，我有义务妥善保管好它们。 我提交至本实验的评测系统的代码，均无意于破坏或妨碍任何计算机系统的正常运转。 我清楚地知道，以上情况均为本课程纪律所禁止，若违反，对应的实验成绩将按“-100”分计。


# uCore Lab4 实验报告
姓名：祝令山 学号：2024319220
## 功能总结
1. 实现系统调用sys_linkat(int olddirfd, uint64 oldpath_addr, int newdirfd, uint64 newpath_addr, unsigned int flags)，实现如下：  
1.1 通过ccopyinstr()获得原文件名oldpath和硬链接文件名newpath
1.2 通过namei()得到oldpath的inode， old_inode  
1.3 ivalid old_inode  
1.4 通过dirlookup检测硬链接文件名newpath是否存在，如果存在则return -1  
1.5 通过dirlink()创建硬链接，文件名为newpath，inode号同old_inode  
1.6 inode的nlink++  
1.7 iupdate & iput  
辅助改动如下： 
在结构体inode和dinode中增加链接计数器nlink，在inode操作相关函数比如iupdate，ivalide中正确处理、更新nlink，特别要在iput和iget中做nlink--和nlink++
2. 实现函数int dirunlink(struct inode *dp, char *name)用来删除文件:   
2.1 通过copyinstr()得到文件名filename  
2.2 判断dp是不是目录，如果不是则失败  
2.3 遍历dp中的每一个文件，通过文件名匹配filename，找到其inode  
2.4 清空de，写inode，nlink--  
2.5 iput

3. 实现int unlinkat(int dirfd, char* path, unsigned int flags)，实现如下：  
2.1 通过copyinstr()得到待删除的硬链接文件名path  
2.2 通过文件名找到其ionode    
2.3 ivalide inode  
2.4 dirunlink，其中目录为root，文件名为path  
2.5 iput inode.

3. int fstat(int fd, struct Stat* st)，实现如下：  
3.1 首先判断fd的合法性：是否小于0,是否大于FD_BUFFER_SIZE， 是否属于当前process   
3.2 通过fd得到inode  
3.3 ivalide inode    
3.4 通过inode的信息填充st，并且copyout返回用户空间

## 作业问答  
1. 在我们的文件系统中，root inode起着什么作用？如果root inode中的内容损坏了，会发生什么？
答：root inode代表根目录，如果root inode损坏，则无法mount文件系统，没办法找到文件系统下的文件

# 问题和看法
1. ch6 branch没有get_id_by_name()和loader()等函数，所以ch5的代码没法在ch6上工作，没有办法执行ch6_usertest  
2. ch6的大部分用户态测试用例都没办法编译，只有filetest和filetest_simple两个用例可用。  
3. ch6直接make会出现invalid init proc name错误

# 荣誉准则
1. 在完成本次实验的过程（含此前学习的过程）中，我曾分别与 以下各位 就（与本次实验相关的）以下方面做过交流，还在代码中对应的位置以注释形式记录了具体的交流对象及内容：
>无
2. 此外，我也参考了 以下资料 ，还在代码中对应的位置以注释形式记录了具体的参考来源及内容：
> 无
3. 我独立完成了本次实验除以上方面之外的所有工作，包括代码与文档。 我清楚地知道，从以上方面获得的信息在一定程度上降低了实验难度，可能会影响起评分。
4. 我从未使用过他人的代码，不管是原封不动地复制，还是经过了某些等价转换。 我未曾也不会向他人（含此后各届同学）复制或公开我的实验代码，我有义务妥善保管好它们。 我提交至本实验的评测系统的代码，均无意于破坏或妨碍任何计算机系统的正常运转。 我清楚地知道，以上情况均为本课程纪律所禁止，若违反，对应的实验成绩将按“-100”分计。




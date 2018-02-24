# Berkeley-CS162
完成Berkeley的课程作业，以及在pintos这个简化版的操作系统中，完善线程调度、文件系统等模块。

## 基本作业：

1. 使用gdb工具，DONE
2. 实现一个http服务器，DONE
3. 使用一个malloc的内存分配，DONE

## pintos：
1. 完成project1：实现sleep优化工作，避免busy loop；实现priority donate，能够使得高priority线程。
速得到cpu。此外，同步元件lock能够传递priority，当lock release时等待队列中的高priority线程优先拿锁；实现

1. 完成project1：实现sleep优化工作，避免busy loop；实现priority donate，能够使得高priority线程。速得到cpu。此外，同步元件lock能够传递priority，当lock release时等待队列中的高priority线程优先拿锁；实现mlsfp多级队列线程调度算法，使用nice和recent_cpu基于指数平均计算线程priority。

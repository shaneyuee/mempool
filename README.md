## **简单介绍**
一个基于CAS的无锁共享内存队列的实现，使用fifo进行通知。OIDB分set实现的核心组件，已经在OIDB 600台机器稳定运行了近一年时间，为OIDB节省了150多台机器。
特点：
 -  1，高性能，800w/s以上；
 -  2，实时通知，可用epoll侦听；
 -  3，高并发，支持同时多写多读。

## **基本功能**
 - 1，分配内存
 - 2，释放内存
 - 3，简单的slab分配算法实现
 - 4，支持多线程访问
 - 5，分配总内存超过最大值限制时遍历回收后重新分配
 - 6，超过最大分组大小（16M）时直接从系统分配
 - 7，Attr_API属性上报监控内存分配

## **快速上手**
- 接口：
class UcMemManager
{
 public:
    static UcMem *Alloc(uint64_t sz);
    static void Free(UcMem *m);
    static void SetMaxSize(uint64_t sz);
};

- 分配：
UcMem *m = UcMemManager::Alloc(1024);

- 使用：
unsigned long lptr = (unsigned long )m->ptr();
lptr[0] = 1;

- 释放：
UcMemManager::Free(m);

- 示例：
UcMem *m = UcMemManager::Alloc(289340);
if(m==NULL)
{   
       cout << "alloc 289340 failed" << endl;
       return -1; 
}   
if(m->GetAllocSize() != 1048576)
{   
       cout << "alloc return mismatch size 1048576 <-> " << m->GetAllocSize() << endl;
}   
UcMemManager::Free(m);





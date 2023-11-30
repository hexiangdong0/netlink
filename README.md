# 演示如何使用netlink在内核和用户空间程序进行通信

## 内核部分
在内核中使用netlink可以参考`1kernel.c`

## 用户空间程序
python的netlink貌似问题比较大，可以将C程序编译成共享库，然后在python中使用共享库来实现收发消息。


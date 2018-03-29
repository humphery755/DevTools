obj-m := do_fork.o

#编译:

#make -C /usr/src/kernels/2.6.32-358.el6.i686 M=$(pwd) modules
#加载内核模块:

#insmod do_fork.ko
#测试效果：

#dmesg| tail



#cat /proc/kallsyms | grep do_fork



#do_fork的地址与kprobe注册的地址一致，可见，在kprobe调试模块在内核停留期间，我们编写的内核监控模块劫持并记录了系统fork出了新的进程信息
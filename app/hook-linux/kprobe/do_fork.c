/*
 * * You will see the trace data in /var/log/messages and on the console
 * * whenever do_fork() is invoked to create a new process.
 * */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kprobes.h>

//定义要Hook的函数，本例中do_fork
static struct kprobe kp = 
{
  .symbol_name = "do_fork",
};

static int handler_pre(struct kprobe *p, struct pt_regs *regs)
{
  struct thread_info *thread = current_thread_info();

  printk(KERN_INFO "pre-handler thread info: flags = %x, status = %d, cpu = %d, task->pid = %d\n",
  thread->flags, thread->status, thread->cpu, thread->task->pid);

  return 0;
}

static void handler_post(struct kprobe *p, struct pt_regs *regs, unsigned long flags)
{  
  struct thread_info *thread = current_thread_info();

  printk(KERN_INFO "post-handler thread info: flags = %x, status = %d, cpu = %d, task->pid = %d\n",
  thread->flags, thread->status, thread->cpu, thread->task->pid);
}

static int handler_fault(struct kprobe *p, struct pt_regs *regs, int trapnr)
{
  printk(KERN_INFO "fault_handler: p->addr = 0x%p, trap #%dn",
  p->addr, trapnr);
  return 0;
}

/*
内核模块加载初始化，这个过程和windows下的内核驱动注册分发例程很类似
*/
static int __init kprobe_init(void)
{
  int ret;
  kp.pre_handler = handler_pre;
  kp.post_handler = handler_post;
  kp.fault_handler = handler_fault;

  ret = register_kprobe(&kp);
  if (ret < 0) 
  {
    printk(KERN_INFO "register_kprobe failed, returned %d\n", ret);
    return ret;
  }
  printk(KERN_INFO "Planted kprobe at %p\n", kp.addr);
  return 0;
}

static void __exit kprobe_exit(void)
{
  unregister_kprobe(&kp);
  printk(KERN_INFO "kprobe at %p unregistered\n", kp.addr);
}

module_init(kprobe_init)
module_exit(kprobe_exit)
MODULE_LICENSE("GPL");

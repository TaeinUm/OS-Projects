#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/kprobes.h>
#include <linux/sched.h>

static int pre_count = 0;
static int post_count = 0;
static int context_switch_count = 0;

static struct kretprobe my_kretprobe;
static struct proc_dir_entry *perftop_proc_file;

static int perftop_show(struct seq_file *m, void *v) {
    // seq_printf(m, "Hello World\n");
    // seq_printf(m, "Pre-event count: %d\n", pre_count);
    // seq_printf(m, "Post-event count: %d\n", post_count);
    seq_printf(m, "Context switch count: %d\n", context_switch_count);
    return 0;
}

static int perftop_open(struct inode *inode, struct file *file) {
  return single_open(file, perftop_show, NULL);
}

static const struct proc_ops perftop_fops = {
  .proc_open = perftop_open,
  .proc_read = seq_read,
  .proc_lseek = seq_lseek,
  .proc_release = single_release,
};

static int entry_pick_next_fair(struct kretprobe_instance *ri, struct pt_regs *regs) {
    *((struct task_struct **)ri->data) = (struct task_struct *)regs_get_kernel_argument(regs, 0);
    pre_count++;
    return 0;
}

static int ret_pick_next_fair(struct kretprobe_instance *ri, struct pt_regs *regs) {
    struct task_struct *next = (struct task_struct *)regs_return_value(regs);
    struct task_struct *prev = *((struct task_struct **)ri->data);
    
    if (prev != next) {
        context_switch_count++;
    }

    post_count++;
    return 0;
}

static int __init perftop_init(void) {
    int ret;

    perftop_proc_file = proc_create("perftop", 0, NULL, &perftop_fops);
    if (!perftop_proc_file) {
        return -ENOMEM;
    }

    my_kretprobe.kp.symbol_name = "pick_next_task_fair";
    my_kretprobe.handler = ret_pick_next_fair;
    my_kretprobe.entry_handler = entry_pick_next_fair;
    my_kretprobe.data_size = sizeof(struct task_struct *);
    my_kretprobe.maxactive = 20;

    ret = register_kretprobe(&my_kretprobe);
    if (ret < 0) {
        printk(KERN_INFO "register_kretprobe failed, returned %d\n", ret);
        return -1;
    }
    printk(KERN_INFO "Planted return probe at %s: %p\n",
           my_kretprobe.kp.symbol_name, my_kretprobe.kp.addr);
    return 0;
}

static void __exit perftop_exit(void) {
    unregister_kretprobe(&my_kretprobe);
    printk(KERN_INFO "kretprobe at %p unregistered\n", my_kretprobe.kp.addr);

    printk(KERN_INFO "Missed probing %d instances of %s\n",
           my_kretprobe.nmissed, my_kretprobe.kp.symbol_name);

    proc_remove(perftop_proc_file);
}

MODULE_LICENSE("GPL");
module_init(perftop_init);
module_exit(perftop_exit);

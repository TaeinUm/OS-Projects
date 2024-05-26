#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/kprobes.h>
#include <linux/sched.h>
#include <linux/rbtree.h>
#include <linux/hashtable.h>
#include <linux/slab.h>  // Include for kmalloc and kfree
#include <linux/spinlock.h>
#include <asm/msr.h>  // Include for rdtsc


// Define a structure for the red-black tree node
struct task_info {
    struct rb_node node;
    pid_t pid;
    u64 total_cpu_time;
};

// Define a structure for the hash table
struct task_start_time {
    pid_t pid;
    u64 start_time;
    struct hlist_node hnode;
};

// Global declaration of the red-black tree root and the hash table
static struct rb_root rb_root = RB_ROOT;
static DEFINE_HASHTABLE(start_time_hash, 10); // Adjust the size as needed

static struct kretprobe my_kretprobe;
static struct proc_dir_entry *perftop_proc_file;

static DEFINE_SPINLOCK(rbtree_lock);  // Spinlock for red-black tree synchronization

// Function to add a start time for a task
void add_start_time(pid_t pid, u64 start_time) {
    struct task_start_time *item;

    item = kmalloc(sizeof(*item), GFP_ATOMIC);
    if (!item)
        return;

    item->pid = pid;
    item->start_time = start_time;
    hash_add(start_time_hash, &item->hnode, pid);
}

// Function to find a start time for a task
u64 find_start_time(pid_t pid) {
    struct task_start_time *item;
    u64 start_time = 0;

    hash_for_each_possible(start_time_hash, item, hnode, pid) {
        if (item->pid == pid) {
            start_time = item->start_time;
            break;
        }
    }

    return start_time;
}

void delete_start_time(pid_t pid) {
    struct task_start_time *item;
    struct hlist_node *tmp;

    hash_for_each_possible_safe(start_time_hash, item, tmp, hnode, pid) {
        if (item->pid == pid) {
            hash_del(&item->hnode);
            kfree(item);
            break;
        }
    }
}

// Helper function to create a new task_info node
struct task_info *create_task_info_node(pid_t pid, u64 cpu_time) {
    struct task_info *new_node;

    new_node = kmalloc(sizeof(*new_node), GFP_KERNEL);
    if (!new_node)
        return NULL;

    new_node->pid = pid;
    new_node->total_cpu_time = cpu_time;
    return new_node;
}

// Function to insert a task into the red-black tree
void insert_task_rbtree(pid_t pid, u64 cpu_time) {
    struct rb_node **new = &(rb_root.rb_node), *parent = NULL;
    struct task_info *data, *this;

    spin_lock(&rbtree_lock);  // Acquire the lock

    while (*new) {
        this = container_of(*new, struct task_info, node);
        parent = *new;

        if (pid < this->pid)
            new = &((*new)->rb_left);
        else if (pid > this->pid)
            new = &((*new)->rb_right);
        else
            return; // Duplicate pid, should handle as needed
    }

    data = create_task_info_node(pid, cpu_time);
    if (!data)
        return;

    // Add new node and rebalance tree
    rb_link_node(&data->node, parent, new);
    rb_insert_color(&data->node, &rb_root);

    spin_unlock(&rbtree_lock);  // Release the lock
}

// Function to find a task in the red-black tree
struct task_info *find_task_rbtree(pid_t pid) {
    struct rb_node *node = rb_root.rb_node;

    while (node) {
        struct task_info *data = container_of(node, struct task_info, node);

        if (pid < data->pid)
            node = node->rb_left;
        else if (pid > data->pid)
            node = node->rb_right;
        else
            return data; // Found
    }

    return NULL; // Not found
}

// Helper function to add task to top tasks array
void add_to_top_tasks(struct task_info *task_array[], struct task_info *task) {
    int i;
    for (i = 9; i >= 0 && (!task_array[i] || task_array[i]->total_cpu_time < task->total_cpu_time); i--) {
        if (i < 9)
            task_array[i + 1] = task_array[i];
    }
    if (i < 9)
        task_array[i + 1] = task;
}

// Helper function to traverse the red-black tree
void traverse_rbtree(struct rb_root *root, struct task_info *task_array[]) {
    struct rb_node *node = rb_first(root);
    while (node) {
        struct task_info *task = container_of(node, struct task_info, node);
        add_to_top_tasks(task_array, task);
        node = rb_next(node);
    }
}

static int perftop_show(struct seq_file *m, void *v) {
    struct task_info *top_tasks[10] = {0};
    int i;

    spin_lock(&rbtree_lock);  // Acquire the lock
    traverse_rbtree(&rb_root, top_tasks);
    spin_unlock(&rbtree_lock);  // Release the lock

    seq_printf(m, "Top 10 CPU consuming tasks:\n");

    for (i = 0; i < 10; i++) {
        if (top_tasks[i]) {
            seq_printf(m, "PID: %d, CPU Time: %llu ns\n", top_tasks[i]->pid, top_tasks[i]->total_cpu_time);
        }
    }

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

void update_rb_tree(pid_t pid, u64 cpu_time) {
    struct task_info *task_node = find_task_rbtree(pid);

    if (task_node) {
        // Task already in tree, update CPU time
        task_node->total_cpu_time += cpu_time;
    } else {
        // Task not in tree, insert new node
        insert_task_rbtree(pid, cpu_time);
    }
}

static int entry_pick_next_fair(struct kretprobe_instance *ri, struct pt_regs *regs) {
    struct task_struct *task = (struct task_struct *)regs_get_kernel_argument(regs, 0);
    u64 start_time = rdtsc_ordered(); // Get the current time-stamp counter value

    if (task) {
        add_start_time(task->pid, start_time); // Store the start time for this task
    }

    return 0;
}

static int ret_pick_next_fair(struct kretprobe_instance *ri, struct pt_regs *regs) {
    struct task_struct *prev = *((struct task_struct **)ri->data);
    struct task_struct *next = (struct task_struct *)regs_return_value(regs);
    u64 end_time = rdtsc_ordered(); // Get the current time-stamp counter value

    if (prev && prev != next) {
        u64 start_time = find_start_time(prev->pid);
        u64 total_time = end_time - start_time;

        // Update the total CPU time in the red-black tree
        update_rb_tree(prev->pid, total_time);

        // Delete the start time from the hash table
        delete_start_time(prev->pid);
    }

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

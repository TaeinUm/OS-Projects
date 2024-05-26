#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/pagemap.h>

#define S2FS_MAGIC 0x19980122

MODULE_LICENSE("GPL");

static struct dentry *s2fs_mount(struct file_system_type *fs_type, int flags, const char *dev_name, void *data);
static int s2fs_fill_super(struct super_block *sb, void *data, int silent);
static struct inode *s2fs_make_inode(struct super_block *sb, int mode);
static struct dentry *s2fs_create_dir(struct super_block *sb, struct dentry *parent, const char *dir_name);
static struct dentry *s2fs_create_file(struct super_block *sb, struct dentry *parent, const char *file_name);
static int s2fs_open(struct inode *inode, struct file *filp);
static ssize_t s2fs_read_file(struct file *filp, char __user *buf, size_t count, loff_t *offset);
static ssize_t s2fs_write_file(struct file *filp, const char __user *buf, size_t count, loff_t *offset);

static struct super_operations s2fs_super_ops = {
    .statfs = simple_statfs,
    .drop_inode = generic_delete_inode,
};

static struct file_system_type s2fs_type = {
    .owner = THIS_MODULE,
    .name = "s2fs",
    .mount = s2fs_mount,
    .kill_sb = kill_litter_super,
    .fs_flags = FS_USERNS_MOUNT,
};

static struct file_operations s2fs_fops = {
    .open = s2fs_open,
    .read = s2fs_read_file,
    .write = s2fs_write_file,
};

static struct dentry *s2fs_mount(struct file_system_type *fs_type, int flags, const char *dev_name, void *data) {
    struct dentry *ret;

    ret = mount_nodev(fs_type, flags, data, s2fs_fill_super);

    if (IS_ERR(ret))
        printk(KERN_ERR "s2fs: Error mounting file system\n");

    return ret;
}

static int s2fs_fill_super(struct super_block *sb, void *data, int silent) {
    struct inode *root_inode;
    struct dentry *root_dentry, *foo_dir, *bar_file;

    sb->s_magic = S2FS_MAGIC;
    sb->s_blocksize = PAGE_SIZE;
    sb->s_blocksize_bits = PAGE_SHIFT;
    sb->s_op = &s2fs_super_ops;

    root_inode = s2fs_make_inode(sb, S_IFDIR | 0755);
    if (!root_inode) {
        printk(KERN_ERR "s2fs: Error creating root inode\n");
        return -ENOMEM;
    }

    root_inode->i_op = &simple_dir_inode_operations;
    root_inode->i_fop = &simple_dir_operations;
    root_inode->i_atime = root_inode->i_mtime = root_inode->i_ctime = current_time(root_inode);

    root_dentry = d_make_root(root_inode);
    if (!root_dentry) {
        printk(KERN_ERR "s2fs: Error creating root dentry\n");
        return -ENOMEM;
    }

    sb->s_root = root_dentry;

    foo_dir = s2fs_create_dir(sb, root_dentry, "foo");
    if (!foo_dir) {
        printk(KERN_ERR "s2fs: Error creating foo directory\n");
        return -ENOMEM;
    }

    bar_file = s2fs_create_file(sb, foo_dir, "bar");
    if (!bar_file) {
        printk(KERN_ERR "s2fs: Error creating bar file\n");
        return -ENOMEM;
    }

    return 0;
}

static struct inode *s2fs_make_inode(struct super_block *sb, int mode) {
    struct inode *ret = new_inode(sb);

    if (ret) {
        ret->i_ino = get_next_ino();
        ret->i_mode = mode;
        ret->i_uid.val = ret->i_gid.val = 0;
        ret->i_blocks = 0;
        ret->i_atime = ret->i_mtime = ret->i_ctime = current_time(ret);
    }
    return ret;
}

static struct dentry *s2fs_create_dir(struct super_block *sb, struct dentry *parent, const char *dir_name) {
    struct dentry *dentry;
    struct inode *inode;
    struct qstr qname;

    qname.name = dir_name;
    qname.len = strlen(dir_name);
    qname.hash = full_name_hash(parent, dir_name, qname.len);

    dentry = d_alloc(parent, &qname);
    if (!dentry)
        return NULL;

    inode = s2fs_make_inode(sb, S_IFDIR | 0755);
    if (!inode) {
        dput(dentry);
        return NULL;
    }

    inode->i_op = &simple_dir_inode_operations;
    inode->i_fop = &simple_dir_operations;

    d_add(dentry, inode);
    return dentry;
}

static struct dentry *s2fs_create_file(struct super_block *sb, struct dentry *parent, const char *file_name) {
    struct dentry *dentry;
    struct inode *inode;
    struct qstr qname;

    qname.name = file_name;
    qname.len = strlen(file_name);
    qname.hash = full_name_hash(parent, file_name, qname.len);

    dentry = d_alloc(parent, &qname);
    if (!dentry)
        return NULL;

    inode = s2fs_make_inode(sb, S_IFREG | 0644);
    if (!inode) {
        dput(dentry);
        return NULL;
    }

    inode->i_fop = &s2fs_fops;

    d_add(dentry, inode);
    return dentry;
}

static int s2fs_open(struct inode *inode, struct file *filp) {
    return 0;
}

static ssize_t s2fs_read_file(struct file *filp, char __user *buf, size_t count, loff_t *offset) {
    char *data = "Hello World!\n";
    size_t datalen = strlen(data);

    if (*offset > datalen)
        return 0;

    if (count > datalen - *offset)
        count = datalen - *offset;

    if (copy_to_user(buf, data + *offset, count)) {
        return -EFAULT;
    }

    *offset += count;
    return count;
}

static ssize_t s2fs_write_file(struct file *filp, const char __user *buf, size_t count, loff_t *offset) {
    return 0;
}

int s2fs_init(void) {
    int ret;

    ret = register_filesystem(&s2fs_type);
    if (ret != 0) {
        printk(KERN_ERR "s2fs: Failed to register file system\n");
        return ret;
    }

    printk(KERN_INFO "s2fs: File system registered\n");
    return 0;
}

void s2fs_exit(void) {
    int ret;

    ret = unregister_filesystem(&s2fs_type);
    if (ret != 0)
        printk(KERN_ERR "s2fs: Failed to unregister file system\n");

    printk(KERN_INFO "s2fs: File system unregistered\n");
}

module_init(s2fs_init);
module_exit(s2fs_exit);

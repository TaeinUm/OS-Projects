#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/rbtree.h>
#include <linux/hashtable.h>
#include <linux/radix-tree.h>
#include <linux/xarray.h>

MODULE_LICENSE("GPL");

static char *int_str = "";
module_param(int_str, charp, S_IRUGO);
MODULE_PARM_DESC(int_str, "Input string containing integers");



// Linked List
struct int_node {
    int value;
    struct list_head list;
};
LIST_HEAD(int_list);

// RB Tree
struct rb_int_node {
  int value;
  struct rb_node rb_node;
};
static struct rb_root mytree = RB_ROOT;

// Hash Table
#define HASH_TABLE_SIZE 1024
struct hash_int_node {
    int value;
    struct hlist_node hnode;
};
static DEFINE_HASHTABLE(myhashtable, 10); // 2^10 = 1024 buckets

// Radix Tree
static RADIX_TREE(my_radix_tree, GFP_KERNEL);

// XArray
DEFINE_XARRAY(my_xarray);

// Bitmap
DECLARE_BITMAP(my_bitmap, 1001);



// RB Tree
static int rb_insert(int value, struct rb_root *root) {
    struct rb_node **new = &(root->rb_node), *parent = NULL;
    struct rb_int_node *this;

    while (*new) {
      this = container_of(*new, struct rb_int_node, rb_node);

      parent = *new;
      if (this->value < value)
        new = &((*new)->rb_right);
      else if (this->value > value)
        new = &((*new)->rb_left);
      else
        return false; // Value already exists.
    }

    this = kmalloc(sizeof(struct rb_int_node), GFP_KERNEL);
        if (!this)
            return -ENOMEM;
    this->value = value;
    rb_link_node(&this->rb_node, parent, new);
    rb_insert_color(&this->rb_node, root);

    return 0;
}

// Hash Table
static void hash_insert(int value) {
    struct hash_int_node *new_node = kmalloc(sizeof(*new_node), GFP_KERNEL);
    if (!new_node) {
        pr_err("Failed to allocate memory for hash table node\n");
        return;
    }

    new_node->value = value;
    hash_add(myhashtable, &new_node->hnode, value);
}

static void hash_lookup_and_print(int value) {
    struct hash_int_node *hash_node;

    hash_for_each_possible(myhashtable, hash_node, hnode, value) {
        if (hash_node->value == value) {
            pr_info("HashTable (possible) value: %d\n", hash_node->value);
        }
    }
}

// Radix Tree
static int radix_tree_insert_num(int num) {
    int *item = kmalloc(sizeof(int), GFP_KERNEL);
    *item = num;

    return radix_tree_insert(&my_radix_tree, *((int *)item), item);
}

static void radix_tree_print(void) {
    struct radix_tree_iter iter;
    void **slot;

    radix_tree_for_each_slot(slot, &my_radix_tree, &iter, 0) {
        pr_info("RadixTree value: %d\n", *(int *)*slot);
    }
}

static void radix_tree_tag_odds(void) {
    struct radix_tree_iter iter;
    void **slot;

    radix_tree_for_each_slot(slot, &my_radix_tree, &iter, 0) {
        if (iter.index & 1) { // Check if the number is odd
            radix_tree_tag_set(&my_radix_tree, iter.index, 1);
        }
    }
}

static void radix_tree_print_tagged(void) {
    void **results;
    unsigned int count;
    int num_found, i = 0;

    results = kmalloc_array(10, sizeof(*results), GFP_KERNEL); // Example for 10 results at a time.
    if (!results)
        return;

    while ((num_found = radix_tree_gang_lookup_tag(&my_radix_tree, results, i, 10, 1)) > 0) {
        for (count = 0; count < num_found; count++) {
	    pr_info("Tagged RadixTree value: %d\n", *(int *)(uintptr_t)results[count]);
        }
	i = (int)(uintptr_t)results[num_found - 1] + 1;
    }

    kfree(results);
}

//XArray
static int xarray_insert_num(int num) {
    int *item = kmalloc(sizeof(int), GFP_KERNEL);
    if (!item)
        return -ENOMEM;
    *item = num;
    xa_store(&my_xarray, num, item, GFP_KERNEL);
    return 0;
}

static void xarray_print(void) {
    unsigned long index = 0;
    int *item;

    xa_for_each(&my_xarray, index, item) {
        pr_info("XArray value: %d\n", *item);
    }
}

static void xarray_tag_odds(void) {
    unsigned long index = 0;
    int *item;

    xa_for_each(&my_xarray, index, item) {
        if (*item & 1)
	    xa_set_mark(&my_xarray, index, XA_MARK_0);
    }
}
static void xarray_print_tagged(void) {
    unsigned long index = 0;
    int *item;

    xa_for_each_marked(&my_xarray, index, item, XA_MARK_0) {
        pr_info("Tagged XArray value: %d\n", *item);
    }
}

static void xarray_clear(void) {
    unsigned long index = 0;
    int *item;

    xa_for_each(&my_xarray, index, item) {
        xa_erase(&my_xarray, index);
        kfree(item);
    }
}




//kds_init
static int __init kds_init(void) {
    char *token;
    char *temp_str = kstrdup(int_str, GFP_KERNEL);
    struct rb_node *rb_node;
    struct int_node *itr, *list_node;
    struct hash_int_node *hash_node;
    int bkt;
    int i;

    if (!temp_str) {
        pr_err("Failed to allocate memory for temporary string\n");
        return -ENOMEM;
    }

    pr_info("kds module loaded with string: %s\n", int_str);

    while ((token = strsep(&temp_str, " "))) {
        int num;

        if (sscanf(token, "%d", &num) == 1 && num >= 0 && num <= 1000) {
            pr_info("Parsed number: %d\n", num);

            // For Linked List
            list_node = kmalloc(sizeof(*list_node), GFP_KERNEL);
            if (!list_node) {
                kfree(temp_str);
                return -ENOMEM;
            }
            list_node->value = num;
            list_add_tail(&list_node->list, &int_list);

            // For Red-black Tree
            rb_insert(num, &mytree);

            // For Hash Table
            hash_insert(num);
	    hash_lookup_and_print(num);


            // For Radix Tree
	    radix_tree_insert_num(num);

	    // For XArray
	    xarray_insert_num(num);

	    // For Bitmap
	    set_bit(num, my_bitmap);
        }
    }
    kfree(temp_str);


    // Print Linked List values
    list_for_each_entry(itr, &int_list, list) {
        pr_info("Linked list value: %d\n", itr->value);
    }

    // Print RB Tree values
    for (rb_node = rb_first(&mytree); rb_node; rb_node = rb_next(rb_node)) {
      pr_info("RBTree value: %d\n", container_of(rb_node, struct rb_int_node, rb_node)->value);
    }

    // Print Hash Table values
    hash_for_each(myhashtable, bkt, hash_node, hnode) {
        pr_info("HashTable value: %d\n", hash_node->value);
    }

    // Radix Tree
    // Print the initial Radix Tree values
    pr_info("Initial Radix Tree Values:\n");
    radix_tree_print();

    // Tag odd numbers in Radix Tree
    radix_tree_tag_odds();

    // Print tagged odd numbers from the Radix Tree
    pr_info("Tagged Odd Radix Tree Values:\n");
    radix_tree_print_tagged();


    // XArray
    // Print the XArray values
    pr_info("Initial XArray Values:\n");
    xarray_print();

    // Tag odd numbers in XArray
    xarray_tag_odds();

    // Print tagged odd numbers from the XArray
    pr_info("Tagged Odd XArray Values:\n");
    xarray_print_tagged();

    // Print Bitmap values
    for (i = 0; i <= 1000; i++) {
        if (test_bit(i, my_bitmap)) {
            pr_info("Bitmap bit turned on for: %d\n", i);
        }
    }

    return 0;
}



static void __exit kds_exit(void) {
    struct int_node *tmp_node, *itr;
    struct rb_node *node, *node_next;
    struct hash_int_node *hash_node;
    struct hlist_node *tmp;
    int bkt;
    void **slot;
    struct radix_tree_iter iter;

    // Remove all inserted numbers in the Linked List
    list_for_each_entry_safe(itr, tmp_node, &int_list, list) {
        list_del(&itr->list);
        kfree(itr);
    }

    // Remove all inserted numbers in the RB Tree
    for (node = rb_first(&mytree); node; node = node_next) {
        node_next = rb_next(node);
        rb_erase(node, &mytree);
        kfree(container_of(node, struct rb_int_node, rb_node));
    }

    // Remove all inserted numbers in the Hash Table
    hash_for_each_safe(myhashtable, bkt, tmp, hash_node, hnode) {
        hash_del(&hash_node->hnode);
        kfree(hash_node);
    }

    // Remove all inserted numbers in the Radix Tree
    radix_tree_for_each_slot(slot, &my_radix_tree, &iter, 0) {
        radix_tree_delete(&my_radix_tree, iter.index);
    }

    // Remove all inserted numbers in the XArray
    xarray_clear();

    // Remove all inserted numbers in Bitmap
    bitmap_zero(my_bitmap, 1001);

    pr_info("kds module unloaded\n");
}

module_init(kds_init);
module_exit(kds_exit);

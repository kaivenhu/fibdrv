#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kdev_t.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/ktime.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/sysfs.h>
#include <linux/uaccess.h>

#include "fibdrv.h"

MODULE_LICENSE("Dual MIT/GPL");
MODULE_AUTHOR("National Cheng Kung University, Taiwan");
MODULE_DESCRIPTION("Fibonacci engine driver");
MODULE_VERSION("0.1");

#define DEV_FIBONACCI_NAME "fibonacci"

static dev_t fib_dev = 0;
static struct cdev *fib_cdev;
static struct class *fib_class;
static DEFINE_MUTEX(fib_mutex);

static ktime_t fib_cal_kt;
static ktime_t kernel_user_kt;

static ssize_t fib_cal_time_show(struct kobject *kobj,
                                 struct kobj_attribute *attr,
                                 char *buf)
{
    return snprintf(buf, PAGE_SIZE, "%llu\n", ktime_to_ns(fib_cal_kt));
}
static struct kobj_attribute fib_cal_time_attribute =
    __ATTR(fib_cal_time, 0444, fib_cal_time_show, NULL);

static ssize_t kernel_to_user_time_show(struct kobject *kobj,
                                        struct kobj_attribute *attr,
                                        char *buf)
{
    return snprintf(buf, PAGE_SIZE, "%llu\n", ktime_to_ns(kernel_user_kt));
}
static struct kobj_attribute kernel_to_user_time_attribute =
    __ATTR(kernel_to_user_time, 0444, kernel_to_user_time_show, NULL);


static struct attribute *attrs[] = {
    &fib_cal_time_attribute.attr,
    &kernel_to_user_time_attribute.attr,
    NULL,
};
static struct attribute_group attr_group = {
    .attrs = attrs,
};

static struct kobject *fibdrv_kobj;

static f_uint fib_sequence(unsigned long long k)
{
    /* FIXME: use clz/ctz and fast algorithms to speed up */
    f_uint f[MAX_LENGTH + 2] = {0};

    f[1] = 1;

    for (int i = 2; i <= k; i++) {
        f[i] = f[i - 1] + f[i - 2];
    }

    return f[k];
}

static int fib_open(struct inode *inode, struct file *file)
{
    if (!mutex_trylock(&fib_mutex)) {
        printk(KERN_ALERT "fibdrv is in use");
        return -EBUSY;
    }
    return 0;
}

static int fib_release(struct inode *inode, struct file *file)
{
    mutex_unlock(&fib_mutex);
    return 0;
}

/* calculate the fibonacci number at given offset */
static ssize_t fib_read(struct file *file,
                        char *buf,
                        size_t size,
                        loff_t *offset)
{
    ktime_t prev_kt;
    unsigned long ret;
    f_uint result;

    prev_kt = ktime_get();
    result = fib_sequence(*offset);
    fib_cal_kt = ktime_sub(ktime_get(), prev_kt);

    prev_kt = ktime_get();
    ret = copy_to_user(buf, &result, sizeof(result));
    kernel_user_kt = ktime_sub(ktime_get(), prev_kt);
    if (!ret) {
        return sizeof(result);
    } else {
        return 0;
    }
}

/* write operation is skipped */
static ssize_t fib_write(struct file *file,
                         const char *buf,
                         size_t size,
                         loff_t *offset)
{
    return 1;
}

static loff_t fib_device_lseek(struct file *file, loff_t offset, int orig)
{
    loff_t new_pos = 0;
    switch (orig) {
    case 0: /* SEEK_SET: */
        new_pos = offset;
        break;
    case 1: /* SEEK_CUR: */
        new_pos = file->f_pos + offset;
        break;
    case 2: /* SEEK_END: */
        new_pos = MAX_LENGTH - offset;
        break;
    }

    if (new_pos > MAX_LENGTH)
        new_pos = MAX_LENGTH;  // max case
    if (new_pos < 0)
        new_pos = 0;        // min case
    file->f_pos = new_pos;  // This is what we'll use now
    return new_pos;
}

const struct file_operations fib_fops = {
    .owner = THIS_MODULE,
    .read = fib_read,
    .write = fib_write,
    .open = fib_open,
    .release = fib_release,
    .llseek = fib_device_lseek,
};

static int __init init_fib_dev(void)
{
    int rc = 0;

    mutex_init(&fib_mutex);

    // Let's register the device
    // This will dynamically allocate the major number
    rc = alloc_chrdev_region(&fib_dev, 0, 1, DEV_FIBONACCI_NAME);

    if (rc < 0) {
        printk(KERN_ALERT
               "Failed to register the fibonacci char device. rc = %i",
               rc);
        return rc;
    }

    fib_cdev = cdev_alloc();
    if (fib_cdev == NULL) {
        printk(KERN_ALERT "Failed to alloc cdev");
        rc = -1;
        goto failed_cdev;
    }
    cdev_init(fib_cdev, &fib_fops);
    rc = cdev_add(fib_cdev, fib_dev, 1);

    if (rc < 0) {
        printk(KERN_ALERT "Failed to add cdev");
        rc = -2;
        goto failed_cdev;
    }

    fib_class = class_create(THIS_MODULE, DEV_FIBONACCI_NAME);

    if (!fib_class) {
        printk(KERN_ALERT "Failed to create device class");
        rc = -3;
        goto failed_class_create;
    }

    if (!device_create(fib_class, NULL, fib_dev, NULL, DEV_FIBONACCI_NAME)) {
        printk(KERN_ALERT "Failed to create device");
        rc = -4;
        goto failed_device_create;
    }

    fibdrv_kobj = kobject_create_and_add("fibdrv", kernel_kobj);
    if (!fibdrv_kobj) {
        rc = -5;
        goto failed_kobj_create;
    }
    /* Create the files associated with this kobject */
    if (sysfs_create_group(fibdrv_kobj, &attr_group)) {
        rc = -6;
        goto failed_kobj_create;
    }

    return rc;
failed_kobj_create:
    kobject_put(fibdrv_kobj);
failed_device_create:
    class_destroy(fib_class);
failed_class_create:
    cdev_del(fib_cdev);
failed_cdev:
    unregister_chrdev_region(fib_dev, 1);
    return rc;
}

static void __exit exit_fib_dev(void)
{
    kobject_put(fibdrv_kobj);
    mutex_destroy(&fib_mutex);
    device_destroy(fib_class, fib_dev);
    class_destroy(fib_class);
    cdev_del(fib_cdev);
    unregister_chrdev_region(fib_dev, 1);
}

module_init(init_fib_dev);
module_exit(exit_fib_dev);

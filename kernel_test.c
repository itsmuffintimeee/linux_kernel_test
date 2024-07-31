#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/cdev.h>

#define DEVICE_NAME "memdev" // 设备名称
#define IOCTL_ALLOC_MEM _IOW('a', 1, size_t) // 分配内存
#define IOCTL_WRITE_MEM _IOW('a', 2, char *) // 写入内存
#define IOCTL_READ_MEM _IOR('a', 3, char *) // 读取内存

// ioctl系统调用的处理函数
static long device_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

static int major; // 主设备号
static char *kernel_buffer = NULL; // 内核缓冲区
static size_t buffer_size = 0; // 缓冲区
static struct class *memdev_class = NULL; // 设备类指针
static struct device *memdev_device = NULL; // 设备指针

// 文件操作结构体
static struct file_operations fops = {
    .owner = THIS_MODULE, // 模块所有者
    .unlocked_ioctl = device_ioctl, // ioctl处理函数
};

// ioctl系统调用的处理函数
static long device_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    switch (cmd) {
        case IOCTL_ALLOC_MEM:
            if (kernel_buffer) {
                kfree(kernel_buffer);  // 如果之前已分配内存，则释放
            }
            buffer_size = arg; // 从用户空间获取分配的内存大小
            kernel_buffer = kmalloc(buffer_size, GFP_KERNEL); // 分配内存
            if (!kernel_buffer) {
                return -ENOMEM; // 内存不足
            }
            break;
        case IOCTL_WRITE_MEM:
            if (copy_from_user(kernel_buffer, (char *)arg, buffer_size)) {
                return -EFAULT; // 无效地址
            }
            break;
        case IOCTL_READ_MEM:
            if (copy_to_user((char *)arg, kernel_buffer, buffer_size)) {
                return -EFAULT; // 无效地址
            }
            break;
        default:
            return -EINVAL; // 无效参数
    }
    return 0;
}



// 模块初始化函数
static int __init memdev_init(void) {
    major = register_chrdev(0, DEVICE_NAME, &fops); // 注册字符设备
    if (major < 0) {
        printk(KERN_ALERT "注册字符设备失败 %d\n", major);
        return major;
    }

    memdev_class = class_create(THIS_MODULE, DEVICE_NAME); // 创建设备类
    if (IS_ERR(memdev_class)) {
        unregister_chrdev(major, DEVICE_NAME); // 注销字符设备
        return PTR_ERR(memdev_class);
    }

    memdev_device = device_create(memdev_class, NULL, MKDEV(major, 0), NULL, DEVICE_NAME); // 创建设备
    if (IS_ERR(memdev_device)) {
        class_destroy(memdev_class); // 销毁设备类
        unregister_chrdev(major, DEVICE_NAME); // 注销字符设备
        return PTR_ERR(memdev_device);
    }

    printk(KERN_INFO "memdev: 注册设备号为 %d\n", major);
    return 0;
}

// 模块清理函数
static void __exit memdev_exit(void) {
    device_destroy(memdev_class, MKDEV(major, 0)); // 销毁设备
    class_unregister(memdev_class); // 注销设备类
    class_destroy(memdev_class); // 销毁设备类
    unregister_chrdev(major, DEVICE_NAME); // 注销字符设备
    if (kernel_buffer) {
        kfree(kernel_buffer); // 如果内存已分配，释放内存
    }
    printk(KERN_INFO "memdev: 设备注销\n");
}

module_init(memdev_init); // 模块初始化函数
module_exit(memdev_exit); // 模块清理函数

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("使用ioctl分配内存模块");

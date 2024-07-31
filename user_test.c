#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#define DEVICE_NAME "/dev/memdev" // 设备文件路径
#define IOCTL_ALLOC_MEM _IOW('a', 1, size_t) // 分配内存
#define IOCTL_WRITE_MEM _IOW('a', 2, char *) // 写内存
#define IOCTL_READ_MEM _IOR('a', 3, char *) // 读内存

size_t buffer_size = 0; // 缓冲区大小

// 将文件内容写入内核内存
void write_to_kernel_memory(const char *filename) {
    int fd = open(DEVICE_NAME, O_RDWR); // 打开设备文件
    if (fd < 0) {
        perror("打开设备失败");
        exit(1);
    }

    struct stat st;
    if (stat(filename, &st) != 0) { 
        perror("获取文件状态失败");
        close(fd);
        exit(1);
    }

    buffer_size = st.st_size; // 获取文件大小
    ioctl(fd, IOCTL_ALLOC_MEM, buffer_size); // 分配内存

    FILE *file = fopen(filename, "r"); // 打开输入文件
    if (!file) {
        perror("打开输入文件失败");
        close(fd);
        exit(1);
    }

    char *buffer = (char *)malloc(buffer_size); // 分配缓冲区
    fread(buffer, 1, buffer_size, file); // 读取文件内容到缓冲区
    fclose(file); // 关闭文件

    ioctl(fd, IOCTL_WRITE_MEM, buffer); // 调用ioctl命令写入内存

    free(buffer);
    close(fd);
}

// 从内核内存中读取内容并写入文件
void read_from_kernel_memory(const char *filename) {
    int fd = open(DEVICE_NAME, O_RDWR); // 打开设备文件
    if (fd < 0) {
        perror("打开设备失败");
        exit(1);
    }

    char *buffer = (char *)malloc(buffer_size); // 分配缓冲区
    ioctl(fd, IOCTL_READ_MEM, buffer); // 调用ioctl命令从内存读取

    FILE *file = fopen(filename, "w"); // 打开输出文件
    if (!file) {
        perror("打开出书文件失败");
        free(buffer);
        close(fd);
        exit(1);
    }

    fwrite(buffer, 1, buffer_size, file); // 写入缓冲区内容到文件
    fclose(file); // 关闭文件

    free(buffer); // 释放缓冲区
    close(fd); // 关闭设备文件
}

// 主函数
int main(int argc, char *argv[]) {
    // 判断参数数量
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <input_file> <output_file>\n", argv[0]); // 打印用法
        exit(1);
    }

    write_to_kernel_memory(argv[1]); // 将输入文件内容写入内核内存
    read_from_kernel_memory(argv[2]); // 从内核内存读取内容并写入输出文件

    return 0;
}

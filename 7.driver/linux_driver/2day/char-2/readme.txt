ioctl

ioctl在新版本的内核中不可用   改为unsigned ioctl形式即可

1.file_operations中将ioctl改为unlocked_ioctl
2.将hello_ioctl中的第一个参数删除
3.hello_ioctl的返回值为long，将前面的int改为long

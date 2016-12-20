一、open函数的实现
hello_open(struct inode *inode,struct file *file)
{
	增加使用计数，检查错误
	如果为初始化，则调用初始化
	识别次设备号，如果必要，更新f_op指针
	分配并填写被置于filp->private_data的数据结构
}
注意：open函数会被应用层的open调用
      程序中不一定一定要写open，如果不写，则操作系统会自动添加一个默认的open函数，open函数中也可以什么都不做，直接返回

二、release函数的实现
hello_release(struct inode *inode,struct file *file)
{
	open的逆操作
}
注意：release函数会被应用程序的close函数调用

三、read函数的实现
    read函数会被应用层的read函数调用
hello_read(struct file *file,char __user *buf,size_t count,loff_t *loff)
参数2.用户空间数据存放地址，指针传入
    3.用户希望读取的数据长度，单位为字节
在read函数中通过：copy_to_user(buf,data,count)将数据从内核空间data中复制到用户空间buf中，再通过buf传送给用户程序，该函数的返回值比较特殊，需注意：成功时返回0，失败时返回一个正数

四、write函数的实现
    write函数会被应用层的write函数调用
hello_write(struct file *file,const char __user *buf,size_t count,loff_t *loff)
参数2.用户空间中要写入的数据存放地址，指针方式传入
    3.用户要写入的数据长度，单位为字节
在write函数中通过：copy_from_user(data,buf,count)将数据从用户空间的buf中复制到内核空间的data中，该函数的返回值比较特殊，成功时返回0，失败时返回一个正数
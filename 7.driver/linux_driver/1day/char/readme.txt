在加载函数中：
申请设备号：静态和动态两种方式

申请完了以后要注册设备：即向内核填充一个struct cdev结构体(为内核提供对字符设备的管理方法，每个设备必须有一个结构快进行管理)，向内核添加一个字符设备就是添加一个struct cdev结构体，从内核删除一个字符设备就是删除一个struct cdev结构体。

struct cdev cdev
static struct file_operations hello_fops = {.owner = THIS_MODULE，}；
1.初始化字符设备驱动控制块：cdev_init(要初始化的字符设备驱动控制块 指针 &cdev，绑定给cdev的API函数集 指针 &hello_fops)
2.向内核添加字符设备驱动控制块：cdev_add(&cdev，与该控制块相对的设备号devno，number_of_device),添加失败时应该释放掉设备号
3.在卸载函数中注销设备控制块：cdev_del(&cdev)，然后释放设备号

注意：
1.在static struct file_operations hello_fops中可以添加接口函数，以便应用层调用
2.在向内核注册了设备控制块后，内核便可以管理该设备，并用给设备为应用层服务，服务的接口就是static struct file_operations hello_fops中定义的函数。
3.insmod加载模块
4.lsmod可查看加载到内核的模块
5.dmesg可以查看到insmod调用的加载函数中的打印信息
6.cat /proc/devices可看到申请的设备号以及设备的名字

7.此时可以创建设备节点：mknod /dev/hello c 250 0  (dev下的设备名要和申请时添的名字一样，c表示是一个字符设备，250为主设备号，0为次设备号)  创建好的设备节点如果不删除则会一直存在，删除时用：sudo rm /dev/hello
  在/dev目录下创建一个字符设备节点，此时应用程序使用open("/dev/hello",O_RDWR)可以打开该设备，内核通过/dev/hello以及250 0 自动寻找到该字符设备驱动模块，这样应用程序和驱动程序就通过设备节点连接起来了，应用程序通过读写设备节点进而调用驱动为其服务，应用程序中的函数和驱动中的函数有对应关系，只是参数不同，他们通过虚拟文件系统NFS进行转换。(此处驱动中没有open函数，字符设备内核系统自动添加了一个默认的open函数用于与应用程序的open函数对应)


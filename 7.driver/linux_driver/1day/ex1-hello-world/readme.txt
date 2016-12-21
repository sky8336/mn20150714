该模块中包括
1.许可证声明：MODULE_LICENSE
2.加载函数：init_module    ----insmod
3.卸载函数：clearnup_module    ------rmmod
注：这种方式只能静态加载，如果要动态加载，则写成ext2-init-exit的形式(这种形式既可以静态加载，也可以动态加载，如果不加最后的两行，则只能动态加载，不能静态加载)。


实验步骤：
1.编译：make
2.向内核中插入一个模块，即加载：sudo insmod hello.ko
3.查看动态加载的模块：lsmod | grep hello(只查看hello模块)   或者 lsmod可查看全部模块
4.卸载模块：sudo rmmod hello(模块名是lsmod查看得到的那个名字，没有.ko)
5.加载或卸载后查看加载和卸载的详细打印信息：dmesg      清除打印在屏幕的信息：sudo dmesg -c
6.删除编译的模块：make clean   可删除编译的所有文件
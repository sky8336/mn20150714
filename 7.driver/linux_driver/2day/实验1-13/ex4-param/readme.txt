方法如同ex1。

insmod后用dmesg可查看程序中传给模块的参数

在insmod时可以修改参数，举例如下
sudo insmod hello.ko myshort=1 myint=1 mylong=1 mystring="mahongwei" array=1,2  
注意：myshort=1等中间不能有空格，否则系统会当三个参数处理，也可只改变其中的几个。
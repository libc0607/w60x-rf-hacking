# 关于扩展中心频率的一点探索

W600仅有libwlan没有开源，但是保留了符号。
首先用7z解压，其中的1.txt记录了每个.o文件中对外提供的函数接口

![7z](https://github.com/libc0607/w60x-rf-hacking/blob/main/screenshots/20201101141251.jpg)

![1txt](https://github.com/libc0607/w60x-rf-hacking/blob/main/screenshots/20201101141230.jpg)


通过搜索channel等关键词，觉得tls_wl_rf.o中应该有比较有趣的东西
载入IDA Pro，看到函数列表

![func-list](https://github.com/libc0607/w60x-rf-hacking/blob/main/screenshots/20201101140901.jpg)

wm_rf_set_channel很是显眼，反编译出如图

![wm_rf_set_channel很是显眼](https://github.com/libc0607/w60x-rf-hacking/blob/main/screenshots/20201101141313.jpg)


这里明显调用了一个 rf_spi_write()，内部实现如下图
《补个图》

看到这里直接操作了寄存器，查找手册中的寄存器分布

![reg1](https://github.com/libc0607/w60x-rf-hacking/blob/main/screenshots/20201101144440.jpg)

![reg2](https://github.com/libc0607/w60x-rf-hacking/blob/main/screenshots/20201101145004.jpg)


可以猜测，他们在Wi-Fi部分使用的频率合成器是通过SPI接口控制的，他们在APB上挂了一个简易的SPI控制器用来控制频率合成器
那接下来的方向就是找出他们往频率合成器中写了什么数据


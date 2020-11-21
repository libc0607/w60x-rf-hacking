### 一个简单的跳频点灯 Demo

代码在 [w60x-rf-rx/ （接收端）](https://github.com/libc0607/w60x-rf-hacking/tree/main/w60x-rf-rx) 和 [w60x-rf-tx/ （发射端）](https://github.com/libc0607/w60x-rf-hacking/tree/main/w60x-rf-tx)  

这些代码需要运行在两块 W60x 开发板上，我这里使用了两块 TB-01  

这个 Demo 的功能描述如下：  

发射端会循环在 2.17GHz ~ 2.87GHz 之间，以 100MHz 步进跳频，发送一个包含自定义信息的 RTS 包；
每发送一个包改变一次 PB_14 上连接的 LED 的状态  

接收端上电后会在其中一个频点监听，当收到包后便跟随跳到下一个频点；  
每收到一个包改变一次 PB_14 上连接的 LED 的状态

所以如果它们可以正常通信，两边的 LED 会一起闪  



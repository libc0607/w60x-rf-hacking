# 关于扩展中心频率的一点探索  

W600仅有libwlan没有开源，但是保留了符号。  
首先用7z解压，其中的1.txt记录了每个.o文件中对外提供的函数接口  

![7z](https://github.com/libc0607/w60x-rf-hacking/blob/main/screenshots/20201101141251.jpg)

![1txt](https://github.com/libc0607/w60x-rf-hacking/blob/main/screenshots/20201101141230.jpg)


通过搜索channel等关键词，觉得tls_wl_rf.o中应该有比较有趣的东西  
载入IDA Pro，看到函数列表  

![func-list](https://github.com/libc0607/w60x-rf-hacking/blob/main/screenshots/20201101140901.jpg)

wm_rf_set_channel很是显眼，反编译出如下 

```
int *__fastcall wm_rf_set_channel(int a1, unsigned int a2)
{
  unsigned int v2; // r4
  int v3; // r5
  int v4; // r0
  int v5; // r1
  unsigned int v6; // r0
  int v7; // r6
  int v8; // r0
  int *result; // r0

  v2 = a2;
  v3 = a1;
  if ( a2 <= 1 )
  {
    rf_spi_write(rf_chan24g_20m[2 * a1]);
    rf_spi_write(rf_chan24g_20m[2 * v3 + 1]);
    v4 = wm_rf_band_switch(0);
    v5 = tls_os_set_critical(v4);
    v6 = MEMORY[0x40001400] & 0xFFFFFFDF;
LABEL_8:
    MEMORY[0x40001400] = v6;
    tls_os_release_critical(v5);
    goto LABEL_9;
  }
  if ( a2 <= 3 )
  {
    if ( a2 == 2 )
      v7 = central_freq[2 * a1];
    else
      v7 = central_freq[2 * a1 + 1];
    rf_spi_write(rf_chan24g_20m[2 * v7]);
    rf_spi_write(rf_chan24g_20m[2 * v7 + 1]);
    v8 = wm_rf_band_switch(1);
    v5 = tls_os_set_critical(v8);
    if ( v2 == 2 )
      v6 = MEMORY[0x40001400] & 0xFFFEFF3F | 0xA0;
    else
      v6 = MEMORY[0x40001400] & 0xFFFFFF3F | 0x100E0;
    goto LABEL_8;
  }
LABEL_9:
  result = &hed_rf_current_channel;
  hed_rf_current_channel = v3;
  *(_DWORD *)&hed_rf_chan_type = v2;
  return result;
}
```



这里明显调用了一个 rf_spi_write()
```
int __fastcall rf_spi_write(int a1)
{
  MEMORY[0x40011408] = a1 << 10;
  tls_wl_delay(1);
  MEMORY[0x40011400] = 1;
  while ( MEMORY[0x40011404] & 0x40000 )
    ;
  return tls_wl_delay(1);
}
int __fastcall rf_spi_read(int a1)
{
  MEMORY[0x40011408] = (a1 << 27) | 0x4000000;
  tls_wl_delay(10);
  MEMORY[0x40011400] = 1;
  while ( MEMORY[0x40011404] & 0x40000 )
    ;
  tls_wl_delay(200);
  return MEMORY[0x4001140C] >> 16;
}
```

看到这里直接操作了寄存器，查找手册中的寄存器分布  

![reg1](https://github.com/libc0607/w60x-rf-hacking/blob/main/screenshots/20201101144440.jpg)  

![reg2](https://github.com/libc0607/w60x-rf-hacking/blob/main/screenshots/20201101145004.jpg)  


可以猜测，他们在Wi-Fi部分使用的频率合成器是通过SPI接口控制的，他们在APB上挂了一个简易的SPI控制器用来控制频率合成器  
根据读写函数，容易猜测该简易SPI控制器的寄存器 MEMORY[0x40011408] 的结构为  
```
5'b<addr> + 1'b<read/write> + 16'b<data> + 10'b <unknown/padding>
```

那接下来的方向就是找出他们往频率合成器中写了什么数据  

wm_rf_set_channel 这个函数中涉及了两个表：rf_chan24g_20m[] 与 central_freq[]  
```
.data:000009B4                 EXPORT rf_chan24g_20m
.data:000009B4 ; _DWORD rf_chan24g_20m[30]
.data:000009B4 rf_chan24g_20m  DCD 0, 0, 0x2A203, 0x43333, 0x2A204, 0x48888, 0x2A205
.data:000009B4                                         ; DATA XREF: wm_rf_set_channel+4↑o
.data:000009B4                                         ; wm_rf_set_channel:loc_1BA↑o ...
.data:000009B4                 DCD 0x4DDDD, 0x2A207, 0x43333, 0x2A208, 0x48888, 0x2A209
.data:000009B4                 DCD 0x4DDDD, 0x2A20B, 0x43333, 0x2A20C, 0x48888, 0x2A20D
.data:000009B4                 DCD 0x4DDDD, 0x2A20F, 0x43333, 0x2A600, 0x48888, 0x2A601
.data:000009B4                 DCD 0x4DDDD, 0x2A603, 0x43333, 0x2A606, 0x46666
```
```
.constdata:000008BC                 EXPORT central_freq
.constdata:000008BC ; _DWORD central_freq[30]
.constdata:000008BC central_freq    DCD 0, 0, 3, 3, 4, 4, 1, 5, 2, 6, 3, 7, 4, 8, 5, 9, 6
.constdata:000008BC                                         ; DATA XREF: wm_rf_set_channel+3C↑o
.constdata:000008BC                                         ; wm_rf_init:off_438↑o ...
.constdata:000008BC                 DCD 0xA, 7, 0xB, 8, 0xC, 9, 0xD, 0xA, 0xA, 0xB, 0xB, 0 ; Alternative name is 'BuildAttributes$$THM_ISAv4$P$D$K$B$S$PE$A:L22UL41UL21$X:L11$S22US41US21$IEEE1$IW$USESV6$~STKCKD$USESV7$~SHL$OSPACE$EBA8$REQ8$PRES8$EABIv2'
.constdata:000008BC                 DCD 0
.constdata:000008BC ; .constdata    ends
.constdata:000008BC
```

这种 0x3333, 0x8888, 0xDDDD 的感觉很像是频率合成器的小数部分，而central_freq这个一看就明白了是给ht40-和ht40+用的。。  
这样 wm_rf_set_channel 的两个参数就容易明白了，分别为频道与带宽模式  
结合前面猜出的结构，整理往 rf synth 中某个寄存器可能写入的数据如下：
```
const uint32_t rf_chan24g_20m[] = {
	
//	<reg:0x1>	<reg:0x2>	chan 	freq	reg			    int		frac
	0x2A203, 	0x43333, //	1		  2412	0xA2033333	40		0x33333
	0x2A204, 	0x48888, // 2		  2417	0xA2048888	40		0x48888
	0x2A205, 	0x4DDDD, // 3		  2422	0xA205DDDD	40		0x5DDDD
	0x2A207, 	0x43333, // 4		  2427	0xA2073333	40		0x73333
	0x2A208, 	0x48888, // 5		  2432	0xA2088888	40		0x88888
	0x2A209, 	0x4DDDD, // 6		  2437	0xA209DDDD	40		0x9DDDD
	0x2A20B, 	0x43333, // 7		  2442	0xA20B3333	40		0xB3333
	0x2A20C, 	0x48888, // 8		  2447	0xA20C8888	40		0xC8888
	0x2A20D, 	0x4DDDD, // 9		  2452	0xA20DDDDD	40		0xDDDDD
	0x2A20F, 	0x43333, // 10		2457	0xA20F3333	40		0xF3333
	0x2A600, 	0x48888, // 11		2462	0xA6008888	41		0x08888
	0x2A601, 	0x4DDDD, // 12		2467	0xA601DDDD	41		0x1DDDD
	0x2A603, 	0x43333, // 13		2472	0xA6033333	41		0x33333
	0x2A606, 	0x46666, // 14		2484	0xA6066666	41		0x66666
}
```

这里的推导过程省略，主要是观察到 1.在2457-2462之间发生了进位 2. 在1-10频道中每次增加的小数分频值代表5MHz   
最终得出 W60x 的频率合成器计算公式为   
```
F = 60 MHz * (int + frac/0xFFFFF)
```
并且该寄存器的结构为   
```
6'b<int> + 6'b<unknown???> + 20'b<frac>
```
最后作为老缝合怪，可以在 arduino 环境中糊出一堆勉强能用的测试代码  
在我的这块 TB-01 开发板上，频率范围在 2161MHz~2931MHz 之间调节时，频谱都没有出现明显不对劲（PLL锁上了大概  
但射频性能在 Wi-Fi 频段以外会明显劣化，故不建议使用偏移过远的中心频率  
```
// Notice that 'extern "C"' 
extern "C" {
  #include <wm_wifi.h>
  #include <wm_osal.h>
}

void _rf_spi_w(int arg)
{
  *((uint32_t*)0x40011408) = arg << 10;    
  delayMicroseconds(1);
  *((uint32_t*)0x40011400) = 1;
  while(*((uint32_t*)0x40011404) & 0x40000);
  delayMicroseconds(1);
  return;
}

void w60x_set_center_frequency_20mbw(uint32_t freq_mhz) 
{
  uint32_t spi_reg1, spi_reg2, reg_synth;
  uint32_t chan_div_int, chan_div_frac;
  uint32_t cpu_sr;
  
  chan_div_int = 0x0000003F & (freq_mhz/60);
  chan_div_frac = 0x000FFFFF & ( ((freq_mhz-(chan_div_int*60)) * (0xFFFFF)) / 60 );
  
  reg_synth = 0x02000000 | (chan_div_int << 26) | (chan_div_frac);
  spi_reg1 = 0x00020000 | (reg_synth >> 16);
  spi_reg2 = 0x00040000 | (reg_synth & 0x0000FFFF);
  
  _rf_spi_w(spi_reg1);
  _rf_spi_w(spi_reg2);

  // wm_rf_band_switch @20MHz BW
  _rf_spi_w(0x102640);  //write(0x8, 0x2640)
  _rf_spi_w(0x146967);  //write(0xA, 0x6967)
  _rf_spi_w(0x102641);  //write(0x8, 0x2641)
  delayMicroseconds(180);
  _rf_spi_w(0x102740);  //write(0x8, 0x2740)
  _rf_spi_w(0x145327);  //write(0xA, 0x5327)
  _rf_spi_w(0x102741);  //write(0x8, 0x2741)
  delayMicroseconds(180);
  _rf_spi_w(0x1027C1);  //write(0x8, 0x27C1)
  _rf_spi_w(0x10CDC0);  //write(0x8, 0xCDC0)
  // end of wm_rf_band_switch @20MHz BW

  // I don't know what's 0x40001400 controls
  cpu_sr  = tls_os_set_critical();
  *((uint32_t*)0x40001400) = *((uint32_t*)0x40001400) & 0xFFFFFFDF;
  tls_os_release_critical(cpu_sr);  

  return;
}

```

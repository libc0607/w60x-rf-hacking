# 关于 5MHz/10MHz 带宽
W60X 传输时占用的频宽可以通过调节 WLAN 系统根时钟分频系数来改变。  
蛋疼的事是APB时钟又从这个时钟分频而来，改变这里的时钟会导致APB下面挂的所有设备跟着改变；  
即，假如写了 Serial.begin(230400); 又写了 w60x_set_wlan_channel_bw(10);  那串口实际只有 115200。  
关于更改时钟分频系数的部分，寄存器手册里说得比较清楚， 直接上代码：  
```
// usable bw_mhz: 20(default), 10, 5
// note1: this func assume cpu is running at 80MHz
// note2: this func affect all devices' clock under APB
// (e.g. set channel bw to 10MHz & set UART to 115200 ==> you'll get a 57600 UART)
void w60x_set_wlan_channel_bw(uint32_t bw_mhz)
{
  uint32_t reg, clk_mhz;
  uint8_t wlan_div, bus2_factor; 
  
  reg = *((uint32_t*)0x40000710);
  clk_mhz = bw_mhz * 8;
  
  switch(clk_mhz) {
  case 80:
    wlan_div = 2;
    bus2_factor = 4;
  break;
  case 40:
    wlan_div = 4;
    bus2_factor = 8;
  break;
  default:  // 160mhz
    wlan_div = 1;
    bus2_factor = 2;
  break;
  }
  
  reg &= ~(0x00000FF0);
  reg |= (wlan_div <<4);
  reg |= (bus2_factor <<8);

  *((uint32_t*)0x40000710) = reg;
  delayMicroseconds(1);
  *((uint32_t*)0x40000710) = reg | 0x80000000;
  delayMicroseconds(1);
  
  return;
}
```

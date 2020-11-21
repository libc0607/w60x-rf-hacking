// Github @libc0607

#include <Arduino.h>
#include <W600WiFi.h>
extern "C" {
#include <wm_wifi.h>
#include <wm_osal.h>
}

#define LED_PIN  PB_14

uint8_t packet_counter = 0;
tls_wifi_tx_rate_t rate_t;
uint32_t freq_hopping_list[8] = {2170, 2270, 2370, 2470, 2570, 2670, 2770, 2870};

uint8_t ieee_packet_rts[] = {
  // rts header
  0xb4, 0x01, 
  0x00, 0x00, 
  0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  // custom payload
  0xde, 0xad, 0xbe, 0xef,   
  0x00, 0x00, 0x00, 0x00, 
};

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

  Serial.println(reg);
  
  *((uint32_t*)0x40000710) = reg;
  delayMicroseconds(1);
  *((uint32_t*)0x40000710) = reg | 0x80000000;
  delayMicroseconds(1);
  
  return;
}

void setup() {
  Serial.begin(230400);    // Initialize the UART0 TX PA4 RX PA5
  Serial.println("Demo Start");
  pinMode(LED_PIN, OUTPUT);    
  digitalWrite(LED_PIN, HIGH);
  
  tls_wifi_change_chanel(0);
  w60x_set_wlan_channel_bw(10);
  //w60x_set_center_frequency_20mbw(2312);
  rate_t.tx_rate = WM_WIFI_TX_RATEIDX_6M;
  rate_t.tx_gain = tls_wifi_get_tx_gain_max(rate_t.tx_rate);
}

void loop() {
  packet_counter++;
  Serial.println(packet_counter);
  
  w60x_set_center_frequency_20mbw(freq_hopping_list[packet_counter%8]);
  delay(1);
  
  ieee_packet_rts[sizeof(ieee_packet_rts)-4] = packet_counter;
  tls_wifi_send_data(NULL, ieee_packet_rts, sizeof(ieee_packet_rts), &rate_t);
  
  digitalWrite(LED_PIN, digitalRead(LED_PIN) == LOW? HIGH: LOW);
  delay(100);
}

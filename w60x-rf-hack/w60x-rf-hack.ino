/*
  WinnerMicro W600 Wi-Fi Center Frequency Hacking Demo
  
  Github @libc0607
*/

#include <Arduino.h>
#include <W600WiFi.h>

extern "C" {
  #include <wm_wifi.h>
  #include <wm_osal.h>
}

#define LED_PIN  PB_14

uint8_t send_data[] = {
  // rts header
  0xb4, 0x01, 
  0x00, 0x00, 
  0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  // custom payload
  0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 
  0xde, 0xad, 0xbe, 0xef
};

// arg: 10'b<padding> + 5'b<reg-addr> + 1'b<r/w> + 16'b<reg-data> 
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

void setup() {
  Serial.begin(115200);    // Initialize the UART0 TX PA4 RX PA5
  Serial.println("Demo Start");
  pinMode(LED_PIN, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
  
  
  tls_wifi_change_chanel(0);
  //tls_wifi_set_listen_mode(1);
  
}

uint32_t freq = 2412;

void loop() {
  tls_wifi_tx_rate_t rate_t;
  rate_t.tx_rate = WM_WIFI_TX_RATEIDX_6M;
  rate_t.tx_gain = tls_wifi_get_tx_gain_max(rate_t.tx_rate);
  for (int i=0; i<2000; i++) {
    tls_wifi_send_data(NULL, send_data, sizeof(send_data), &rate_t);
    delayMicroseconds(100);
  }
  freq++;
  w60x_set_center_frequency_20mbw(freq);
  Serial.println(freq);
}

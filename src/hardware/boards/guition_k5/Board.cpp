#include "Board.h"
#include "PanelInit.h"
#include <Wire.h>
#include <esp_heap_caps.h>
#include <esp_timer.h>
#include <algorithm>
namespace voltra::guition {
HardwareCapabilities Board::capabilities() const {
    HardwareCapabilities c;
    c.displayWidth=c.displayHeight=resolution;c.displayShape=DisplayShape::Round;
    c.touchAvailable=c.rotaryAvailable=c.rotaryPressAvailable=ready_;
#ifdef VOLTRA_PHYSICAL
    c.bluetoothAvailable=true;
#endif
    return c;
}
uint64_t Board::nowMs() const { return uint64_t(esp_timer_get_time())/1000; }
bool Board::transfer(uint8_t reg,const void* data,size_t bytes,bool quad) {
    spi_transaction_t t{};t.cmd=quad ? 0x32 : 0x02;t.addr=uint32_t(reg)<<8;
    t.tx_buffer=data;t.length=bytes*8;t.flags=quad ? SPI_TRANS_MODE_QIO : 0;
    const auto err=spi_device_polling_transmit(spi_,&t);
    if(err!=ESP_OK) Serial.printf("SPI failure %d\n",int(err));
    return err==ESP_OK;
}
bool Board::begin() {
    // Arduino reports allocator-usable PSRAM, slightly below the physical 8 MiB.
    constexpr uint32_t minimumUsablePsram=7*1024*1024;
    if(!psramFound() || ESP.getPsramSize()<minimumUsablePsram || ESP.getFlashChipSize()!=16*1024*1024) {
        Serial.println("Unexpected memory geometry; hardware init stopped");return false;
    }
    // Keep small objects internal; LVGL layers and its full-frame buffer use PSRAM.
    heap_caps_malloc_extmem_enable(4096);
    ledcSetup(0,5000,10);ledcAttachPin(backlightPin,0);ledcWrite(0,0);
    pixels_=static_cast<uint8_t*>(heap_caps_malloc(resolution*rowsPerTransfer*2,MALLOC_CAP_DMA|MALLOC_CAP_INTERNAL));
    if(!pixels_) return false;
    spi_bus_config_t bus{};bus.sclk_io_num=clockPin;bus.mosi_io_num=d0Pin;bus.miso_io_num=d1Pin;
    bus.data4_io_num=bus.data5_io_num=bus.data6_io_num=bus.data7_io_num=-1;
    bus.quadwp_io_num=d2Pin;bus.quadhd_io_num=d3Pin;bus.max_transfer_sz=resolution*rowsPerTransfer*2;
    if(spi_bus_initialize(SPI2_HOST,&bus,SPI_DMA_CH_AUTO)!=ESP_OK) return false;
    spi_device_interface_config_t device{};device.command_bits=8;device.address_bits=24;
    device.clock_speed_hz=20000000;device.mode=0;device.spics_io_num=csPin;
    device.queue_size=1;device.flags=SPI_DEVICE_HALFDUPLEX;
    if(spi_bus_add_device(SPI2_HOST,&device,&spi_)!=ESP_OK) return false;
    pinMode(resetPin,OUTPUT);digitalWrite(resetPin,LOW);delay(20);digitalWrite(resetPin,HIGH);delay(150);
    uint8_t format=0x55,orientation=0;
    if(!transfer(0x36,&orientation,1) || !transfer(0x3a,&format,1)) return false;
    for(const auto& c:panelCommands) { if(!transfer(c.reg,c.data,c.size)) return false;if(c.waitMs) delay(c.waitMs); }
    if(!transfer(0x21,nullptr,0) || !transfer(0x29,nullptr,0)) return false;
    pinMode(touchReset,OUTPUT);digitalWrite(touchReset,LOW);delay(10);digitalWrite(touchReset,HIGH);delay(100);
    pinMode(touchIrq,INPUT_PULLUP);Wire.begin(touchSda,touchScl,400000);Wire.setTimeOut(20);
    Wire.beginTransmission(0x15);
    if(Wire.endTransmission()!=0) { Serial.println("CST816 not responding; init stopped");return false; }
    // CST816S DisAutoSleep register; keep touch responsive while UI is awake.
    Wire.beginTransmission(0x15);Wire.write(0xFE);Wire.write(1);Wire.endTransmission();
    pinMode(encoderA,INPUT_PULLUP);pinMode(encoderB,INPUT_PULLUP);pinMode(pressPin,INPUT_PULLUP);
    encoderState_=(digitalRead(encoderA)<<1)|digitalRead(encoderB);
    attachInterruptArg(encoderA,encoderInterrupt,this,CHANGE);attachInterruptArg(encoderB,encoderInterrupt,this,CHANGE);
    ready_=true;ledcWrite(0,700);
    Serial.println("Display initialized; touch ACK; encoder inputs active.");return true;
}
void IRAM_ATTR Board::encoderInterrupt(void* context) {
    auto* b=static_cast<Board*>(context);
    const uint32_t now=uint32_t(esp_timer_get_time());
    const uint8_t next=(digitalRead(encoderA)<<1)|digitalRead(encoderB);
    portENTER_CRITICAL_ISR(&b->mux_);
    // Each contact independently pulses low once per click in its direction.
    // Count release after a stable low pulse; short contact bounces do not count.
    // Interrupt capture preserves clicks while the display is transferring.
    for(int channel=0;channel<2;++channel) {
        const uint8_t mask=channel==0 ? 2 : 1;
        if((next&mask)==(b->encoderState_&mask)) continue;
        if(!(next&mask)) {
            b->pulseStarted_[channel]=now;b->pulseActive_[channel]=true;
        } else {
            if(b->pulseActive_[channel] && uint32_t(now-b->pulseStarted_[channel])>=minimumPulseUs)
                b->clicks_+=channel==0 ? 1 : -1;
            b->pulseActive_[channel]=false;
        }
    }
    b->encoderState_=next;
    portEXIT_CRITICAL_ISR(&b->mux_);
}
int Board::getDelta() {
    portENTER_CRITICAL(&mux_);const int delta=clicks_;clicks_=0;portEXIT_CRITICAL(&mux_);
    if(delta) Serial.printf("Knob %d\n",delta*encoderDirection);
    return delta*encoderDirection;
}
void Board::poll() {
    const uint32_t now=millis();const bool down=digitalRead(pressPin)==LOW;
    if(down!=pressCandidate_) {pressCandidate_=down;pressChanged_=now;}
    if(now-pressChanged_>=30 && down!=pressStable_) {pressStable_=down;if(down) {pressed_=true;Serial.println("Press");}}
    if(now-lastTouch_<8) return;lastTouch_=now;
    Wire.beginTransmission(0x15);Wire.write(0x02);
    if(Wire.endTransmission(false)!=0 || Wire.requestFrom(uint8_t(0x15),uint8_t(5))!=5) {touchPoint_.pressed=false;return;}
    uint8_t data[5];for(auto& byte:data) byte=Wire.read();
    const int x=((data[1]&15)<<8)|data[2],y=((data[3]&15)<<8)|data[4];
    const bool touched=(data[0]&15)>0 && x<resolution && y<resolution;
    if(touched && !touchPoint_.pressed) Serial.printf("Touch %d,%d\n",x,y);
    if(touched) touchPoint_={x/float(resolution-1),y/float(resolution-1),true};
    else touchPoint_.pressed=false;
}
void Board::flush(int x,int y,int w,int h,const uint32_t* source) {
    if(!ready_ || x<0 || y<0 || w<=0 || h<=0 || x+w>resolution || y+h>resolution) return;
    for(int row=0;row<h;row+=rowsPerTransfer) {
        const int count=std::min(rowsPerTransfer,h-row);
        for(int i=0;i<w*count;++i) {
            const uint32_t rgb=source[row*w+i];
            const uint16_t c=((rgb>>8)&0xf800)|((rgb>>5)&0x07e0)|((rgb>>3)&0x001f);
            pixels_[2*i]=c>>8;pixels_[2*i+1]=c&255;
        }
        const int xEnd=x+w-1,yStart=y+row,yEnd=yStart+count-1;
        uint8_t cols[]={uint8_t(x>>8),uint8_t(x),uint8_t(xEnd>>8),uint8_t(xEnd)};
        uint8_t rows[]={uint8_t(yStart>>8),uint8_t(yStart),uint8_t(yEnd>>8),uint8_t(yEnd)};
        if(!transfer(0x2a,cols,4) || !transfer(0x2b,rows,4) || !transfer(0x2c,pixels_,w*count*2,true)) return;
    }
}
void Board::setDisplayPower(bool asleep,bool dimmed) { if(ready_) ledcWrite(0,asleep ? 0 : dimmed ? 100 : 700); }
}

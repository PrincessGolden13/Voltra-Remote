#include "Board.h"
#include "PanelInit.h"
#include <Wire.h>
#include <esp_heap_caps.h>
#include <esp_timer.h>
#include <algorithm>
#include <iterator>
#include "diagnostics/EventLog.h"
namespace voltra::guition {
HardwareCapabilities Board::capabilities() const {
    HardwareCapabilities c;
    c.displayWidth=c.displayHeight=resolution;c.displayShape=DisplayShape::Round;
    c.touchAvailable=c.rotaryAvailable=c.rotaryPressAvailable=ready_;
#ifdef VOLTRA_PHYSICAL
    c.bluetoothAvailable=true;
    c.wifiAvailable=true;
#endif
    c.hapticsAvailable=hapticsReady_;
    c.batteryAvailable=batteryPercent().has_value();
    return c;
}
uint64_t Board::nowMs() const { return uint64_t(esp_timer_get_time())/1000; }
std::optional<uint8_t> Board::batteryPercent() const {
    const auto now=nowMs();
    if(!ready_ || !batteryAdcReady_ || !batteryPercent_ || now<batteryAt_ || now-batteryAt_>batteryFreshMs)
        return std::nullopt;
    return batteryPercent_;
}
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
    // CST816S register declaration: 0xFB auto-resets a stationary touch (default
    // 5 s); 0xFC resets a long press (default 10 s). Both interrupt a 6 s gesture.
    // Zero disables these touch-controller resets, not any VOLTRA interlock.
    // Keep auto sleep disabled too. See BUG_FIXES_2026_11_14.md for provenance.
    for(const auto reg:{uint8_t(0xFB),uint8_t(0xFC),uint8_t(0xFE)}) {
        Wire.beginTransmission(0x15);Wire.write(reg);Wire.write(reg==0xFE ? 1 : 0);
        const bool written=Wire.endTransmission()==0;
        Wire.beginTransmission(0x15);Wire.write(reg);
        const bool readable=Wire.endTransmission(false)==0 && Wire.requestFrom(uint8_t(0x15),uint8_t(1))==1;
        const int actual=readable ? Wire.read() : -1;
        diagnostic("Touch config reg="+std::to_string(reg)+" ack="+std::to_string(written)+" read="+std::to_string(actual));
    }
    beginHaptics(); // Probe/configure with zero output; never buzz on boot.
    beginBattery(); // Passive ADC input only; never configures charger/power rails.
    pinMode(encoderA,INPUT_PULLUP);pinMode(encoderB,INPUT_PULLUP);pinMode(pressPin,INPUT_PULLUP);
    encoderState_=(digitalRead(encoderA)<<1)|digitalRead(encoderB);
    attachInterruptArg(encoderA,encoderInterrupt,this,CHANGE);attachInterruptArg(encoderB,encoderInterrupt,this,CHANGE);
    // This task owns Wire after initialization. It publishes input only; LVGL,
    // the application and BLE remain exclusively on the application loop.
    if(xTaskCreatePinnedToCore(touchTask,"touch",4096,this,2,&touchTask_,ARDUINO_RUNNING_CORE)!=pdPASS) {
        Serial.println("Touch sampler could not start; init stopped");return false;
    }
    ready_=true;ledcWrite(0,700);
    Serial.println("Display initialized; buffered touch sampler active; encoder inputs active.");return true;
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
    pollBattery();
    const uint32_t now=millis();const bool down=digitalRead(pressPin)==LOW;
    if(down!=pressCandidate_) {pressCandidate_=down;pressChanged_=now;}
    if(now-pressChanged_>=30 && down!=pressStable_) {pressStable_=down;if(down) {pressed_=true;Serial.println("Press");}}
}
namespace {
// Confirmed pin/divider in the pinned K518C sources and board schematic;
// voltage-to-charge remains an inferred single-cell profile, not a fuel gauge.
// See docs/REMOTE_BATTERY_2026_11_20.md for evidence and physical limits.
constexpr unsigned batterySamples=32,batteryDivider=2;
constexpr uint32_t minimumBatteryRailMv=3000,maximumBatteryRailMv=4300;
struct BatteryPoint { uint32_t mv; uint8_t percent; };
constexpr BatteryPoint batteryCurve[]={{3300,0},{3600,10},{3700,25},{3850,50},{4000,75},{4200,100}};
uint8_t estimatedBatteryPercent(uint32_t mv) {
    if(mv<=batteryCurve[0].mv) return 0;
    for(size_t i=1;i<std::size(batteryCurve);++i) {
        const auto lo=batteryCurve[i-1],hi=batteryCurve[i];
        if(mv<=hi.mv) return uint8_t(lo.percent+(mv-lo.mv)*(hi.percent-lo.percent)/(hi.mv-lo.mv));
    }
    return 100;
}
}
void Board::beginBattery() {
    // ESP32-S3 GPIO1 is ADC1, so this read path remains available with Wi-Fi.
    // Arduino 2.0.17 calls the attenuation ADC_11db (12 dB in newer cores).
    pinMode(batteryAdcPin,INPUT);
    analogReadResolution(12);
    analogSetPinAttenuation(batteryAdcPin,ADC_11db);
    batteryAdcReady_=true;
    pollBattery();
}
void Board::pollBattery() {
    if(!batteryAdcReady_) return;
    const auto now=nowMs();
    if(batteryPollAt_ && now>=batteryPollAt_ && now-batteryPollAt_<batteryPollMs) return;
    batteryPollAt_=now;
    uint32_t sum=0;bool readable=true;
    for(unsigned i=0;i<batterySamples;++i) {
        const auto mv=analogReadMilliVolts(batteryAdcPin);
        // A zero/error sample is unavailable, never an empty-battery reading.
        if(!mv || mv>3300) {readable=false;break;}
        sum+=mv;
    }
    const uint32_t railMv=readable ? (sum*batteryDivider+batterySamples/2)/batterySamples : 0;
    if(!readable || railMv<minimumBatteryRailMv || railMv>maximumBatteryRailMv) {
        // USB normally raises this shared rail to about 5 V. Reject it before
        // filtering so a plug/unplug cannot look like a charged battery.
        batteryPercent_.reset();batteryFilteredMv_=0;
    } else {
        batteryFilteredMv_=batteryFilteredMv_ ? (batteryFilteredMv_*7+railMv+4)/8 : railMv;
        batteryPercent_=estimatedBatteryPercent(batteryFilteredMv_);
    }
    batteryAt_=now;
    const int percent=batteryPercent_ ? int(*batteryPercent_) : -1;
    const auto change=railMv>batteryLoggedMv_ ? railMv-batteryLoggedMv_ : batteryLoggedMv_-railMv;
    if(percent!=batteryLoggedPercent_ || change>=50) {
        diagnostic("Remote battery railMv="+std::to_string(railMv)+" estimatePct="+std::to_string(percent));
        batteryLoggedMv_=railMv;batteryLoggedPercent_=percent;
    }
}
void Board::touchTask(void* context) {
    auto* board=static_cast<Board*>(context);
    for(;;) {board->pollHaptics();board->pollTouch();vTaskDelay(pdMS_TO_TICKS(touchPollMs));}
}
namespace {
// TI DRV2605/DRV2605L register map; see docs/REST_ALERT_2026_11_09.md.
constexpr uint8_t hapticAddress=0x5a, hapticStatus=0x00, hapticMode=0x01;
constexpr uint8_t hapticRtp=0x02, hapticGo=0x0c, hapticFeedback=0x1a, hapticControl3=0x1d;
constexpr uint32_t maximumRestBuzzMs=20000;
constexpr uint8_t hapticStrength=0x7f; // Full positive signed RTP amplitude.
}
bool Board::hapticWrite(uint8_t reg,uint8_t value) {
    Wire.beginTransmission(hapticAddress);Wire.write(reg);Wire.write(value);
    return Wire.endTransmission()==0;
}
bool Board::hapticRead(uint8_t reg,uint8_t& value) {
    Wire.beginTransmission(hapticAddress);Wire.write(reg);
    if(Wire.endTransmission(false)!=0 || Wire.requestFrom(hapticAddress,uint8_t(1))!=1) return false;
    value=Wire.read();return true;
}
void Board::beginHaptics() {
    uint8_t status=0,feedback=0,control3=0;
    if(!hapticRead(hapticStatus,status) || ((status>>5)!=3 && (status>>5)!=7)) {
        Serial.println("DRV2605 not detected; rest notification remains visual");return;
    }
    // The matching K5 reference uses an LRA. Preserve the driver's remaining
    // feedback/voltage defaults; no guessed motor GPIO or calibration actuation.
    hapticsReady_=hapticWrite(hapticMode,0x40) && hapticWrite(hapticGo,0) && hapticWrite(hapticRtp,0) &&
        hapticRead(hapticFeedback,feedback) && hapticWrite(hapticFeedback,feedback|0x80) &&
        hapticRead(hapticControl3,control3) && hapticWrite(hapticControl3,control3 & ~0x08); // Signed RTP; remain in standby.
    Serial.printf("DRV2605 rest haptics %s\n",hapticsReady_ ? "ready" : "unavailable");
}
void Board::restAlert(uint32_t durationMs) {
    const auto until=durationMs ? nowMs()+std::min(durationMs,maximumRestBuzzMs) : 0;
    portENTER_CRITICAL(&touchMux_);hapticUntil_=until;portEXIT_CRITICAL(&touchMux_);
}
void Board::pollHaptics() {
    if(!hapticsReady_) return;
    portENTER_CRITICAL(&touchMux_);const auto until=hapticUntil_;portEXIT_CRITICAL(&touchMux_);
    const bool active=until && nowMs()<until;
    if(active==hapticActive_) return;
    bool written;
    if(active) written=hapticWrite(hapticRtp,0) && hapticWrite(hapticMode,0x05) && hapticWrite(hapticRtp,hapticStrength);
    else {
        // Attempt both stopping mechanisms even if either bus write fails.
        const bool zeroed=hapticWrite(hapticRtp,0);
        const bool standby=hapticWrite(hapticMode,0x40);
        written=zeroed && standby;
    }
    if(written) {
        hapticActive_=active;
        Serial.printf("Rest vibration %s\n",active ? "on" : "off");
    } else if(active) {
        // An ACK failure cannot prove the motor stayed off. Treat it as on so
        // the deadline/cancel must still send zero; never retry a buzz blindly.
        hapticActive_=true;
        Serial.println("Rest vibration start unconfirmed");
    }
    // Retry failed zero writes; the independent deadline survives UI/BLE stalls.
}
void Board::cancelTouchLocked(uint64_t now,uint8_t reason) {
    touchBlocked_=true;
    touchCurrent_.point.pressed=false;touchCurrent_.point.cancelled=true;touchCurrent_.at=now;
    touchCurrent_.cancelReason=reason; // 0 screen, 1 stale, 2 read/coordinate, 3 overflow.
    touchHead_=0;touchCount_=1;touchQueue_[0]=touchCurrent_;
}
void Board::publishTouchLocked(const CapturedTouch& sample) {
    const auto& a=touchCurrent_.point;const auto& b=sample.point;
    const bool changed=a.pressed!=b.pressed || a.cancelled!=b.cancelled || a.x!=b.x || a.y!=b.y;
    touchCurrent_=sample;
    if(!changed) return;
    if(touchCount_==touchQueueCapacity) {cancelTouchLocked(sample.at,3);return;}
    touchQueue_[(touchHead_+touchCount_)%touchQueueCapacity]=sample;++touchCount_;
}
void Board::pollTouch() {
    Wire.beginTransmission(0x15);Wire.write(0x02);
    const bool readOk=Wire.endTransmission(false)==0 && Wire.requestFrom(uint8_t(0x15),uint8_t(5))==5;
    uint8_t data[5]{};if(readOk) for(auto& byte:data) byte=Wire.read();
    const int x=((data[1]&15)<<8)|data[2],y=((data[3]&15)<<8)|data[4];
    const bool down=(data[0]&15)>0;
    portENTER_CRITICAL(&touchMux_);
    const auto now=nowMs();
    if(!readOk || (down && (x>=resolution || y>=resolution))) cancelTouchLocked(now,2);
    else if(touchBlocked_ && down) touchCurrent_.at=now;
    else {
        touchBlocked_=false;
        auto point=touchCurrent_.point;point.pressed=down;point.cancelled=false;
        if(down) {point.x=x/float(resolution-1);point.y=y/float(resolution-1);}
        publishTouchLocked({point,now});
    }
    portEXIT_CRITICAL(&touchMux_);
}
TouchPoint Board::sample() {
    portENTER_CRITICAL(&touchMux_);
    const auto now=nowMs();
    const auto oldest=touchCount_ ? touchQueue_[touchHead_].at : touchCurrent_.at;
    if(now>=oldest && now-oldest>maximumTouchAgeMs) cancelTouchLocked(now,1);
    auto captured=touchCurrent_;
    if(touchCount_) {captured=touchQueue_[touchHead_];touchHead_=(touchHead_+1)%touchQueueCapacity;--touchCount_;}
    portEXIT_CRITICAL(&touchMux_);
    auto point=captured.point;point.sampledAtMs=captured.at;
    if(point.cancelled && !deliveredTouch_.cancelled)
        diagnostic("Touch cancelled reason="+std::to_string(captured.cancelReason));
    if(point.pressed!=deliveredTouch_.pressed || point.cancelled!=deliveredTouch_.cancelled)
        Serial.printf("%llu Touch %s %d,%d ageMs=%llu reason=%u\n",static_cast<unsigned long long>(now),
            point.cancelled ? "cancel" : point.pressed ? "down" : "up",
            int(point.x*(resolution-1)),int(point.y*(resolution-1)),static_cast<unsigned long long>(now-captured.at),unsigned(captured.cancelReason));
    deliveredTouch_=point;
    return point;
}
bool Board::hasPending() const {
    portENTER_CRITICAL(&touchMux_);const bool pending=touchCount_!=0;portEXIT_CRITICAL(&touchMux_);return pending;
}
void Board::discardPending() {
    portENTER_CRITICAL(&touchMux_);cancelTouchLocked(nowMs());portEXIT_CRITICAL(&touchMux_);
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

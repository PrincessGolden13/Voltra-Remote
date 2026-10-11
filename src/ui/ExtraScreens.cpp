#include "UiController.h"
#include "UiActions.h"
#include <cstdio>
namespace voltra {
namespace {
constexpr uint32_t white = 0xf5f8ff, dim = 0x647384;
const char* themes[] = {"Red", "Green", "Blue", "Yellow", "Purple", "Pink", "Orange", "Teal", "Gray"};
std::string number(int index) { char b[8]; std::snprintf(b,sizeof(b),"P%02d",index+1); return b; }
void text(lv_obj_t* o,const std::string& s) { if(o) lv_label_set_text(o,s.c_str()); }
}
uint32_t UiController::accent() const {
    constexpr uint32_t colors[] = {0xff4949,0x48e092,0x479cff,0xffda45,0xad77ff,0xff75bf,0xffa04d,0x00d9e8,0xb9c5ce};
    return colors[config_.theme];
}
uint32_t UiController::limitAccent() const {
    // Paired high-contrast limit colors in the same order as the nine themes.
    constexpr uint32_t colors[] = {0x45e5ff,0xff79cb,0xffcb60,0xbf95ff,0x64efb5,0xb7ee64,0x65cfff,0xff8273,0xffffff};
    return colors[config_.theme];
}
void UiController::saveConfiguration() {
    if (store_ && !store_->save(config_)) text(message_,"Could not save settings");
}
void UiController::separator(int y,int inset) {
    auto* line = lv_obj_create(face_); lv_obj_remove_style_all(line);
    lv_obj_set_size(line,width_*(100-2*inset)/100,1); lv_obj_set_pos(line,width_*inset/100,height_*y/100);
    lv_obj_set_style_bg_color(line,lv_color_hex(accent()),0); lv_obj_set_style_bg_opa(line,90,0);
}
std::string UiController::presetSummary(const Preset& p,bool multiline) const {
    const std::string magicUnit = p.magic.percent ? "%" : unitName(p.unit), eoUnit = p.eccentric.percent ? "%" : unitName(p.unit), gap = multiline ? "\n\n" : " ";
    std::string s = std::string(magicName(p.mode))+(p.magic.enabled ? " "+std::to_string(p.magic.value)+magicUnit : " OFF");
    s += gap+"EO"+(p.eccentric.enabled ? " "+std::to_string(p.eccentric.value)+eoUnit : " OFF");
    if (p.drops.enabled) s += gap+(p.drops.mode == DropMode::Stairs ? "Stairs "+std::to_string(p.drops.step)+unitName(p.unit) :
        "RP "+std::to_string(p.drops.drops)+" / "+std::to_string(p.drops.percent)+"% / "+std::to_string(p.drops.holdSeconds)+"s");
    else if (multiline) s += gap+"Drops OFF";
    return s;
}
void UiController::buildExtra() {
    const auto* medium = width_ < 400 ? &lv_font_montserrat_14 : &lv_font_montserrat_18;
    const auto* heading = width_ < 400 ? &lv_font_montserrat_18 : &lv_font_montserrat_28;
    auto action = [&](const std::string& name,int x,int y,int w,int h,int id) {
        auto* b=button(face_,name,width_*x/100,height_*y/100,width_*w/100,height_*h/100,id);
        glow(b,accent(),true); return b;
    };
    if(page_ == Screen::PresetNotice) {
        label(face_,"PRESET UNAVAILABLE",width_*76/100,height_*22/100,heading);
        label(face_,presetNotice_,width_*72/100,height_*38/100,medium);
        action("OK",33,64,34,12,CancelRecipe);
        message_=label(face_,"",width_*74/100,height_*81/100); bind(message_,Unload);
        return;
    }
    if (page_ == Screen::Live) {
        title_ = label(face_,app_.name(),width_*70/100,height_*13/100,heading); bind(title_,BackDevices); lv_label_set_long_mode(title_,LV_LABEL_LONG_DOT);
        status_ = label(face_,"",width_*60/100,height_*22/100,medium);
        start_ = label(face_,"",width_*65/100,height_*31/100,heading);
        center_ = lv_obj_create(face_); lv_obj_remove_style_all(center_);
        lv_obj_set_size(center_,width_*76/100,height_*28/100); lv_obj_set_pos(center_,width_*12/100,height_*38/100); bind(center_,Weight);
        lv_obj_add_flag(center_,LV_OBJ_FLAG_OVERFLOW_VISIBLE);
        weight_ = label(center_,"",width_*70/100,height_*5/100,&lv_font_montserrat_48);
        lv_obj_set_style_transform_pivot_x(weight_,width_*35/100,0);
        lv_obj_set_style_transform_pivot_y(weight_,24,0);
        lv_obj_set_style_transform_scale(weight_,width_*600/480,0);
        nextWeight_ = label(face_,"",width_*68/100,height_*61/100,heading);
        const auto r=app_.report(); const auto d=app_.drops();
        const int count=d.enabled && d.mode == DropMode::RestPause ? 3 : 2;
        const char* captions[]={"SET","REP","DROPS"};
        for(int i=0;i<count;++i) {
            const int x=width_*(16+i*68/count)/100, w=width_*68/(100*count);
            auto* caption=label(face_,i==0 && !r.setNumber ? "" : captions[i],w,height_*72/100,medium);
            lv_obj_set_align(caption,LV_ALIGN_TOP_LEFT); lv_obj_set_x(caption,x);
            liveValues_[i]=label(face_,"",w,height_*77/100,width_ < 400 ? &lv_font_montserrat_28 : &lv_font_montserrat_36);
            lv_obj_set_align(liveValues_[i],LV_ALIGN_TOP_LEFT); lv_obj_set_x(liveValues_[i],x);
        }
        separator(69,13); separator(87,22);
        message_ = label(face_,"",width_*76/100,height_*26/100);
        return;
    }
    if (page_ == Screen::Presets || page_ == Screen::SavePreset || page_ == Screen::OverridePreset) {
        label(face_,page_ == Screen::OverridePreset ? "OVERRIDE "+number(presetIndex_)+"?" : "PRESETS",width_*64/100,height_*12/100,heading);
        action(LV_SYMBOL_LEFT,76,19,10,10,CancelRecipe);
        if (page_ == Screen::Presets) {
            const bool isNew = presetIndex_ == int(config_.presets.size());
            auto* selected=disc(face_,width_*31/100,height_*33/100,width_*38/100,accent(),true); bind(selected,ChoosePreset);
            label(selected,number(presetIndex_),width_*36/100,width_*9/100,&lv_font_montserrat_36);
            label(selected,isNew ? "NEW" : presetSummary(config_.presets[presetIndex_]),width_*32/100,width_*22/100);
            for (int side : {-1,1}) {
                int index = presetIndex_+side;
                if (index < 0 || index > std::min(98,int(config_.presets.size()))) continue;
                const int x=side < 0 ? 5 : 73;
                auto* neighbor=disc(face_,width_*x/100,height_*42/100,width_*22/100,accent(),false);
                bind(neighbor,side < 0 ? PrevPreset : NextPreset);
                label(neighbor,number(index),width_*21/100,width_*5/100,heading);
                label(neighbor,index == int(config_.presets.size()) ? "NEW" : "",width_*21/100,width_*13/100);
            }
            label(face_,"Tap to apply  /  Hold to override",width_*75/100,height_*79/100);
        } else if (page_ == Screen::SavePreset) {
            auto* selected=disc(face_,width_*32/100,height_*25/100,width_*36/100,accent(),true);
            label(selected,number(presetIndex_),width_*34/100,width_*7/100,&lv_font_montserrat_36);
            label(selected,"NEW",width_*34/100,width_*21/100,heading);
            label(face_,presetSummary(saveDraft_),width_*76/100,height_*63/100,medium);
            action(LV_SYMBOL_SAVE "  SAVE",33,75,34,12,SaveRecipe);
        } else {
            separator(28,17);
            label(face_,"OLD                 NEW",width_*70/100,height_*31/100,heading);
            auto* old=label(face_,presetSummary(config_.presets[presetIndex_],true),width_*32/100,height_*41/100,medium);
            lv_obj_set_align(old,LV_ALIGN_TOP_LEFT); lv_obj_set_x(old,width_*15/100);
            auto* fresh=label(face_,presetSummary(saveDraft_,true),width_*32/100,height_*41/100,medium);
            lv_obj_set_align(fresh,LV_ALIGN_TOP_LEFT); lv_obj_set_x(fresh,width_*54/100);
            action("SAVE",18,77,29,10,SaveRecipe); action("CANCEL",51,77,32,10,CancelRecipe);
        }
        message_=label(face_,"",width_*75/100,height_*87/100); bind(message_,Unload); lv_obj_set_ext_click_area(message_,height_/50);
        return;
    }
    label(face_,page_ == Screen::Settings ? "SETTINGS" : page_ == Screen::Reset ? "FACTORY RESET?" :
        page_ == Screen::Wifi ? "WI-FI" : page_ == Screen::Updates ? "UPDATES" : "ABOUT",width_*70/100,height_*12/100,heading);
    action(LV_SYMBOL_LEFT,77,19,10,10,CloseSettings);
    if (page_ == Screen::Settings) {
        const char* names[]={"Theme", "Rest alert", "Dim", "Sleep"};
        for(int i=0;i<4;++i) {
            auto* b=action(names[i],19,27+i*10,62,9,ThemeField+i); settingsValues_[i]=lv_obj_get_child(b,0);
        }
        action("Wi-Fi",16,70,31,8,OpenWifi); action("Updates",53,70,31,8,OpenUpdates);
        action("About",21,80,27,8,OpenAbout); action("Reset",52,80,27,8,OpenReset);
    } else if(page_ == Screen::Reset) {
        label(face_,"Erase themes, preferences\nand saved presets?\nRecent devices are kept.",width_*73/100,height_*36/100,medium);
        action("CANCEL",26,62,48,12,CloseSettings);
        auto* erase=action("Erase remote settings",27,79,46,8,ConfirmReset); glow(erase,accent(),false);
    } else {
        std::string body = page_ == Screen::Wifi ? "Phone setup\n\nTemporary access point\nand captive portal\n\nUnavailable on desktop" :
            page_ == Screen::Updates ? "Check for updates\n\nNo firmware update service\nis configured for this target.\n\nNo update installed." :
            "VOLTRA Remote 0.2.0\nOffline desktop simulator\n" __DATE__ "\n\n"+std::to_string(width_)+" x "+std::to_string(height_)+"\nShared LVGL application";
        label(face_,body,width_*74/100,height_*33/100,medium);
    }
    message_=label(face_,"",width_*75/100,height_*88/100);
}
void UiController::updateExtra() {
    const auto r=app_.report();
    if(page_ == Screen::Live) {
        const auto d=app_.drops();
        text(status_,r.twinKnown ? (r.twin ? "Twin: "+r.twinPeerName : "") : "Twin unknown");
        const bool known=app_.fresh() && r.telemetryValid;
        largeValue(known ? (d.enabled ? std::to_string(r.settings.weight) : r.force ? std::to_string(*r.force) : "--") : "--");
        text(start_,d.enabled ? "START "+std::to_string(app_.startingWeight()) : "");
        int next=std::max(1,d.mode == DropMode::Stairs ? nextStairWeight(r,d.step) : r.settings.weight*(100-d.percent)/100);
        if(d.mode == DropMode::RestPause && app_.dropsCompleted() >= d.drops) next=r.settings.weight;
        text(nextWeight_,d.enabled ? "NEXT "+(known ? std::to_string(next) : "--") : unitName(r.unit));
        text(liveValues_[0],known && r.setNumber ? std::to_string(*r.setNumber) : "");
        text(liveValues_[1],known ? std::to_string(r.rep) : "--");
        text(liveValues_[2],known ? std::to_string(app_.dropsCompleted())+"/"+std::to_string(d.drops) : "--");
        text(message_,known ? app_.message() : "State unknown / tap weight to unload");
    }
    if(page_ == Screen::PresetNotice || page_ == Screen::Presets || page_ == Screen::SavePreset || page_ == Screen::OverridePreset) {
        text(message_,r.loaded || r.autoLoadArmed || !app_.fresh() ? "Tap here to unload / cancel" : app_.message());
    }
    if(page_ == Screen::Settings) {
        const auto mins=[](int n){ return n ? std::to_string(n)+" min" : "Never"; };
        text(settingsValues_[0],"Theme    "+std::string(themes[config_.theme]));
        text(settingsValues_[1],"Rest alert    "+(config_.restAlertSeconds ? std::to_string(config_.restAlertSeconds/60)+":"+
            (config_.restAlertSeconds%60 < 10 ? "0" : "")+std::to_string(config_.restAlertSeconds%60) : "Off"));
        text(settingsValues_[2],"Dim    "+mins(config_.dimMinutes)); text(settingsValues_[3],"Sleep    "+mins(config_.sleepMinutes));
        for(int i=0;i<4;++i) lv_obj_set_style_text_color(settingsValues_[i],lv_color_hex(i==settingsField_ ? white : dim),0);
    }
}
}

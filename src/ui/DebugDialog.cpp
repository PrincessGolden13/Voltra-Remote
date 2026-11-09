#include "UiController.h"
namespace voltra {
void UiController::openDebugDialog() {
    if(debugDialog_) return;
    debugEvents_=EventLog::instance().snapshot();
    debugContext_="firmware=" VOLTRA_VERSION " screen="+std::to_string(int(page_))+" "+app_.diagnosticState();
    debugSubmitted_=false;editDirty_=returnAfterEdit_=false;
    if(restButton_) lv_obj_add_flag(restButton_,LV_OBJ_FLAG_HIDDEN);
    debugDialog_=lv_obj_create(lv_layer_top());
    lv_obj_set_size(debugDialog_,width_,height_);lv_obj_center(debugDialog_);
    lv_obj_set_style_bg_color(debugDialog_,lv_color_hex(0x04080b),0);
    lv_obj_set_style_bg_opa(debugDialog_,LV_OPA_COVER,0);
    lv_obj_set_style_border_width(debugDialog_,0,0);lv_obj_set_style_pad_all(debugDialog_,0,0);
    lv_obj_remove_flag(debugDialog_,LV_OBJ_FLAG_SCROLLABLE);
    label(debugDialog_,"DEBUG REPORT",width_*3/4,height_/6);
    debugText_=label(debugDialog_,"Send last events for debugging?\n\n20 recent events and control status.\nNo Wi-Fi passwords or device IDs.",width_*3/4,height_*28/100);
    auto make=[&](const char* title,int x,lv_event_cb_t callback) {
        auto* b=lv_button_create(debugDialog_);lv_obj_set_size(b,width_*28/100,height_/9);
        lv_obj_set_pos(b,x,height_*66/100);
        auto* t=label(b,title,width_/4,0);lv_obj_center(t);
        lv_obj_add_event_cb(b,callback,LV_EVENT_CLICKED,this);return b;
    };
    debugYes_=make("YES",width_/5,[](lv_event_t* e) {
        auto* s=static_cast<UiController*>(lv_event_get_user_data(e));
        if(s->touchMoved_ || s->debugConsumed_ || s->debugSubmitted_) return;
        s->debugSubmitted_=true;
        if(s->reports_) s->reports_->submit(s->debugEvents_,s->debugContext_);
    });
    make("CLOSE / NO",width_*53/100,[](lv_event_t* e) {
        auto* s=static_cast<UiController*>(lv_event_get_user_data(e));
        if(!s->touchMoved_ && !s->debugConsumed_) s->closeDebugDialog();
    });
    debugUnload_=lv_button_create(debugDialog_);
    lv_obj_set_size(debugUnload_,width_*45/100,height_/10);
    lv_obj_set_pos(debugUnload_,width_*275/1000,height_*80/100);
    auto* t=label(debugUnload_,"UNLOAD",width_*4/10,0);lv_obj_center(t);
    lv_obj_add_event_cb(debugUnload_,[](lv_event_t* e) {
        auto* s=static_cast<UiController*>(lv_event_get_user_data(e));
        if(!s->touchMoved_ && !s->debugConsumed_) s->app_.unload();
    },LV_EVENT_CLICKED,this);
    activityAt_=hardware_.nowMs();sleeping_=dimmed_=false;
    hardware_.setDisplayPower(false,false);
}
void UiController::closeDebugDialog() {
    if(debugDialog_) lv_obj_delete_async(debugDialog_);
    debugDialog_=debugText_=debugYes_=debugUnload_=nullptr;
    if(restButton_ && restNotice_) lv_obj_remove_flag(restButton_,LV_OBJ_FLAG_HIDDEN);
    debugEvents_.clear();debugContext_.clear();
    lv_indev_wait_release(touch_);
}
void UiController::updateDebugDialog() {
    if(!debugDialog_) return;
    if(app_.connected()) lv_obj_remove_flag(debugUnload_,LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(debugUnload_,LV_OBJ_FLAG_HIDDEN);
    if(!debugSubmitted_) return;
    lv_obj_add_flag(debugYes_,LV_OBJ_FLAG_HIDDEN);
    const auto status=reports_ ? reports_->status() : ReportStatus{};
    const std::string text=reports_ ? status.message+(status.id.empty() ? "" : "\nReport: "+status.id) : "Simulator: report delivery is disabled.";
    if(text!=lv_label_get_text(debugText_)) lv_label_set_text(debugText_,text.c_str());
}
}

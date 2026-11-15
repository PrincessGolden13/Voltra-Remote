#include "UiController.h"
namespace voltra {
void UiController::openDebugDialog() {
    if(debugDialog_) return;
    if(debugHint_) lv_obj_add_flag(debugHint_,LV_OBJ_FLAG_HIDDEN);
    diagnostic("Debug report dialog opened");
    const auto phase=reports_ ? reports_->status().phase : ReportPhase::Idle;
    debugSubmitted_=debugDisconnectPending_ || phase==ReportPhase::Sending || phase==ReportPhase::Error ||
        (phase==ReportPhase::Queued && !app_.connected() && !app_.connecting());
    debugUnloadRequested_=false;editDirty_=returnAfterEdit_=false;
    if(restButton_) lv_obj_add_flag(restButton_,LV_OBJ_FLAG_HIDDEN);
    debugDialog_=lv_obj_create(lv_layer_top());
    lv_obj_set_size(debugDialog_,width_,height_);lv_obj_center(debugDialog_);
    lv_obj_set_style_bg_color(debugDialog_,lv_color_hex(0x04080b),0);
    lv_obj_set_style_bg_opa(debugDialog_,LV_OPA_COVER,0);
    lv_obj_set_style_border_width(debugDialog_,0,0);lv_obj_set_style_pad_all(debugDialog_,0,0);
    lv_obj_remove_flag(debugDialog_,LV_OBJ_FLAG_SCROLLABLE);
    label(debugDialog_,"DEBUG REPORT",width_*3/4,height_/6);
    debugText_=label(debugDialog_,"20 events and control status.\nNo passwords or device IDs.\n\nSEND disconnects Bluetooth\nand uploads over Wi-Fi.\nDisconnecting does not unload.",width_*80/100,height_*24/100);
    lv_obj_set_height(debugText_,height_*27/100);
    lv_label_set_long_mode(debugText_,LV_LABEL_LONG_DOT);
    debugControlStatus_=label(debugDialog_,"",width_*76/100,height_*51/100);
    lv_obj_set_height(debugControlStatus_,height_/10);
    lv_label_set_long_mode(debugControlStatus_,LV_LABEL_LONG_DOT);
    auto make=[&](const char* title,int x,lv_event_cb_t callback) {
        auto* b=lv_button_create(debugDialog_);lv_obj_set_size(b,width_*28/100,height_/9);
        lv_obj_set_pos(b,x,height_*62/100);
        lv_obj_remove_flag(b,LV_OBJ_FLAG_SCROLLABLE);
        auto* t=label(b,title,width_/4,0);lv_obj_center(t);
        lv_obj_add_event_cb(b,callback,LV_EVENT_CLICKED,this);return b;
    };
    debugSend_=make("SEND",width_/5,[](lv_event_t* e) {
        auto* s=static_cast<UiController*>(lv_event_get_user_data(e));
        if(!s->touchMoved_ && !s->debugConsumed_) s->sendDebugReport();
    });
    debugSendLabel_=lv_obj_get_child(debugSend_,0);
    make("CLOSE",width_*53/100,[](lv_event_t* e) {
        auto* s=static_cast<UiController*>(lv_event_get_user_data(e));
        if(!s->touchMoved_ && !s->debugConsumed_) s->closeDebugDialog();
    });
    debugUnload_=lv_button_create(debugDialog_);
    lv_obj_set_size(debugUnload_,width_/2,height_*12/100);
    lv_obj_set_pos(debugUnload_,width_/4,height_*76/100);
    lv_obj_remove_flag(debugUnload_,LV_OBJ_FLAG_SCROLLABLE);
    auto* t=label(debugUnload_,"UNLOAD",width_*4/10,0);lv_obj_center(t);
    lv_obj_add_event_cb(debugUnload_,[](lv_event_t* e) {
        auto* s=static_cast<UiController*>(lv_event_get_user_data(e));
        // Unload acts on initial contact, even if drift or a lost release would
        // later cancel an ordinary click. A consumed debug hold cannot fire it.
        if(!s->debugConsumed_) s->unloadFromDebugDialog();
    },LV_EVENT_PRESSED,this);
    activityAt_=hardware_.nowMs();sleeping_=dimmed_=false;
    hardware_.setDisplayPower(false,false);
}
void UiController::sendDebugReport() {
    if(debugDisconnectPending_ || (reports_ && reports_->status().busy())) return;
    debugSubmitted_=true;
    if(!reports_) return; // Offline simulator must not disconnect or transmit.
    diagnostic("Debug report SEND requested");
    const auto events=EventLog::instance().snapshot();
    const auto context="firmware=" VOLTRA_VERSION " screen="+std::to_string(int(page_))+" "+app_.diagnosticState();
    // A failed save leaves Bluetooth intact and the retry button available.
    if(reports_->submit(events,context)) {
        debugDisconnectPending_=true;
        app_.setMaintenance(true);
    }
}
void UiController::unloadFromDebugDialog() {
    debugUnloadRequested_=true;
    diagnostic("Debug dialog unload action");
    app_.unload();
}
void UiController::closeDebugDialog() {
    if(debugDialog_) lv_obj_delete_async(debugDialog_);
    debugDialog_=debugText_=debugSend_=debugSendLabel_=debugUnload_=debugControlStatus_=nullptr;
    if(restButton_ && restNotice_) lv_obj_remove_flag(restButton_,LV_OBJ_FLAG_HIDDEN);
    lv_indev_wait_release(touch_);
}
void UiController::updateDebugDialog() {
    const auto now=hardware_.nowMs();
    const auto heldMs=now>=touchStartedAt_ ? now-touchStartedAt_ : 0;
    const bool counting=touchDown_ && !debugMoved_ && !wakeTouch_ && !debugConsumed_ && !debugDialog_ && heldMs>=1000;
    if(counting) {
        if(!debugHint_) {
            debugHint_=label(lv_layer_top(),"",width_*70/100,height_*82/100);
            lv_obj_set_style_bg_color(debugHint_,lv_color_hex(0x04080b),0);
            lv_obj_set_style_bg_opa(debugHint_,LV_OPA_COVER,0);
        }
        const auto remaining=heldMs>=debugHoldMs ? 0 : (debugHoldMs-heldMs+999)/1000;
        const auto caption="Debug log in "+std::to_string(remaining)+"s - keep holding";
        if(caption!=lv_label_get_text(debugHint_)) lv_label_set_text(debugHint_,caption.c_str());
        lv_obj_remove_flag(debugHint_,LV_OBJ_FLAG_HIDDEN);
    } else if(debugHint_) lv_obj_add_flag(debugHint_,LV_OBJ_FLAG_HIDDEN);
    if(!debugDialog_) return;
    const auto r=app_.report();
    std::string controlStatus="Bluetooth disconnected.";
    if(app_.connected()) {
        lv_obj_remove_flag(debugUnload_,LV_OBJ_FLAG_HIDDEN);
        controlStatus=app_.pending() ? (debugUnloadRequested_ ? "Unload requested; awaiting VOLTRA." : "Control request pending.") :
            app_.fresh() && !r.loaded && !r.autoLoadArmed ? "VOLTRA reports unloaded." :
            debugUnloadRequested_ && !app_.message().empty() ? app_.message() :
            !app_.fresh() ? "State unknown. UNLOAD is available." :
            r.autoLoadArmed ? "Auto Load armed. UNLOAD cancels." : "VOLTRA loaded. UNLOAD is available.";
    } else {
        lv_obj_add_flag(debugUnload_,LV_OBJ_FLAG_HIDDEN);
        if(debugUnloadRequested_) controlStatus="Bluetooth disconnected; unload cannot be confirmed.";
    }
    if(controlStatus!=lv_label_get_text(debugControlStatus_)) lv_label_set_text(debugControlStatus_,controlStatus.c_str());
    const auto status=reports_ ? reports_->status() : ReportStatus{};
    if(!debugSubmitted_ && !debugDisconnectPending_) return;
    if(debugDisconnectPending_ || !reports_ || status.phase==ReportPhase::Queued || status.phase==ReportPhase::Sending || status.phase==ReportPhase::Sent)
        lv_obj_add_flag(debugSend_,LV_OBJ_FLAG_HIDDEN);
    else {
        lv_label_set_text(debugSendLabel_,"RETRY");
        lv_obj_remove_flag(debugSend_,LV_OBJ_FLAG_HIDDEN);
    }
    std::string text=reports_ ? status.message : "Simulator: report delivery is disabled.";
    if(debugDisconnectPending_) text="Report saved.\nWaiting for the control request\nbefore disconnecting Bluetooth.\nWi-Fi delivery follows automatically.";
    else if(status.phase==ReportPhase::Queued && wifi_) {
        const auto w=wifi_->status();
        if(!w.saved) text="Report saved.\nSet up Wi-Fi in Settings to send.";
        else if(w.setupActive) text="Report saved.\nFinish Wi-Fi setup to send.";
        else if(!w.connected) text="Report saved. Waiting for Wi-Fi.\n"+w.message;
    }
    if(!status.id.empty()) text+="\nReport: "+status.id;
    if(text!=lv_label_get_text(debugText_)) lv_label_set_text(debugText_,text.c_str());
}
}

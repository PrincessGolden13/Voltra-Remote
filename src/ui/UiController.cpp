#include "StyleUpdates.h"
#include "UiController.h"
#include "UiActions.h"
#include <algorithm>
#include <stdexcept>
#include <cmath>
#include <cstdio>
namespace voltra {
namespace {
constexpr uint32_t bg = 0x04080b, bright = 0xf5f8ff, muted = 0x59677d;
constexpr uint32_t soft = 0x96a7bd, amber = 0xffc374;

void color(lv_obj_t* obj, uint32_t c) { style::set_text_color(obj, lv_color_hex(c), 0); }
void value(lv_obj_t* obj, const std::string& text) { if (obj && text != lv_label_get_text(obj)) lv_label_set_text(obj, text.c_str()); }
int percentOf(int n, int max) { return max > 0 ? std::clamp(n*1000/max, 0, 1000) : 0; }
}
lv_obj_t* UiController::label(lv_obj_t* parent, const std::string& text, int w, int y, const lv_font_t* font) {
    auto* obj = lv_label_create(parent); value(obj, text); lv_obj_set_width(obj, w);
    style::set_text_color(obj, lv_color_hex(bright), 0);
    style::set_text_font(obj, font, 0); style::set_text_align(obj, LV_TEXT_ALIGN_CENTER, 0);
    style::align(obj, LV_ALIGN_TOP_MID, 0, y); lv_obj_remove_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    return obj;
}
void UiController::bind(lv_obj_t* obj, int action, const std::string& id) {
    bindings_.push_back(std::make_unique<Binding>(Binding{this, action, id}));
    lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_PRESS_LOCK);
    lv_obj_add_event_cb(obj, [](lv_event_t* e) {
        auto* b = static_cast<Binding*>(lv_event_get_user_data(e));
        const auto code = lv_event_get_code(e);
        if (code == LV_EVENT_PRESSED) b->held = false;
        if (b->ui->touchMoved_ || b->ui->debugConsumed_ || b->ui->debugDialog_) {
            if(code==LV_EVENT_RELEASED) std::printf("Touch cancelled by movement: action=%d\n",b->action);
            return;
        }
        const bool supportsHold = b->action == Weight || b->action == Magic || b->action == EO ||
            b->action == Drops || b->action == ChoosePreset || b->action == Select;
        if (code == LV_EVENT_LONG_PRESSED && supportsHold) {
            b->held = true;
        } else if (code == LV_EVENT_CLICKED) {
            // Commit ordinary holds on release. A six-second debug hold consumes
            // the entire gesture, including the earlier Auto Load threshold.
            diagnostic("Touch action="+std::to_string(b->action)+" hold="+std::to_string(b->held));
            b->ui->act(b->action, b->id, b->held);
        }
    }, LV_EVENT_ALL, bindings_.back().get());
}
lv_obj_t* UiController::button(lv_obj_t* parent, const std::string& text, int x, int y, int w, int h, int action) {
    auto* obj = lv_button_create(parent); lv_obj_set_size(obj, w, h); lv_obj_set_pos(obj, x, y);
    lv_obj_set_style_radius(obj, LV_RADIUS_CIRCLE, 0); style::set_bg_color(obj, lv_color_hex(0x17202d), 0);
    style::set_shadow_width(obj, 0, 0); lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    auto* textObj = label(obj, text, w, 0); style::center(textObj); bind(obj, action);
    lv_label_set_long_mode(textObj, LV_LABEL_LONG_DOT);
    return obj;
}
lv_obj_t* UiController::arc(lv_obj_t* parent, int size, uint32_t c) {
    auto* obj = lv_arc_create(parent); lv_obj_set_size(obj, size, size); style::center(obj);
    lv_arc_set_rotation(obj, 135); lv_arc_set_bg_angles(obj, 0, 270); lv_arc_set_range(obj, 0, 1000);
    lv_obj_remove_style(obj, nullptr, LV_PART_KNOB); lv_obj_remove_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    style::set_arc_width(obj, std::max(3, width_/80), LV_PART_MAIN);
    style::set_arc_width(obj, std::max(3, width_/80), LV_PART_INDICATOR);
    style::set_arc_color(obj, lv_color_hex(0x202a38), LV_PART_MAIN);
    style::set_arc_color(obj, lv_color_hex(c), LV_PART_INDICATOR);
    return obj;
}
void UiController::glow(lv_obj_t* obj, uint32_t accent, bool active) {
    style::set_border_width(obj, active ? std::max(2, width_/160) : 1, 0);
    style::set_border_color(obj, lv_color_hex(active ? accent : 0x39475b), 0);
    style::set_bg_color(obj, lv_color_hex(bg), 0);
    style::set_bg_grad_color(obj, lv_color_hex(bg), 0);
    style::set_bg_grad_dir(obj, LV_GRAD_DIR_VER, 0);
    style::set_shadow_color(obj, lv_color_hex(accent), 0);
    style::set_shadow_width(obj, active ? width_/28 : 0, 0);
    style::set_shadow_opa(obj, 65, 0);
}
lv_obj_t* UiController::disc(lv_obj_t* parent, int x, int y, int size, uint32_t accent, bool active) {
    auto* obj = lv_obj_create(parent);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(obj, size, size); lv_obj_set_pos(obj, x, y);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(obj, 0, 0); lv_obj_set_style_radius(obj, LV_RADIUS_CIRCLE, 0);
    glow(obj, accent, active); return obj;
}
lv_obj_t* UiController::linkIcon(lv_obj_t* parent, int size, uint32_t c) {
    auto* group = lv_obj_create(parent); lv_obj_remove_style_all(group); lv_obj_set_size(group, size, size/2);
    lv_obj_remove_flag(group, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(group, LV_OBJ_FLAG_SCROLLABLE);
    for (int i=0; i<2; ++i) {
        auto* link = lv_obj_create(group); lv_obj_remove_style_all(link);
        lv_obj_set_size(link, size*58/100, size*38/100); lv_obj_set_pos(link, i*size*40/100, size*5/100);
        lv_obj_set_style_radius(link, LV_RADIUS_CIRCLE, 0);
        style::set_border_width(link, std::max(2, size/12), 0);
        style::set_border_color(link, lv_color_hex(c), 0);
        lv_obj_remove_flag(link, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_remove_flag(link, LV_OBJ_FLAG_SCROLLABLE);
    }
    return group;
}
lv_obj_t* UiController::twinIcon(lv_obj_t* parent, int size, uint32_t c) {
    auto* group = lv_obj_create(parent); lv_obj_remove_style_all(group);
    lv_obj_set_size(group, size, size*2/3);
    lv_obj_remove_flag(group, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(group, LV_OBJ_FLAG_SCROLLABLE);
    for (int side=0; side<2; ++side) {
        auto* ring = disc(group, side*size/3, 0, size*2/3, c, true);
        style::set_bg_opa(ring, LV_OPA_TRANSP, 0);
        style::set_shadow_width(ring, 0, 0);
    }
    return group;
}
void UiController::buildScale() {
    for (int i=0; i<tickCount; ++i) {
        const double a = (180.0 + i*180.0/(tickCount-1))*3.141592653589793/180.0;
        auto& p = tickPoints_[i];
        p[0] = {lv_value_precise_t(width_*.5 + width_*.39*std::cos(a)), lv_value_precise_t(height_*.54 + width_*.39*std::sin(a))};
        p[1] = {lv_value_precise_t(width_*.5 + width_*.36*std::cos(a)), lv_value_precise_t(height_*.54 + width_*.36*std::sin(a))};
        ticks_[i] = lv_line_create(face_); lv_line_set_points(ticks_[i], p.data(), 2);
        style::set_line_width(ticks_[i], std::max(2, width_/125), 0);
        lv_obj_remove_flag(ticks_[i], LV_OBJ_FLAG_CLICKABLE);
    }
    const auto* font = width_ < 400 ? &lv_font_montserrat_12 : &lv_font_montserrat_14;
    for (auto& item : scaleLabels_) item = label(face_, "", width_/7, 0, font);

}
void UiController::updateScale(int selected, int maximum, const std::string& suffix, uint32_t accent, bool negative) {
    const int progress = negative ? percentOf(selected+maximum,maximum*2) : percentOf(selected, maximum);
    for (int i=0; i<tickCount; ++i)
        style::set_line_color(ticks_[i], lv_color_hex((negative ? (i*1000/(tickCount-1) >= std::min(500,progress) && i*1000/(tickCount-1) <= std::max(500,progress)) : progress > 0 && i*1000/(tickCount-1) <= progress) ? accent : 0x46526b), 0);
    const bool weightScale = page_ == Screen::Main || (page_ == Screen::Drops && draftDrops_.mode == DropMode::Stairs) ||
        ((page_ == Screen::Magic || page_ == Screen::Eccentric) && !draftPercent_);
    const int unitTicks = weightScale ? ticksPerUnit(app_.report().unit) : 1;
    const auto signature=std::to_string(maximum)+":"+std::to_string(negative)+":"+
        std::to_string(weightScale)+":"+std::to_string(unitTicks);
    // An overlapping landmark is intentionally blank. Recreating then hiding
    // its text every loop invalidates the whole panel even when nothing changed.
    if(signature==scaleSignature_) return;
    scaleSignature_=signature;
    const int displayMax = maximum/unitTicks;
    const int step = (displayMax <= 10 ? std::max(1,displayMax/4) : displayMax <= 100 ? 20 : displayMax <= 300 ? 50 : 100)*unitTicks;
    std::vector<int> landmarks{0};
    for (int n=step;n<maximum && landmarks.size()<6;n+=step) landmarks.push_back(n);
    if (maximum > 0) {
        // Reserve enough arc distance for the endpoint label.
        while (landmarks.size() > 1 && maximum-landmarks.back() < maximum*0.12) landmarks.pop_back();
        landmarks.push_back(maximum);
    }
    if(negative) landmarks = {-maximum,-maximum/2,0,maximum/2,maximum};
    for (size_t i=0;i<7;++i) {
        if(i >= landmarks.size()) { value(scaleLabels_[i], ""); continue; }
        const int n=landmarks[i];
        const double a=(180.0+(maximum > 0 ? (negative ? 90.0*(n+maximum)/maximum : 180.0*n/maximum) : 0))*3.141592653589793/180;
        value(scaleLabels_[i],std::string(negative && n > 0 ? "+" : "")+(weightScale ? weightText(n,app_.report().unit) : std::to_string(n)));
        style::set_align(scaleLabels_[i],LV_ALIGN_TOP_LEFT);
        lv_obj_set_pos(scaleLabels_[i],int(width_*.5+width_*.445*std::cos(a))-width_/14,
            int(height_*.54+width_*.445*std::sin(a))-7);
    }
    // Use actual text extents as well as angular spacing. Fractional endpoints
    // and small round screens need more room than an integer-only heuristic.
    lv_obj_update_layout(face_);
    if (landmarks.size() > 2 && !negative) {
        auto* endpoint = scaleLabels_[landmarks.size()-1];
        lv_area_t end; lv_obj_get_coords(endpoint, &end);
        for (size_t i=1; i+1<landmarks.size(); ++i) {
            lv_area_t mark; lv_obj_get_coords(scaleLabels_[i], &mark);
            const int gap = std::max(3,width_/90);
            if (mark.x1 < end.x2+gap && mark.x2+gap > end.x1 && mark.y1 < end.y2+gap && mark.y2+gap > end.y1)
                value(scaleLabels_[i], "");
        }
    }
    (void)suffix;

}

UiController::UiController(HardwarePlatform& h, TrainingApplication& app, RemoteConfigurationStore* store, RuntimeMode* runtime, WifiSetup* wifi, FirmwareUpdate* updates, DebugReports* reports) : hardware_(h), app_(app), runtime_(runtime), wifi_(wifi), updates_(updates), reports_(reports), store_(store) {
    if (store_) config_ = store_->load();
    if (!h.capabilities().coreInputsValid()) throw std::runtime_error("Unsupported display/input target");
    lv_init(); width_ = h.display().width(); height_ = h.display().height();
    drawBuffer_.resize(size_t(width_)*height_); display_ = lv_display_create(width_, height_);
    lv_display_set_color_format(display_, LV_COLOR_FORMAT_XRGB8888); lv_display_set_user_data(display_, this);
    lv_display_set_buffers(display_, drawBuffer_.data(), nullptr, uint32_t(drawBuffer_.size()*4), LV_DISPLAY_RENDER_MODE_FULL);
    lv_display_set_flush_cb(display_, [](lv_display_t* display, const lv_area_t* area, uint8_t* data) {
        auto* s = static_cast<UiController*>(lv_display_get_user_data(display));
        s->hardware_.display().flush(area->x1, area->y1, lv_area_get_width(area), lv_area_get_height(area), reinterpret_cast<uint32_t*>(data));
        lv_display_flush_ready(display);
    });
    touch_ = lv_indev_create(); lv_indev_set_type(touch_, LV_INDEV_TYPE_POINTER);
    lv_indev_set_display(touch_, display_); lv_indev_set_user_data(touch_, this);
    lv_indev_set_long_press_time(touch_, 650);
    lv_indev_set_read_cb(touch_, [](lv_indev_t* input, lv_indev_data_t* data) {
        auto* s = static_cast<UiController*>(lv_indev_get_user_data(input)); const auto p = s->hardware_.touch().sample();
        data->continue_reading=s->hardware_.touch().hasPending();
        if(p.cancelled) {
            // A lost sample or old-screen gesture is never a synthetic click.
            s->touchMoved_=true;s->touchDown_=false;s->wakeTouch_=false;
            lv_indev_wait_release(input);
            data->point={int(p.x*(s->width_-1)),int(p.y*(s->height_-1))};
            data->state=LV_INDEV_STATE_RELEASED;return;
        }
        if (p.pressed) {
            s->activityAt_ = s->hardware_.nowMs();
            if (s->sleeping_) { s->sleeping_ = false; s->wakeTouch_ = true; }
        }
        const int x=int(p.x*(s->width_-1)), y=int(p.y*(s->height_-1));
        if(p.pressed && !s->touchDown_) {
            s->touchStartX_=x;s->touchStartY_=y;s->touchMoved_=false;s->modeGestureUntil_=0;
            s->touchStartedAt_=s->hardware_.nowMs();s->debugConsumed_=false;
        }
        // Allow normal finger drift on controls (5% of the short display side:
        // 18 px on K5). Paged pickers keep the stricter 2.5% swipe rejection so
        // a short attempted swipe cannot select a device or apply a preset.
        // Include release position, even if a busy frame skipped the movement.
        const bool paged=s->page_==Screen::Devices || s->page_==Screen::Presets;
        const int tapSlop=std::max(1,std::min(s->width_,s->height_)/(paged ? 40 : 20));
        if ((p.pressed || s->touchDown_) && (std::abs(x-s->touchStartX_) > tapSlop || std::abs(y-s->touchStartY_) > tapSlop)) s->touchMoved_=true;
        const auto holdNow=s->hardware_.nowMs();
        if((p.pressed || s->touchDown_) && !s->touchMoved_ && !s->wakeTouch_ && !s->debugDialog_ &&
           !s->debugConsumed_ && holdNow>=s->touchStartedAt_ && holdNow-s->touchStartedAt_>=debugHoldMs) {
            s->debugPending_=s->debugConsumed_=true;
            lv_indev_wait_release(input);
        }
        if(!p.pressed && s->touchDown_ && s->touchMoved_ && !s->wakeTouch_ && !s->debugConsumed_ && !s->debugDialog_) {
            const int dx=x-s->touchStartX_, dy=y-s->touchStartY_;
            std::printf("Touch drag page=%d dx=%d dy=%d\n",int(s->page_),dx,dy);
            if(std::abs(dx) > s->width_/8 && std::abs(dx) > std::abs(dy)) {
                if(s->page_ == Screen::Devices) s->act(dx<0 ? NextDevices : PreviousDevices,"",false);
                if(s->page_ == Screen::Presets) s->rotate(dx<0 ? 1 : -1);
            }
        }
        s->touchDown_=p.pressed;
        data->point = {x,y};
        data->state = p.pressed && !s->wakeTouch_ && !s->debugConsumed_ ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
        if (!p.pressed) s->wakeTouch_ = false;
    });
    lastTickMs_ = activityAt_ = hardware_.nowMs();
}
UiController::~UiController() { hardware_.restAlert(0); lv_indev_delete(touch_); lv_display_delete(display_); lv_deinit(); }
void UiController::dismissRestAlert() {
    hardware_.restAlert(0);
    restBuzzing_ = restNotice_ = false;
    if(restButton_) lv_obj_add_flag(restButton_,LV_OBJ_FLAG_HIDDEN);
    activityAt_ = hardware_.nowMs();
}
void UiController::updateRestAlert(uint64_t now) {
    const auto rest=app_.restSeconds();
    if(app_.fresh() && (!rest || *rest==0)) restAlerted_=false;
    if(config_.restAlertSeconds==0) {
        if(restNotice_) dismissRestAlert();
        return;
    }
    if(rest && *rest>=config_.restAlertSeconds && !restAlerted_) {
        restAlerted_=true;
        // An unacknowledged notice stays visible; never extend its buzz deadline.
        if(!restNotice_) {
            restNotice_=restBuzzing_=true;restBuzzUntil_=now+restBuzzMs;
            hardware_.restAlert(restBuzzMs);
            // Separate top-layer button survives screen rebuilds. It covers only
            // the upper banner, leaving the weight/unload controls accessible.
            if(!restButton_) {
                restButton_=lv_button_create(lv_display_get_layer_top(display_));
                lv_obj_set_size(restButton_,width_*56/100,height_*13/100);
                lv_obj_set_pos(restButton_,width_*22/100,height_*8/100);
                lv_obj_remove_flag(restButton_,LV_OBJ_FLAG_SCROLLABLE);
                lv_obj_set_style_radius(restButton_,LV_RADIUS_CIRCLE,0);
                lv_obj_set_style_bg_color(restButton_,lv_color_hex(amber),0);
                auto* caption=label(restButton_,"Rest over",width_*52/100,0);
                color(caption,bg);lv_obj_center(caption);
                lv_obj_add_event_cb(restButton_,[](lv_event_t* e) {
                    auto* ui=static_cast<UiController*>(lv_event_get_user_data(e));
                    if(!ui->touchMoved_ && !ui->debugConsumed_ && !ui->debugDialog_) ui->dismissRestAlert();
                },LV_EVENT_CLICKED,this);
            }
            if(touchDown_) lv_indev_wait_release(touch_);
            if(debugDialog_) lv_obj_add_flag(restButton_,LV_OBJ_FLAG_HIDDEN);
            else lv_obj_remove_flag(restButton_,LV_OBJ_FLAG_HIDDEN);
            activityAt_=now;sleeping_=dimmed_=false;
        }
    }
    if(restBuzzing_ && now>=restBuzzUntil_) {
        hardware_.restAlert(0);restBuzzing_=false;
    }
}
void UiController::readTouch() {
    const auto now=hardware_.nowMs();lv_tick_inc(uint32_t(now-lastTickMs_));lastTickMs_=now;
    lv_indev_read(touch_);
}
void UiController::navigate(Screen page) {
    diagnostic("Screen="+std::to_string(int(page)));
    modeGestureUntil_ = 0;
    if (page == Screen::ForgetDevice && forgetId_.empty()) { forgetId_=app_.selectedDevice();forgetName_=app_.name(); }
    rotaryAdjustment_.reset();
    if (page == Screen::OverridePreset && presetIndex_ >= int(config_.presets.size())) page = Screen::SavePreset;
    if (page == Screen::SavePreset || page == Screen::OverridePreset) saveDraft_ = app_.currentPreset();
    editDirty_ = returnAfterEdit_ = false;
    next_ = page; rebuild_ = true;
    const auto r = app_.report();
    if (page == Screen::Magic || page == Screen::Eccentric) {
        rememberModifiers();
        const auto amount = editorModifier(page == Screen::Magic);
        draftMode_ = r.settings.magicMode;
        draftPercent_ = amount.percent;
        if (page == Screen::Magic && !r.magicSupported[int(draftMode_)])
            for (int i = 0; i < 3; ++i) if (r.magicSupported[i]) { draftMode_ = MagicMode(i); break; }
        draftValue_ = amount.value;
    }
    if (page == Screen::Drops) { draftDrops_ = app_.drops(); dropField_ = 1; }
}
void UiController::act(int action, const std::string& id, bool held) {
    // Installation reserves the disconnected remote until failure/restart.
    // Unload is never gated, including if connection state unexpectedly changes.
    if(updates_ && action!=Unload) {
        const auto phase=updates_->status().phase;
        if(phase==UpdatePhase::Installing || phase==UpdatePhase::Ready) return;
    }
    activityAt_ = hardware_.nowMs();
    if(action==DismissConnection) {app_.leave();navigate(Screen::Devices);return;}
    rotaryAdjustment_.reset(); // A new touch selection starts with precise control.
    if (!held && (action == PreviousDevices || action == NextDevices)) {
        const size_t count = (app_.devices().size()+3)/4;
        if (action == PreviousDevices && devicePage_ > 0) --devicePage_;
        if (action == NextDevices && devicePage_+1 < count) ++devicePage_;
        next_ = Screen::Devices; rebuild_ = true; return;
    }
    if (action == Select) {
        if (held && app_.remembered(id)) {
            forgetId_ = id;
            for (const auto& d : app_.devices()) if (d.id == id) forgetName_ = d.name;
            navigate(Screen::ForgetDevice);
        } else if (!held) {if(app_.select(id)) navigate(Screen::Main);else if(!app_.message().empty()) navigate(Screen::ConnectionNotice);}
        return;
    }
    if (action == CancelForget) { navigate(Screen::Devices); return; }
    if (action == ConfirmForget) {
        if (app_.forget(forgetId_)) {
            modifierMemory_.erase(forgetId_); unitMemory_.erase(forgetId_);
            navigate(Screen::Devices);
        }
        else value(message_,app_.message());
        return;
    }
    if (action == BackDevices && !held) { diagnostic("User released Bluetooth connection");app_.leave(); navigate(Screen::Devices); return; }
    if (action == Weight) {
        if (held) app_.autoLoad();
        else app_.toggleWeight();
        return;
    }
    if (action == OpenPresets && !held) { presetIndex_ = std::max(0,activePreset_); navigate(Screen::Presets); return; }
    if (action == ChoosePreset) {
        if (presetIndex_ == int(config_.presets.size())) { saveDraft_ = app_.currentPreset(); navigate(Screen::SavePreset); }
        else if (held) { saveDraft_ = app_.currentPreset(); navigate(Screen::OverridePreset); }
        else if (app_.applyPreset(config_.presets[presetIndex_])) { pendingPreset_ = presetIndex_; navigate(Screen::Main); }
        else if (!app_.presetCompatibilityError().empty()) { presetNotice_ = app_.presetCompatibilityError(); navigate(Screen::PresetNotice); }
        return;
    }
    if (held) {
        if (action != Magic && action != EO && action != Drops) return;
    }
    if (action == PrevPreset || action == NextPreset) { rotate(action == PrevPreset ? -1 : 1); return; }
    if (action == CancelRecipe) { if(page_ == Screen::PresetNotice) { navigate(Screen::Presets); return; } navigate(page_ == Screen::Presets ? Screen::Main : Screen::Presets); return; }
    if (action == SaveRecipe) {
        auto previous = config_;
        if (presetIndex_ == int(config_.presets.size()) && config_.presets.size() < 99) config_.presets.push_back(saveDraft_);
        else if (presetIndex_ < int(config_.presets.size())) config_.presets[presetIndex_] = saveDraft_;
        if (store_ && !store_->save(config_)) { config_ = previous; value(message_, "Could not save preset"); return; }
        if (samePreset(app_.currentPreset(),app_.presetForCurrentDevice(saveDraft_))) activePreset_ = presetIndex_;
        else if (app_.applyPreset(saveDraft_)) pendingPreset_ = presetIndex_;
        navigate(Screen::Main); return;
    }
    if (action == OpenSettings) { navigate(Screen::Settings); return; }
    if(action==CheckUpdates) {if(updates_) updates_->check();return;}
    if(action==InstallUpdate) {
        if(updates_ && !app_.connected() && !app_.connecting() && !app_.pending() && updates_->install()) app_.setMaintenance(true);
        return;
    }
    if (action == CloseSettings) { if(page_==Screen::Wifi && wifi_) wifi_->stop();saveConfiguration(); navigate(page_ == Screen::Settings ? Screen::Devices : Screen::Settings); return; }
    if(action==WifiSetupToggle) {
        if(wifi_ && !app_.connected() && !app_.connecting()) {
            if(wifi_->status().setupActive) wifi_->stop();else wifi_->start();
        }
        return;
    }
    if (action >= ThemeField && action <= SleepField) { settingsField_ = action-ThemeField; return; }
    if (action >= OpenWifi && action <= OpenReset) {
        navigate(action == OpenWifi ? Screen::Wifi : action == OpenUpdates ? Screen::Updates : action == OpenAbout ? Screen::About : Screen::Reset);
        if(action==OpenWifi && wifi_ && !app_.connected() && !app_.connecting()) wifi_->start();
        return;
    }
    if (action == ConfirmReset) {
        auto old = config_; config_ = {};
        if (store_ && !store_->save(config_)) { config_ = old; value(message_, "Reset could not be saved"); return; }
        modifierMemory_.clear(); unitMemory_.clear();
        activePreset_ = pendingPreset_ = -1; navigate(Screen::Settings); return;
    }
    if (action == ModeFocus || action == AmountFocus) return;
    if (action == BackMain) {
        if (editMaximum() == 0) { navigate(Screen::Main); return; }
        if(editDirty_) { returnAfterEdit_ = true; editedAt_ = 0; }
        else if(!app_.report().loaded || app_.report().liveModifierAmountsSupported) { edited(); returnAfterEdit_=true; editedAt_=0; }
        else navigate(Screen::Main);
        return;
    }
    if(action == UnitFocus) return;
    if(action == UnitPercent || action == UnitAbsolute) {
        const bool percent=action == UnitPercent;
        if(app_.report().modifierUnitsSelectable && (!percent || app_.report().percentageSupported) && percent != draftPercent_) {
            auto& memory = unitSelection();
            auto& target = percent ? memory.percent : memory.absolute;
            const int base = app_.report().settings.weight;
            if(percent && base <= 0) return;
            const int converted = target.value_or(int(std::lround(percent ? draftValue_*100.0/base : draftValue_*base/100.0)));
            draftPercent_ = percent; draftValue_ = std::clamp(converted,std::min(1,editMaximum()),editMaximum());
            target = draftValue_; edited();
        }
        return;
    }
    if (action == Unload && !held) { app_.unload(); return; }
    if (action == Magic || action == EO || action == Drops) {
        rememberModifiers();
        weightFocused_ = false; activePreset_ = pendingPreset_ = -1;
        const auto r = app_.report();
        if (action == Magic && std::none_of(r.magicSupported.begin(), r.magicSupported.end(), [](bool b) { return b; })) return;
        if (action == EO && !r.eccentricSupported) return;
        const bool enabled = action == Magic ? r.settings.magic.enabled : action == EO ? r.settings.eccentric.enabled : app_.drops().enabled;
        if (held || !enabled) navigate(action == Magic ? Screen::Magic : action == EO ? Screen::Eccentric : Screen::Drops);
        else if (action == Drops) app_.toggleDrops(); else app_.toggleModifier(action == Magic);
        return;
    }
    if (held) return;
    if (action == Apply) {
        if (page_ == Screen::Drops) {
            draftDrops_.enabled = true;
            if(!app_.applyDrops(draftDrops_)) {
                std::printf("Drops Apply refused: %s\n",app_.message().c_str());return;
            }
        }
        else app_.applyModifier(page_ == Screen::Magic, draftMode_, draftValue_, draftPercent_);
        navigate(Screen::Main);
    } else if (action >= ModeIC && action <= ModeMTN) {
        auto mode = MagicMode(action-ModeIC);
        if (app_.report().magicSupported[int(mode)]) { draftMode_ = mode; unitMemory_[app_.selectedDevice()][0] = {}; edited(); }
    } else if (action == Stairs || action == RP) {
        draftDrops_.mode = action == Stairs ? DropMode::Stairs : DropMode::RestPause;
        next_ = page_; rebuild_ = true;
    } else if (action >= FieldDrops && action <= FieldHold) dropField_ = action-FieldDrops;
}
void UiController::build() {
    scaleSignature_.clear();
    if (touchDown_) lv_indev_wait_release(touch_);
    hardware_.touch().discardPending();
    if (face_) lv_obj_delete(face_);
    bindings_.clear(); deviceRows_.clear();
    weight_ = title_ = status_ = hint_ = message_ = arc_ = unit_ = limit_ = dropTitle_ = nullptr;
    center_ = battery_ = twinIcon_ = nullptr; ticks_.fill(nullptr);
    presetTile_ = presetValue_ = stats_ = start_ = nextWeight_ = nullptr;
    holdBar_ = nullptr;
    wifiBody_=wifiButton_=nullptr;
    updateBody_=updateCheck_=updateInstall_=nullptr;
    for (auto& item : settingsValues_) item = nullptr;
    for (auto& item : liveValues_) item = nullptr;
    for (auto& item : unitChoices_) item = nullptr;
    for (int i=0; i<3; ++i) {
        stairs_[i] = tiles_[i] = miniArc_[i] = miniValue_[i] = modeLabels_[i] = choice_[i] = nullptr;
        choiceNames_[i] = nullptr;
    }
    for (auto& item : scaleLabels_) item = nullptr;
    page_ = next_; rebuild_ = false;
    auto* screen = lv_display_get_screen_active(display_);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    style::set_bg_color(screen, lv_color_hex(0x030507), 0);
    face_ = lv_obj_create(screen); lv_obj_set_size(face_, width_, height_); style::center(face_);
    lv_obj_remove_flag(face_, LV_OBJ_FLAG_SCROLLABLE); lv_obj_set_style_pad_all(face_, 0, 0);
    lv_obj_set_style_radius(face_, LV_RADIUS_CIRCLE, 0); style::set_bg_color(face_, lv_color_hex(bg), 0);
    style::set_text_color(face_, lv_color_hex(bright), 0);
    style::set_border_width(face_, std::max(2, width_/160), 0);
    style::set_border_color(face_, lv_color_hex(0x2c3440), 0);
    const auto* small = width_ < 400 ? &lv_font_montserrat_12 : &lv_font_montserrat_14;
    const auto* medium = width_ < 400 ? &lv_font_montserrat_14 : &lv_font_montserrat_18;
    battery_ = label(face_, "", width_*45/100, height_*91/100, small); color(battery_, soft);
    if (int(page_) >= int(Screen::Presets)) { buildExtra(); return; }
    if (page_ == Screen::Devices) {
        auto* gear = button(face_, LV_SYMBOL_SETTINGS, width_*44/100, height_*4/100, width_*12/100, height_*9/100, OpenSettings);
        color(gear, accent());
        const auto& devices = app_.devices();
        const size_t pages = std::max(size_t(1), (devices.size()+3)/4);
        devicePage_ = std::min(devicePage_, pages-1);
        for (size_t i=0; i<4 && devicePage_*4+i < devices.size(); ++i) {
            const auto& d = devices[devicePage_*4+i];
            const int size = width_*32/100;
            auto* tile = disc(face_, width_*(17+int(i%2)*34)/100, height_*(14+int(i/2)*34)/100, size, accent(), d.online);
            if (d.online || app_.remembered(d.id)) bind(tile, Select, d.id);
            else lv_obj_remove_flag(tile, LV_OBJ_FLAG_CLICKABLE);
            auto* icon = d.twinPeer.empty() ? linkIcon(tile, size*40/100, d.online ? bright : muted) :
                twinIcon(tile, size*45/100, d.online ? accent() : muted);
            style::align(icon, LV_ALIGN_TOP_MID, 0, size*16/100);
            auto* name = label(tile, d.name, size*90/100, size*49/100, small);
            lv_label_set_long_mode(name, LV_LABEL_LONG_DOT);
            lv_obj_set_height(name, small->line_height*2);
            lv_label_set_long_mode(name,LV_LABEL_LONG_WRAP);
            lv_obj_set_y(name,size*49/100);
            color(name, d.online ? bright : muted);
            deviceRows_.push_back({tile, d.id});
        }
        message_ = label(face_, devices.empty() ? "Searching for VOLTRAs..." : "", width_*74/100, height_*83/100, small);
        color(message_, soft);
        if (!runtime_ || runtime_->simulated()) {
            auto* mode = label(face_,"SIMULATOR",width_*50/100,height_*91/100,small);
            color(mode,amber);
            lv_obj_add_flag(battery_,LV_OBJ_FLAG_HIDDEN);
        }
        if (pages > 1) {
            auto* prev = button(face_, "<", width_*33/100, height_*80/100, width_*12/100, height_*10/100, PreviousDevices);
            auto* next = button(face_, ">", width_*58/100, height_*80/100, width_*12/100, height_*10/100, NextDevices);
            color(prev, devicePage_ ? accent() : muted); color(next, devicePage_+1 < pages ? accent() : muted);
            label(face_, std::to_string(devicePage_+1)+" / "+std::to_string(pages), width_*16/100, height_*82/100, small);
        }
        return;
    }
    buildScale();
    const bool rp = page_ == Screen::Drops && draftDrops_.mode == DropMode::RestPause;
    title_ = label(face_, page_ == Screen::Main ? app_.name() : page_ == Screen::Magic ? "" :
        page_ == Screen::Eccentric ? "" : "Dropset", width_*65/100, height_*23/100, medium);
    lv_label_set_long_mode(title_, LV_LABEL_LONG_DOT);
    if (page_ == Screen::Main) {
        // A separate, generous device header ends before the weight hit region.
        lv_obj_set_y(title_,height_*21/100); lv_obj_set_height(title_,height_*10/100);
        lv_obj_set_style_pad_top(title_,height_/50,0);
        bind(title_, BackDevices); lv_obj_set_ext_click_area(title_, height_/100);
    }
    status_ = label(face_, "", width_*62/100, height_*30/100, small);
    color(status_, soft);
    if (page_ == Screen::Main) {
        twinIcon_ = twinIcon(face_, width_*7/100, accent());
        lv_obj_set_pos(twinIcon_, width_*17/100, height_*30/100);
    } else { bind(status_, Unload); lv_obj_set_ext_click_area(status_, height_/100); }
    if (!rp) {
        const uint32_t centerAccent = accent();
        const int size = width_*42/100;
        center_ = disc(face_, width_*29/100, height_*32/100, size, centerAccent, true);
        if (page_ != Screen::Drops) {
            style::set_border_width(center_, 0, 0); style::set_shadow_width(center_, 0, 0);
            style::set_bg_opa(center_, LV_OPA_TRANSP, 0);
        }
        if (page_ == Screen::Main) bind(center_, Weight);
        else lv_obj_remove_flag(center_, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_width(center_,width_*80/100);
        lv_obj_set_x(center_,width_*10/100);
        lv_obj_set_style_radius(center_,0,0);
        style::set_bg_opa(center_,LV_OPA_TRANSP,0);
        style::set_border_width(center_,0,0);
        style::set_shadow_width(center_,0,0);
        lv_obj_add_flag(center_,LV_OBJ_FLAG_OVERFLOW_VISIBLE);
        weight_ = label(center_, "--", size*96/100, size*20/100, &lv_font_montserrat_48);
        lv_obj_set_style_transform_pivot_x(weight_, size*96/200, 0);
        lv_obj_set_style_transform_pivot_y(weight_, lv_font_montserrat_48.line_height/2, 0);
        lv_obj_set_style_transform_scale_x(weight_, width_*560/480, 0);
        lv_obj_set_style_transform_scale_y(weight_, width_*560/480, 0);
        unit_ = label(center_, "", size*90/100, size*64/100, small); color(unit_, soft);
        hint_ = label(center_, "", size*90/100, size*77/100, small); color(hint_, soft);
        if (page_ == Screen::Main) {
            lv_obj_remove_flag(weight_,LV_OBJ_FLAG_CLICKABLE);
            style::align(unit_, LV_ALIGN_TOP_MID, 0, size*59/100);
            style::align(hint_, LV_ALIGN_TOP_MID, 0, size*70/100);
        }
    }
    if (page_ == Screen::Main) {
        lv_obj_set_y(center_,height_*35/100); lv_obj_set_height(center_,height_*23/100);
        lv_obj_move_foreground(title_);
        for (int i=0; i<3; ++i) {
            const int size = width_*19/100;
            tiles_[i] = disc(face_, width_*(9+i*21)/100, height_*61/100, size, accent(), false);
            bind(tiles_[i], i == 0 ? Magic : i == 1 ? EO : Drops);
            // Extend into the small gaps without crossing adjacent tile targets.
            lv_obj_set_ext_click_area(tiles_[i],width_/120);
            if (i < 2) miniArc_[i] = arc(tiles_[i], size*92/100, accent());
            miniValue_[i] = label(tiles_[i], "", size*96/100, size*32/100, medium);
            if (i == 0) {
                modeLabels_[0] = label(face_, "", size, height_*55/100, medium); style::set_align(modeLabels_[0],LV_ALIGN_TOP_LEFT); lv_obj_set_x(modeLabels_[0],width_*9/100);
                bind(modeLabels_[0],Magic);
                lv_obj_set_ext_click_area(modeLabels_[0],width_/120);

            } else if (i == 1) { modeLabels_[2] = label(face_, "EO", size, height_*55/100, medium); bind(modeLabels_[2],EO); lv_obj_set_ext_click_area(modeLabels_[2],width_/120); }
            if(i == 1) { style::set_align(modeLabels_[2],LV_ALIGN_TOP_LEFT); lv_obj_set_x(modeLabels_[2],width_*30/100); }
            if(i == 2) {
                dropTitle_ = label(face_, "", size, height_*55/100, medium); bind(dropTitle_,Drops);
                lv_obj_set_ext_click_area(dropTitle_,width_/120);
                style::set_align(dropTitle_,LV_ALIGN_TOP_LEFT); lv_obj_set_x(dropTitle_,width_*51/100);
                for (int step=0; step<3; ++step) {
                    auto* bar = lv_obj_create(face_); lv_obj_remove_style_all(bar);
                    stairs_[step] = bar;
                    lv_obj_set_size(bar, size/9, (3-step)*size/16);
                    lv_obj_set_pos(bar, width_*57/100+step*size/9, height_*59/100-(3-step)*size/16);
                    style::set_bg_color(bar, lv_color_hex(muted), 0); style::set_bg_opa(bar, LV_OPA_COVER, 0);
                    lv_obj_remove_flag(bar, LV_OBJ_FLAG_CLICKABLE);
                }
            }
        }
        presetTile_ = disc(face_, width_*72/100, height_*61/100, width_*19/100, accent(), false);
        bind(presetTile_, OpenPresets);
        auto* presetCaption=label(face_, LV_SYMBOL_SAVE, width_*19/100, height_*55/100, medium); bind(presetCaption,OpenPresets);
        style::set_align(presetCaption,LV_ALIGN_TOP_LEFT); lv_obj_set_x(presetCaption,width_*72/100);
        presetValue_ = label(presetTile_, "P", width_*18/100, 0, &lv_font_montserrat_18);
        style::center(presetValue_);
        stats_ = label(face_, "", width_*64/100, height_*82/100, &lv_font_montserrat_18);
        lv_obj_set_y(battery_,height_*94/100);
        message_ = label(face_, "", width_*68/100, height_*88/100, small); color(message_, soft);
        lv_obj_add_flag(hint_,LV_OBJ_FLAG_HIDDEN);
        return;
    }
    if (page_ == Screen::Magic) {
        const MagicMode order[] = {MagicMode::InverseChains, MagicMode::Mountain, MagicMode::Chains};
        auto* picker=button(face_,"",width_*18/100,height_*64/100,width_*64/100,height_*13/100,ModeFocus);
        glow(picker,accent(),true);
        for (int i=0;i<3;++i) {
            const int index=int(order[i]);
            choice_[index]=label(picker,magicName(order[i]),width_*20/100,height_*3/100,medium);
            style::set_align(choice_[index],LV_ALIGN_TOP_LEFT);
            lv_obj_set_x(choice_[index],width_*(1+i*21)/100); bind(choice_[index],ModeIC+index);
        }
    } else if (page_ == Screen::Drops) {
        lv_obj_set_y(title_,height_*19/100);
        // Separate mode selection from the status/unload target. Captured taps
        // just below the old 7%-high mode button reached unload instead.
        lv_obj_set_y(status_,height_*74/100);
        style::set_text_font(status_,small,0);
        for (int i=0; i<2; ++i) {
            choice_[i] = button(face_, i == 0 ? "Stairs" : "RP", width_*(24+i*28)/100, height_*26/100,
                width_*24/100, height_*9/100, i == 0 ? Stairs : RP);
            lv_obj_set_ext_click_area(choice_[i],std::max(4,width_/60));
        }
        if (rp) {
            for (int i=0; i<3; ++i) {
                const int size = width_*24/100;
                tiles_[i] = disc(face_, width_*(12+i*26)/100, height_*39/100, size, accent(), i == dropField_);
                bind(tiles_[i], FieldDrops+i);
                miniArc_[i] = arc(tiles_[i], size*91/100, accent());
                auto* caption = label(tiles_[i], i == 0 ? "Drops" : i == 1 ? "Drop %" : "Hold", size*90/100, size*24/100, small);
                color(caption, soft);
                miniValue_[i] = label(tiles_[i], "", size*96/100, size*45/100,
                    width_ < 400 ? &lv_font_montserrat_28 : &lv_font_montserrat_36);
                color(miniValue_[i], bright);
            }
            hint_ = label(face_, "", width_*55/100, height_*67/100, &lv_font_montserrat_18);
        } else {
            lv_obj_set_y(center_, height_*39/100); bind(center_,Apply); bind(weight_,Apply);
        }
    }
    if(page_ == Screen::Magic || page_ == Screen::Eccentric) {
        bind(center_,BackMain);
        bind(weight_,BackMain);
        // Mode and unit selectors are touch-only; rotation always edits amount.
        for(auto* tick : ticks_) { bind(tick,AmountFocus); lv_obj_set_ext_click_area(tick,width_/60); }
        for(auto* mark : scaleLabels_) { bind(mark,AmountFocus); lv_obj_set_ext_click_area(mark,width_/60); }
        const int y=page_ == Screen::Magic ? 78 : 68;
        const auto r=app_.report();
        const bool both=r.modifierUnitsSelectable && r.percentageSupported;
        auto* units=button(face_,"",width_*20/100,height_*y/100,width_*60/100,height_*12/100,UnitFocus);
        glow(units,accent(),true);
        for(int i=0;i<2;++i) {
            const bool percent=i == 0;
            if(!both && percent != draftPercent_) continue;
            unitChoices_[i]=label(units,percent ? "%" : unitName(r.unit),width_*(both ? 28 : 58)/100,height_*2/100,medium);
            style::set_align(unitChoices_[i],LV_ALIGN_TOP_LEFT);
            lv_obj_set_x(unitChoices_[i],width_*(both ? 1+i*30 : 1)/100);
            bind(unitChoices_[i],percent ? UnitPercent : UnitAbsolute);
        }
        // Returning never disables the modifier; toggling lives on the main tile.

        lv_obj_add_flag(battery_,LV_OBJ_FLAG_HIDDEN);
        return;
    }
    const int applyY = page_ == Screen::Magic ? 85 : page_ == Screen::Eccentric ? 78 : 81;
    const int applyH = page_ == Screen::Magic ? 7 : 10;
    auto* apply = button(face_, "Apply", width_*26/100, height_*applyY/100, width_*48/100, height_*applyH/100, Apply);
    lv_obj_set_ext_click_area(apply,std::max(4,width_/50));
    glow(apply, accent(), true);
    style::set_text_font(lv_obj_get_child(apply, 0), medium, 0);
    for (auto* legend : scaleLabels_) lv_obj_move_foreground(legend);
}
int UiController::editMaximum() const {
    const auto r = app_.report();
    if (page_ == Screen::Drops) return draftDrops_.mode == DropMode::Stairs ? r.maxWeight : dropField_ == 1 ? 80 : 9;
    auto s = r.settings;
    if(r.sharedModifierUnits) convertModifierUnit(page_ == Screen::Magic ? s.eccentric : s.magic,draftPercent_,s.weight);
    if (page_ == Screen::Magic) { s.magic.enabled = true; s.magic.percent = draftPercent_; }
    else { s.eccentric.enabled = true; s.eccentric.percent = draftPercent_; }
    return modifierMaximum(r, s, page_ == Screen::Magic);
}
void UiController::rotate(int delta) {
    if(page_ == Screen::Devices) {
        if (runtime_ && modeGestureUntil_ && hardware_.nowMs() <= modeGestureUntil_) {
            if (delta < 0) {
                modeGestureClicks_ += -delta;
                if (modeGestureClicks_ >= 3) {
                    modeGestureUntil_ = 0;
                    app_.leave();
                    if (!runtime_->switchMode()) value(message_,"Could not switch mode");
                }
                return;
            }
            modeGestureUntil_ = 0;
        }
        act(delta > 0 ? NextDevices : PreviousDevices,"",false); return;
    }
    if(page_ == Screen::Main || page_ == Screen::Live || page_ == Screen::Magic || page_ == Screen::Eccentric || (page_ == Screen::Drops && draftDrops_.mode == DropMode::Stairs)) {
        if (!app_.fresh()) { rotaryAdjustment_.reset(); return; }
        const auto r = app_.report();
        const bool weight = page_ == Screen::Main || page_ == Screen::Live;
        const int current = weight ? app_.weightPreview().value_or(r.settings.weight) : page_ == Screen::Drops ? draftDrops_.step : draftValue_;
        const int wholeUnit = weight || page_ == Screen::Drops || !draftPercent_ ? ticksPerUnit(r.unit) : 1;
        delta = rotaryAdjustment_.delta(delta,current,hardware_.nowMs(),wholeUnit);
        if(!delta) return; // Coarse cadence may be collecting the next detent.
    }
    if (page_ == Screen::Presets) {
        presetIndex_ = std::clamp(presetIndex_+delta,0,std::min(98,int(config_.presets.size())));
        next_ = page_; rebuild_ = true; return;
    }
    if (page_ == Screen::Settings) {
        if (settingsField_ == 0) config_.theme = (config_.theme + delta%9 + 9)%9;
        // Keep encoder sensitivity; each detent selects the next 5-second mark.
        // Existing saved values migrate only on the user's next adjustment.
        if (settingsField_ == 1) config_.restAlertSeconds = std::clamp(
            (delta>0 ? config_.restAlertSeconds/5 : (config_.restAlertSeconds+4)/5)*5+delta*5,0,3595);
        if (settingsField_ == 2) config_.dimMinutes = std::clamp(config_.dimMinutes+delta,0,120);
        if (settingsField_ == 3) config_.sleepMinutes = std::clamp(config_.sleepMinutes+delta,0,120);
        saveConfiguration();
        if(settingsField_==0) {next_=page_;rebuild_=true;}
        return;
    }
    if (page_ == Screen::Live) { app_.adjustWeight(delta); return; }
    if (page_ == Screen::Devices || int(page_) >= int(Screen::Presets) || !app_.fresh()) return;
    if (page_ == Screen::Main) { weightFocused_ = true; app_.adjustWeight(delta); return; }
    if (page_ != Screen::Drops) {
        auto& memory = unitSelection();
        draftValue_ = std::clamp(draftValue_+delta, std::min(1,editMaximum()), editMaximum());
        memory.percent.reset(); memory.absolute.reset();
        (draftPercent_ ? memory.percent : memory.absolute) = draftValue_;
        edited(); return;
    }
    if (draftDrops_.mode == DropMode::Stairs) draftDrops_.step = std::clamp(draftDrops_.step+delta, -editMaximum(), editMaximum());
    else if (dropField_ == 0) draftDrops_.drops = std::clamp(draftDrops_.drops+delta, 0, 9);
    else if (dropField_ == 1) draftDrops_.percent = std::clamp(draftDrops_.percent+delta, 0, 80);
    else draftDrops_.holdSeconds = std::clamp(draftDrops_.holdSeconds+delta, 1, std::max(1,restPauseHoldMaximum(app_.report())));
}
void UiController::update() {
    const auto battery = hardware_.batteryPercent();
    const char* batteryIcon = !battery ? LV_SYMBOL_BATTERY_EMPTY : *battery > 85 ? LV_SYMBOL_BATTERY_FULL :
        *battery > 60 ? LV_SYMBOL_BATTERY_3 : *battery > 35 ? LV_SYMBOL_BATTERY_2 :
        *battery > 10 ? LV_SYMBOL_BATTERY_1 : LV_SYMBOL_BATTERY_EMPTY;
    std::string batteryText = std::string(batteryIcon)+"  "+(battery ? std::to_string(*battery)+"%" : "--");
#ifdef VOLTRA_OFFLINE
    batteryText += "  SIM";
#endif
    value(battery_, batteryText);
    if (page_ == Screen::Devices) {
        value(message_, app_.message().empty() ? (app_.devices().empty() ? "Searching for VOLTRAs..." : "") : app_.message());
        return;
    }
    const auto r = app_.report();
    if (int(page_) >= int(Screen::Presets)) { updateExtra(); return; }
    if (page_ == Screen::Main) {
        const auto preview=app_.weightPreview();
        const int shownWeight=preview.value_or(r.settings.weight);
        largeValue(r.revision ? weightText(shownWeight,r.unit) : "--");
        const bool atMaximum = app_.fresh() && r.maxWeight > 0 && shownWeight >= r.maxWeight;
        const uint32_t maximumColor = limitAccent();
        // Retain a darker unloaded number, even when its hue indicates the limit.
        const uint32_t numberColor = r.loaded ? maximumColor : ((maximumColor & 0xfefefeu) >> 1);
        color(weight_, !app_.fresh() ? amber : preview ? muted : r.loaded ? bright : atMaximum ? numberColor : muted);
        value(unit_, "");
        value(status_, r.twin ? (r.twinKnown ? "Twin: "+r.twinPeerName : "Twin status unavailable") : "");
        color(status_, r.twinKnown ? accent() : amber);
        if(r.twinKnown && r.twin) lv_obj_remove_flag(twinIcon_, LV_OBJ_FLAG_HIDDEN); else lv_obj_add_flag(twinIcon_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_width(status_,width_*76/100);
        style::set_text_font(status_, &lv_font_montserrat_14,0);
        style::align(status_, LV_ALIGN_TOP_MID, 0, height_*30/100);
        updateScale(shownWeight, r.maxWeight, "", atMaximum ? maximumColor : accent());
        for (int i=0; i<2; ++i) {
            const auto m = i == 0 ? r.settings.magic : r.settings.eccentric;
            const uint32_t c = accent();
            color(tiles_[i], m.enabled ? bright : muted);
            value(miniValue_[i], !m.enabled && m.value == 0 ? "OFF" :
                (m.percent ? std::to_string(m.value) : weightText(m.value,r.unit))+(m.percent ? "%" : unitName(r.unit)));
            color(miniValue_[i], m.enabled ? bright : muted);
            style::set_arc_color(miniArc_[i], lv_color_hex(m.enabled ? c : 0x46536a), LV_PART_INDICATOR);
            lv_arc_set_value(miniArc_[i], percentOf(m.value, modifierMaximum(r, r.settings, i == 0)));
            style::set_border_color(tiles_[i], lv_color_hex(m.enabled ? c : 0x39475b), 0);
        }
        value(modeLabels_[0], magicName(r.settings.magicMode));
        color(modeLabels_[0], r.settings.magic.enabled ? bright : muted);
        color(modeLabels_[2], r.settings.eccentric.enabled ? bright : muted);
        const bool supported[] = {std::any_of(r.magicSupported.begin(), r.magicSupported.end(), [](bool b) { return b; }),
            r.eccentricSupported, true}; // Local dropset editor remains visible when execution is unavailable.
        for (int i=0; i<3; ++i) {
            if (supported[i]) lv_obj_remove_flag(tiles_[i], LV_OBJ_FLAG_HIDDEN);
            else lv_obj_add_flag(tiles_[i], LV_OBJ_FLAG_HIDDEN);
        }
        const auto d = app_.drops();
        // Recipe selection is local intent, independent of countdown/readiness.
        // Keep its values visible; amber distinguishes waiting from executable.
        const bool dropsActive = d.enabled;
        const uint32_t dropColor=dropsActive ? (r.liveWeightSupported ? bright : amber) : muted;
        color(tiles_[2], dropColor);
        for (auto* step : stairs_) {
            if (d.mode == DropMode::RestPause) lv_obj_add_flag(step, LV_OBJ_FLAG_HIDDEN);
            else lv_obj_remove_flag(step, LV_OBJ_FLAG_HIDDEN);
            style::set_bg_color(step, lv_color_hex(dropsActive ? accent() : muted), 0);
        }
        value(dropTitle_, d.mode == DropMode::RestPause ? LV_SYMBOL_PAUSE " " LV_SYMBOL_DOWN : "");
        value(miniValue_[2], d.mode == DropMode::Stairs ? weightText(d.step,r.unit)+unitName(r.unit) :
            std::to_string(d.drops)+"\n"+std::to_string(d.percent)+"%\n"+std::to_string(d.holdSeconds)+"s");
        style::set_text_font(miniValue_[2],width_ < 400 ? &lv_font_montserrat_14 : &lv_font_montserrat_18,0);
        style::set_text_line_space(miniValue_[2],0,0);
        style::center(miniValue_[2]);
        color(miniValue_[2], dropColor);
        glow(tiles_[2], accent(), dropsActive);
        // Prefer the explicitly selected/saved slot when duplicate recipes exist.
        if (!app_.fresh() || activePreset_ < 0 || activePreset_ >= int(config_.presets.size()) ||
            !samePreset(app_.currentPreset(),app_.presetForCurrentDevice(config_.presets[activePreset_]))) {
            activePreset_=-1;
            if(app_.fresh()) for(size_t i=0;i<config_.presets.size();++i)
                if(samePreset(app_.currentPreset(),app_.presetForCurrentDevice(config_.presets[i]))) { activePreset_=int(i); break; }
        }
        glow(presetTile_, accent(), activePreset_ >= 0);
        value(presetValue_, activePreset_ >= 0 ? "P"+std::to_string(activePreset_+1) : "P");
        color(presetValue_,activePreset_ >= 0 ? bright : muted);
        std::string summary;
        const auto counters=app_.displayCounters();
        if (counters.completedSets && counters.lastSetReps) summary = std::to_string(*counters.completedSets)+" | "+std::to_string(*counters.lastSetReps);
        if (const auto rest=app_.restSeconds()) summary += "    "+std::to_string(*rest/60)+":"+(*rest%60 < 10 ? "0" : "")+std::to_string(*rest%60);
        value(stats_,summary);
        value(message_, !app_.fresh() ? (app_.message().empty() ? "State unknown / tap weight to unload" : app_.message()) :
            r.autoLoadArmed ? "Auto Load armed / tap to cancel" : !app_.message().empty() ? app_.message() :
            preview ? "Setting weight..." : atMaximum ? "MAX" : "");
        color(message_,atMaximum && app_.message().empty() ? maximumColor : soft);
        return;
    }
    value(status_, !app_.fresh() ? "Unknown / tap to unload" :
        !app_.message().empty() ? app_.message() :
        r.loaded ? weightText(r.settings.weight,r.unit)+" "+unitName(r.unit)+" / tap to unload" :
        r.autoLoadArmed ? "Auto Load / tap to cancel" : app_.message());
    if(app_.fresh() && !r.loaded && !r.autoLoadArmed && !app_.pending())
        lv_obj_remove_flag(status_,LV_OBJ_FLAG_CLICKABLE);
    else lv_obj_add_flag(status_,LV_OBJ_FLAG_CLICKABLE);
    if (page_ == Screen::Drops) {
        if (app_.fresh() && !r.loaded && !r.autoLoadArmed && !r.liveWeightSupported)
            value(status_,"Preview only / drops unavailable");
        for (int i=0; i<2; ++i) {
            glow(choice_[i], accent(), int(draftDrops_.mode) == i);
            color(choice_[i], int(draftDrops_.mode) == i ? bright : muted);
        }
        if (draftDrops_.mode == DropMode::RestPause) {
            const int holdMaximum=restPauseHoldMaximum(r);
            draftDrops_.holdSeconds=std::clamp(draftDrops_.holdSeconds,1,std::max(1,holdMaximum));
            const int values[] = {draftDrops_.drops, draftDrops_.percent, draftDrops_.holdSeconds};
            const int maxima[] = {9, 80, holdMaximum};
            const char* suffix[] = {"", "%", "s"};
            const char* names[] = {"Drops", "Drop %", "Hold"};
            for (int i=0; i<3; ++i) {
                value(miniValue_[i], i==2 && !holdMaximum ? "--" : std::to_string(values[i])+suffix[i]);
                // All values stay active; the bright outer ring marks rotary focus.
                glow(tiles_[i], accent(), i == dropField_);
                lv_arc_set_value(miniArc_[i], percentOf(values[i], maxima[i]));
            }
            value(hint_, names[dropField_]); color(hint_, bright);
            updateScale(values[dropField_], maxima[dropField_], suffix[dropField_], accent());
            if(app_.fresh() && r.autoUnloadHoldSeconds && *r.autoUnloadHoldSeconds>0 && !r.loaded && !r.autoLoadArmed && app_.message().empty())
                value(status_,holdMaximum ? "Auto Unload: "+std::to_string(*r.autoUnloadHoldSeconds)+"s / max pause "+std::to_string(holdMaximum)+"s" :
                    "Auto Unload 1s / RP unavailable");
        } else {
            largeValue((draftDrops_.step > 0 ? "+" : "")+weightText(draftDrops_.step,r.unit)); color(weight_, bright);
            value(unit_, unitName(r.unit)); value(hint_, "After each rep");
            updateScale(draftDrops_.step, r.maxWeight, "", accent(), true);
        }
    } else {
        draftValue_ = std::clamp(draftValue_, std::min(1,editMaximum()), editMaximum());
        largeValue(editMaximum() == 0 ? "OFF" : (draftPercent_ ? std::to_string(draftValue_) : weightText(draftValue_,r.unit))+(draftPercent_ ? "%" : ""));
        const bool atMaximum = app_.fresh() && draftValue_ >= editMaximum();
        color(weight_, atMaximum ? limitAccent() : bright);
        value(unit_, "");
        value(hint_, "");
        if (editMaximum() == 0 && app_.fresh() && !r.loaded && !r.autoLoadArmed && app_.message().empty())
            value(status_,"No headroom");
        updateScale(draftValue_, editMaximum(), draftPercent_ ? "%" : "", atMaximum ? limitAccent() : accent());
        for(int i=0;i<2;++i) if(unitChoices_[i]) {
            const bool selected=(i==0) == draftPercent_;
            style::set_text_font(unitChoices_[i],selected ? &lv_font_montserrat_28 : &lv_font_montserrat_18,0);
            lv_obj_set_y(unitChoices_[i],(height_*12/100-(selected ? lv_font_montserrat_28.line_height : lv_font_montserrat_18.line_height))/2);
            color(unitChoices_[i],selected ? bright : muted);
        }
        if (page_ == Screen::Magic) for (int i=0;i<3;++i) {
            if(r.magicSupported[i]) lv_obj_remove_flag(choice_[i],LV_OBJ_FLAG_HIDDEN);
            else lv_obj_add_flag(choice_[i],LV_OBJ_FLAG_HIDDEN);
            style::set_text_font(choice_[i],int(draftMode_) == i ? &lv_font_montserrat_28 : &lv_font_montserrat_18,0);
            lv_obj_set_y(choice_[i],(height_*13/100-(int(draftMode_) == i ? lv_font_montserrat_28.line_height : lv_font_montserrat_18.line_height))/2);
            color(choice_[i],int(draftMode_) == i ? bright : muted);
        }
    }
}
void UiController::largeValue(const std::string& text) {
    if(!weight_) return;
    if(text == lv_label_get_text(weight_)) return;
    lv_label_set_text(weight_,text.c_str());
    lv_obj_set_width(weight_,LV_SIZE_CONTENT);
    lv_obj_update_layout(weight_);
    const int scale = std::min(width_*560/480, width_*64/100*256/std::max(1,int(lv_obj_get_width(weight_))));
    lv_obj_set_style_transform_pivot_x(weight_,lv_obj_get_width(weight_)/2,0);
    lv_obj_set_style_transform_pivot_y(weight_,lv_obj_get_height(weight_)/2,0);
    lv_obj_set_style_transform_scale(weight_,scale,0);
    style::align(weight_,LV_ALIGN_TOP_MID,0,width_*(page_ == Screen::Main ? 2 : 8)/100);
}
UiController::UnitSelections& UiController::unitSelection() {
    const auto r=app_.report();
    auto& memory=unitMemory_[app_.selectedDevice()][page_ == Screen::Magic ? 0 : 1];
    const auto current=draftPercent_ ? memory.percent : memory.absolute;
    if(memory.base != r.settings.weight || memory.unit != r.unit || (current && *current != draftValue_)) memory={};
    memory.base=r.settings.weight; memory.unit=r.unit;
    (draftPercent_ ? memory.percent : memory.absolute)=draftValue_;
    return memory;
}
void UiController::rememberModifiers() {
    if (!app_.fresh() || app_.selectedDevice().empty()) return;
    const auto r = app_.report();
    auto& memory = modifierMemory_[app_.selectedDevice()];
    for (int i = 0; i < 2; ++i) {
        const auto m = i == 0 ? r.settings.magic : r.settings.eccentric;
        const bool supported = i == 0 ? r.magicSupported[int(r.settings.magicMode)] : r.eccentricSupported;
        if (supported && m.enabled && m.value > 0) memory[i] = ModifierSelection{m,r.unit};
    }
}
Modifier UiController::editorModifier(bool magic) const {
    const auto r = app_.report();
    auto amount = magic ? r.settings.magic : r.settings.eccentric;
    const auto found = modifierMemory_.find(app_.selectedDevice());
    if (amount.value > 0 || !app_.fresh() || found == modifierMemory_.end()) return amount;
    const auto& saved = found->second[magic ? 0 : 1];
    if (!saved || (saved->amount.percent && !r.percentageSupported) ||
        (!r.modifierUnitsSelectable && saved->amount.percent != amount.percent)) return amount;
    amount.percent = saved->amount.percent;
    amount.value = amount.percent ? saved->amount.value : convertWeightUnits(saved->amount.value,saved->unit,r.unit);
    return amount; // Enabled flag is still the machine's; only a tap/edit can apply.
}
void UiController::edited() {
    editedAt_ = hardware_.nowMs(); editDirty_ = true;
    activePreset_ = pendingPreset_ = -1;
}
void UiController::tick() {
    const auto now = hardware_.nowMs(); lv_tick_inc(uint32_t(now-lastTickMs_)); lastTickMs_ = now;
    EventLog::instance().clock(now);
    if(updates_) {
        const auto phase=updates_->status().phase;
        app_.setMaintenance(phase==UpdatePhase::Installing || phase==UpdatePhase::Ready || (reports_ && reports_->status().busy()));
        // Saved Wi-Fi gets the shared radio first at boot. Discovery resumes
        // after the bounded startup attempt/check; manual selection still works.
        const bool savedWifi=wifi_ && wifi_->status().saved;
        app_.setDiscoveryPaused((savedWifi && updates_->startupPending()) || updates_->status().busy() ||
            (reports_ && (reports_->status().phase==ReportPhase::Queued || reports_->status().busy())));
    }
    app_.tick(now);
    if(wifi_) wifi_->tick(!app_.connected() && !app_.connecting());
    if(reports_) reports_->tick(wifi_ && wifi_->status().connected && !wifi_->status().setupActive,
        !app_.connected() && !app_.connecting() && !app_.pending() && (!updates_ || !updates_->status().busy()));
    app_.setMaintenance((reports_ && reports_->status().busy()) || (updates_ &&
        (updates_->status().phase==UpdatePhase::Installing || updates_->status().phase==UpdatePhase::Ready)));
    if(updates_) {
        const auto w=wifi_ ? wifi_->status() : WifiSetupStatus{};
        updates_->tick(now,w.connected && !w.setupActive,!app_.connected() && !app_.connecting() && !app_.pending() && (!reports_ || !reports_->status().busy()));
        if(updates_->status().phase==UpdatePhase::Available && !updatePromptShown_ &&
           !debugDialog_ && !debugPending_ && !app_.connected() && !app_.connecting() && page_==Screen::Devices && !touchDown_) {
            updatePromptShown_=true;navigate(Screen::Updates);
        }
    }
    if(app_.dropConfirmationRevision()!=dropCueRevision_) {
        dropCueRevision_=app_.dropConfirmationRevision();
        if(app_.fresh() && app_.confirmedDropWeight()) {
            hardware_.dropAlert(300);dropCueUntil_=now+2000;
        }
    }
    if(!app_.fresh() || !app_.confirmedDropWeight() || !app_.report().loaded) dropCueUntil_=0;
    rememberModifiers();
    if (page_ != Screen::Devices && int(page_) < int(Screen::Settings) && !app_.connected() && !app_.connecting() &&
        !(rebuild_ && next_==Screen::Devices)) {
        app_.leave(true);navigate(app_.message().empty() ? Screen::Devices : Screen::ConnectionNotice);
    }
    if (page_ == Screen::Devices) {
        if (!debugDialog_ && !debugPending_ && (!reports_ || (reports_->status().phase!=ReportPhase::Queued && !reports_->status().busy())) && !touchDown_ && !modeGestureUntil_ && !rebuild_ &&
            (!updates_ || (!updates_->startupPending() && !updates_->status().busy())) && app_.tryAutomaticSelection()) navigate(Screen::Main);
        std::string signature;
        for (const auto& d : app_.devices()) signature += d.id+d.name+d.twinPeer+(d.online ? "1" : "0")+(app_.remembered(d.id) ? "S" : "D");
        // Do not destroy a tile underneath a held finger when a scan completes.
        if (!touchDown_ && signature != deviceSignature_) { deviceSignature_ = signature; rebuild_ = true; }
    }
    const auto r = app_.report();
    if(page_ == Screen::Magic || page_ == Screen::Eccentric) {
        // Coalesce consecutive rotary events for 150 ms. A report never creates
        // an edit or retry; only a new input event can set editDirty_.
        constexpr uint64_t editSettleMs = 150;
        if(editDirty_ && !app_.pending() && now-editedAt_ >= editSettleMs) {
            draftValue_ = std::clamp(draftValue_,std::min(1,editMaximum()),editMaximum());
            const bool accepted = app_.applyModifier(page_ == Screen::Magic,draftMode_,draftValue_,draftPercent_);
            editDirty_=false;
            if(returnAfterEdit_ && accepted) navigate(Screen::Main);
            returnAfterEdit_=false;
        } else if(!editDirty_ && !app_.pending()) {
            const auto m=editorModifier(page_ == Screen::Magic);
            draftValue_=m.value; draftPercent_=m.percent; draftMode_=r.settings.magicMode;
        }
    }
    if (pendingPreset_ >= 0 && !app_.pending()) {
        if (app_.fresh() && samePreset(app_.currentPreset(),app_.presetForCurrentDevice(config_.presets[pendingPreset_]))) activePreset_ = pendingPreset_;
        pendingPreset_ = -1;
    }
    if (activePreset_ >= 0 && (!app_.fresh() || !samePreset(app_.currentPreset(),app_.presetForCurrentDevice(config_.presets[activePreset_])))) activePreset_ = -1;
    if (app_.fresh() && r.loaded && r.setActive.value_or(true) && r.telemetryValid && r.repsStarted.value_or(false) && page_ == Screen::Main) navigate(Screen::Live);
    if (app_.fresh() && (!r.loaded || r.setActive==false) && page_ == Screen::Live) navigate(Screen::Main);
    updateRestAlert(now);
    if (rebuild_) build();
    const int delta = hardware_.rotary().getDelta();
    const bool pressed = hardware_.rotary().wasPressed();
    const bool wasSleeping = sleeping_;
    if (delta || pressed) { activityAt_ = now; sleeping_ = false; }
    if (modeGestureUntil_ && now > modeGestureUntil_) modeGestureUntil_ = 0;
    if (pressed && !wasSleeping && !debugDialog_ && page_ == Screen::Devices && runtime_) {
        modeGestureUntil_ = now+3000; modeGestureClicks_ = 0;
    }
    if (delta && !wasSleeping && !debugDialog_) {
        diagnostic("Dial delta="+std::to_string(delta)+" screen="+std::to_string(int(page_)));
        rotate(delta);
    }
    if (pressed && !wasSleeping && !debugDialog_) {
        if (page_ == Screen::Presets) act(ChoosePreset,"",false);
        else if (page_ == Screen::PresetNotice) act(Unload,"",false);
        else if (page_ != Screen::Devices && int(page_) < int(Screen::Settings)) app_.toggleWeight();
    }
    // Keep unload visible while loaded, pending, armed, or connected with unknown state.
    const bool mustStayAwake = restNotice_ || debugDialog_ || (reports_ && reports_->status().busy()) || (updates_ && updates_->status().busy()) || (app_.connected() && (!app_.fresh() || r.loaded || r.autoLoadArmed || app_.pending()));
    // An action above can block on GATT and record activity later than `now`.
    // Sample again; unsigned subtraction from the old loop clock sleeps instantly.
    const auto powerNow=hardware_.nowMs();
    if (mustStayAwake) activityAt_ = powerNow;
    const auto idleMs=powerNow>=activityAt_ ? powerNow-activityAt_ : 0;
    sleeping_ = !mustStayAwake && config_.sleepMinutes > 0 && idleMs >= uint64_t(config_.sleepMinutes)*60000;
    dimmed_ = !mustStayAwake && config_.dimMinutes > 0 && idleMs >= uint64_t(config_.dimMinutes)*60000;
    hardware_.setDisplayPower(sleeping_,dimmed_);
    style::set_opa(face_,sleeping_ ? 0 : dimmed_ ? 65 : 255,0);
    update();
    if(app_.message()!=lastDiagnosticMessage_) {
        lastDiagnosticMessage_=app_.message();
        if(!lastDiagnosticMessage_.empty()) diagnostic("Status: "+(app_.connecting() ? std::string("Connecting to VOLTRA") : lastDiagnosticMessage_));
    }
    lv_timer_handler();
    if(debugPending_) {debugPending_=false;openDebugDialog();}
    updateDebugDialog();
    // Navigation is deferred until after input callbacks return.
    if (rebuild_) { build(); update(); }
}
}

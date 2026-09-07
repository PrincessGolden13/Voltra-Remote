#include "UiController.h"
#include <algorithm>
#include <stdexcept>
#include <cmath>
namespace voltra {
namespace {
constexpr uint32_t bg = 0x04080b, bright = 0xf5f8ff, muted = 0x59677d;
constexpr uint32_t cyan = 0x00d9ff, pink = 0xff386f, soft = 0x96a7dd, amber = 0xffc374;
const char* modeName(MagicMode mode) {
    return mode == MagicMode::InverseChains ? "Inverse Chains" : mode == MagicMode::Chains ? "Chains" : "Mountain";
}
enum Action { Select = 1, BackDevices, Weight, Magic, EO, Drops, Apply, ModeIC, ModeC, ModeMTN,
    Stairs, RP, FieldDrops, FieldPercent, FieldHold, Unload, PreviousDevices, NextDevices };
void color(lv_obj_t* obj, uint32_t c) { lv_obj_set_style_text_color(obj, lv_color_hex(c), 0); }
void value(lv_obj_t* obj, const std::string& text) { if (obj) lv_label_set_text(obj, text.c_str()); }
int percentOf(int n, int max) { return max > 0 ? std::clamp(n*1000/max, 0, 1000) : 0; }
}
lv_obj_t* UiController::label(lv_obj_t* parent, const std::string& text, int w, int y, const lv_font_t* font) {
    auto* obj = lv_label_create(parent); value(obj, text); lv_obj_set_width(obj, w);
    lv_obj_set_style_text_font(obj, font, 0); lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(obj, LV_ALIGN_TOP_MID, 0, y); lv_obj_remove_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    return obj;
}
void UiController::bind(lv_obj_t* obj, int action, const std::string& id) {
    bindings_.push_back(std::make_unique<Binding>(Binding{this, action, id}));
    lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(obj, [](lv_event_t* e) {
        auto* b = static_cast<Binding*>(lv_event_get_user_data(e));
        const auto code = lv_event_get_code(e);
        if (code == LV_EVENT_SHORT_CLICKED || code == LV_EVENT_LONG_PRESSED)
            b->ui->act(b->action, b->id, code == LV_EVENT_LONG_PRESSED);
    }, LV_EVENT_ALL, bindings_.back().get());
}
lv_obj_t* UiController::button(lv_obj_t* parent, const std::string& text, int x, int y, int w, int h, int action) {
    auto* obj = lv_button_create(parent); lv_obj_set_size(obj, w, h); lv_obj_set_pos(obj, x, y);
    lv_obj_set_style_radius(obj, LV_RADIUS_CIRCLE, 0); lv_obj_set_style_bg_color(obj, lv_color_hex(0x17202d), 0);
    lv_obj_set_style_shadow_width(obj, 0, 0); lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    auto* textObj = label(obj, text, w, 0); lv_obj_center(textObj); bind(obj, action);
    lv_label_set_long_mode(textObj, LV_LABEL_LONG_DOT);
    return obj;
}
lv_obj_t* UiController::arc(lv_obj_t* parent, int size, uint32_t c) {
    auto* obj = lv_arc_create(parent); lv_obj_set_size(obj, size, size); lv_obj_center(obj);
    lv_arc_set_rotation(obj, 135); lv_arc_set_bg_angles(obj, 0, 270); lv_arc_set_range(obj, 0, 1000);
    lv_obj_remove_style(obj, nullptr, LV_PART_KNOB); lv_obj_remove_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_width(obj, std::max(3, width_/80), LV_PART_MAIN);
    lv_obj_set_style_arc_width(obj, std::max(3, width_/80), LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(obj, lv_color_hex(0x202a38), LV_PART_MAIN);
    lv_obj_set_style_arc_color(obj, lv_color_hex(c), LV_PART_INDICATOR);
    return obj;
}
void UiController::glow(lv_obj_t* obj, uint32_t accent, bool active) {
    lv_obj_set_style_border_width(obj, active ? std::max(2, width_/160) : 1, 0);
    lv_obj_set_style_border_color(obj, lv_color_hex(active ? accent : 0x39475b), 0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(active ? 0x073043 : 0x0c131b), 0);
    lv_obj_set_style_bg_grad_color(obj, lv_color_hex(bg), 0);
    lv_obj_set_style_bg_grad_dir(obj, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_shadow_color(obj, lv_color_hex(accent), 0);
    lv_obj_set_style_shadow_width(obj, active ? width_/28 : 0, 0);
    lv_obj_set_style_shadow_opa(obj, 65, 0);
}
lv_obj_t* UiController::disc(lv_obj_t* parent, int x, int y, int size, uint32_t accent, bool active) {
    auto* obj = lv_obj_create(parent);
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
        lv_obj_set_style_border_width(link, std::max(2, size/12), 0);
        lv_obj_set_style_border_color(link, lv_color_hex(c), 0);
        lv_obj_remove_flag(link, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_remove_flag(link, LV_OBJ_FLAG_SCROLLABLE);
    }
    return group;
}
void UiController::buildScale() {
    for (int i=0; i<tickCount; ++i) {
        const double a = (150.0 + i*240.0/(tickCount-1))*3.141592653589793/180.0;
        auto& p = tickPoints_[i];
        p[0] = {lv_value_precise_t(width_*.5 + width_*.39*std::cos(a)), lv_value_precise_t(height_*.44 + width_*.39*std::sin(a))};
        p[1] = {lv_value_precise_t(width_*.5 + width_*.36*std::cos(a)), lv_value_precise_t(height_*.44 + width_*.36*std::sin(a))};
        ticks_[i] = lv_line_create(face_); lv_line_set_points(ticks_[i], p.data(), 2);
        lv_obj_set_style_line_width(ticks_[i], std::max(2, width_/125), 0);
        lv_obj_remove_flag(ticks_[i], LV_OBJ_FLAG_CLICKABLE);
    }
    const auto* font = width_ < 400 ? &lv_font_montserrat_12 : &lv_font_montserrat_14;
    scaleLabels_[1] = label(face_, "", width_/4, height_/100, font);
    scaleLabels_[0] = label(face_, "", width_/6, height_*64/100, font);
    scaleLabels_[2] = label(face_, "", width_/6, height_*64/100, font);
    lv_obj_align(scaleLabels_[0], LV_ALIGN_TOP_MID, -width_*39/100, height_*64/100);
    lv_obj_align(scaleLabels_[2], LV_ALIGN_TOP_MID, width_*39/100, height_*64/100);
    for (auto* item : scaleLabels_) color(item, soft);
}
void UiController::updateScale(int selected, int maximum, const std::string& suffix, uint32_t accent, bool negative) {
    const int progress = percentOf(selected, maximum);
    for (int i=0; i<tickCount; ++i)
        lv_obj_set_style_line_color(ticks_[i], lv_color_hex(progress > 0 && i*1000/(tickCount-1) <= progress ? accent : 0x46526b), 0);
    value(scaleLabels_[0], "0"+suffix);
    const std::string sign = negative ? "-" : "";
    value(scaleLabels_[1], sign+std::to_string(maximum/2)+(maximum%2 ? ".5" : ""));
    value(scaleLabels_[2], sign+std::to_string(maximum)+suffix);
}

UiController::UiController(HardwarePlatform& h, TrainingApplication& app) : hardware_(h), app_(app) {
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
        data->point = {int32_t(p.x*(s->width_-1)), int32_t(p.y*(s->height_-1))};
        data->state = p.pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
    });
    lastTickMs_ = hardware_.nowMs();
}
UiController::~UiController() { lv_indev_delete(touch_); lv_display_delete(display_); lv_deinit(); }
void UiController::readTouch() { lv_indev_read(touch_); }
void UiController::navigate(Screen page) {
    next_ = page; rebuild_ = true;
    const auto r = app_.report();
    if (page == Screen::Magic || page == Screen::Eccentric) {
        draftMode_ = r.settings.magicMode;
        if (page == Screen::Magic && !r.magicSupported[int(draftMode_)])
            for (int i = 0; i < 3; ++i) if (r.magicSupported[i]) { draftMode_ = MagicMode(i); break; }
        draftValue_ = page == Screen::Magic ? r.settings.magic.value : r.settings.eccentric.value;
    }
    if (page == Screen::Drops) { draftDrops_ = app_.drops(); dropField_ = 1; }
}
void UiController::act(int action, const std::string& id, bool held) {
    if (!held && (action == PreviousDevices || action == NextDevices)) {
        const size_t count = (app_.devices().size()+3)/4;
        if (action == PreviousDevices && devicePage_ > 0) --devicePage_;
        if (action == NextDevices && devicePage_+1 < count) ++devicePage_;
        next_ = Screen::Devices; rebuild_ = true; return;
    }
    if (action == Select) { if (!held && app_.select(id)) navigate(Screen::Main); return; }
    if (action == BackDevices && !held) { app_.leave(); navigate(Screen::Devices); return; }
    if (action == Weight) { if (held) app_.autoLoad(); else app_.toggleWeight(); return; }
    if (action == Unload && !held) { if (app_.report().loaded || app_.report().autoLoadArmed || !app_.fresh()) app_.toggleWeight(); return; }
    if (action == Magic || action == EO || action == Drops) {
        const auto r = app_.report();
        if (action == Magic && std::none_of(r.magicSupported.begin(), r.magicSupported.end(), [](bool b) { return b; })) return;
        if (action == EO && !r.eccentricSupported) return;
        if (action == Drops && !r.liveWeightSupported) return;
        if (held) navigate(action == Magic ? Screen::Magic : action == EO ? Screen::Eccentric : Screen::Drops);
        else if (action == Drops) app_.toggleDrops(); else app_.toggleModifier(action == Magic);
        return;
    }
    if (held) return;
    if (action == Apply) {
        if (page_ == Screen::Drops) { draftDrops_.enabled = true; app_.applyDrops(draftDrops_); }
        else app_.applyModifier(page_ == Screen::Magic, draftMode_, draftValue_);
        navigate(Screen::Main);
    } else if (action >= ModeIC && action <= ModeMTN) {
        auto mode = MagicMode(action-ModeIC);
        if (app_.report().magicSupported[int(mode)]) draftMode_ = mode;
    } else if (action == Stairs || action == RP) {
        draftDrops_.mode = action == Stairs ? DropMode::Stairs : DropMode::RestPause;
        next_ = page_; rebuild_ = true;
    } else if (action >= FieldDrops && action <= FieldHold) dropField_ = action-FieldDrops;
}
void UiController::build() {
    if (face_) lv_obj_delete(face_);
    bindings_.clear(); deviceRows_.clear();
    weight_ = title_ = status_ = hint_ = message_ = arc_ = unit_ = limit_ = dropTitle_ = nullptr;
    center_ = battery_ = twinIcon_ = nullptr; ticks_.fill(nullptr);
    for (int i=0; i<3; ++i) {
        stairs_[i] = tiles_[i] = miniArc_[i] = miniValue_[i] = modeLabels_[i] = choice_[i] = nullptr;
        scaleLabels_[i] = choiceNames_[i] = nullptr;
    }
    page_ = next_; rebuild_ = false;
    auto* screen = lv_display_get_screen_active(display_);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x030507), 0);
    face_ = lv_obj_create(screen); lv_obj_set_size(face_, width_, height_); lv_obj_center(face_);
    lv_obj_remove_flag(face_, LV_OBJ_FLAG_SCROLLABLE); lv_obj_set_style_pad_all(face_, 0, 0);
    lv_obj_set_style_radius(face_, LV_RADIUS_CIRCLE, 0); lv_obj_set_style_bg_color(face_, lv_color_hex(bg), 0);
    lv_obj_set_style_text_color(face_, lv_color_hex(bright), 0);
    lv_obj_set_style_border_width(face_, std::max(2, width_/160), 0);
    lv_obj_set_style_border_color(face_, lv_color_hex(0x2c3440), 0);
    const auto* small = width_ < 400 ? &lv_font_montserrat_12 : &lv_font_montserrat_14;
    const auto* medium = width_ < 400 ? &lv_font_montserrat_14 : &lv_font_montserrat_18;
    battery_ = label(face_, "", width_*45/100, height_*94/100, small); color(battery_, soft);
    if (page_ == Screen::Devices) {
        const auto& devices = app_.devices();
        const size_t pages = std::max(size_t(1), (devices.size()+3)/4);
        devicePage_ = std::min(devicePage_, pages-1);
        for (size_t i=0; i<4 && devicePage_*4+i < devices.size(); ++i) {
            const auto& d = devices[devicePage_*4+i];
            const int size = width_*32/100;
            auto* tile = disc(face_, width_*(17+int(i%2)*34)/100, height_*(14+int(i/2)*34)/100, size, cyan, d.online);
            if (d.online) bind(tile, Select, d.id);
            else lv_obj_remove_flag(tile, LV_OBJ_FLAG_CLICKABLE);
            auto* icon = linkIcon(tile, size*40/100, d.online ? bright : muted);
            lv_obj_align(icon, LV_ALIGN_TOP_MID, 0, size*24/100);
            auto* name = label(tile, d.name, size*90/100, size*49/100, small);
            lv_label_set_long_mode(name, LV_LABEL_LONG_DOT);
            lv_obj_set_height(name, small->line_height);
            color(name, d.online ? bright : muted);
            auto* badge = lv_obj_create(tile); lv_obj_remove_style_all(badge);
            lv_obj_set_size(badge, size*61/100, size*19/100); lv_obj_align(badge, LV_ALIGN_TOP_MID, 0, size*69/100);
            lv_obj_set_style_radius(badge, LV_RADIUS_CIRCLE, 0); lv_obj_set_style_border_width(badge, 1, 0);
            lv_obj_set_style_border_color(badge, lv_color_hex(d.online ? cyan : muted), 0);
            lv_obj_remove_flag(badge, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_remove_flag(badge, LV_OBJ_FLAG_SCROLLABLE);
            auto* online = label(badge, d.online ? "Online" : "Offline", size*61/100, 0, small);
            lv_obj_center(online); color(online, d.online ? cyan : muted);
            deviceRows_.push_back({tile, d.id});
        }
        message_ = label(face_, devices.empty() ? "Searching for VOLTRAs..." : "Tap an available device", width_*74/100, height_*83/100, small);
        color(message_, soft);
        if (pages > 1) {
            auto* prev = button(face_, "<", width_*33/100, height_*87/100, width_*9/100, height_*6/100, PreviousDevices);
            auto* next = button(face_, ">", width_*58/100, height_*87/100, width_*9/100, height_*6/100, NextDevices);
            color(prev, devicePage_ ? cyan : muted); color(next, devicePage_+1 < pages ? cyan : muted);
            label(face_, std::to_string(devicePage_+1)+" / "+std::to_string(pages), width_*16/100, height_*88/100, small);
        }
        return;
    }
    buildScale();
    const bool rp = page_ == Screen::Drops && draftDrops_.mode == DropMode::RestPause;
    title_ = label(face_, page_ == Screen::Main ? app_.name() : page_ == Screen::Magic ? "Magic Modes" :
        page_ == Screen::Eccentric ? "Eccentric Overload" : "Dropset", width_*65/100, height_*15/100, medium);
    lv_label_set_long_mode(title_, LV_LABEL_LONG_DOT);
    if (page_ == Screen::Main) { bind(title_, BackDevices); lv_obj_set_ext_click_area(title_, height_/100); }
    status_ = label(face_, "", width_*62/100, height_*21/100, small);
    color(status_, soft);
    if (page_ == Screen::Main) {
        twinIcon_ = linkIcon(face_, width_*7/100, cyan);
        lv_obj_set_pos(twinIcon_, width_*38/100, height_*21/100);
    } else { bind(status_, Unload); lv_obj_set_ext_click_area(status_, height_/100); }
    if (!rp) {
        const uint32_t accent = page_ == Screen::Eccentric ? pink : cyan;
        const int size = width_*42/100;
        center_ = disc(face_, width_*29/100, height_*25/100, size, accent, true);
        if (page_ == Screen::Eccentric) {
            lv_obj_set_style_bg_color(center_, lv_color_hex(0x310c1d), 0);
        }
        if (page_ == Screen::Main) bind(center_, Weight);
        else lv_obj_remove_flag(center_, LV_OBJ_FLAG_CLICKABLE);
        weight_ = label(center_, "--", size*96/100, size*27/100, &lv_font_montserrat_48);
        lv_obj_set_style_transform_pivot_x(weight_, size*96/200, 0);
        lv_obj_set_style_transform_pivot_y(weight_, lv_font_montserrat_48.line_height/2, 0);
        lv_obj_set_style_transform_scale_x(weight_, width_*340/480, 0);
        lv_obj_set_style_transform_scale_y(weight_, width_*340/480, 0);
        unit_ = label(center_, "", size*90/100, size*64/100, small); color(unit_, soft);
        hint_ = label(center_, "", size*90/100, size*77/100, small); color(hint_, soft);
        if (page_ == Screen::Main) {
            lv_obj_align(unit_, LV_ALIGN_TOP_MID, 0, size*59/100);
            lv_obj_align(hint_, LV_ALIGN_TOP_MID, 0, size*70/100);
        }
    }
    if (page_ == Screen::Main) {
        for (int i=0; i<3; ++i) {
            const int size = width_*21/100;
            tiles_[i] = disc(face_, width_*(15+i*25)/100, height_*65/100, size, i == 1 ? pink : cyan, false);
            bind(tiles_[i], i == 0 ? Magic : i == 1 ? EO : Drops);
            miniArc_[i] = arc(tiles_[i], size*92/100, i == 1 ? pink : cyan);
            miniValue_[i] = label(tiles_[i], "", size*92/100, size*48/100, medium);
            if (i == 0) {
                modeLabels_[0] = label(tiles_[i], "", size*85/100, size*24/100, medium);
                modeLabels_[1] = label(tiles_[i], "", size*83/100, size*76/100, small);
            } else if (i == 1) label(tiles_[i], "EO", size*90/100, size*24/100, medium);
            else {
                dropTitle_ = label(tiles_[i], "", size*90/100, size*19/100, small);
                for (int step=0; step<3; ++step) {
                    auto* bar = lv_obj_create(tiles_[i]); lv_obj_remove_style_all(bar);
                    stairs_[step] = bar;
                    lv_obj_set_size(bar, size/9, (step+1)*size/16);
                    lv_obj_set_pos(bar, size*34/100+step*size/9, size*45/100-(step+1)*size/16);
                    lv_obj_set_style_bg_color(bar, lv_color_hex(muted), 0); lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
                    lv_obj_remove_flag(bar, LV_OBJ_FLAG_CLICKABLE);
                }
            }
        }
        message_ = label(face_, "", width_*70/100, height_*89/100, small); color(message_, soft);
        return;
    }
    if (page_ == Screen::Magic) {
        // Visual order follows the mockup; actions retain their semantic enum mapping.
        const MagicMode order[] = {MagicMode::InverseChains, MagicMode::Mountain, MagicMode::Chains};
        for (int i=0; i<3; ++i) {
            const int index = int(order[i]), size = width_*14/100;
            choice_[index] = disc(face_, width_*(22+i*22)/100, height_*62/100, size, cyan, false);
            bind(choice_[index], ModeIC+index);
            auto* txt = label(choice_[index], magicName(order[i]), size, 0, medium); lv_obj_center(txt);
            choiceNames_[index] = label(face_, order[i] == MagicMode::InverseChains ? "Inverse\nChains" :
                modeName(order[i]), width_*22/100, height_*77/100, small);
            lv_obj_align(choiceNames_[index], LV_ALIGN_TOP_MID, width_*(-21+i*22)/100, height_*77/100);
        }
    } else if (page_ == Screen::Drops) {
        for (int i=0; i<2; ++i) {
            choice_[i] = button(face_, i == 0 ? "Stairs" : "RP", width_*(25+i*25)/100, height_*27/100,
                width_*25/100, height_*7/100, i == 0 ? Stairs : RP);
        }
        if (rp) {
            for (int i=0; i<3; ++i) {
                const int size = width_*24/100;
                tiles_[i] = disc(face_, width_*(12+i*26)/100, height_*39/100, size, cyan, i == dropField_);
                bind(tiles_[i], FieldDrops+i);
                miniArc_[i] = arc(tiles_[i], size*91/100, i == 2 ? 0x728cff : cyan);
                auto* caption = label(tiles_[i], i == 0 ? "Drops" : i == 1 ? "Drop %" : "Hold", size*90/100, size*24/100, small);
                color(caption, soft);
                miniValue_[i] = label(tiles_[i], "", size*96/100, size*45/100,
                    width_ < 400 ? &lv_font_montserrat_28 : &lv_font_montserrat_36);
                color(miniValue_[i], bright);
            }
            hint_ = label(face_, "", width_*55/100, height_*70/100, &lv_font_montserrat_18);
        } else {
            lv_obj_set_y(center_, height_*36/100);
        }
    }
    const int applyY = page_ == Screen::Magic ? 85 : page_ == Screen::Eccentric ? 78 : 81;
    const int applyH = page_ == Screen::Magic ? 8 : 10;
    auto* apply = button(face_, "Apply", width_*30/100, height_*applyY/100, width_*40/100, height_*applyH/100, Apply);
    glow(apply, cyan, true);
    lv_obj_set_style_text_font(lv_obj_get_child(apply, 0), medium, 0);
    for (auto* legend : scaleLabels_) lv_obj_move_foreground(legend);
}
int UiController::editMaximum() const {
    const auto r = app_.report();
    if (page_ == Screen::Drops) return draftDrops_.mode == DropMode::Stairs ? r.maxWeight : dropField_ == 1 ? 80 : 9;
    auto s = r.settings;
    if (page_ == Screen::Magic) s.magic.enabled = true; else s.eccentric.enabled = true;
    return modifierMaximum(r, s, page_ == Screen::Magic);
}
void UiController::rotate(int delta) {
    if (page_ == Screen::Devices || !app_.fresh()) return;
    if (page_ == Screen::Main) { app_.adjustWeight(delta); return; }
    if (page_ != Screen::Drops) { draftValue_ = std::clamp(draftValue_+delta, 0, editMaximum()); return; }
    if (draftDrops_.mode == DropMode::Stairs) draftDrops_.step = std::clamp(draftDrops_.step+delta, -editMaximum(), 0);
    else if (dropField_ == 0) draftDrops_.drops = std::clamp(draftDrops_.drops+delta, 0, 9);
    else if (dropField_ == 1) draftDrops_.percent = std::clamp(draftDrops_.percent+delta, 0, 80);
    else draftDrops_.holdSeconds = std::clamp(draftDrops_.holdSeconds+delta, 1, 9);
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
        if (!app_.message().empty()) value(message_, app_.message());
        return;
    }
    const auto r = app_.report();
    if (page_ == Screen::Main) {
        value(weight_, r.revision ? std::to_string(r.settings.weight) : "--");
        color(weight_, !app_.fresh() ? amber : r.loaded ? bright : muted);
        value(unit_, unitName(r.unit));
        value(hint_, !app_.fresh() ? "State unknown\nTap to unload" : r.autoLoadArmed ? "Auto Load armed\nTap to cancel" :
            r.loaded ? "Tap to unload" : "Tap: On / Off\nHold: Autoload");
        value(status_, r.twinKnown ? (r.twin ? "Twin" : "") : "Twin unknown");
        color(status_, r.twinKnown ? cyan : amber);
        if (r.twinKnown && r.twin) {
            lv_obj_remove_flag(twinIcon_, LV_OBJ_FLAG_HIDDEN);
            lv_obj_align(status_, LV_ALIGN_TOP_MID, width_*4/100, height_*21/100);
        } else {
            lv_obj_add_flag(twinIcon_, LV_OBJ_FLAG_HIDDEN);
            lv_obj_align(status_, LV_ALIGN_TOP_MID, 0, height_*21/100);
        }
        updateScale(r.settings.weight, r.maxWeight, "", cyan);
        for (int i=0; i<2; ++i) {
            const auto m = i == 0 ? r.settings.magic : r.settings.eccentric;
            const uint32_t c = i == 0 ? cyan : pink;
            color(tiles_[i], m.enabled ? bright : muted);
            value(miniValue_[i], std::to_string(m.value)+(r.percent ? "%" : unitName(r.unit)));
            color(miniValue_[i], m.enabled ? bright : muted);
            lv_obj_set_style_arc_color(miniArc_[i], lv_color_hex(m.enabled ? c : 0x46536a), LV_PART_INDICATOR);
            lv_arc_set_value(miniArc_[i], percentOf(m.value, modifierMaximum(r, r.settings, i == 0)));
            lv_obj_set_style_border_color(tiles_[i], lv_color_hex(m.enabled ? c : 0x39475b), 0);
        }
        value(modeLabels_[0], magicName(r.settings.magicMode));
        color(modeLabels_[0], r.settings.magic.enabled ? bright : muted);
        std::string otherModes;
        for (int i=0; i<3; ++i) if (i != int(r.settings.magicMode) && r.magicSupported[i]) {
            if (!otherModes.empty()) otherModes += "  ";
            otherModes += magicName(MagicMode(i));
        }
        value(modeLabels_[1], otherModes); color(modeLabels_[1], muted);
        const bool supported[] = {std::any_of(r.magicSupported.begin(), r.magicSupported.end(), [](bool b) { return b; }),
            r.eccentricSupported, r.liveWeightSupported};
        for (int i=0; i<3; ++i) {
            if (supported[i]) lv_obj_remove_flag(tiles_[i], LV_OBJ_FLAG_HIDDEN);
            else lv_obj_add_flag(tiles_[i], LV_OBJ_FLAG_HIDDEN);
        }
        const auto d = app_.drops();
        color(tiles_[2], d.enabled ? bright : muted);
        for (auto* step : stairs_) {
            if (d.mode == DropMode::RestPause) lv_obj_add_flag(step, LV_OBJ_FLAG_HIDDEN);
            else lv_obj_remove_flag(step, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_style_bg_color(step, lv_color_hex(d.enabled ? cyan : muted), 0);
        }
        value(dropTitle_, d.mode == DropMode::RestPause ? "RP" : "");
        value(miniValue_[2], d.mode == DropMode::Stairs ? std::to_string(d.step)+unitName(r.unit) :
            std::to_string(d.drops)+" / "+std::to_string(d.percent)+"%\n"+std::to_string(d.holdSeconds)+"s");
        lv_obj_set_style_text_font(miniValue_[2], d.mode == DropMode::RestPause ?
            (width_ < 400 ? &lv_font_montserrat_12 : &lv_font_montserrat_14) :
            (width_ < 400 ? &lv_font_montserrat_14 : &lv_font_montserrat_18), 0);
        color(miniValue_[2], d.enabled ? bright : muted);
        lv_obj_set_style_arc_color(miniArc_[2], lv_color_hex(d.enabled ? cyan : 0x46536a), LV_PART_INDICATOR);
        lv_arc_set_value(miniArc_[2], d.enabled ? 1000 : 0);
        value(message_, app_.message().empty() ? "Hold icon to change" : app_.message());
        return;
    }
    value(status_, !app_.fresh() ? "Unknown / tap to unload" :
        r.loaded ? std::to_string(r.settings.weight)+" "+unitName(r.unit)+" / tap to unload" :
        r.autoLoadArmed ? "Auto Load / tap to cancel" : "");
    if (page_ == Screen::Drops) {
        for (int i=0; i<2; ++i) {
            glow(choice_[i], cyan, int(draftDrops_.mode) == i);
            color(choice_[i], int(draftDrops_.mode) == i ? bright : muted);
        }
        if (draftDrops_.mode == DropMode::RestPause) {
            const int values[] = {draftDrops_.drops, draftDrops_.percent, draftDrops_.holdSeconds};
            const int maxima[] = {9, 80, 9};
            const char* suffix[] = {"", "%", "s"};
            const char* names[] = {"Drops", "Drop %", "Hold"};
            for (int i=0; i<3; ++i) {
                value(miniValue_[i], std::to_string(values[i])+suffix[i]);
                // All values stay active; the bright outer ring marks rotary focus.
                glow(tiles_[i], cyan, i == dropField_);
                lv_arc_set_value(miniArc_[i], percentOf(values[i], maxima[i]));
            }
            value(hint_, names[dropField_]); color(hint_, bright);
            updateScale(values[dropField_], maxima[dropField_], suffix[dropField_], cyan);
        } else {
            value(weight_, std::to_string(draftDrops_.step)); color(weight_, bright);
            value(unit_, unitName(r.unit)); value(hint_, "After each rep");
            updateScale(std::abs(draftDrops_.step), r.maxWeight, "", cyan, true);
        }
    } else {
        draftValue_ = std::clamp(draftValue_, 0, editMaximum());
        value(weight_, std::to_string(draftValue_)+(r.percent ? "%" : ""));
        color(weight_, bright);
        value(unit_, r.percent ? "" : unitName(r.unit));
        value(hint_, page_ == Screen::Magic ? modeName(draftMode_) : "");
        updateScale(draftValue_, editMaximum(), r.percent ? "%" : "", page_ == Screen::Magic ? cyan : pink);
        if (page_ == Screen::Magic) for (int i=0; i<3; ++i) {
            if (r.magicSupported[i]) {
                lv_obj_remove_flag(choice_[i], LV_OBJ_FLAG_HIDDEN);
                lv_obj_remove_flag(choiceNames_[i], LV_OBJ_FLAG_HIDDEN);
            } else {
                lv_obj_add_flag(choice_[i], LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(choiceNames_[i], LV_OBJ_FLAG_HIDDEN);
            }
            glow(choice_[i], cyan, int(draftMode_) == i);
            color(choice_[i], int(draftMode_) == i ? cyan : soft);
            color(choiceNames_[i], int(draftMode_) == i ? cyan : soft);
        }
    }
}
void UiController::tick() {
    const auto now = hardware_.nowMs(); lv_tick_inc(uint32_t(now-lastTickMs_)); lastTickMs_ = now;
    app_.tick(now);
    if (page_ != Screen::Devices && !app_.connected()) { app_.leave(); navigate(Screen::Devices); }
    if (page_ == Screen::Devices) {
        std::string signature;
        for (const auto& d : app_.devices()) signature += d.id+d.name+(d.online ? "1" : "0");
        if (signature != deviceSignature_) { deviceSignature_ = signature; rebuild_ = true; }
    }
    if (rebuild_) build();
    const int delta = hardware_.rotary().getDelta(); if (delta) rotate(delta);
    if (hardware_.rotary().wasPressed() && page_ != Screen::Devices) app_.toggleWeight();
    update(); lv_timer_handler();
    // Navigation is deferred until after input callbacks return.
    if (rebuild_) { build(); update(); }
}
}

#include "UiController.h"
#include <algorithm>
#include <stdexcept>
namespace voltra {
namespace {
constexpr uint32_t bg = 0x080b10, bright = 0xe8eef6, muted = 0x536070;
constexpr uint32_t green = 0xaaff64, blue = 0x69baff, purple = 0xbc9aff, amber = 0xffc374;
enum Action { Select = 1, BackDevices, Weight, Magic, EO, Drops, Apply, ModeIC, ModeC, ModeMTN,
    Stairs, RP, FieldDrops, FieldPercent, FieldHold, Unload };
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
    if (page == Screen::Drops) { draftDrops_ = app_.drops(); dropField_ = 0; }
}
void UiController::act(int action, const std::string& id, bool held) {
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
    for (int i=0; i<3; ++i) stairs_[i] = tiles_[i] = miniArc_[i] = miniValue_[i] = modeLabels_[i] = choice_[i] = nullptr;
    page_ = next_; rebuild_ = false;
    auto* screen = lv_display_get_screen_active(display_);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x05090d), 0);
    face_ = lv_obj_create(screen); lv_obj_set_size(face_, width_, height_); lv_obj_center(face_);
    lv_obj_remove_flag(face_, LV_OBJ_FLAG_SCROLLABLE); lv_obj_set_style_pad_all(face_, 0, 0);
    lv_obj_set_style_radius(face_, LV_RADIUS_CIRCLE, 0); lv_obj_set_style_bg_color(face_, lv_color_hex(bg), 0);
    lv_obj_set_style_text_color(face_, lv_color_hex(bright), 0);
    lv_obj_set_style_border_width(face_, 2, 0); lv_obj_set_style_border_color(face_, lv_color_hex(0x27313e), 0);
    const int cw = width_*70/100;
    if (page_ == Screen::Devices) {
        title_ = label(face_, "YOUR VOLTRAS", cw, height_*13/100, &lv_font_montserrat_18);
        label(face_, "Tap a device to connect", cw, height_*20/100);
        auto* list = lv_obj_create(face_); lv_obj_set_size(list, cw, height_*56/100);
        lv_obj_align(list, LV_ALIGN_TOP_MID, 0, height_*28/100);
        lv_obj_set_style_bg_opa(list, LV_OPA_TRANSP, 0); lv_obj_set_style_border_width(list, 0, 0);
        lv_obj_set_style_pad_all(list, 0, 0); lv_obj_set_scroll_dir(list, LV_DIR_VER);
        int i=0;
        for (const auto& d : app_.devices()) {
            auto* row = button(list, d.name, 0, i++*height_*12/100, cw-4, height_*10/100, 0);
            bind(row, Select, d.id);
            color(row, d.online ? bright : muted);
            lv_obj_set_style_bg_color(row, lv_color_hex(d.online ? 0x1c2b36 : 0x10151b), 0);
            if (!d.online) lv_obj_remove_flag(row, LV_OBJ_FLAG_CLICKABLE);
            deviceRows_.push_back({row, d.id});
        }
        if (app_.devices().empty()) label(list, "Searching for VOLTRAs...", cw, height_*10/100);
        message_ = label(face_, "", width_*66/100, height_*85/100);
        color(message_, muted);
        return;
    }
    arc_ = arc(face_, width_*91/100, page_ == Screen::Main ? green : page_ == Screen::Magic ? purple : page_ == Screen::Eccentric ? blue : amber);
    auto* scale = lv_scale_create(face_); lv_obj_set_size(scale, width_*92/100, height_*92/100); lv_obj_center(scale);
    lv_scale_set_mode(scale, LV_SCALE_MODE_ROUND_INNER); lv_scale_set_angle_range(scale, 270); lv_scale_set_rotation(scale, 135);
    lv_scale_set_total_tick_count(scale, 55); lv_scale_set_major_tick_every(scale, 3); lv_scale_set_label_show(scale, false);
    lv_obj_set_style_line_width(scale, 0, LV_PART_MAIN);
    lv_obj_set_style_line_color(scale, lv_color_hex(0x58697f), LV_PART_ITEMS);
    lv_obj_set_style_line_color(scale, lv_color_hex(0x8b9aaf), LV_PART_INDICATOR);
    lv_obj_set_style_line_width(scale, 2, LV_PART_ITEMS); lv_obj_set_style_line_width(scale, 2, LV_PART_INDICATOR);
    lv_obj_set_style_length(scale, width_*2/100, LV_PART_ITEMS); lv_obj_set_style_length(scale, width_*4/100, LV_PART_INDICATOR);
    lv_obj_remove_flag(scale, LV_OBJ_FLAG_CLICKABLE);
    title_ = label(face_, page_ == Screen::Main ? app_.name() : page_ == Screen::Magic ? "MAGIC MODES" :
        page_ == Screen::Eccentric ? "ECCENTRIC" : "DROPSET", cw, height_*14/100, &lv_font_montserrat_18);
    if (page_ == Screen::Main) bind(title_, BackDevices);
    lv_label_set_long_mode(title_, LV_LABEL_LONG_DOT);
    status_ = label(face_, "", cw, height_*21/100);
    if (page_ != Screen::Main) {
        bind(status_, Unload);
        lv_obj_set_ext_click_area(status_, height_*2/100);
    }
    if (page_ == Screen::Drops) {
        lv_obj_align(title_, LV_ALIGN_TOP_MID, 0, height_*11/100);
        lv_obj_align(status_, LV_ALIGN_TOP_MID, 0, height_*18/100);
    }
    auto* target = lv_obj_create(face_); lv_obj_remove_style_all(target);
    lv_obj_set_size(target, width_*64/100, height_*23/100);
    lv_obj_align(target, LV_ALIGN_TOP_MID, 0, height_*27/100); lv_obj_remove_flag(target, LV_OBJ_FLAG_SCROLLABLE);
    if (page_ == Screen::Main) bind(target, Weight);
    weight_ = label(target, "--", width_*52/100, 0, &lv_font_montserrat_48); lv_obj_center(weight_);
    lv_obj_set_style_transform_pivot_x(weight_, width_*52/200, 0);
    lv_obj_set_style_transform_pivot_y(weight_, lv_font_montserrat_48.line_height/2, 0);
    lv_obj_set_style_transform_scale_x(weight_, width_*400/480, 0); lv_obj_set_style_transform_scale_y(weight_, width_*400/480, 0);
    unit_ = label(face_, "", cw, height_*48/100);
    hint_ = label(face_, "", cw, height_*54/100);
    if (page_ == Screen::Main) {
        for (int i=0; i<3; ++i) {
            const int size = width_*23/100;
            auto* tile = lv_obj_create(face_); lv_obj_remove_style_all(tile); lv_obj_set_size(tile, size, size);
            tiles_[i] = tile;
            lv_obj_set_pos(tile, width_*(14+i*25)/100, height_*62/100); lv_obj_remove_flag(tile, LV_OBJ_FLAG_SCROLLABLE);
            bind(tile, i == 0 ? Magic : i == 1 ? EO : Drops);
            miniArc_[i] = arc(tile, size, i == 0 ? purple : i == 1 ? blue : amber);
            miniValue_[i] = label(tile, "", size, size*44/100, &lv_font_montserrat_18);
            if (i == 0) {
                for (int j=0; j<3; ++j) {
                    modeLabels_[j] = label(tile, magicName(MagicMode(j)), size*(j == 2 ? 44 : 26)/100, size*22/100);
                    lv_obj_set_align(modeLabels_[j], LV_ALIGN_TOP_LEFT);
                    lv_obj_set_pos(modeLabels_[j], size*(3+j*25)/100, size*22/100);
                }
            } else if (i == 1) label(tile, "EO", size, size*22/100);
            else {
                dropTitle_ = label(tile, "", size, size*16/100);
                // Code-native stair icon, scaled with the round display.
                for (int step=0; step<3; ++step) {
                    auto* bar = lv_obj_create(tile); lv_obj_remove_style_all(bar);
                    stairs_[step] = bar;
                    lv_obj_set_size(bar, size/9, (step+1)*size/20);
                    lv_obj_set_pos(bar, size*35/100+step*size/9, size*37/100-(step+1)*size/20);
                    lv_obj_set_style_bg_color(bar, lv_color_hex(muted), 0); lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
                }
            }
        }
        message_ = label(face_, "", width_*65/100, height_*87/100);
        color(label(face_, "SIM", width_*30/100, height_*93/100), muted);
    } else {
        if (page_ == Screen::Magic) {
            for (int i=0; i<3; ++i)
                choice_[i] = button(face_, magicName(MagicMode(i)), width_*(19+i*22)/100, height_*63/100, width_*18/100, height_*10/100, ModeIC+i);
        } else if (page_ == Screen::Drops) {
            choice_[0] = button(face_, "STAIRS", width_*24/100, height_*24/100, width_*25/100, height_*9/100, Stairs);
            choice_[1] = button(face_, "RP", width_*51/100, height_*24/100, width_*25/100, height_*9/100, RP);
            // Shift the central value below the mode picker.
            lv_obj_align(target, LV_ALIGN_TOP_MID, 0, height_*34/100);
            lv_obj_align(unit_, LV_ALIGN_TOP_MID, 0, height_*54/100);
            lv_obj_align(hint_, LV_ALIGN_TOP_MID, 0, height_*60/100);
            if (draftDrops_.mode == DropMode::RestPause) {
                for (int i=0; i<3; ++i) {
                    auto* field = button(face_, i == 0 ? "DROPS" : i == 1 ? "DROP %" : "HOLD",
                        width_*(17+i*23)/100, height_*66/100, width_*21/100, height_*12/100, FieldDrops+i);
                    miniValue_[i] = lv_obj_get_child(field, 0); miniArc_[i] = field;
                    lv_label_set_long_mode(miniValue_[i], LV_LABEL_LONG_WRAP);
                    lv_obj_set_style_radius(field, width_/40, 0);
                    lv_obj_set_style_border_width(field, 2, 0);
                }
            }
        }
        button(face_, "APPLY & BACK", width_*29/100, height_*79/100, width_*42/100, height_*10/100, Apply);
        message_ = label(face_, "", width_*64/100, height_*91/100);
    }
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
    if (page_ == Screen::Devices) {
        value(message_, app_.message().empty() ? "Auto scan 5s  /  Simulation" : app_.message()); return;
    }
    const auto r = app_.report();
    if (page_ == Screen::Main) {
        value(weight_, r.revision ? std::to_string(r.settings.weight) : "--");
        color(weight_, !app_.fresh() ? amber : r.loaded ? green : muted);
        value(unit_, unitName(r.unit));
        value(status_, !app_.fresh() ? "STATE UNKNOWN" : r.autoLoadArmed ? "AUTO LOAD ARMED" :
            (r.twinKnown ? (r.twin ? "TWIN  /  " : "") : "TWIN UNKNOWN  /  ") + std::string(r.loaded ? "LOADED" : "UNLOADED"));
        value(hint_, r.loaded ? "TAP TO UNLOAD" : r.autoLoadArmed ? "TAP TO CANCEL" : "TAP TO LOAD / HOLD AUTO");
        lv_arc_set_value(arc_, percentOf(r.settings.weight, r.maxWeight));
        for (int i=0; i<2; ++i) {
            const auto m = i == 0 ? r.settings.magic : r.settings.eccentric;
            const uint32_t c = m.enabled ? (i == 0 ? purple : blue) : muted;
            color(tiles_[i], c);
            value(miniValue_[i], std::to_string(m.value)+(r.percent ? "%" : unitName(r.unit))); color(miniValue_[i], c);
            lv_obj_set_style_arc_color(miniArc_[i], lv_color_hex(c), LV_PART_INDICATOR);
            lv_arc_set_value(miniArc_[i], percentOf(m.value, modifierMaximum(r, r.settings, i == 0)));
        }
        for (int i=0; i<3; ++i) color(modeLabels_[i], int(r.settings.magicMode) == i ?
            (r.settings.magic.enabled ? purple : muted) : 0x303c4a);
        const bool supported[] = {std::any_of(r.magicSupported.begin(), r.magicSupported.end(), [](bool b) { return b; }),
            r.eccentricSupported, r.liveWeightSupported};
        for (int i=0; i<3; ++i) {
            if (supported[i]) lv_obj_remove_flag(tiles_[i], LV_OBJ_FLAG_HIDDEN);
            else lv_obj_add_flag(tiles_[i], LV_OBJ_FLAG_HIDDEN);
        }
        auto d = app_.drops();
        color(tiles_[2], d.enabled ? amber : muted);
        for (auto* step : stairs_) {
            if (d.mode == DropMode::RestPause) lv_obj_add_flag(step, LV_OBJ_FLAG_HIDDEN);
            else lv_obj_remove_flag(step, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_style_bg_color(step, lv_color_hex(d.enabled ? amber : muted), 0);
        }
        value(dropTitle_, d.mode == DropMode::RestPause ? "RP" : "");
        value(miniValue_[2], d.mode == DropMode::Stairs ? std::to_string(d.step)+unitName(r.unit) :
            std::to_string(d.drops)+" / "+std::to_string(d.percent)+"%\n"+std::to_string(d.holdSeconds)+"s");
        lv_obj_set_style_text_font(miniValue_[2], d.mode == DropMode::RestPause ? &lv_font_montserrat_14 : &lv_font_montserrat_18, 0);
        color(miniValue_[2], d.enabled ? amber : muted);
        lv_obj_set_style_arc_color(miniArc_[2], lv_color_hex(d.enabled ? amber : muted), LV_PART_INDICATOR);
        lv_arc_set_value(miniArc_[2], d.enabled ? 1000 : 0);
        value(message_, app_.message().empty() ? "Hold a dial to edit" : app_.message());
    } else {
        value(status_, !app_.fresh() ? "UNKNOWN / UNLOAD" :
            r.loaded ? std::to_string(r.settings.weight)+" "+unitName(r.unit)+" / UNLOAD" :
            r.autoLoadArmed ? "AUTO LOAD / CANCEL" : "Turn dial to adjust");
        if (page_ == Screen::Drops) {
            int n = draftDrops_.mode == DropMode::Stairs ? draftDrops_.step :
                dropField_ == 0 ? draftDrops_.drops : dropField_ == 1 ? draftDrops_.percent : draftDrops_.holdSeconds;
            value(weight_, std::to_string(n)); color(weight_, amber);
            value(unit_, draftDrops_.mode == DropMode::Stairs ? unitName(r.unit) : dropField_ == 0 ? "DROPS" : dropField_ == 1 ? "%" : "SECONDS");
            value(hint_, draftDrops_.mode == DropMode::Stairs ? "Removed after each rep" : dropField_ == 0 ?
                std::to_string(draftDrops_.drops+1)+" total sets" : dropField_ == 1 ? "Reduction each set" : "Hold steady to drop");
            lv_arc_set_value(arc_, percentOf(std::abs(n), editMaximum()));
            for (int i=0; i<2; ++i) color(choice_[i], int(draftDrops_.mode) == i ? amber : muted);
            if (draftDrops_.mode == DropMode::RestPause) {
                value(miniValue_[0], "DROPS\n"+std::to_string(draftDrops_.drops));
                value(miniValue_[1], "DROP %\n"+std::to_string(draftDrops_.percent)+"%");
                value(miniValue_[2], "HOLD\n"+std::to_string(draftDrops_.holdSeconds)+"s");
                for (int i=0; i<3; ++i) {
                    // Every RP parameter is active. The border denotes rotary focus only.
                    color(miniArc_[i], amber);
                    lv_obj_set_style_border_color(miniArc_[i], lv_color_hex(dropField_ == i ? amber : 0x344152), 0);
                    lv_obj_set_style_bg_color(miniArc_[i], lv_color_hex(dropField_ == i ? 0x293140 : 0x17202d), 0);
                }
            }
        } else {
            draftValue_ = std::clamp(draftValue_, 0, editMaximum());
            value(weight_, std::to_string(draftValue_)); color(weight_, page_ == Screen::Magic ? purple : blue);
            value(unit_, r.percent ? "%" : unitName(r.unit)); value(hint_, "MAX "+std::to_string(editMaximum())+(r.percent ? "%" : unitName(r.unit)));
            lv_arc_set_value(arc_, percentOf(draftValue_, editMaximum()));
            if (page_ == Screen::Magic)
                for (int i=0; i<3; ++i) {
                    color(choice_[i], int(draftMode_) == i && r.magicSupported[i] ? purple : muted);
                    if (!r.magicSupported[i]) lv_obj_add_flag(choice_[i], LV_OBJ_FLAG_HIDDEN);
                }
        }
        value(message_, "SIMULATION");
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

#pragma once
#include "controller/TrainingApplication.h"
#include "hardware/interfaces/HardwarePlatform.h"
#include <lvgl.h>
#include <memory>
#include <vector>
namespace voltra {
enum class Screen { Devices, Main, Magic, Eccentric, Drops };
class UiController {
public:
    UiController(HardwarePlatform&, TrainingApplication&);
    ~UiController();
    void tick();
    void readTouch();
    void navigate(Screen); // Also used by the offline render preview.
private:
    struct Binding { UiController* ui; int action; std::string id; };
    HardwarePlatform& hardware_;
    TrainingApplication& app_;
    lv_display_t* display_ = nullptr;
    lv_indev_t* touch_ = nullptr;
    lv_obj_t *face_ = nullptr, *weight_ = nullptr, *title_ = nullptr, *status_ = nullptr,
        *hint_ = nullptr, *message_ = nullptr, *arc_ = nullptr, *unit_ = nullptr, *limit_ = nullptr;
    lv_obj_t *miniArc_[3]{}, *miniValue_[3]{}, *modeLabels_[3]{}, *choice_[3]{};
    lv_obj_t *tiles_[3]{};
    lv_obj_t *stairs_[3]{};
    lv_obj_t* dropTitle_ = nullptr;
    std::vector<std::unique_ptr<Binding>> bindings_;
    std::vector<std::pair<lv_obj_t*, DeviceId>> deviceRows_;
    std::vector<uint32_t> drawBuffer_;
    Screen page_ = Screen::Devices, next_ = Screen::Devices;
    bool rebuild_ = true;
    int width_ = 0, height_ = 0, draftValue_ = 0, dropField_ = 0;
    MagicMode draftMode_ = MagicMode::Chains;
    DropSettings draftDrops_;
    std::string deviceSignature_;
    uint64_t lastTickMs_ = 0;
    lv_obj_t* label(lv_obj_t*, const std::string&, int w, int y, const lv_font_t* = &lv_font_montserrat_14);
    lv_obj_t* button(lv_obj_t*, const std::string&, int x, int y, int w, int h, int action);
    lv_obj_t* arc(lv_obj_t*, int size, uint32_t color);
    void bind(lv_obj_t*, int action, const std::string& id = {});
    void act(int action, const std::string& id, bool held);
    void build();
    void update();
    void rotate(int delta);
    int editMaximum() const;
};
}

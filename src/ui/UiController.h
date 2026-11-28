#pragma once
#include "controller/TrainingApplication.h"
#include "controller/RuntimeMode.h"
#include "hardware/interfaces/HardwarePlatform.h"
#include "RotaryAdjustment.h"
#include "network/WifiSetup.h"
#include "update/FirmwareUpdate.h"
#include "diagnostics/DebugReports.h"
#include <lvgl.h>
#include <memory>
#include <vector>
#include <array>
#include <map>
namespace voltra {
enum class Screen { Devices, Main, Magic, Eccentric, Drops, Presets, SavePreset, OverridePreset, Live, Settings, Reset, Wifi, Updates, About, PresetNotice, ForgetDevice, ConnectionNotice, ControlNotice, DirectLoadConfirm };
class UiController {
public:
    UiController(HardwarePlatform&, TrainingApplication&, RemoteConfigurationStore* = nullptr, RuntimeMode* = nullptr, WifiSetup* = nullptr, FirmwareUpdate* = nullptr, DebugReports* = nullptr);
    ~UiController();
    void tick();
    void readTouch();
    void navigate(Screen); // Also used by the offline render preview.
private:
    struct Binding { UiController* ui; int action; std::string id; };
    HardwarePlatform& hardware_;
    TrainingApplication& app_;
    RuntimeMode* runtime_ = nullptr;
    WifiSetup* wifi_ = nullptr;
    FirmwareUpdate* updates_ = nullptr;
    DebugReports* reports_=nullptr;
    static constexpr uint64_t controlHoldMs=650; // Existing ordinary hold threshold; dispatch on release.
    static constexpr uint64_t autoLoadHintMs=300; // A weight hold becomes visible and no longer counts as a tap.
    static constexpr uint64_t autoLoadHoldMs=2000; // User-requested touch hold; independent of VOLTRA's cable countdown.
    static constexpr uint64_t debugHintMs=3000; // Leave ordinary ~2-second holds unobstructed.
    static constexpr uint64_t debugHoldMs=6000; // User-requested global diagnostic gesture.
    lv_obj_t *debugDialog_=nullptr,*debugText_=nullptr,*debugSend_=nullptr,*debugSendLabel_=nullptr,
        *debugUnload_=nullptr,*debugControlStatus_=nullptr,*debugHint_=nullptr;
    bool debugPending_=false,debugConsumed_=false,debugSubmitted_=false,debugMoved_=false;
    bool debugDisconnectPending_=false,debugUnloadRequested_=false;
    uint64_t touchStartedAt_=0,touchEventAt_=0;
    std::string lastDiagnosticMessage_;
    uint64_t loadRefusalRevision_=0;
    bool confirmDirectLoadQueued_=false; // One explicit release, revalidated after the BLE poll.
    void openDebugDialog();
    void closeDebugDialog();
    void updateDebugDialog();
    void sendDebugReport();
    void unloadFromDebugDialog();
    static constexpr uint8_t lowBatteryPercent=10, batteryRearmPercent=15;
    static constexpr uint64_t batteryAlertIdleMs=600;
    bool lowBatteryEpisode_=false, lowBatteryPending_=false, batteryUnloadRequested_=false;
    uint8_t batteryAlertPercent_=0;
    uint64_t batteryInteractionAt_=0;
    lv_obj_t *batteryAlertDialog_=nullptr,*batteryAlertValue_=nullptr,*batteryAlertText_=nullptr,*batteryAlertUnload_=nullptr;
    std::optional<uint8_t> remoteBatteryPercent() const;
    void updateBatteryAlert(uint64_t now);
    void openBatteryAlert();
    void closeBatteryAlert(bool acknowledged);
    bool updatePromptShown_=false;
    // The Wi-Fi join has a 20-second deadline; allow a bounded UI handoff too.
    static constexpr uint64_t updateWifiWaitMs=25000;
    bool updateCheckPending_=false,updateWifiRequested_=false;
    uint64_t updateCheckAt_=0;
    uint64_t updateCheckRevision_=0;
    std::string updateCheckMessage_;
    void requestUpdateCheck();
    void tickUpdateCheck(uint64_t now);
    uint64_t modeGestureUntil_ = 0;
    int modeGestureClicks_ = 0;
    DeviceId forgetId_;
    std::string forgetName_;
    lv_display_t* display_ = nullptr;
    lv_indev_t* touch_ = nullptr;
    lv_obj_t *face_ = nullptr, *weight_ = nullptr, *title_ = nullptr, *status_ = nullptr,
        *hint_ = nullptr, *message_ = nullptr, *arc_ = nullptr, *unit_ = nullptr, *limit_ = nullptr;
    lv_obj_t *autoLoadArc_=nullptr,*autoLoadTitle_=nullptr,*autoLoadPrompt_=nullptr;
    lv_obj_t *autoLoadTarget_=nullptr,*autoLoadDetail_=nullptr,*autoLoadCountdown_=nullptr,*autoLoadCancel_=nullptr;
    bool weightHoldActive_=false,weightUnloadGesture_=false,autoFeedbackVisible_=false;
    void updateAutoLoadFeedback();
    // Presentation only: confirmed Auto Load completion can precede the first
    // workout packet. Keep Live through that handoff without changing control state.
    DeviceId autoLiveDevice_;
    bool autoLiveArmedSeen_=false,autoLiveCompletionConsumed_=false,autoLiveAwaitingSet_=false;
    int autoLiveWorkoutPhase_=0;
    uint64_t autoLiveSetEndRevision_=0;
    void updateLiveNavigation(const TrainingReport&);
    static std::string deviceNames(const std::string& first,const std::string& second);
    std::vector<const DeviceEntry*> deviceChoices() const;
    lv_obj_t *miniArc_[3]{}, *miniValue_[3]{}, *modeLabels_[3]{}, *choice_[3]{};
    lv_obj_t *tiles_[3]{};
    lv_obj_t *stairs_[3]{};
    lv_obj_t* dropTitle_ = nullptr;
    lv_obj_t* dropsCommitLabel_ = nullptr;
    lv_obj_t *center_ = nullptr, *battery_ = nullptr;
    lv_obj_t *scaleLabels_[7]{}, *choiceNames_[3]{};
    static constexpr int tickCount = 41;
    std::array<lv_obj_t*, tickCount> ticks_{};
    std::array<std::array<lv_point_precise_t, 2>, tickCount> tickPoints_{};
    size_t devicePage_ = 0;
    std::vector<std::unique_ptr<Binding>> bindings_;
    std::vector<std::pair<lv_obj_t*, DeviceId>> deviceRows_;
    std::vector<uint32_t> drawBuffer_;
    Screen page_ = Screen::Devices, next_ = Screen::Devices;
    bool rebuild_ = true;
    int width_ = 0, height_ = 0, draftValue_ = 0, dropField_ = 0;
    MagicMode draftMode_ = MagicMode::Chains;
    DropSettings draftDrops_;
    std::string deviceSignature_, scaleSignature_;
    uint64_t lastTickMs_ = 0;
    RotaryAdjustment rotaryAdjustment_;
    bool touchDown_ = false, touchMoved_ = false, touchPressTwin_ = false;
    bool touchScopeInvalid_ = false;
    int touchStartX_ = 0, touchStartY_ = 0;
    RemoteConfigurationStore* store_ = nullptr;
    RemoteConfiguration config_;
    int presetIndex_ = 0, activePreset_ = -1, pendingPreset_ = -1, settingsField_ = 0;
    bool draftPercent_ = true, editDirty_ = false, returnAfterEdit_ = false;
    bool lastInputTwin_ = false, lastInputCheckingWorkout_ = false;
    uint64_t editedAt_ = 0;
    lv_obj_t* unitChoices_[2]{};
    void edited();
    void largeValue(const std::string&);
    struct UnitSelections { int base = -1; WeightUnit unit = WeightUnit::Pounds; std::optional<int> percent, absolute; };
    std::map<DeviceId, std::array<UnitSelections, 2>> unitMemory_;
    UnitSelections& unitSelection();
    // Editor defaults only. Disabled device reports remain disabled/zero, and
    // recalling an amount never sends it or restores resistance on reconnect.
    struct ModifierSelection { Modifier amount; WeightUnit unit; };
    std::map<DeviceId, std::array<std::optional<ModifierSelection>, 2>> modifierMemory_;
    void rememberModifiers();
    Modifier editorModifier(bool magic) const;
    std::string presetNotice_;

    bool weightFocused_ = true, restAlerted_ = false;
    uint64_t activityAt_ = 0;
    bool sleeping_ = false, dimmed_ = false;
    bool wakeTouch_ = false;
    static constexpr uint32_t restBuzzMs = 20000; // User-requested maximum alert duration.
    bool restNotice_ = false, restBuzzing_ = false;
    uint64_t restBuzzUntil_ = 0;
    lv_obj_t* restButton_ = nullptr;
    void dismissRestAlert();
    void updateRestAlert(uint64_t now);
    uint64_t dropCueRevision_=0, dropCueUntil_=0;
    Preset saveDraft_;
    lv_obj_t *presetTile_ = nullptr, *presetValue_ = nullptr, *presetCaption_=nullptr, *stats_ = nullptr, *start_ = nullptr, *nextWeight_ = nullptr;
    lv_obj_t* holdBar_ = nullptr;
    lv_obj_t *wifiBody_=nullptr,*wifiButton_=nullptr,*wifiReset_=nullptr,*wifiBack_=nullptr;
    bool wifiConfiguring_=false;
    lv_obj_t *updateBody_=nullptr,*updateInstall_=nullptr;
    lv_obj_t* settingsValues_[4]{};
    lv_obj_t *bleTraceButton_=nullptr,*loadedAdjustmentButton_=nullptr;
    bool bleTraceEnabled_=false;
    void setBleTrace(bool enabled);
    lv_obj_t* liveValues_[3]{};
    uint32_t accent() const;
    uint32_t limitAccent() const;
    bool saveConfiguration();
    void buildExtra();
    void updateExtra();
    void separator(int y, int inset = 12);
    std::string presetSummary(const Preset&, bool multiline = false) const;
    lv_obj_t* label(lv_obj_t*, const std::string&, int w, int y, const lv_font_t* = &lv_font_montserrat_14);
    lv_obj_t* button(lv_obj_t*, const std::string&, int x, int y, int w, int h, int action);
    lv_obj_t* arc(lv_obj_t*, int size, uint32_t color);
    lv_obj_t* disc(lv_obj_t*, int x, int y, int size, uint32_t accent, bool active);
    lv_obj_t* linkIcon(lv_obj_t*, int size, uint32_t color);
    void glow(lv_obj_t*, uint32_t accent, bool active);
    void buildScale();
    void updateScale(int selected, int maximum, const std::string& suffix, uint32_t accent, bool negative = false);
    void bind(lv_obj_t*, int action, const std::string& id = {});
    void act(int action, const std::string& id, bool held);
    void build();
    void update();
    std::string controlFeedback() const;
    bool saveDropsOnly(const TrainingReport&) const;
    void rotate(int delta);
    int editMaximum() const;
    int editValue(int amount) const;
};
}

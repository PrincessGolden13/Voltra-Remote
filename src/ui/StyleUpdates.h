#pragma once
#include <lvgl.h>
namespace voltra::style {
// LVGL 9.3 invalidates an object even when a local style is set to its existing
// value. Avoid needless full-panel refreshes without caching pointers to objects
// that may be destroyed during navigation. Compare local (not transient) style.
inline bool unchanged(lv_obj_t* obj, lv_style_prop_t prop, lv_style_value_t v,
                      lv_style_selector_t selector) {
    lv_style_value_t old{};
    if(lv_obj_get_local_style_prop(obj,prop,&old,selector)!=LV_STYLE_RES_FOUND) return false;
    if(prop==LV_STYLE_TEXT_FONT) return old.ptr==v.ptr;
    switch(prop) {
    case LV_STYLE_TEXT_COLOR: case LV_STYLE_BG_COLOR: case LV_STYLE_BG_GRAD_COLOR:
    case LV_STYLE_BORDER_COLOR: case LV_STYLE_SHADOW_COLOR: case LV_STYLE_ARC_COLOR:
    case LV_STYLE_LINE_COLOR: return lv_color_eq(old.color,v.color);
    default: return old.num==v.num;
    }
}
#define VOLTRA_STYLE(name,prop,type,field) \
inline void set_##name(lv_obj_t* o,type value,lv_style_selector_t s) { \
    lv_style_value_t v{};v.field=value; \
    if(!unchanged(o,prop,v,s)) lv_obj_set_style_##name(o,value,s); }
VOLTRA_STYLE(text_color,LV_STYLE_TEXT_COLOR,lv_color_t,color)
VOLTRA_STYLE(bg_color,LV_STYLE_BG_COLOR,lv_color_t,color)
VOLTRA_STYLE(bg_grad_color,LV_STYLE_BG_GRAD_COLOR,lv_color_t,color)
VOLTRA_STYLE(border_color,LV_STYLE_BORDER_COLOR,lv_color_t,color)
VOLTRA_STYLE(shadow_color,LV_STYLE_SHADOW_COLOR,lv_color_t,color)
VOLTRA_STYLE(arc_color,LV_STYLE_ARC_COLOR,lv_color_t,color)
VOLTRA_STYLE(line_color,LV_STYLE_LINE_COLOR,lv_color_t,color)
VOLTRA_STYLE(text_font,LV_STYLE_TEXT_FONT,const lv_font_t*,ptr)
VOLTRA_STYLE(bg_grad_dir,LV_STYLE_BG_GRAD_DIR,lv_grad_dir_t,num)
VOLTRA_STYLE(text_align,LV_STYLE_TEXT_ALIGN,lv_text_align_t,num)
VOLTRA_STYLE(align,LV_STYLE_ALIGN,lv_align_t,num)
VOLTRA_STYLE(opa,LV_STYLE_OPA,lv_opa_t,num)
VOLTRA_STYLE(bg_opa,LV_STYLE_BG_OPA,lv_opa_t,num)
VOLTRA_STYLE(shadow_opa,LV_STYLE_SHADOW_OPA,lv_opa_t,num)
VOLTRA_STYLE(border_width,LV_STYLE_BORDER_WIDTH,int32_t,num)
VOLTRA_STYLE(shadow_width,LV_STYLE_SHADOW_WIDTH,int32_t,num)
VOLTRA_STYLE(arc_width,LV_STYLE_ARC_WIDTH,int32_t,num)
VOLTRA_STYLE(line_width,LV_STYLE_LINE_WIDTH,int32_t,num)
VOLTRA_STYLE(text_line_space,LV_STYLE_TEXT_LINE_SPACE,int32_t,num)
#undef VOLTRA_STYLE
inline void set_align(lv_obj_t* obj,lv_align_t alignment) { set_align(obj,alignment,0); }
inline void align(lv_obj_t* obj,lv_align_t alignment,int32_t x,int32_t y) {
    set_align(obj,alignment);lv_obj_set_pos(obj,x,y);
}
inline void center(lv_obj_t* obj) { align(obj,LV_ALIGN_CENTER,0,0); }
}

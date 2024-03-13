// This file was generated by SquareLine Studio
// SquareLine Studio version: SquareLine Studio 1.3.4
// LVGL version: 8.3.6
// Project name: SquareLine_Project

#include "../ui.h"

void ui_Screen6_screen_init(void)
{
    ui_Screen6 = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_Screen6, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_bg_color(ui_Screen6, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Screen6, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_Image18 = lv_img_create(ui_Screen6);
    lv_img_set_src(ui_Image18, &ui_img_9_png);
    lv_obj_set_width(ui_Image18, LV_SIZE_CONTENT);   /// 40
    lv_obj_set_height(ui_Image18, LV_SIZE_CONTENT);    /// 31
    lv_obj_set_x(ui_Image18, -26);
    lv_obj_set_y(ui_Image18, 217);
    lv_obj_set_align(ui_Image18, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_Image18, LV_OBJ_FLAG_ADV_HITTEST);     /// Flags
    lv_obj_clear_flag(ui_Image18, LV_OBJ_FLAG_SCROLLABLE);      /// Flags

    ui_Image19 = lv_img_create(ui_Screen6);
    lv_img_set_src(ui_Image19, &ui_img_1_png);
    lv_obj_set_width(ui_Image19, LV_SIZE_CONTENT);   /// 144
    lv_obj_set_height(ui_Image19, LV_SIZE_CONTENT);    /// 234
    lv_obj_set_x(ui_Image19, -163);
    lv_obj_set_y(ui_Image19, 3);
    lv_obj_set_align(ui_Image19, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_Image19, LV_OBJ_FLAG_ADV_HITTEST);     /// Flags
    lv_obj_clear_flag(ui_Image19, LV_OBJ_FLAG_SCROLLABLE);      /// Flags

    ui_Image20 = lv_img_create(ui_Screen6);
    lv_img_set_src(ui_Image20, &ui_img_15_png);
    lv_obj_set_width(ui_Image20, LV_SIZE_CONTENT);   /// 148
    lv_obj_set_height(ui_Image20, LV_SIZE_CONTENT);    /// 115
    lv_obj_set_x(ui_Image20, 153);
    lv_obj_set_y(ui_Image20, 3);
    lv_obj_set_align(ui_Image20, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_Image20, LV_OBJ_FLAG_ADV_HITTEST);     /// Flags
    lv_obj_clear_flag(ui_Image20, LV_OBJ_FLAG_SCROLLABLE);      /// Flags

    ui_Image50 = lv_img_create(ui_Screen6);
    lv_img_set_src(ui_Image50, &ui_img_i_png);
    lv_obj_set_width(ui_Image50, LV_SIZE_CONTENT);   /// 83
    lv_obj_set_height(ui_Image50, LV_SIZE_CONTENT);    /// 76
    lv_obj_set_x(ui_Image50, -24);
    lv_obj_set_y(ui_Image50, 7);
    lv_obj_set_align(ui_Image50, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_Image50, LV_OBJ_FLAG_ADV_HITTEST);     /// Flags
    lv_obj_clear_flag(ui_Image50, LV_OBJ_FLAG_SCROLLABLE);      /// Flags

    ui_Label12 = lv_label_create(ui_Screen6);
    lv_obj_set_width(ui_Label12, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_Label12, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_Label12, -24);
    lv_obj_set_y(ui_Label12, -197);
    lv_obj_set_align(ui_Label12, LV_ALIGN_CENTER);
    lv_label_set_text(ui_Label12, "配网成功\n正在进入");
    lv_obj_set_style_text_font(ui_Label12, &ui_font_R108, LV_PART_MAIN | LV_STATE_DEFAULT);

}

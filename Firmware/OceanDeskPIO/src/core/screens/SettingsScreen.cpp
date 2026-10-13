#include "SettingsScreen.h"
#include <lvgl.h>
#include "config/Theme.h"
#include "core/TimeOfDay.h"
#include "services/ConfigService.h"
#include "services/DeviceIdentity.h"
#include "services/WiFiService.h"
#include "locations/BeachCatalog.h"

namespace {
lv_obj_t *overlay = nullptr, *brightnessLabel = nullptr, *nightLabel = nullptr;
void text(lv_obj_t *o, const char *s, const lv_font_t *f, uint32_t c) { lv_label_set_text(o,s); lv_obj_set_style_text_font(o,f,0); lv_obj_set_style_text_color(o,Theme::color(c),0); }
void refresh() { const DeviceConfig &c=ConfigService::get(); char b[24], nightStart[6], nightEnd[6], night[32]; snprintf(b,sizeof(b),"Brilho  %u%%",c.brightness); formatTimeOfDay(c.nightStartMinute,nightStart,sizeof(nightStart)); formatTimeOfDay(c.nightEndMinute,nightEnd,sizeof(nightEnd)); snprintf(night,sizeof(night),"Ativo %s - %s",nightStart,nightEnd); lv_label_set_text(brightnessLabel,b); lv_label_set_text(nightLabel,c.nightModeEnabled?night:"Inativo"); }
void back(lv_event_t *) { SettingsScreen::close(); }
void brightness(lv_event_t *e) { const DeviceConfig &c=ConfigService::get(); int n=c.brightness+reinterpret_cast<intptr_t>(lv_event_get_user_data(e)); n=n<25?25:(n>100?100:n); ConfigService::saveBrightness(n,c.nightModeEnabled); refresh(); }
void night(lv_event_t *) { const DeviceConfig &c=ConfigService::get(); ConfigService::saveBrightness(c.brightness,!c.nightModeEnabled); refresh(); }
lv_obj_t *button(lv_obj_t *p,const char *s,int x,int y,int w,lv_event_cb_t cb,void *d=nullptr) { lv_obj_t *b=lv_btn_create(p); lv_obj_set_size(b,w,54); lv_obj_set_style_radius(b,14,0); lv_obj_set_style_bg_color(b,Theme::color(Theme::Text),0); lv_obj_align(b,LV_ALIGN_TOP_LEFT,x,y); lv_obj_add_event_cb(b,cb,LV_EVENT_CLICKED,d); lv_obj_t *l=lv_label_create(b); text(l,s,Theme::FontMedium,Theme::Background); lv_obj_center(l); return l; }
void systemLine(lv_obj_t *parent, const String &value, int y)
{
    lv_obj_t *line = lv_label_create(parent);
    text(line, value.c_str(), Theme::FontSmall, Theme::TextSecondary);
    lv_obj_set_width(line, 430);
    lv_label_set_long_mode(line, LV_LABEL_LONG_WRAP);
    lv_obj_align(line, LV_ALIGN_TOP_LEFT, 520, y);
}
void createSystemSummary(lv_obj_t *parent)
{
    const DeviceConfig &config = ConfigService::get();
    const DeviceIdentity &identity = DeviceIdentityService::get();
    const BeachCatalog::TideModel *model = BeachCatalog::findTideModel(config.tideStation.c_str());
    String shortDeviceId = identity.deviceId;
    if (shortDeviceId.length() > 10) shortDeviceId = shortDeviceId.substring(shortDeviceId.length() - 10);
    lv_obj_t *heading = lv_label_create(parent);
    text(heading, "Sistema", Theme::FontMedium, Theme::Text);
    lv_obj_align(heading, LV_ALIGN_TOP_LEFT, 520, 150);
    systemLine(parent, String("Série: ") + identity.serialNumber, 195);
    systemLine(parent, String("ID: ") + shortDeviceId + "  ·  FW " + identity.firmwareVersion, 230);
    systemLine(parent, String("Praia: ") + (config.beachName.isEmpty() ? "Por configurar" : config.beachName), 275);
    systemLine(parent, String("Maré: ") + (model != nullptr ? model->displayName : config.tideStation), 310);
    systemLine(parent, String("Wi-Fi: ") + WiFiService::statusLabel(), 345);
}
}
void SettingsScreen::create() { if(overlay) return; overlay=lv_obj_create(lv_scr_act()); lv_obj_remove_style_all(overlay); lv_obj_set_size(overlay,LV_PCT(100),LV_PCT(100)); lv_obj_set_style_bg_color(overlay,Theme::color(Theme::Background),0); lv_obj_set_style_bg_opa(overlay,LV_OPA_COVER,0); lv_obj_clear_flag(overlay,LV_OBJ_FLAG_SCROLLABLE); lv_obj_t *t=lv_label_create(overlay); text(t,"Definições",Theme::FontLarge,Theme::Text); lv_obj_align(t,LV_ALIGN_TOP_LEFT,48,38); button(overlay,"Voltar",836,32,140,back); brightnessLabel=lv_label_create(overlay); text(brightnessLabel,"",Theme::FontMedium,Theme::Text); lv_obj_align(brightnessLabel,LV_ALIGN_TOP_LEFT,48,150); button(overlay,"Menos",48,205,150,brightness,reinterpret_cast<void *>(static_cast<intptr_t>(-25))); button(overlay,"Mais",216,205,150,brightness,reinterpret_cast<void *>(static_cast<intptr_t>(25))); lv_obj_t *n=lv_label_create(overlay); text(n,"Modo noturno",Theme::FontMedium,Theme::Text); lv_obj_align(n,LV_ALIGN_TOP_LEFT,48,310); nightLabel=button(overlay,"",48,365,420,night); createSystemSummary(overlay); refresh(); }
void SettingsScreen::close() { if(!overlay) return; lv_obj_del_async(overlay); overlay=nullptr; brightnessLabel=nullptr; nightLabel=nullptr; }

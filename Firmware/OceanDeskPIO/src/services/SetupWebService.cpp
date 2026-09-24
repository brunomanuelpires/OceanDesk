#include "SetupWebService.h"

#include <Arduino.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include "AlarmService.h"
#include "BuzzerService.h"
#include "ConfigService.h"
#include "WiFiService.h"
#include "core/TimeOfDay.h"
#include "locations/BeachCatalog.h"

namespace
{
WebServer server(80);
unsigned long restartAt = 0;
bool mdnsStarted = false;

String escapeHtml(String value)
{
    value.replace("&", "&amp;"); value.replace("\"", "&quot;");
    value.replace("<", "&lt;"); value.replace(">", "&gt;");
    return value;
}

String colorValue(uint32_t color)
{
    char value[8];
    snprintf(value, sizeof(value), "#%06lX", static_cast<unsigned long>(color & 0xFFFFFF));
    return String(value);
}

bool saveSetupNetwork(DeviceConfig &config, const String &ssid, const String &password)
{
    if (ssid == config.wifiSsid)
    {
        config.wifiPassword = password;
        return true;
    }
    if (ssid == config.wifiSsid2)
    {
        config.wifiPassword2 = password;
        return true;
    }
    if (ssid == config.wifiSsid3)
    {
        config.wifiPassword3 = password;
        return true;
    }
    if (config.wifiSsid2.isEmpty())
    {
        config.wifiSsid2 = ssid;
        config.wifiPassword2 = password;
        return true;
    }
    if (config.wifiSsid3.isEmpty())
    {
        config.wifiSsid3 = ssid;
        config.wifiPassword3 = password;
        return true;
    }
    return false;
}

String tideModelLabel(const String &modelId)
{
    const BeachCatalog::TideModel *model = BeachCatalog::findTideModel(modelId.c_str());
    return model != nullptr ? String(model->displayName) : modelId;
}

void appendBeachOptions(String &page, const DeviceConfig &config)
{
    const BeachCatalog::BeachLocation *selected =
        BeachCatalog::findBeach(config.beachId.c_str());
    if (selected == nullptr && !ConfigService::isConfigured())
    {
        selected = &BeachCatalog::defaultBeach();
    }
    else if (selected == nullptr)
    {
        page += "<option value=\"__legacy__\" selected data-station=\"";
        page += escapeHtml(tideModelLabel(config.tideStation));
        page += "\" data-timezone=\"" + escapeHtml(config.timezone) + "\">Configuração anterior — ";
        page += escapeHtml(config.beachName) + "</option>";
    }

    String openRegion;
    for (size_t index = 0; index < BeachCatalog::beachCount(); ++index)
    {
        const BeachCatalog::BeachLocation &beach = BeachCatalog::beachAt(index);
        if (openRegion != beach.region)
        {
            if (!openRegion.isEmpty()) page += "</optgroup>";
            openRegion = beach.region;
            page += "<optgroup label=\"" + escapeHtml(openRegion) + "\">";
        }

        const BeachCatalog::TideModel *model = BeachCatalog::findTideModel(beach.tideModelId);
        page += "<option value=\"" + escapeHtml(beach.id) + "\"";
        if (selected == &beach) page += " selected";
        page += " data-station=\"" + escapeHtml(model != nullptr ? model->displayName : beach.tideModelId) + "\"";
        page += " data-timezone=\"" + escapeHtml(BeachCatalog::geographicAreaName(beach.area)) + "\">";
        page += escapeHtml(beach.displayName) + "</option>";
    }
    if (!openRegion.isEmpty()) page += "</optgroup>";
}

String buildPage()
{
    const DeviceConfig &config = ConfigService::get();
    const bool addingNetwork = ConfigService::isConfigured() && WiFiService::isSetupMode();
    char alarmTime[6];
    char nightStart[6];
    char nightEnd[6];
    snprintf(alarmTime, sizeof(alarmTime), "%02u:%02u", config.alarmHour, config.alarmMinute);
    formatTimeOfDay(config.nightStartMinute, nightStart, sizeof(nightStart));
    formatTimeOfDay(config.nightEndMinute, nightEnd, sizeof(nightEnd));
    String page = R"HTML(<!doctype html><html lang="pt"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>OceanDesk</title><style>
:root{color-scheme:dark;font-family:system-ui,-apple-system,sans-serif;background:#0b1620;color:#fff}body{margin:0;min-height:100vh;display:grid;place-items:center;padding:24px;box-sizing:border-box}main{width:min(100%,520px);background:#132430;border:1px solid #20313e;border-radius:22px;padding:30px;box-sizing:border-box}h1{margin:0 0 8px;font-size:30px}p,.hint{color:#a9bfcc;line-height:1.5}.notice{background:#143d36;border:1px solid #38c172;color:#dff8ec;padding:12px 14px;border-radius:12px;margin:16px 0;transition:opacity .25s ease}.details{display:grid;grid-template-columns:1fr 1fr;gap:12px;margin-top:10px;padding:13px;border-radius:12px;background:#0b1620;color:#a9bfcc;font-size:13px}.details strong{display:block;color:#fff;margin-top:3px}label{display:block;margin:18px 0 7px;font-weight:650}input,select{width:100%;box-sizing:border-box;border:1px solid #355063;border-radius:12px;background:#0b1620;color:#fff;padding:13px;font-size:16px}input[type=time]{width:100%;max-width:100%;min-width:0;height:52px;min-height:52px;padding:0;line-height:52px}input[type=color]{height:52px;padding:4px;background:transparent}.row{display:grid;grid-template-columns:1fr 1fr;gap:16px}.alarm-fields{display:grid;grid-template-columns:minmax(0,1fr);gap:14px;width:100%}.alarm-fields label{min-width:0;margin:0;font-weight:400}.alarm-fields input,.alarm-fields select{display:block;margin-top:7px}.hint{font-size:13px;margin-top:7px}button{width:100%;margin-top:24px;border:0;border-radius:12px;background:#55c9f3;color:#08202b;padding:14px;font-size:16px;font-weight:750}</style></head><body><main><h1>OceanDesk</h1><p>)HTML";
    page += addingNetwork ? "Liga o OceanDesk a uma rede Wi-Fi disponível." : "Configura a localização e o aspeto do teu OceanDesk.";
    page += R"HTML(</p>
)HTML";
    if (server.hasArg("saved"))
    {
        page += R"HTML(<div class="notice" id="saved-notice">As alterações foram enviadas para o OceanDesk.</div>)HTML";
    }
    page += R"HTML(<form method="post" action="/configure">)HTML";
    if (!ConfigService::isConfigured() || addingNetwork)
    {
        page += R"HTML(<label>Nome da rede Wi-Fi</label><input name="ssid" maxlength="32" required><label>Palavra-passe</label><input name="password" type="password" maxlength="63">)HTML";
    }
    if (addingNetwork)
    {
        page += R"HTML(<div class="hint">O OceanDesk guarda esta rede e liga-se automaticamente sempre que ela estiver disponível.</div><button type="submit">Guardar e ligar</button></form></main></body></html>)HTML";
        return page;
    }
    page += R"HTML(<label>Praia ou localização</label><select id="beach_id" name="beach_id" required>)HTML";
    appendBeachOptions(page, config);
    page += R"HTML(</select><div class="hint">Escolhe a praia; o OceanDesk atribui automaticamente o modelo de maré e o fuso horário.</div><div class="details"><div>Modelo de maré<strong id="tide_model">—</strong></div><div>Fuso horário<strong id="location_timezone">—</strong></div></div>
<label>Estilo do ecrã</label><select name="display_style"><option value="classic">Classic — equilibrado</option><option value="minimal">Minimal — estado e próximo evento</option><option value="graph">Graph — gráfico em destaque</option><option value="info">Info — mais dados</option></select><div class="hint">O OceanDesk aplica o novo layout depois de reiniciar.</div>
<label>Alarme diário</label><div class="alarm-fields"><label>Hora<input name="alarm_time" type="time" value=")HTML";
    page += alarmTime;
    page += R"HTML(" required></label><label>Estado<select name="alarm_enabled"><option value="0")HTML";
    if (!config.alarmEnabled) page += " selected";
    page += R"HTML(>Inativo</option><option value="1")HTML";
    if (config.alarmEnabled) page += " selected";
    page += R"HTML(>Ativo</option></select></label></div><div class="hint">Toca diariamente à hora escolhida, durante no máximo um minuto.</div>
<label>Brilho</label><select name="brightness"><option value="25")HTML";
    if (config.brightness == 25) page += " selected";
    page += R"HTML(>25% — baixo</option><option value="50")HTML";
    if (config.brightness == 50) page += " selected";
    page += R"HTML(>50% — médio</option><option value="75")HTML";
    if (config.brightness == 75) page += " selected";
    page += R"HTML(>75% — alto</option><option value="100")HTML";
    if (config.brightness == 100) page += " selected";
    page += R"HTML(>100% — máximo</option></select><label>Modo noturno</label><select name="night_mode"><option value="0")HTML";
    if (!config.nightModeEnabled) page += " selected";
    page += R"HTML(>Inativo</option><option value="1")HTML";
    if (config.nightModeEnabled) page += " selected";
    page += R"HTML(>Ativo</option></select><div class="row"><div>Início<input name="night_start" type="time" value=")HTML";
    page += nightStart;
    page += R"HTML(" required></div><div>Fim<input name="night_end" type="time" value=")HTML";
    page += nightEnd;
    page += R"HTML(" required></div></div><label>Brilho noturno</label><select name="night_brightness"><option value="10")HTML";
    if (config.nightBrightness == 10) page += " selected";
    page += R"HTML(>10% — muito baixo</option><option value="20")HTML";
    if (config.nightBrightness == 20) page += " selected";
    page += R"HTML(>20% — baixo</option><option value="35")HTML";
    if (config.nightBrightness == 35) page += " selected";
    page += R"HTML(>35% — moderado</option><option value="50")HTML";
    if (config.nightBrightness == 50) page += " selected";
    page += R"HTML(>50% — alto</option></select><div class="hint">À noite o OceanDesk reduz visualmente a luminosidade. O controlo físico desta placa é apenas ligado/desligado.</div>
<label>Cores</label><div class="row"><div>Texto<input name="text_color" type="color" value=")HTML";
    page += colorValue(config.textColor) + "\"></div><div>Fundo<input name=\"background_color\" type=\"color\" value=\"" + colorValue(config.backgroundColor) + "\"></div></div>";
    page += R"HTML(<button type="submit">Guardar alterações</button></form></main><script>document.querySelector('[name=display_style]').value=")HTML";
    page += ConfigService::displayStyleName(config.displayStyle);
    page += R"HTML(";const beach=document.getElementById('beach_id');function syncLocation(){const option=beach.selectedOptions[0];document.getElementById('tide_model').textContent=option?.dataset.station||'—';document.getElementById('location_timezone').textContent=option?.dataset.timezone||'—'}beach.addEventListener('change',syncLocation);syncLocation();const notice=document.getElementById('saved-notice');if(notice){setTimeout(()=>{notice.style.opacity='0';setTimeout(()=>notice.remove(),250)},4000)}if(location.search)history.replaceState({},'',location.pathname);</script></body></html>)HTML";
    return page;
}

bool parseColor(const String &value, uint32_t &color)
{
    if (value.length() != 7 || value[0] != '#') return false;
    char *end = nullptr;
    color = strtoul(value.c_str() + 1, &end, 16);
    return end != nullptr && *end == '\0';
}

bool parseAlarmTime(const String &value, uint8_t &hour, uint8_t &minute)
{
    if (value.length() != 5 || value[2] != ':' ||
        value[0] < '0' || value[0] > '9' || value[1] < '0' || value[1] > '9' ||
        value[3] < '0' || value[3] > '9' || value[4] < '0' || value[4] > '9')
    {
        return false;
    }

    hour = static_cast<uint8_t>((value[0] - '0') * 10 + value[1] - '0');
    minute = static_cast<uint8_t>((value[3] - '0') * 10 + value[4] - '0');
    return hour <= 23 && minute <= 59;
}

void handleConfigure()
{
    DeviceConfig config = ConfigService::get();
    const bool addingNetwork = ConfigService::isConfigured() && WiFiService::isSetupMode();
    String ssid = server.arg("ssid"); ssid.trim();
    String beachId = server.hasArg("beach_id") ? server.arg("beach_id") : config.beachId;
    beachId.trim();
    if (beachId.isEmpty() && config.beachId.isEmpty()) beachId = "__legacy__";
    if ((!ConfigService::isConfigured() || addingNetwork) && ssid.isEmpty())
    { server.send(400, "text/plain; charset=utf-8", "Indica o nome da rede Wi-Fi."); return; }
    if (!ConfigService::isConfigured())
    {
        config.wifiSsid = ssid;
        config.wifiPassword = server.arg("password");
    }
    else if (addingNetwork && !saveSetupNetwork(config, ssid, server.arg("password")))
    {
        server.send(400, "text/plain; charset=utf-8", "O OceanDesk já tem três redes Wi-Fi guardadas.");
        return;
    }
    if (addingNetwork)
    {
        if (!ConfigService::save(config))
        { server.send(400, "text/plain; charset=utf-8", "Não foi possível guardar a rede Wi-Fi."); return; }
        AlarmService::suspendUntilRestart();
        server.sendHeader("Location", "/?saved=1");
        server.send(303);
        restartAt = millis() + 1200;
        return;
    }
    if (beachId == "__legacy__" && !ConfigService::isConfigured())
    {
        // A new device has no legacy location. Use the catalog default unless
        // the user explicitly selected a beach in the form.
        ConfigService::selectBeach(config, BeachCatalog::defaultBeach().id);
    }
    else if (beachId == "__legacy__")
    {
        if (!config.beachId.isEmpty()) { server.send(400, "text/plain", "Localização inválida."); return; }
    }
    else if (!ConfigService::selectBeach(config, beachId))
    {
        server.send(400, "text/plain", "Praia ou localização inválida."); return;
    }
    if (!ConfigService::parseDisplayStyle(server.hasArg("display_style") ? server.arg("display_style") : String(ConfigService::displayStyleName(config.displayStyle)), config.displayStyle))
    { server.send(400, "text/plain", "Estilo de ecrã inválido."); return; }
    const String alarmEnabled = server.hasArg("alarm_enabled") ? server.arg("alarm_enabled") : String(config.alarmEnabled ? "1" : "0");
    char storedAlarmTime[6];
    snprintf(storedAlarmTime, sizeof(storedAlarmTime), "%02u:%02u", config.alarmHour, config.alarmMinute);
    const String alarmTime = server.hasArg("alarm_time") ? server.arg("alarm_time") : String(storedAlarmTime);
    if ((alarmEnabled != "0" && alarmEnabled != "1") ||
        !parseAlarmTime(alarmTime, config.alarmHour, config.alarmMinute))
    { server.send(400, "text/plain", "Configuração do alarme inválida."); return; }
    config.alarmEnabled = alarmEnabled == "1";
    const String brightness = server.hasArg("brightness") ? server.arg("brightness") : String(config.brightness);
    const String nightMode = server.hasArg("night_mode") ? server.arg("night_mode") : String(config.nightModeEnabled ? "1" : "0");
    if ((brightness != "25" && brightness != "50" && brightness != "75" && brightness != "100") ||
        (nightMode != "0" && nightMode != "1"))
    { server.send(400, "text/plain", "Configuração de brilho inválida."); return; }
    config.brightness = static_cast<uint8_t>(brightness.toInt());
    config.nightModeEnabled = nightMode == "1";
    const String nightBrightness = server.hasArg("night_brightness") ? server.arg("night_brightness") : String(config.nightBrightness);
    char storedNightStart[6], storedNightEnd[6];
    formatTimeOfDay(config.nightStartMinute, storedNightStart, sizeof(storedNightStart));
    formatTimeOfDay(config.nightEndMinute, storedNightEnd, sizeof(storedNightEnd));
    const String nightStartValue = server.hasArg("night_start") ? server.arg("night_start") : String(storedNightStart);
    const String nightEndValue = server.hasArg("night_end") ? server.arg("night_end") : String(storedNightEnd);
    uint8_t nightStartHour, nightStartMinute, nightEndHour, nightEndMinute;
    if ((nightBrightness != "10" && nightBrightness != "20" && nightBrightness != "35" &&
         nightBrightness != "50") ||
        !parseAlarmTime(nightStartValue, nightStartHour, nightStartMinute) ||
        !parseAlarmTime(nightEndValue, nightEndHour, nightEndMinute))
    { server.send(400, "text/plain; charset=utf-8", "Configuração noturna inválida."); return; }
    config.nightBrightness = static_cast<uint8_t>(nightBrightness.toInt());
    config.nightStartMinute = nightStartHour * 60U + nightStartMinute;
    config.nightEndMinute = nightEndHour * 60U + nightEndMinute;
    const String textColor = server.hasArg("text_color") ? server.arg("text_color") : colorValue(config.textColor);
    const String backgroundColor = server.hasArg("background_color") ? server.arg("background_color") : colorValue(config.backgroundColor);
    if (!parseColor(textColor, config.textColor) ||
        !parseColor(backgroundColor, config.backgroundColor) || !ConfigService::save(config))
    { server.send(400, "text/plain; charset=utf-8", "Configuração inválida."); return; }
    AlarmService::suspendUntilRestart();
    server.sendHeader("Location", "/?saved=1");
    server.send(303);
    restartAt = millis() + 2500;
}

}

void SetupWebService::begin()
{
    server.on("/", HTTP_GET, []() { server.sendHeader("Cache-Control", "no-store"); server.send(200, "text/html; charset=utf-8", buildPage()); });
    server.on("/configure", HTTP_POST, handleConfigure);
    server.onNotFound([]() {
        const String host = WiFiService::isSetupMode() ? WiFiService::setupAddress() : "oceandesk.local";
        server.sendHeader("Location", String("http://") + host + "/");
        server.send(302);
    });
    server.begin();
}

void SetupWebService::update()
{
    server.handleClient();
    if (!mdnsStarted && WiFiService::isConnected() && MDNS.begin("oceandesk"))
    { mdnsStarted = true; MDNS.addService("http", "tcp", 80); Serial.println("Settings: http://oceandesk.local"); }
    if (restartAt != 0 && static_cast<long>(millis() - restartAt) >= 0 &&
        !BuzzerService::isActive())
    {
        ESP.restart();
    }
}

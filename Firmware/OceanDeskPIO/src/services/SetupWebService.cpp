#include "SetupWebService.h"

#include <Arduino.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include "ConfigService.h"
#include "WiFiService.h"

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

String buildPage()
{
    const DeviceConfig &config = ConfigService::get();
    String page = R"HTML(<!doctype html><html lang="pt"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>OceanDesk</title><style>
:root{color-scheme:dark;font-family:system-ui,-apple-system,sans-serif;background:#0b1620;color:#fff}body{margin:0;min-height:100vh;display:grid;place-items:center;padding:24px;box-sizing:border-box}main{width:min(100%,520px);background:#132430;border:1px solid #20313e;border-radius:22px;padding:30px;box-sizing:border-box}h1{margin:0 0 8px;font-size:30px}p,.hint{color:#a9bfcc;line-height:1.5}.notice{background:#143d36;border:1px solid #38c172;color:#dff8ec;padding:12px 14px;border-radius:12px;margin:16px 0}label{display:block;margin:18px 0 7px;font-weight:650}input,select{width:100%;box-sizing:border-box;border:1px solid #355063;border-radius:12px;background:#0b1620;color:#fff;padding:13px;font-size:16px}input[type=color]{height:52px;padding:4px;background:transparent}.row{display:grid;grid-template-columns:1fr 1fr;gap:16px}.hint{font-size:13px;margin-top:7px}button{width:100%;margin-top:24px;border:0;border-radius:12px;background:#55c9f3;color:#08202b;padding:14px;font-size:16px;font-weight:750}</style></head><body><main><h1>OceanDesk</h1><p>Configura a localização e o aspeto do teu OceanDesk.</p>
)HTML";
    if (server.hasArg("saved"))
    {
        page += R"HTML(<div class="notice">As alterações foram enviadas para o OceanDesk.</div>)HTML";
    }
    page += R"HTML(<form method="post" action="/configure">)HTML";
    if (!ConfigService::isConfigured())
    {
        page += R"HTML(<label>Nome da rede Wi-Fi</label><input name="ssid" maxlength="32" required><label>Palavra-passe</label><input name="password" type="password" maxlength="63">)HTML";
    }
    page += R"HTML(<label>Praia apresentada</label><input name="beach" maxlength="48" required value=")HTML";
    page += escapeHtml(config.beachName) + "\">";
    page += R"HTML(<label>Estação de marés</label><select name="station"><option value="nazare">Nazaré</option></select><div class="hint">A versão atual contém dados harmónicos validados da estação da Nazaré.</div>
<label>Fuso horário</label><select name="timezone"><option value="WET0WEST,M3.5.0/1,M10.5.0">Portugal continental / Madeira</option><option value="AZOT1AZOST0,M3.5.0/0,M10.5.0/1">Açores</option><option value="CET-1CEST,M3.5.0/2,M10.5.0/3">Espanha continental</option><option value="UTC0">UTC</option></select>
<label>Estilo do ecrã</label><select name="display_style"><option value="classic">Classic — equilibrado</option><option value="minimal">Minimal — estado e próximo evento</option><option value="graph">Graph — gráfico em destaque</option><option value="info">Info — mais dados</option></select><div class="hint">O OceanDesk aplica o novo layout depois de reiniciar.</div>
<label>Cores</label><div class="row"><div>Texto<input name="text_color" type="color" value=")HTML";
    page += colorValue(config.textColor) + "\"></div><div>Fundo<input name=\"background_color\" type=\"color\" value=\"" + colorValue(config.backgroundColor) + "\"></div></div>";
    page += R"HTML(<button type="submit">Guardar alterações</button></form></main><script>document.querySelector('[name=timezone]').value=")HTML";
    page += config.timezone;
    page += R"HTML(";document.querySelector('[name=display_style]').value=")HTML";
    page += ConfigService::displayStyleName(config.displayStyle);
    page += R"HTML(";if(location.search)history.replaceState({},'',location.pathname);</script></body></html>)HTML";
    return page;
}

bool parseColor(const String &value, uint32_t &color)
{
    if (value.length() != 7 || value[0] != '#') return false;
    char *end = nullptr;
    color = strtoul(value.c_str() + 1, &end, 16);
    return end != nullptr && *end == '\0';
}

void handleConfigure()
{
    DeviceConfig config = ConfigService::get();
    String ssid = server.arg("ssid"); ssid.trim();
    String beach = server.arg("beach"); beach.trim();
    if (beach.isEmpty() || (!ConfigService::isConfigured() && ssid.isEmpty())) { server.send(400, "text/plain", "Preenche os campos obrigatórios."); return; }
    if (!ConfigService::isConfigured()) { config.wifiSsid = ssid; config.wifiPassword = server.arg("password"); }
    config.beachName = beach;
    config.tideStation = server.arg("station");
    config.timezone = server.arg("timezone");
    if (!ConfigService::parseDisplayStyle(server.arg("display_style"), config.displayStyle))
    { server.send(400, "text/plain", "Estilo de ecrã inválido."); return; }
    if (!parseColor(server.arg("text_color"), config.textColor) ||
        !parseColor(server.arg("background_color"), config.backgroundColor) || !ConfigService::save(config))
    { server.send(400, "text/plain", "Configuração inválida."); return; }
    server.sendHeader("Location", "/?saved=1");
    server.send(303);
    restartAt = millis() + 2500;
}
}

void SetupWebService::begin()
{
    server.on("/", HTTP_GET, []() { server.send(200, "text/html; charset=utf-8", buildPage()); });
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
    if (restartAt != 0 && static_cast<long>(millis() - restartAt) >= 0) ESP.restart();
}

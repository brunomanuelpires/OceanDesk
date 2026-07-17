#include <Arduino.h>

void setup()
{
    Serial.begin(115200);
    delay(1000);

    Serial.println();
    Serial.println("OceanDesk PlatformIO");
    Serial.println("ESP32-S3 iniciado com sucesso.");
    Serial.printf("Flash: %u MB\n", ESP.getFlashChipSize() / 1024 / 1024);
    Serial.printf("PSRAM: %u MB\n", ESP.getPsramSize() / 1024 / 1024);
}

void loop()
{
    delay(1000);
}
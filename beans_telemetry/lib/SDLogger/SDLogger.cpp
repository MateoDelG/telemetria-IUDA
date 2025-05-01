#include "SDLogger.h"

// Definición de pines SPI para la SD
#define SD_MISO 2
#define SD_MOSI 15
#define SD_SCLK 14
#define SD_CS   13

SDLogger::SDLogger() : spi(VSPI) {}

bool SDLogger::begin() {
    spi.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS);

    if (!checkSD()) {
        Serial.println("⚠️ Fallo al montar SD en primer intento, reintentando...");
        delay(1000);
        if (!checkSD()) {
            Serial.println("❌ Error crítico: no se pudo montar la SD.");
            ESP.restart();
            return false;
        }
    }

    if (!SD.exists(filename)) {
        File file = SD.open(filename, FILE_WRITE);
        if (file) {
            file.println("[]");
            file.close();
        }
    }

    Serial.println("✅ SD montada correctamente");
    return true;
}

bool SDLogger::checkSD() {
    return SD.begin(SD_CS, spi);
}

void SDLogger::logSensorData(const String& dateTime) {
    JsonDocument entry;

    entry["timestamp"] = dateTime;
    entry["temperatureIndoor"]  = sensorData.getTemperatureIndoor();
    entry["humidityIndoor"]     = sensorData.getHumidityIndoor();
    entry["temperatureOutdoor"] = sensorData.getTemperatureOutdoor();
    entry["humidityOutdoor"]    = sensorData.getHumidityOutdoor();
    entry["lux1"] = sensorData.getLux1();
    entry["lux2"] = sensorData.getLux2();
    entry["lux3"] = sensorData.getLux3();

    String output;
    serializeJson(entry, output);
    writeToFile(output);
}

void SDLogger::writeToFile(const String& jsonData) {
    File file = SD.open(filename, FILE_READ);
    if (!file) {
        Serial.println("❌ No se pudo abrir el archivo para lectura.");
        return;
    }

    String content = file.readString();
    file.close();

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, content);

    JsonArray array;
    if (error || !doc.is<JsonArray>()) {
        Serial.println("⚠️ JSON corrupto o vacío. Se crea nuevo arreglo.");
        doc.clear();
        array = doc.to<JsonArray>();
    } else {
        array = doc.as<JsonArray>();
    }

    // Agregar nuevo objeto
    JsonDocument newEntry;
    deserializeJson(newEntry, jsonData);
    array.add(newEntry.as<JsonObject>());

    File writeFile = SD.open(filename, FILE_WRITE);
    if (!writeFile) {
        Serial.println("❌ No se pudo abrir archivo para escritura.");
        return;
    }

    writeFile.seek(0);  // sobrescribe
    serializeJson(doc, writeFile);
    writeFile.close();
    Serial.println("📁 Registro añadido a datalog.json");
}

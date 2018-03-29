#define USEDS  // Раскомментируйте это макроопределение, чтобы не использовать датчики темпиратуры OneWire Dalass
#define USEBM180  // Раскомментируйте это макроопределение, чтобы не использовать датчики BMP180
#define USEDISPLAY // Раскомментируйте это макроопределение, чтобы не использовать дисплей

#include <Arduino.h>
#include <pgmspace.h>
#include <EEPROM.h>
#include "ESPWeb.h"
#include "Date.h"
#include "Schedule.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP8266HTTPClient.h>
#include <Adafruit_BMP085.h>
#include <LiquidCrystal_PCF8574.h>
#include <Wire.h>

#define SDA_PIN 12
#define SCL_PIN 14

#define KEY_1 2
#define KEY_2 13
#define KEY_3 0
#define KEY_4 15 //INPUT_PULLUP не работает, всегда нажата


#ifdef USEDISPLAY
LiquidCrystal_PCF8574 lcd(0x27);
#endif

#ifdef USEBM180
Adafruit_BMP085 BMP180;
#endif

#ifdef USEDS
#define ONWIRE_PIN 5
OneWire oneWire(ONWIRE_PIN);
DallasTemperature ds(&oneWire);
#endif

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Не забудь поправить версию прошивки !!!!!!!!!!!!!!!!!!!
const char *const SWver PROGMEM = "1.1"; //!!!!!!!!!!!!!!!!!!!!!!!!!!! Не забудь поправить версию прошивки !!!!!!!!!!!!!!!!!!!
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Не забудь поправить версию прошивки !!!!!!!!!!!!!!!!!!!


const int8_t maxRelays = 4; // Количество каналов реле
const int8_t maxSchedules = 10; // Количество элементов расписания

const char *const overSSID PROGMEM = "ESP_Sem_Controll"; // Имя точки доступа по умолчанию

const int8_t defRelayPin = -1; // Пин реле по умолчанию (-1 - не подключено)
const bool defRelayLevel = HIGH; // Уровень срабатывания реле по умолчанию
const bool defRelayOnBoot = false; // Состояние реле при старте модуля по умолчанию
const uint32_t defRelayAutoRelease = 0; // Время в миллисекундах до автоотключения реле по умолчанию (0 - нет автоотключения)

const int8_t defBtnPin = -1; // Пин кнопки по умолчанию (-1 - не подключено)
const bool defBtnLevel = HIGH; // Уровень кнопки при замыкании по умолчанию
const uint32_t defDebounceTime = 20; // Время стабилизации уровня в миллисекундах для борьбы с дребезгом по умолчанию (0 - не используется)


const char *const pathRelay PROGMEM = "/relay"; // Путь до страницы настройки параметров реле
const char *const pathControl PROGMEM = "/control"; // Путь до страницы настройки параметров кнопок/ДУ
const char *const pathSwitch PROGMEM = "/switch"; // Путь до страницы управления переключением реле
const char *const pathSchedule PROGMEM = "/schedule"; // Путь до страницы настройки параметров расписания
const char *const pathGetSchedule PROGMEM = "/getschedule"; // Путь до страницы, возвращающей JSON-пакет элемента расписания
const char *const pathSetSchedule PROGMEM = "/setschedule"; // Путь до страницы изменения элемента расписания
const char *const pathSemMinning PROGMEM = "/semminig"; // Путь до страницы sem mininig

// Имена параметров для Web-форм
const char *const paramGPIO PROGMEM = "relaygpio";
const char *const paramLevel PROGMEM = "relaylevel";
const char *const paramOnBoot PROGMEM = "relayonboot";
const char *const paramAutoRelease PROGMEM = "relayautorelease";
const char *const paramBtnGPIO PROGMEM = "btngpio";
const char *const paramBtnLevel PROGMEM = "btnlevel";
const char *const paramBtnSwitch PROGMEM = "btnswitch";
const char *const paramDebounce PROGMEM = "debounce";
const char *const paramSchedulePeriod PROGMEM = "period";
const char *const paramScheduleHour PROGMEM = "hour";
const char *const paramScheduleMinute PROGMEM = "minute";
const char *const paramScheduleSecond PROGMEM = "second";
const char *const paramScheduleWeekdays PROGMEM = "weekdays";
const char *const paramScheduleDay PROGMEM = "day";
const char *const paramScheduleMonth PROGMEM = "month";
const char *const paramScheduleYear PROGMEM = "year";
const char *const paramScheduleRelay PROGMEM = "relay";
const char *const paramScheduleTurn PROGMEM = "turn";


// Имена JSON-переменных
const char *const jsonRelay PROGMEM = "relay";
const char *const jsonRelayAutoRelease PROGMEM = "relayautorelease";
const char *const jsonSchedulePeriod PROGMEM = "period";
const char *const jsonScheduleHour PROGMEM = "hour";
const char *const jsonScheduleMinute PROGMEM = "minute";
const char *const jsonScheduleSecond PROGMEM = "second";
const char *const jsonScheduleWeekdays PROGMEM = "weekdays";
const char *const jsonScheduleDay PROGMEM = "day";
const char *const jsonScheduleMonth PROGMEM = "month";
const char *const jsonScheduleYear PROGMEM = "year";
const char *const jsonScheduleRelay PROGMEM = "relay";
const char *const jsonScheduleTurn PROGMEM = "turn";
const char *const jsonTempatm PROGMEM = "tempatm";
const char *const jsonPressatm PROGMEM = "pressatm";

const int8_t gpios[] PROGMEM = {-1, 0, 1, 2, 3, 4, 5, 12, 13, 14, 15, 16}; // Доступные для подключения GPIO
const char *const strNone PROGMEM = "(None)";

class ESPSemControll : public ESPWebBase {
public:
    ESPSemControll() : ESPWebBase() {}

#ifdef USEBM180

    bool bmp180isGood;
#endif

protected:

#ifdef USEBM180

    void setupBMP180();

    float bmpGetTemperature();

    int32_t bmpGetPressure();

#endif

#ifdef USEDISPLAY

    void setupDisplay();

    void printSetupDisplay();

    void outputToDisplay();

    uint32_t displayCycleTime; // Вспомогательная переменная периода вывода на дисплей.
    uint32_t displayUpdateTime = 3; // Периода вывода на дисплей.
    bool displaySetupMark = false; // Вспомогательная переменная для вывода сетап дисплея

#endif

    void setupExtra();

    void loopExtra();

    uint16_t readConfig();

    uint16_t writeConfig(bool commit = true);

    void defaultConfig(uint8_t level = 0);

    bool setConfigParam(const String &name, const String &value);

    void setupHttpServer();

    void handleRootPage();

    String jsonData();

#ifdef USEDS

    void setupDS();

    uint8 dsQuantity;
    uint8 dsMeasureIndex;
    uint32_t measureCycleTime; // Вспомогательная переменная периода измерения.
    uint32_t measureTime = 3; // Периода измерения.

    void measureDS();

    float tempDS[];

#endif

    void restPost(String message);

    void sendJsonData();

    void handleRelayConfig(); // Обработчик страницы настройки параметров реле
    void handleControlConfig(); // Обработчик страницы настройки параметров кнопок и пульта ДУ
    void handleRelaySwitch(); // Обработчик страницы управления переключением реле
    void handleScheduleConfig(); // Обработчик страницы настройки параметров расписания
    void handleGetSchedule(); // Обработчик страницы, возвращающей JSON-пакет элемента расписания
    void handleSetSchedule(); // Обработчик страницы изменения элемента расписания
    void handleSemMining(); // Обработчик страницы спиртуозности

    String navigator();

    String btnRelayConfig(); // HTML-код кнопки вызова настройки реле
    String btnControlConfig(); // HTML-код кнопки вызова настройки кнопок и пульта ДУ
    String btnScheduleConfig(); // HTML-код кнопки вызова настройки расписания
    String btnSemMining(); // HTML-код кнопки вызова спеиртуозности


private:
    void switchRelay(int8_t id, bool on); // Процедура включения/выключения реле
    void toggleRelay(int8_t id); // Процедура переключения реле

    bool debounceRead(int8_t id, uint32_t debounceTime); // Чтение кнопки с подавлением дребезга

    uint16_t readScheduleConfig(uint16_t offset); // Чтение из EEPROM порции параметров расписания
    uint16_t writeScheduleConfig(uint16_t offset); // Запись в EEPROM порции параметров расписания

    int8_t relayPin[maxRelays]; // Пины, к которым подключены реле (-1 - не подключено)
    bool relayLevel[maxRelays]; // Уровни срабатывания реле
    bool relayOnBoot[maxRelays]; // Состояние реле при старте модуля
    uint32_t relayAutoRelease[maxRelays]; // Значения задержки реле в миллисекундах до автоотключения (0 - нет автоотключения)
    uint32_t autoRelease[maxRelays]; // Значение в миллисекундах для сравнения с millis(), когда реле должно отключиться автоматически (0 - нет автоотключения)

    int8_t btnPin[maxRelays]; // Пины, к которым подключены кнопки (-1 - не подключено)
    uint8_t btnLevel[maxRelays]; // Уровни на цифровом входе при замыкании кнопки (младший бит) и признак фиксируемого выключателя (старший бит)
    uint32_t debounceTime[maxRelays]; // Длительность в миллисекундах для подавления дребезга (0 - не используется, например для сенсорных кнопок)

    Schedule schedule[maxSchedules]; // Массив расписания событий
    uint8_t scheduleRelay[maxSchedules]; // Какой канал реле и что с ним делать по срабатыванию события (6 младших бит - номер канала реле, 2 старших бита - вкл/выкл/перекл)
};

void ESPSemControll::setupExtra() {
    ESPWebBase::setupExtra();
    Wire.begin(SDA_PIN, SCL_PIN);   //Map Ic2 bus pins
    Wire.setClock(100000);

#ifdef USEDISPLAY
    setupDisplay();
#endif

    measureCycleTime = getTime();

#ifdef USEDS
    setupDS();
#endif

#ifdef USEBM180
    setupBMP180();
#endif

    for (int8_t i = 0; i < maxRelays; i++) {
        if (relayPin[i] != -1) {
            digitalWrite(relayPin[i], relayLevel[i] == relayOnBoot[i]);
            pinMode(relayPin[i], OUTPUT);
        }

        autoRelease[i] = 0;

        if (btnPin[i] != -1)
            pinMode(btnPin[i], (btnLevel[i] & 0x01) != 0 ? INPUT : INPUT_PULLUP);
    }

}

void ESPSemControll::loopExtra() {
    ESPWebBase::loopExtra();

    for (int8_t i = 0; i < maxRelays; i++) {
        if ((relayPin[i] != -1) && autoRelease[i] && ((int32_t) (millis() - autoRelease[i]) >= 0)) {
            switchRelay(i, false);
            autoRelease[i] = 0;
        }

        if (btnPin[i] != -1) {
            static bool btnLast[maxRelays];
            bool btnPressed = debounceRead(i, debounceTime[i]);

            if (btnPressed != btnLast[i]) {
                if ((btnLevel[i] & 0x80) != 0) { // Switch
                    switchRelay(i, btnPressed);
                } else { // Button
                    if (btnPressed)
                        toggleRelay(i);
                }
#ifndef NOSERIAL
                Serial.print(F("Button["));
                Serial.print(i + 1);
                Serial.print(F("] "));
                Serial.println(btnPressed ? F("pressed") : F("released"));
#endif
                btnLast[i] = btnPressed;
            }
        }
    }

    uint32_t now = getTime();
    if (now) {
        for (int8_t i = 0; i < maxSchedules; i++) {
            if ((schedule[i].period() != Schedule::NONE) && ((scheduleRelay[i] & 0x3F) < maxRelays)) {
                if (schedule[i].check(now)) {
                    if ((scheduleRelay[i] & 0x80) != 0) // toggle bit is set
                        toggleRelay(scheduleRelay[i] & 0x3F);
                    else
                        switchRelay(scheduleRelay[i] & 0x3F, (scheduleRelay[i] >> 6) & 0x01);
#ifndef NOSERIAL
                    Serial.print(dateTimeToStr(now));
                    Serial.print(F(" Schedule \""));
                    Serial.print(schedule[i]);
                    Serial.print(F("\" turned relay #"));
                    Serial.print((scheduleRelay[i] & 0x3F) + 1);
                    if ((scheduleRelay[i] & 0x80) != 0)
                        Serial.println(F(" opposite"));
                    else
                        Serial.println((scheduleRelay[i] & 0x40) != 0 ? F(" on") : F(" off"));
#endif
                }
            }
        }
    }
    if ((now - measureCycleTime) >= measureTime) {
        measureDS();
        measureCycleTime = now;
        Serial.println("BMP180 is = " + String(bmp180isGood));
        if (dsMeasureIndex == 0) {
            sendJsonData();
        }
    }
    if ((now - displayCycleTime) >= displayUpdateTime) {
        if (!displaySetupMark) {
            printSetupDisplay();
            displaySetupMark = true;
        } else {
            outputToDisplay();
        }
        displayCycleTime = now;
    }
}


#ifdef USEDISPLAY

void ESPSemControll::setupDisplay() {
    displayCycleTime = getTime();
    lcd.begin(16, 2);
    lcd.setBacklight(255);
    lcd.clear();
    lcd.print("-=SEM  MACHINE=-");
    lcd.setCursor(6, 1);
    lcd.print("V");
    lcd.print(SWver);
}

void ESPSemControll::printSetupDisplay() {
    lcd.clear();
    lcd.home();
    lcd.print(" DS q-ty=");
    lcd.print(dsQuantity);
}

void ESPSemControll::outputToDisplay() {
    lcd.clear();
    lcd.home();
    lcd.setCursor(0, 0);
    if (dsQuantity != 0) {
        for (int i = 0; i < dsQuantity; ++i) {
            lcd.print("T");
            lcd.print(String(i));
            lcd.print("=");
            lcd.print(String(tempDS[i]));
            if (i==1){
                lcd.setCursor(0, 1);
            }
        }
    } else {
        lcd.print("DS not found");
    };
    if (bmp180isGood) {
        lcd.print("Pa=");
        lcd.print(bmpGetPressure()*0.00750062,0);
        lcd.print("mmHg");
    } else {
        lcd.print("BMP180 not found");
    }
}

#endif

#ifdef USEBM180

void ESPSemControll::setupBMP180() {
    if (BMP180.begin()) {
        bmp180isGood = true;
    } else {
        Serial.println("Could not find a valid BMP180 sensor, check wiring!");
        bmp180isGood = false;
    }
    Serial.println("BMP180 is = " + String(bmp180isGood));
}

float ESPSemControll::bmpGetTemperature() {
    if (bmp180isGood) {
        return BMP180.readTemperature();
    }
    return 0;
}

int32_t ESPSemControll::bmpGetPressure() {
    if (bmp180isGood) {
        return BMP180.readPressure();
    }
    return 0;
}

#endif

void ESPSemControll::sendJsonData() {
    String data;
    data += F("{\"resource\":[");
    data += charOpenBrace;
    data += jsonData();
    data += charCloseBrace;
    data += F("]}");
    restPost(data);
}

void ESPSemControll::restPost(String message) {
    //String message = F("{\"resource\": [{\"swver\":\"0.0\",\"uid\":\"139699914584001\",\"temp1\":0.00,\"temp2\":0.00,\"temp3\":0.00,\"tempatm\":0.00,\"pressatm\":0.00}]}");
    HTTPClient http;
    http.begin("http://www.durnev.ml:10080/api/v2/mysqlsem/_table/colldata");
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Accept", "application/json");
    http.addHeader("X-DreamFactory-Api-Key", "990e0634e5fe0438df12928f6bf24be795130427d06b2ab9f93c5497b53968e9");
    int httpCode = http.POST(message);
    if (httpCode > 0) {
        Serial.printf("[HTTP] POST... code: %d\n", httpCode);
        if (httpCode == HTTP_CODE_OK) {
            String payload = http.getString();
            Serial.println(payload);
        }
    } else {
        Serial.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }
    http.end();
}

#ifdef USEDS

void ESPSemControll::setupDS() {
    ds.begin();
    ds.setResolution(12);
    dsQuantity = ds.getDeviceCount();
    Serial.println(dsQuantity);
    dsMeasureIndex = 0;
}

void ESPSemControll::measureDS() {
    ds.requestTemperaturesByIndex(dsMeasureIndex);
    tempDS[dsMeasureIndex] = ds.getTempCByIndex(dsMeasureIndex);
    Serial.print(String(dsMeasureIndex) + "=");
    Serial.println(String(tempDS[dsMeasureIndex]));
    if (dsMeasureIndex == (dsQuantity - 1)) {
        dsMeasureIndex = 0;
    } else {
        dsMeasureIndex++;
    }
}

#endif

uint16_t ESPSemControll::readConfig() {
    uint16_t offset = ESPWebBase::readConfig();

    if (offset) {
        uint16_t start = offset;
        int8_t i;

        for (i = 0; i < maxRelays; i++) {
            EEPROM.get(offset, relayPin[i]);
            offset += sizeof(relayPin[i]);
            EEPROM.get(offset, relayLevel[i]);
            offset += sizeof(relayLevel[i]);
            EEPROM.get(offset, relayOnBoot[i]);
            offset += sizeof(relayOnBoot[i]);
            EEPROM.get(offset, relayAutoRelease[i]);
            offset += sizeof(relayAutoRelease[i]);
            EEPROM.get(offset, btnPin[i]);
            offset += sizeof(btnPin[i]);
            EEPROM.get(offset, btnLevel[i]);
            offset += sizeof(btnLevel[i]);
            EEPROM.get(offset, debounceTime[i]);
            offset += sizeof(debounceTime[i]);
        }
        offset = readScheduleConfig(offset);

        uint8_t crc = ESPWebBase::crc8EEPROM(start, offset);
        if (EEPROM.read(offset++) != crc) {
#ifndef NOSERIAL
            Serial.println(F("CRC mismatch! Use default relay parameters."));
#endif
            defaultConfig(2);
        }
    }
    return offset;
}

uint16_t ESPSemControll::writeConfig(bool commit) {
    uint16_t offset = ESPWebBase::writeConfig(false);
    uint16_t start = offset;
    int8_t i;

    for (i = 0; i < maxRelays; i++) {
        EEPROM.put(offset, relayPin[i]);
        offset += sizeof(relayPin[i]);
        EEPROM.put(offset, relayLevel[i]);
        offset += sizeof(relayLevel[i]);
        EEPROM.put(offset, relayOnBoot[i]);
        offset += sizeof(relayOnBoot[i]);
        EEPROM.put(offset, relayAutoRelease[i]);
        offset += sizeof(relayAutoRelease[i]);
        EEPROM.put(offset, btnPin[i]);
        offset += sizeof(btnPin[i]);
        EEPROM.put(offset, btnLevel[i]);
        offset += sizeof(btnLevel[i]);
        EEPROM.put(offset, debounceTime[i]);
        offset += sizeof(debounceTime[i]);
    }
    offset = writeScheduleConfig(offset);
    uint8_t crc = ESPWebBase::crc8EEPROM(start, offset);
    EEPROM.write(offset++, crc);
    if (commit)
        commitConfig();

    return offset;
}

void ESPSemControll::defaultConfig(uint8_t level) {
    int8_t i;

    if (level < 2) {
        ESPWebBase::defaultConfig(level);
        if (level < 1)
            _ssid = FPSTR(overSSID);
    }

    if (level < 3) {
        for (i = 0; i < maxRelays; i++) {
            relayPin[i] = defRelayPin;
            relayLevel[i] = defRelayLevel;
            relayOnBoot[i] = defRelayOnBoot;
            relayAutoRelease[i] = defRelayAutoRelease;
            btnPin[i] = defBtnPin;
            btnLevel[i] = defBtnLevel;
            debounceTime[i] = defDebounceTime;
        }
        for (i = 0; i < maxSchedules; i++) {
            schedule[i].clear();
            scheduleRelay[i] = 0;
        }

    }
}

bool ESPSemControll::setConfigParam(const String &name, const String &value) {
    if (!ESPWebBase::setConfigParam(name, value)) {
        int8_t id = name.length();

        while ((id > 0) && ((name[id - 1] >= '0') && (name[id - 1] <= '9')))
            id--;
        if (id < name.length()) { // Name ends with digits
            id = name.substring(id).toInt();
            if ((id < 0) || (id >= maxRelays)) {
#ifndef NOSERIAL
                Serial.println(F("Wrong relay index!"));
#endif
                return false;
            }
            if (name.startsWith(FPSTR(paramGPIO)))
                relayPin[id] = constrain(value.toInt(), -1, 16);
            else if (name.startsWith(FPSTR(paramLevel)))
                relayLevel[id] = constrain(value.toInt(), 0, 1);
            else if (name.startsWith(FPSTR(paramOnBoot)))
                relayOnBoot[id] = constrain(value.toInt(), 0, 1);
            else if (name.startsWith(FPSTR(paramAutoRelease)))
                relayAutoRelease[id] = _max(0, value.toInt());
            else if (name.startsWith(FPSTR(paramBtnGPIO)))
                btnPin[id] = constrain(value.toInt(), -1, 16);
            else if (name.startsWith(FPSTR(paramBtnLevel)))
                btnLevel[id] = value.toInt() & 0x01;
            else if (name.startsWith(FPSTR(paramBtnSwitch)))
                btnLevel[id] |= (value.toInt() & 0x80);
            else if (name.startsWith(FPSTR(paramDebounce)))
                debounceTime[id] = _max(0, value.toInt());
            else
                return false;
        } else {
            return false;
        }
    }

    return true;
}

void ESPSemControll::setupHttpServer() {
    ESPWebBase::setupHttpServer();
    httpServer->on(String(FPSTR(pathRelay)).c_str(), std::bind(&ESPSemControll::handleRelayConfig, this));
    httpServer->on(String(FPSTR(pathControl)).c_str(), std::bind(&ESPSemControll::handleControlConfig, this));
    httpServer->on(String(FPSTR(pathSwitch)).c_str(), std::bind(&ESPSemControll::handleRelaySwitch, this));
    httpServer->on(String(FPSTR(pathSchedule)).c_str(), std::bind(&ESPSemControll::handleScheduleConfig, this));
    httpServer->on(String(FPSTR(pathGetSchedule)).c_str(), std::bind(&ESPSemControll::handleGetSchedule, this));
    httpServer->on(String(FPSTR(pathSetSchedule)).c_str(), std::bind(&ESPSemControll::handleSetSchedule, this));
    httpServer->on(String(FPSTR(pathSemMinning)).c_str(), std::bind(&ESPSemControll::handleSemMining, this));
}

void ESPSemControll::handleRootPage() {
    String style = F(".checkbox {\n"
                             "vertical-align:top;\n"
                             "margin:0 3px 0 0;\n"
                             "width:17px;\n"
                             "height:17px;\n"
                             "}\n"
                             ".checkbox + label {\n"
                             "cursor:pointer;\n"
                             "}\n"
                             ".checkbox:not(checked) {\n"
                             "position:absolute;\n"
                             "opacity:0;\n"
                             "}\n"
                             ".checkbox:not(checked) + label {\n"
                             "position:relative;\n"
                             "padding:0 0 0 60px;\n"
                             "}\n"
                             ".checkbox:not(checked) + label:before {\n"
                             "content:'';\n"
                             "position:absolute;\n"
                             "top:-4px;\n"
                             "left:0;\n"
                             "width:50px;\n"
                             "height:26px;\n"
                             "border-radius:13px;\n"
                             "background:#CDD1DA;\n"
                             "box-shadow:inset 0 2px 3px rgba(0,0,0,.2);\n"
                             "}\n"
                             ".checkbox:not(checked) + label:after {\n"
                             "content:'';\n"
                             "position:absolute;\n"
                             "top:-2px;\n"
                             "left:2px;\n"
                             "width:22px;\n"
                             "height:22px;\n"
                             "border-radius:10px;\n"
                             "background:#FFF;\n"
                             "box-shadow:0 2px 5px rgba(0,0,0,.3);\n"
                             "transition:all .2s;\n"
                             "}\n"
                             ".checkbox:checked + label:before {\n"
                             "background:#9FD468;\n"
                             "}\n"
                             ".checkbox:checked + label:after {\n"
                             "left:26px;\n"
                             "}\n");

    String script = FPSTR(getXmlHttpRequest);
    script += F("function openUrl(url) {\n"
                        "var request = getXmlHttpRequest();\n"
                        "request.open('GET', url, false);\n"
                        "request.send(null);\n"
                        "}\n"
                        "function refreshData() {\n"
                        "var request = getXmlHttpRequest();\n"
                        "request.open('GET', '");
    script += FPSTR(pathData);
    script += F("?dummy=' + Date.now(), true);\n"
                        "request.onreadystatechange = function() {\n"
                        "if (request.readyState == 4) {\n"
                        "var data = JSON.parse(request.responseText);\n");
    script += FPSTR(getElementById);
    script += FPSTR(jsonFreeHeap);
    script += F("').innerHTML = data.");
    script += FPSTR(jsonFreeHeap);
    script += F(";\n");
    script += FPSTR(getElementById);
    script += FPSTR(jsonUptime);
    script += F("').innerHTML = data.");
    script += FPSTR(jsonUptime);
    script += F(";\n");
#ifdef USEDS
    if (dsQuantity != 0) {
        for (int i = 0; i < dsQuantity; ++i) {
            script += FPSTR(getElementById);
            script += FPSTR(jsonTemp);
            script += String(i);
            script += F("').innerHTML = data.");
            script += FPSTR(jsonTemp);
            script += String(i);
            script += F(";\n");
        }
    }
#endif
#ifdef USEBM180
    if (bmp180isGood) {
        script += FPSTR(getElementById);
        script += FPSTR(jsonTempatm);
        script += F("').innerHTML = data.");
        script += FPSTR(jsonTempatm);
        script += F(";\n");
        script += FPSTR(getElementById);
        script += FPSTR(jsonPressatm);
        script += F("').innerHTML = (data.");
        script += FPSTR(jsonPressatm);
        script += F("*0.00750062).toFixed(1);\n"); // Для перевода в mmHg
    }
#endif
/*    for (int8_t id = 0; id < maxRelays; id++) {
        script += FPSTR(getElementById);
        script += FPSTR(jsonRelay);
        script += String(id);
        script += F("').checked = data.");
        script += FPSTR(jsonRelay);
        script += String(id);
        script += F(";\n"
                            "if (data.");
        script += FPSTR(jsonRelayAutoRelease);
        script += String(id);
        script += F(" > 0)\n");
        script += FPSTR(getElementById);
        script += FPSTR(jsonRelayAutoRelease);
        script += String(id);
        script += F("').innerHTML = \" (\" + data.");
        script += FPSTR(jsonRelayAutoRelease);
        script += String(id);
        script += F(" + \" sec. to auto off)\";\n"
                            "else\n");
        script += FPSTR(getElementById);
        script += FPSTR(jsonRelayAutoRelease);
        script += String(id);
        script += F("').innerHTML = \"\";\n");
    }*/
    script += F("}\n"
                        "}\n"
                        "request.send(null);\n"
                        "}\n"
                        "setInterval(refreshData, 5000);\n");

    String page = ESPWebBase::webPageStart(F("ESP Relay"));
    page += ESPWebBase::webPageStyle(style);
    page += ESPWebBase::webPageScript(script);
    page += ESPWebBase::webPageBody();
    page += F("<h3>ESP Relay</h3>\n"
                      "<p>\n");
    page += F("Heap free size: <span id=\"");
    page += FPSTR(jsonFreeHeap);
    page += F("\">0</span> bytes<br/>\n"
                      "Uptime: <span id=\"");
    page += FPSTR(jsonUptime);
    page += F("\">0</span> seconds<br/>\n");
#ifdef USEDS
    if (dsQuantity != 0) {
        for (int i = 0; i < dsQuantity; ++i) {
            page += F("Temp");
            page += String(i);
            page += F(": <span id=\"");
            page += FPSTR(jsonTemp);
            page += String(i);
            page += F("\">0</span> C<br/>\n");
        }
    }
#endif
#ifdef USEBM180
    if (bmp180isGood) {
        page += F("Temp ATM");
        page += F(": <span id=\"");
        page += FPSTR(jsonTempatm);
        page += F("\">0</span> C<br/>\n");
        page += F("Press ATM");
        page += F(": <span id=\"");
        page += FPSTR(jsonPressatm);
        page += F("\">0</span> mmHg<br/>\n");
    }
#endif
    page += F("</p>\n");
    /*for (int8_t id = 0; id < maxRelays; id++) {
        page += F("<input type=\"checkbox\" class=\"checkbox\" id=\"");
        page += FPSTR(jsonRelay);
        page += String(id);
        page += F("\" onchange=\"openUrl('");
        page += FPSTR(pathSwitch);
        page += F("?id=");
        page += String(id);
        page += F("&on=' + this.checked + '&dummy=' + Date.now());\" ");
        if (relayPin[id] == -1)
            page += F("disabled ");
        else {
            if (digitalRead(relayPin[id]) == relayLevel[id])
                page += FPSTR(extraChecked);
        }
        page += F(">\n"
                          "<label for=\"");
        page += FPSTR(jsonRelay);
        page += String(id);
        page += F("\">Relay ");
        page += String(id + 1);
        page += F("</label>\n"
                          "<span id=\"");
        page += FPSTR(jsonRelayAutoRelease);
        page += String(id);
        page += F("\"></span>\n"
                          "<p>\n");
    }*/
    page += navigator();
    page += ESPWebBase::webPageEnd();

    httpServer->send(200, FPSTR(textHtml), page);
}

String ESPSemControll::jsonData() {
    String result = ESPWebBase::jsonData();
    result += F(",\"");
    result += FPSTR(jsonSWver);
    result += F("\":\"");
    result += SWver;
    result += F("\",\"");
    result += FPSTR(jsonUid);
    result += F("\":\"");
    result += String(ESP.getFlashChipId());
    result += String(ESP.getChipId());
    result += F("\"");
#ifdef USEDS
    if (dsQuantity != 0) {
        for (int i = 0; i < dsQuantity; ++i) {
            result += F(",\"");
            result += FPSTR(jsonTemp);
            result += String(i);
            result += F("\":");
            result += String(tempDS[i]);
        }
    }
#endif
#ifdef USEBM180
    result += F(",\"");
    result += FPSTR(jsonTempatm);
    result += F("\":");
    result += String(bmpGetTemperature());
    result += F(",\"");
    result += FPSTR(jsonPressatm);
    result += F("\":");
    result += String(bmpGetPressure());
#endif
    for (int8_t id = 0; id < maxRelays; id++) {
        result += F(",\"");
        result += FPSTR(jsonRelay);
        result += String(id);
        result += F("\":");
        if ((relayPin[id] != -1) && (digitalRead(relayPin[id]) == relayLevel[id]))
            result += FPSTR(bools[1]);
        else
            result += FPSTR(bools[0]);
        result += F(",\"");
        result += FPSTR(jsonRelayAutoRelease);
        result += String(id);
        result += F("\":");
        if (autoRelease[id])
            result += String((int32_t) (autoRelease[id] - millis()) / 1000);
        else
            result += '0';
    }
    return result;
}


void ESPSemControll::handleRelayConfig() {
    String page = ESPWebBase::webPageStart(F("Relay Setup"));
    page += ESPWebBase::webPageBody();
    page += F("<form name=\"relay\" method=\"GET\" action=\"");
    page += FPSTR(pathStore);
    page += F("\">\n"
                      "<table><caption><h3>Relay Setup</h3></caption>\n"
                      "<tr><th>#</th><th>GPIO</th><th>Level</th><th>On boot</th><th>Auto release<sup>*</sup></th></tr>\n");

    for (int8_t id = 0; id < maxRelays; id++) {
        page += F("<tr><td>");
        page += String(id + 1);
        page += F("</td><td><select name=\"");
        page += FPSTR(paramGPIO);
        page += String(id);
        page += F("\" size=1>\n");

        for (byte i = 0; i < sizeof(gpios) / sizeof(gpios[0]); i++) {
            int8_t gpio = pgm_read_byte(gpios + i);
            page += F("<option value=\"");
            page += String(gpio);
            page += charQuote;
            if (relayPin[id] == gpio)
                page += F(" selected");
            page += charGreater;
            if (gpio == -1)
                page += FPSTR(strNone);
            else
                page += String(gpio);
            page += F("</option>\n");
        }
        page += F("</select></td>\n"
                          "<td><input type=\"radio\" name=\"");
        page += FPSTR(paramLevel);
        page += String(id);
        page += F("\" value=\"1\" ");
        if (relayLevel[id])
            page += FPSTR(extraChecked);
        page += F(">HIGH\n"
                          "<input type=\"radio\" name=\"");
        page += FPSTR(paramLevel);
        page += String(id);
        page += F("\" value=\"0\" ");
        if (!relayLevel[id])
            page += FPSTR(extraChecked);
        page += F(">LOW</td>\n"
                          "<td><input type=\"radio\" name=\"");
        page += FPSTR(paramOnBoot);
        page += String(id);
        page += F("\" value=\"1\" ");
        if (relayOnBoot[id])
            page += FPSTR(extraChecked);
        page += F(">On\n"
                          "<input type=\"radio\" name=\"");
        page += FPSTR(paramOnBoot);
        page += String(id);
        page += F("\" value=\"0\" ");
        if (!relayOnBoot[id])
            page += FPSTR(extraChecked);
        page += F(">Off</td>\n"
                          "<td><input type=\"text\" name=\"");
        page += FPSTR(paramAutoRelease);
        page += String(id);
        page += F("\" value=\"");
        page += String(relayAutoRelease[id]);
        page += F("\" maxlength=10></td>\n"
                          "</tr>\n");
    }
    page += F("</table>\n"
                      "<sup>*</sup> time in milliseconds to auto off relay (0 to disable this feature)\n"
                      "<p>\n");

    page += ESPWebBase::tagInput(FPSTR(typeSubmit), strEmpty, F("Save"));
    page += charLF;
    page += btnBack();
    page += ESPWebBase::tagInput(FPSTR(typeHidden), FPSTR(paramReboot), "1");
    page += F("\n"
                      "</form>\n");
    page += ESPWebBase::webPageEnd();

    httpServer->send(200, FPSTR(textHtml), page);
}

void ESPSemControll::handleControlConfig() {

    String page = ESPWebBase::webPageStart(F("Control Setup"));
    page += ESPWebBase::webPageBody();
    page += F("<form name=\"control\" method=\"GET\" action=\"");
    page += FPSTR(pathStore);
    page += F("\">\n"
                      "<table><caption><h3>Button Setup</h3></caption>\n"
                      "<tr><th>#</th><th>GPIO</th><th>Level</th><th>Switch</th><th>Debounce</th></tr>\n");

    for (int8_t id = 0; id < maxRelays; id++) {
        page += F("<tr><td>");
        page += String(id + 1);
        page += F("</td><td><select name=\"");
        page += FPSTR(paramBtnGPIO);
        page += String(id);
        page += F("\" size=1>\n");

        for (byte i = 0; i < sizeof(gpios) / sizeof(gpios[0]); i++) {
            int8_t gpio = pgm_read_byte(gpios + i);
            page += F("<option value=\"");
            page += String(gpio);
            page += charQuote;
            if (btnPin[id] == gpio)
                page += F(" selected");
            page += charGreater;
            if (gpio == -1)
                page += FPSTR(strNone);
            else
                page += String(gpio);
            page += F("</option>\n");
        }
        page += F("</select></td>\n"
                          "<td><input type=\"radio\" name=\"");
        page += FPSTR(paramBtnLevel);
        page += String(id);
        page += F("\" value=\"1\" ");
        if (btnLevel[id] & 0x01)
            page += FPSTR(extraChecked);
        page += F(">HIGH\n"
                          "<input type=\"radio\" name=\"");
        page += FPSTR(paramBtnLevel);
        page += String(id);
        page += F("\" value=\"0\" ");
        if (!(btnLevel[id] & 0x01))
            page += FPSTR(extraChecked);
        page += F(">LOW</td>\n"
                          "<td><center><input type=\"checkbox\" name=\"");
        page += FPSTR(paramBtnSwitch);
        page += String(id);
        page += F("\" value=\"128\" ");
        if (btnLevel[id] & 0x80)
            page += FPSTR(extraChecked);
        page += F("></center></td>\n"
                          "<td><input type=\"text\" name=\"");
        page += FPSTR(paramDebounce);
        page += String(id);
        page += F("\" value=\"");
        page += String(debounceTime[id]);
        page += F("\" maxlength=10></td>\n"
                          "</tr>\n");
    }
    page += F("</table>\n"
                      "<p>\n");
    page += ESPWebBase::tagInput(FPSTR(typeSubmit), strEmpty, F("Save"));
    page += charLF;
    page += btnBack();
    page += ESPWebBase::tagInput(FPSTR(typeHidden), FPSTR(paramReboot), "1");
    page += F("\n"
                      "</form>\n");
    page += ESPWebBase::webPageEnd();

    httpServer->send(200, FPSTR(textHtml), page);
}

void ESPSemControll::handleRelaySwitch() {
    String id = httpServer->arg("id");
    String on = httpServer->arg("on");

#ifndef NOSERIAL
    Serial.print(F("/switch("));
    Serial.print(id);
    Serial.print(F(", "));
    Serial.print(on);
    Serial.println(F(")"));
#endif

    switchRelay(id.toInt(), on == "true");

    httpServer->send(200, FPSTR(textHtml), strEmpty);
}

void ESPSemControll::handleScheduleConfig() {
    int8_t i;

    String style = F(".modal {\n"
                             "display: none;\n"
                             "position: fixed;\n"
                             "z-index: 1;\n"
                             "left: 0;\n"
                             "top: 0;\n"
                             "width: 100%;\n"
                             "height: 100%;\n"
                             "overflow: auto;\n"
                             "background-color: rgb(0,0,0);\n"
                             "background-color: rgba(0,0,0,0.4);\n"
                             "}\n"
                             ".modal-content {\n"
                             "background-color: #fefefe;\n"
                             "margin: 15% auto;\n"
                             "padding: 20px;\n"
                             "border: 1px solid #888;\n"
                             "width: 400px;\n"
                             "}\n"
                             ".close {\n"
                             "color: #aaa;\n"
                             "float: right;\n"
                             "font-size: 28px;\n"
                             "font-weight: bold;\n"
                             "}\n"
                             ".close:hover,\n"
                             ".close:focus {\n"
                             "color: black;\n"
                             "text-decoration: none;\n"
                             "cursor: pointer;\n"
                             "}\n"
                             ".hidden {\n"
                             "display: none;\n"
                             "}\n");

    String script = FPSTR(getXmlHttpRequest);
    script += F("function loadData(form) {\n"
                        "var request = getXmlHttpRequest();\n"
                        "request.open('GET', '");
    script += FPSTR(pathGetSchedule);
    script += F("?id=' + form.id.value + '&dummy=' + Date.now(), false);\n"
                        "request.send(null);\n"
                        "if (request.status == 200) {\n"
                        "var data = JSON.parse(request.responseText);\n"
                        "form.");
    script += FPSTR(paramSchedulePeriod);
    script += F(".value = data.");
    script += FPSTR(jsonSchedulePeriod);
    script += F(";\n"
                        "form.");
    script += FPSTR(paramScheduleHour);
    script += F(".value = data.");
    script += FPSTR(jsonScheduleHour);
    script += F(";\n"
                        "form.");
    script += FPSTR(paramScheduleMinute);
    script += F(".value = data.");
    script += FPSTR(jsonScheduleMinute);
    script += F(";\n"
                        "form.");
    script += FPSTR(paramScheduleSecond);
    script += F(".value = data.");
    script += FPSTR(jsonScheduleSecond);
    script += F(";\n"
                        "if (data.");
    script += FPSTR(jsonSchedulePeriod);
    script += F(" == 3) {\n"
                        "var weekdaysdiv = document.getElementById('weekdays');\n"
                        "var elements = weekdaysdiv.getElementsByTagName('input');\n"
                        "for (var i = 0; i < elements.length; i++) {\n"
                        "if (elements[i].type == 'checkbox') {\n"
                        "if ((data.");
    script += FPSTR(jsonScheduleWeekdays);
    script += F(" & elements[i].value) != 0)\n"
                        "elements[i].checked = true;\n"
                        "else\n"
                        "elements[i].checked = false;\n"
                        "}\n"
                        "}\n"
                        "form.");
    script += FPSTR(paramScheduleWeekdays);
    script += F(".value = data.");
    script += FPSTR(jsonScheduleWeekdays);
    script += F(";\n"
                        "} else {\n"
                        "form.");
    script += FPSTR(paramScheduleWeekdays);
    script += F(".value = 0;\n"
                        "form.");
    script += FPSTR(paramScheduleDay);
    script += F(".value = data.");
    script += FPSTR(jsonScheduleDay);
    script += F(";\n"
                        "form.");
    script += FPSTR(paramScheduleMonth);
    script += F(".value = data.");
    script += FPSTR(jsonScheduleMonth);
    script += F(";\n"
                        "form.");
    script += FPSTR(paramScheduleYear);
    script += F(".value = data.");
    script += FPSTR(jsonScheduleYear);
    script += F(";\n"
                        "}\n"
                        "form.");
    script += FPSTR(paramScheduleRelay);
    script += F(".value = data.");
    script += FPSTR(jsonScheduleRelay);
    script += F(";\n"
                        "var radios = document.getElementsByName('");
    script += FPSTR(paramScheduleTurn);
    script += F("');\n"
                        "for (var i = 0; i < radios.length; i++) {\n"
                        "if (radios[i].value == data.");
    script += FPSTR(jsonScheduleTurn);
    script += F(") radios[i].checked = true;\n"
                        "}\n"
                        "}\n"
                        "}\n"
                        "function openForm(form, id) {\n"
                        "form.id.value = id;\n"
                        "loadData(form);\n"
                        "form.");
    script += FPSTR(paramSchedulePeriod);
    script += F(".onchange();\n"
                        "document.getElementById(\"form\").style.display = \"block\";\n"
                        "}\n"
                        "function closeForm() {\n"
                        "document.getElementById(\"form\").style.display = \"none\";\n"
                        "}\n"
                        "function checkNumber(field, minvalue, maxvalue) {\n"
                        "var val = parseInt(field.value);\n"
                        "if (isNaN(val) || (val < minvalue) || (val > maxvalue))\n"
                        "return false;\n"
                        "return true;\n"
                        "}\n"
                        "function validateForm(form) {\n"
                        "if (form.");
    script += FPSTR(paramSchedulePeriod);
    script += F(".value > 0) {\n"
                        "if ((form.");
    script += FPSTR(paramSchedulePeriod);
    script += F(".value > 2) && (! checkNumber(form.");
    script += FPSTR(paramScheduleHour);
    script += F(", 0, 23))) {\n"
                        "alert(\"Wrong hour!\");\n"
                        "form.");
    script += FPSTR(paramScheduleHour);
    script += F(".focus();\n"
                        "return false;\n"
                        "}\n"
                        "if ((form.");
    script += FPSTR(paramSchedulePeriod);
    script += F(".value > 1) && (! checkNumber(form.");
    script += FPSTR(paramScheduleMinute);
    script += F(", 0, 59))) {\n"
                        "alert(\"Wrong minute!\");\n"
                        "form.");
    script += FPSTR(paramScheduleMinute);
    script += F(".focus();\n"
                        "return false;\n"
                        "}\n"
                        "if (! checkNumber(form.");
    script += FPSTR(paramScheduleSecond);
    script += F(", 0, 59)) {\n"
                        "alert(\"Wrong second!\");\n"
                        "form.");
    script += FPSTR(paramScheduleSecond);
    script += F(".focus();\n"
                        "return false;\n"
                        "}\n"
                        "if ((form.");
    script += FPSTR(paramSchedulePeriod);
    script += F(".value == 3) && (form.");
    script += FPSTR(paramScheduleWeekdays);
    script += F(".value == 0)) {\n"
                        "alert(\"None of weekdays selected!\");\n"
                        "return false;\n"
                        "}\n"
                        "if ((form.");
    script += FPSTR(paramSchedulePeriod);
    script += F(".value >= 4) && (! checkNumber(form.");
    script += FPSTR(paramScheduleDay);
    script += F(", 1, ");
    script += String(Schedule::LASTDAYOFMONTH);
    script += F("))) {\n"
                        "alert(\"Wrong day!\");\n"
                        "form.");
    script += FPSTR(paramScheduleDay);
    script += F(".focus();\n"
                        "return false;\n"
                        "}\n"
                        "if ((form.");
    script += FPSTR(paramSchedulePeriod);
    script += F(".value >= 5) && (! checkNumber(form.");
    script += FPSTR(paramScheduleMonth);
    script += F(", 1, 12))) {\n"
                        "alert(\"Wrong month!\");\n"
                        "form.");
    script += FPSTR(paramScheduleMonth);
    script += F(".focus();\n"
                        "return false;\n"
                        "}\n"
                        "if ((form.");
    script += FPSTR(paramSchedulePeriod);
    script += F(".value == 6) && (! checkNumber(form.");
    script += FPSTR(paramScheduleYear);
    script += F(", 2017, 2099))) {\n"
                        "alert(\"Wrong year!\");\n"
                        "form.");
    script += FPSTR(paramScheduleYear);
    script += F(".focus();\n"
                        "return false;\n"
                        "}\n"
                        "if (! checkNumber(form.");
    script += FPSTR(paramScheduleRelay);
    script += F(", 0, ");
    script += String(maxRelays - 1);
    script += F(")) {\n"
                        "alert(\"Wrong relay id!\");\n"
                        "form.");
    script += FPSTR(paramScheduleRelay);
    script += F(".focus();\n"
                        "return false;\n"
                        "}\n"
                        "var radios = document.getElementsByName('");
    script += FPSTR(paramScheduleTurn);
    script += F("');\n"
                        "var checkedCount = 0;\n"
                        "for (var i = 0; i < radios.length; i++) {\n"
                        "if (radios[i].checked == true) checkedCount++;\n"
                        "}\n"
                        "if (checkedCount != 1) {\n"
                        "alert(\"Wrong relay turn!\");\n"
                        "return false;\n"
                        "}\n"
                        "}\n"
                        "return true;\n"
                        "}\n"
                        "function periodChanged(period) {\n"
                        "document.getElementById(\"time\").style.display = (period.value != 0) ? \"inline\" : \"none\";\n"
                        "document.getElementById(\"hh\").style.display = (period.value > 2) ? \"inline\" : \"none\";\n"
                        "document.getElementById(\"mm\").style.display = (period.value > 1) ? \"inline\" : \"none\";\n"
                        "document.getElementById(\"weekdays\").style.display = (period.value == 3) ? \"block\" : \"none\";\n"
                        "document.getElementById(\"date\").style.display = (period.value > 3) ? \"block\" : \"none\";\n"
                        "document.getElementById(\"month\").style.display = (period.value > 4) ? \"inline\" : \"none\";\n"
                        "document.getElementById(\"year\").style.display = (period.value == 6) ? \"inline\" : \"none\";\n"
                        "document.getElementById(\"relay\").style.display = (period.value != 0) ? \"block\" : \"none\";\n"
                        "}\n"
                        "function weekChanged(wd) {\n"
                        "var weekdays = document.form.");
    script += FPSTR(paramScheduleWeekdays);
    script += F(".value;\n"
                        "if (wd.checked == \"\") weekdays &= ~wd.value; else weekdays |= wd.value;\n"
                        "document.form.");
    script += FPSTR(paramScheduleWeekdays);
    script += F(".value = weekdays;\n"
                        "}\n");

    String page = ESPWebBase::webPageStart(F("Schedule Setup"));
    page += ESPWebBase::webPageStyle(style);
    page += ESPWebBase::webPageScript(script);
    page += ESPWebBase::webPageBody();
    page += F("<table><caption><h3>Schedule Setup</h3></caption>\n"
                      "<tr><th>#</th><th>Event</th><th>Next time</th><th>Relay</th></tr>\n");

    for (i = 0; i < maxSchedules; i++) {
        page += F("<tr><td><a href=\"#\" onclick=\"openForm(document.form, ");
        page += String(i);
        page += F(")\">");
        page += String(i + 1);
        page += F("</a></td><td>");
        page += schedule[i];
        page += F("</td><td>");
        page += schedule[i].nextTimeStr();
        page += F("</td><td>");
        if (schedule[i].period() != Schedule::NONE) {
            if ((scheduleRelay[i] & 0x3F) < maxRelays) {
                page += String((scheduleRelay[i] & 0x3F) + 1);
                if ((scheduleRelay[i] & 0x80) != 0)
                    page += F(" toggle");
                else
                    page += (scheduleRelay[i] & 0x40) != 0 ? F(" on") : F(" off");
            }
        }
        page += F("</td></tr>\n");
    }
    page += F("</table>\n"
                      "<p>\n"
                      "<i>Don't forget to save changes!</i>\n"
                      "<p>\n");

    page += ESPWebBase::tagInput(FPSTR(typeButton), strEmpty, F("Save"),
                                 String(F("onclick=\"location.href='")) + String(FPSTR(pathStore)) +
                                 String(F("?reboot=0'\"")));
    page += charLF;
    page += btnBack();
    page += ESPWebBase::tagInput(FPSTR(typeHidden), FPSTR(paramReboot), "0");
    page += F("\n"
                      "<div id=\"form\" class=\"modal\">\n"
                      "<div class=\"modal-content\">\n"
                      "<span class=\"close\" onclick=\"closeForm()\">&times;</span>\n"
                      "<form name=\"form\" method=\"GET\" action=\"");
    page += FPSTR(pathSetSchedule);
    page += F("\" onsubmit=\"if (validateForm(this)) closeForm(); else return false;\">\n"
                      "<input type=\"hidden\" name=\"id\" value=\"0\">\n"
                      "<select name=\"");
    page += FPSTR(paramSchedulePeriod);
    page += F("\" size=\"1\" onchange=\"periodChanged(this)\">\n"
                      "<option value=\"0\">Never!</option>\n"
                      "<option value=\"1\">Every minute</option>\n"
                      "<option value=\"2\">Every hour</option>\n"
                      "<option value=\"3\">Every week</option>\n"
                      "<option value=\"4\">Every month</option>\n"
                      "<option value=\"5\">Every year</option>\n"
                      "<option value=\"6\">Once</option>\n"
                      "</select>\n"
                      "<span id=\"time\" class=\"hidden\">at\n"
                      "<span id=\"hh\" class=\"hidden\">");
    page += ESPWebBase::tagInput(typeText, FPSTR(paramScheduleHour), "0", F("size=\"2\" maxlength=\"2\""));
    page += F("\n:</span>\n"
                      "<span id=\"mm\" class=\"hidden\">");
    page += ESPWebBase::tagInput(typeText, FPSTR(paramScheduleMinute), "0", F("size=\"2\" maxlength=\"2\""));
    page += F("\n:</span>\n");
    page += ESPWebBase::tagInput(typeText, FPSTR(paramScheduleSecond), "0", F("size=\"2\" maxlength=\"2\""));
    page += F("</span><br/>\n"
                      "<div id=\"weekdays\" class=\"hidden\">\n"
                      "<input type=\"hidden\" name=\"");
    page += FPSTR(paramScheduleWeekdays);
    page += F("\" value=\"0\">\n");

    for (i = 0; i < 7; i++) {
        page += F("<input type=\"checkbox\" value=\"");
        page += String(1 << i);
        page += F("\" onchange=\"weekChanged(this)\">");
        page += weekdayName(i);
        page += charLF;
    }
    page += F("</div>\n"
                      "<div id=\"date\" class=\"hidden\">\n"
                      "<select name=\"");
    page += FPSTR(paramScheduleDay);
    page += F("\" size=\"1\">\n");

    for (i = 1; i <= 31; i++) {
        page += F("<option value=\"");
        page += String(i);
        page += F("\">");
        page += String(i);
        page += F("</option>\n");
    }
    page += F("<option value=\"");
    page += String(Schedule::LASTDAYOFMONTH);
    page += F("\">Last</option>\n"
                      "</select>\n"
                      "day\n"
                      "<span id=\"month\" class=\"hidden\">of\n"
                      "<select name=\"");
    page += FPSTR(paramScheduleMonth);
    page += F("\" size=\"1\">\n");

    for (i = 1; i <= 12; i++) {
        page += F("<option value=\"");
        page += String(i);
        page += F("\">");
        page += monthName(i);
        page += F("</option>\n");
    }
    page += F("</select>\n"
                      "</span>\n"
                      "<span id=\"year\" class=\"hidden\">");
    page += ESPWebBase::tagInput(typeText, FPSTR(paramScheduleYear), "2017", F("size=\"4\" maxlength=\"4\""));
    page += F("</span>\n"
                      "</div>\n"
                      "<div id=\"relay\" class=\"hidden\">\n"
                      "<label>Relay #</label>\n"
                      "<select name=\"");
    page += FPSTR(paramScheduleRelay);
    page += F("\" size=\"1\">\n");

    for (i = 0; i < maxRelays; i++) {
        page += F("<option value=\"");
        page += String(i);
        page += F("\">");
        page += String(i + 1);
        page += F("</option>\n");
    }
    page += F("</select>\n"
                      "turn\n"
                      "<input type=\"radio\" name=\"");
    page += FPSTR(paramScheduleTurn);
    page += F("\" value=\"1\">ON\n"
                      "<input type=\"radio\" name=\"");
    page += FPSTR(paramScheduleTurn);
    page += F("\" value=\"0\">OFF\n"
                      "<input type=\"radio\" name=\"");
    page += FPSTR(paramScheduleTurn);
    page += F("\" value=\"2\">TOGGLE\n"
                      "</div>\n"
                      "<p>\n"
                      "<input type=\"submit\" value=\"Update\">\n"
                      "</form>\n"
                      "</div>\n"
                      "</div>\n");
    page += ESPWebBase::webPageEnd();

    httpServer->send(200, FPSTR(textHtml), page);
}

void ESPSemControll::handleGetSchedule() {
    int id = -1;

    if (httpServer->hasArg("id"))
        id = httpServer->arg("id").toInt();

    if ((id >= 0) && (id < maxSchedules)) {
        String page;

        page += charOpenBrace;
        page += charQuote;
        page += FPSTR(jsonSchedulePeriod);
        page += F("\":");
        page += String(schedule[id].period());
        page += F(",\"");
        page += FPSTR(jsonScheduleHour);
        page += F("\":");
        page += String(schedule[id].hour());
        page += F(",\"");
        page += FPSTR(jsonScheduleMinute);
        page += F("\":");
        page += String(schedule[id].minute());
        page += F(",\"");
        page += FPSTR(jsonScheduleSecond);
        page += F("\":");
        page += String(schedule[id].second());
        page += F(",\"");
        page += FPSTR(jsonScheduleWeekdays);
        page += F("\":");
        page += String(schedule[id].weekdays());
        page += F(",\"");
        page += FPSTR(jsonScheduleDay);
        page += F("\":");
        page += String(schedule[id].day());
        page += F(",\"");
        page += FPSTR(jsonScheduleMonth);
        page += F("\":");
        page += String(schedule[id].month());
        page += F(",\"");
        page += FPSTR(jsonScheduleYear);
        page += F("\":");
        page += String(schedule[id].year());
        page += F(",\"");
        page += FPSTR(jsonScheduleRelay);
        page += F("\":");
        page += String(scheduleRelay[id] & 0x3F);
        page += F(",\"");
        page += FPSTR(jsonScheduleTurn);
        page += F("\":");
        page += String((scheduleRelay[id] >> 6) & 0x03);
        page += charCloseBrace;

        httpServer->send(200, FPSTR(textJson), page);
    } else {
        httpServer->send(204, FPSTR(textJson), strEmpty); // No content
    }
}

void ESPSemControll::handleSetSchedule() {
    String argName, argValue;
    int8_t id = -1;
    Schedule::period_t period = Schedule::NONE;
    int8_t hour = -1;
    int8_t minute = -1;
    int8_t second = -1;
    uint8_t weekdays = 0;
    int8_t day = 0;
    int8_t month = 0;
    int16_t year = 0;
    int8_t relay = 0;
    uint8_t turn = 0;

#ifndef NOSERIAL
    Serial.print(F("/setschedule("));
#endif
    for (byte i = 0; i < httpServer->args(); i++) {
#ifndef NOSERIAL
        if (i)
            Serial.print(F(", "));
#endif
        argName = httpServer->argName(i);
        argValue = httpServer->arg(i);
#ifndef NOSERIAL
        Serial.print(argName);
        Serial.print(F("=\""));
        Serial.print(argValue);
        Serial.print(charQuote);
#endif
        if (argName.equals("id")) {
            id = argValue.toInt();
        } else if (argName.equals(FPSTR(paramSchedulePeriod))) {
            period = (Schedule::period_t) argValue.toInt();
        } else if (argName.equals(FPSTR(paramScheduleHour))) {
            hour = argValue.toInt();
        } else if (argName.equals(FPSTR(paramScheduleMinute))) {
            minute = argValue.toInt();
        } else if (argName.equals(FPSTR(paramScheduleSecond))) {
            second = argValue.toInt();
        } else if (argName.equals(FPSTR(paramScheduleWeekdays))) {
            weekdays = argValue.toInt();
        } else if (argName.equals(FPSTR(paramScheduleDay))) {
            day = argValue.toInt();
        } else if (argName.equals(FPSTR(paramScheduleMonth))) {
            month = argValue.toInt();
        } else if (argName.equals(FPSTR(paramScheduleYear))) {
            year = argValue.toInt();
        } else if (argName.equals(FPSTR(paramScheduleRelay))) {
            relay = argValue.toInt();
        } else if (argName.equals(FPSTR(paramScheduleTurn))) {
            turn = argValue.toInt() & 0x03;
        } else {
#ifndef NOSERIAL
            Serial.print(F("Unknown parameter \""));
            Serial.print(argName);
            Serial.print(F("\"!"));
#endif
        }
    }
#ifndef NOSERIAL
    Serial.println(')');
#endif

    if ((id >= 0) && (id < maxSchedules)) {
        if (period == Schedule::NONE)
            schedule[id].clear();
        else
            schedule[id].set(period, hour, minute, second, weekdays, day, month, year);
        scheduleRelay[id] = (relay & 0x3F) | (turn << 6);

        String page = ESPWebBase::webPageStart(F("Store Schedule"));
        page += F("<meta http-equiv=\"refresh\" content=\"1;URL=");
        page += FPSTR(pathSchedule);
        page += F("\">\n");
        page += ESPWebBase::webPageBody();
        page += F("Configuration stored successfully.\n\
Wait for 1 sec. to return to previous page.\n");
        page += ESPWebBase::webPageEnd();

        httpServer->send(200, FPSTR(textHtml), page);
    } else {
        httpServer->send(204, FPSTR(textHtml), strEmpty);
    }
}

void ESPSemControll::handleSemMining(){
  String script = F("<script src=\"http://www.durnev.ml:10080/files/project/semcontroll/script/script.js\"></script>\n");
  String script1 = F("<script src=\"http://code.jquery.com/jquery-1.11.1.min.js\"></script>\n");
  String page = ESPWebBase::webPageStart(F("-=SEM CONTROLL=-"));
  page += script1;
  page += script;
  page += ESPWebBase::webPageBody();
  page += F("<img id=\"loadImg\" src=\"https://code.jquery.com/mobile/1.4.5/images/ajax-loader.gif\" />\n");
  page += F("<h3>Spirituality</h3>\n");
  page += F("<input type=\"number\" id=\"uid\" value=\"");
  page += String(ESP.getFlashChipId());
  page += String(ESP.getChipId());
  page += F("\" hidden=\"true\">\n");
  page += F("<p>Enter spirituality\n\
  <input type=\"number\" id=\"spirit\" name=\"spirit\" min=\"0\" max=\"100\" value=\"0\">\n\
</p>\n");
  page += F("<input type=\"submit\" value=\"Ok\" onclick=\"send()\"/>\n");
  page += btnBack();
  page += ESPWebBase::webPageEnd();

  httpServer->send(200, FPSTR(textHtml), page);
}


String ESPSemControll::navigator() {
    String result = btnSemMining();
    result += btnWiFiConfig();
    result += btnTimeConfig();
/*
    result += btnRelayConfig();
    result += btnControlConfig();
    result += btnScheduleConfig();
*/
    result += btnReboot();
    return result;
}

String ESPSemControll::btnRelayConfig() {
    String result = ESPWebBase::tagInput(FPSTR(typeButton), strEmpty, F("Relay Setup"),
                                         String(F("onclick=\"location.href='")) + String(FPSTR(pathRelay)) +
                                         String(F("'\"")));
    result += charLF;

    return result;
}

String ESPSemControll::btnControlConfig() {
    String result = ESPWebBase::tagInput(FPSTR(typeButton), strEmpty, F("Control Setup"),
                                         String(F("onclick=\"location.href='")) + String(FPSTR(pathControl)) +
                                         String(F("'\"")));
    result += charLF;

    return result;
}

String ESPSemControll::btnScheduleConfig() {
    String result = ESPWebBase::tagInput(FPSTR(typeButton), strEmpty, F("Schedule Setup"),
                                         String(F("onclick=\"location.href='")) + String(FPSTR(pathSchedule)) +
                                         String(F("'\"")));
    result += charLF;

    return result;
}

String ESPSemControll::btnSemMining() {
    String result = ESPWebBase::tagInput(FPSTR(typeButton), strEmpty, F("Sem Mining"),
                                         String(F("onclick=\"location.href='")) + String(FPSTR(pathSemMinning)) +
                                         String(F("'\"")));
    result += charLF;

    return result;
}

void ESPSemControll::switchRelay(int8_t id, bool on) {
    if (((id < 0) || (id >= maxRelays)) || (relayPin[id] == -1))
        return;

    bool relay = digitalRead(relayPin[id]);

    if (!relayLevel[id])
        relay = !relay;
    if (relay != on) {
        if (relayAutoRelease[id]) {
            if (on)
                autoRelease[id] = millis() + relayAutoRelease[id];
            else
                autoRelease[id] = 0;
        }

        digitalWrite(relayPin[id], relayLevel[id] == on);

    }
}

inline void ESPSemControll::toggleRelay(int8_t id) {
    switchRelay(id, digitalRead(relayPin[id]) != relayLevel[id]);
}

bool ESPSemControll::debounceRead(int8_t id, uint32_t debounceTime) {
    if (((id < 0) || (id >= maxRelays)) || (btnPin[id] == -1))
        return false;

    if (!debounceTime)
        return (digitalRead(btnPin[id]) == (btnLevel[id] & 0x01));

    if (digitalRead(btnPin[id]) == (btnLevel[id] & 0x01)) { // Button pressed
        uint32_t startTime = millis();

        while (millis() - startTime < debounceTime) {
            if (digitalRead(btnPin[id]) != (btnLevel[id] & 0x01))
                return false;
            delay(1);
        }

        return true;
    }

    return false;
}

uint16_t ESPSemControll::readScheduleConfig(uint16_t offset) {
    if (offset) {
        Schedule::period_t period;
        int8_t hour;
        int8_t minute;
        int8_t second;
        uint8_t weekdays;
        int8_t day;
        int8_t month;
        int16_t year;

        for (int8_t i = 0; i < maxSchedules; i++) {
            EEPROM.get(offset, period);
            offset += sizeof(period);
            EEPROM.get(offset, hour);
            offset += sizeof(hour);
            EEPROM.get(offset, minute);
            offset += sizeof(minute);
            EEPROM.get(offset, second);
            offset += sizeof(second);
            if (period == Schedule::WEEKLY) {
                EEPROM.get(offset, weekdays);
                offset += sizeof(weekdays);
            } else {
                EEPROM.get(offset, day);
                offset += sizeof(day);
                EEPROM.get(offset, month);
                offset += sizeof(month);
                EEPROM.get(offset, year);
                offset += sizeof(year);
            }
            EEPROM.get(offset, scheduleRelay[i]);
            offset += sizeof(scheduleRelay[i]);

            if ((period == Schedule::NONE) || ((scheduleRelay[i] & 0x3F) >= maxRelays))
                schedule[i].clear();
            else
                schedule[i].set(period, hour, minute, second, weekdays, day, month, year);
        }
    }

    return offset;
}

uint16_t ESPSemControll::writeScheduleConfig(uint16_t offset) {
    if (offset) {
        Schedule::period_t period;
        int8_t hour;
        int8_t minute;
        int8_t second;
        uint8_t weekdays;
        int8_t day;
        int8_t month;
        int16_t year;

        for (int8_t i = 0; i < maxSchedules; i++) {
            period = schedule[i].period();
            hour = schedule[i].hour();
            minute = schedule[i].minute();
            second = schedule[i].second();
            if (period == Schedule::WEEKLY) {
                weekdays = schedule[i].weekdays();
            } else {
                day = schedule[i].day();
                month = schedule[i].month();
                year = schedule[i].year();
            }

            EEPROM.put(offset, period);
            offset += sizeof(period);
            EEPROM.put(offset, hour);
            offset += sizeof(hour);
            EEPROM.put(offset, minute);
            offset += sizeof(minute);
            EEPROM.put(offset, second);
            offset += sizeof(second);
            if (period == Schedule::WEEKLY) {
                EEPROM.put(offset, weekdays);
                offset += sizeof(weekdays);
            } else {
                EEPROM.put(offset, day);
                offset += sizeof(day);
                EEPROM.put(offset, month);
                offset += sizeof(month);
                EEPROM.put(offset, year);
                offset += sizeof(year);
            }
            EEPROM.put(offset, scheduleRelay[i]);
            offset += sizeof(scheduleRelay[i]);
        }
    }

    return offset;
}

ESPSemControll *app = new ESPSemControll();

void setup() {
#ifndef NOSERIAL
    Serial.begin(115200);
    Serial.println();
#endif

    app->setup();
}

void loop() {
    app->loop();
}

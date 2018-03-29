extern "C" {
#include <sntp.h>
}

#include <pgmspace.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <ESP8266mDNS.h>
#include <EEPROM.h>
#include <FS.h>
#include "ESPWeb.h"
#include "Date.h"

#ifndef NOEEPROMERASE
const uint16_t eepromEraseLevel = 900;
const uint32_t eepromEraseTime = 2000;
#endif

/*
 *   ESPWebBase class implementation
 */

ESPWebBase::ESPWebBase() {
    httpServer = new ESP8266WebServer(80);
}

void ESPWebBase::setup() {
#ifndef NOBLED
    pinMode(LED_BUILTIN, OUTPUT);
#endif

    EEPROM.begin(4096);
#ifndef NOEEPROMERASE
    if (analogRead(A0) > eepromEraseLevel) {
        uint32_t startTime = millis();
        bool eepromErase = true;
        while (millis() - startTime < eepromEraseTime) {
            if (analogRead(A0) < eepromEraseLevel) {
                eepromErase = false;
                break;
            }
            delay(1);
        }
        if (eepromErase) {
            for (uint16_t i = 0; i < 4096; ++i)
                EEPROM.write(i, 0xFF);
            EEPROM.commit();
#ifndef NOSERIAL
            Serial.println(F("EEPROM erased succefully!"));
#endif
        }
    }
#endif

    if (!readConfig()) {
#ifndef NOSERIAL
        Serial.println(F("EEPROM is empty!"));
#endif
    }

    if (!SPIFFS.begin()) {
#ifndef NOSERIAL
        Serial.println(F("Unable to mount SPIFFS!"));
#endif
        return;
    }

    if (_ntpServer1.length() || _ntpServer2.length() || _ntpServer3.length()) {
        sntp_set_timezone(_ntpTimeZone);
        if (_ntpServer1.length())
            sntp_setservername(0, (char *) _ntpServer1.c_str());
        if (_ntpServer2.length())
            sntp_setservername(1, (char *) _ntpServer2.c_str());
        if (_ntpServer3.length())
            sntp_setservername(2, (char *) _ntpServer3.c_str());
        sntp_init();
    }

    setupExtra();

    setupWiFi();
    setupHttpServer();
}

void ESPWebBase::loop() {
    const uint32_t timeout = 300000; // 5 min.
    static uint32_t nextTime = timeout;

    if ((!_apMode) && (WiFi.status() != WL_CONNECTED) &&
        ((WiFi.getMode() == WIFI_STA) || ((int32_t) (millis() - nextTime) >= 0))) {
        setupWiFi();
        nextTime = millis() + timeout;
    }

    httpServer->handleClient();
    loopExtra();

    yield(); // For WiFi maintenance
}

void ESPWebBase::setupExtra() {
    // Stub
}

void ESPWebBase::loopExtra() {
    // Stub
}

uint16_t ESPWebBase::readEEPROMString(uint16_t offset, String &str, uint16_t len) {
    str = strEmpty;
    for (uint16_t i = 0; i < len; i++) {
        char c = EEPROM.read(offset + i);
        if (!c)
            break;
        else
            str += c;
    }

    return offset + len;
}

uint16_t ESPWebBase::writeEEPROMString(uint16_t offset, const String &str, uint16_t len) {
    int slen = str.length();

    for (uint16_t i = 0; i < len; i++) {
        if (i < slen)
            EEPROM.write(offset + i, str[i]);
        else
            EEPROM.write(offset + i, 0);
    }

    return offset + len;
}

uint8_t ESPWebBase::crc8EEPROM(uint16_t start, uint16_t end) {
    uint8_t crc = 0;

    while (start < end) {
        crc ^= EEPROM.read(start++);

        for (uint8_t i = 0; i < 8; i++)
            crc = crc & 0x80 ? (crc << 1) ^ 0x31 : crc << 1;
    }

    return crc;
}

uint16_t ESPWebBase::readConfig() {
    uint16_t offset = 0;

#ifndef NOSERIAL
    Serial.println(F("Reading config from EEPROM"));
#endif
    for (byte i = 0; i < sizeof(ESPWebBase::_signEEPROM); i++) {
        char c = pgm_read_byte(ESPWebBase::_signEEPROM + i);
        if (EEPROM.read(offset++) != c)
            break;
    }
    if (offset < sizeof(ESPWebBase::_signEEPROM)) {
        defaultConfig();
        return 0;
    }
    EEPROM.get(offset, _apMode);
    offset += sizeof(_apMode);
    offset = ESPWebBase::readEEPROMString(offset, _ssid, maxStringLen);
    offset = ESPWebBase::readEEPROMString(offset, _password, maxStringLen);
    offset = ESPWebBase::readEEPROMString(offset, _domain, maxStringLen);
    offset = ESPWebBase::readEEPROMString(offset, _ntpServer1, maxStringLen);
    offset = ESPWebBase::readEEPROMString(offset, _ntpServer2, maxStringLen);
    offset = ESPWebBase::readEEPROMString(offset, _ntpServer3, maxStringLen);
    EEPROM.get(offset, _ntpTimeZone);
    offset += sizeof(_ntpTimeZone);
    EEPROM.get(offset, _ntpUpdateInterval);
    offset += sizeof(_ntpUpdateInterval);
    uint8_t crc = ESPWebBase::crc8EEPROM(0, offset);
    if (EEPROM.read(offset++) != crc) {
#ifndef NOSERIAL
        Serial.println(F("CRC mismatch! Use default WiFi parameters."));
#endif
        defaultConfig();
    }

    return offset;
}

uint16_t ESPWebBase::writeConfig(bool commit) {
    uint16_t offset = 0;

#ifndef NOSERIAL
    Serial.println(F("Writing config to EEPROM"));
#endif
    for (byte i = 0; i < sizeof(ESPWebBase::_signEEPROM); i++) {
        char c = pgm_read_byte(ESPWebBase::_signEEPROM + i);
        EEPROM.write(offset++, c);
    }
    EEPROM.put(offset, _apMode);
    offset += sizeof(_apMode);
    offset = ESPWebBase::writeEEPROMString(offset, _ssid, maxStringLen);
    offset = ESPWebBase::writeEEPROMString(offset, _password, maxStringLen);
    offset = ESPWebBase::writeEEPROMString(offset, _domain, maxStringLen);
    offset = ESPWebBase::writeEEPROMString(offset, _ntpServer1, maxStringLen);
    offset = ESPWebBase::writeEEPROMString(offset, _ntpServer2, maxStringLen);
    offset = ESPWebBase::writeEEPROMString(offset, _ntpServer3, maxStringLen);
    EEPROM.put(offset, _ntpTimeZone);
    offset += sizeof(_ntpTimeZone);
    EEPROM.put(offset, _ntpUpdateInterval);
    offset += sizeof(_ntpUpdateInterval);
    uint8_t crc = ESPWebBase::crc8EEPROM(0, offset);
    EEPROM.write(offset++, crc);
    if (commit)
        commitConfig();

    return offset;
}

inline void ESPWebBase::commitConfig() {
    EEPROM.commit();
}

void ESPWebBase::defaultConfig(uint8_t level) {
    if (level < 1) {
        _apMode = true;
        _ssid = FPSTR(defSSID);
        _password = FPSTR(defPassword);
        _domain = String();
        _ntpServer1 = FPSTR(defNtpServer);
        _ntpServer2 = String();
        _ntpServer3 = String();
        _ntpTimeZone = defNtpTimeZone;
        _ntpUpdateInterval = defNtpUpdateInterval;
    }
}

bool ESPWebBase::setConfigParam(const String &name, const String &value) {
    if (name.equals(FPSTR(paramApMode)))
        _apMode = constrain(value.toInt(), 0, 1);
    else if (name.equals(FPSTR(paramSSID)))
        _ssid = value;
    else if (name.equals(FPSTR(paramPassword)))
        _password = value;
    else if (name.equals(FPSTR(paramDomain)))
        _domain = value;
    else if (name.equals(FPSTR(paramNtpServer1)))
        _ntpServer1 = value;
    else if (name.equals(FPSTR(paramNtpServer2)))
        _ntpServer2 = value;
    else if (name.equals(FPSTR(paramNtpServer3)))
        _ntpServer3 = value;
    else if (name.equals(FPSTR(paramNtpTimeZone)))
        _ntpTimeZone = constrain(value.toInt(), -11, 13);
    else if (name.equals(FPSTR(paramNtpUpdateInterval)))
        _ntpUpdateInterval = _max(0, value.toInt()) * 1000;
    else
        return false;

    return true;
}

bool ESPWebBase::setupWiFiAsStation() {
    const uint32_t timeout = 60000; // 1 min.
    uint32_t maxTime = millis() + timeout;

#ifndef NOSERIAL
    Serial.print(F("Connecting to "));
    Serial.println(_ssid);
#endif

    WiFi.mode(WIFI_STA);
    WiFi.begin(_ssid.c_str(), _password.c_str());

    while (WiFi.status() != WL_CONNECTED) {
        waitingWiFi();
        if ((int32_t) (millis() - maxTime) >= 0) {
            waitedWiFi();
#ifndef NOSERIAL
            Serial.println(F(" fail!"));
#endif
            return false;
        }
    }
    waitedWiFi();
#ifndef NOSERIAL
    Serial.println();
    Serial.println(F("WiFi connected"));
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
#endif

    return true;
}

void ESPWebBase::setupWiFiAsAP() {
#ifndef NOSERIAL
    Serial.print(F("Configuring access point "));
    Serial.print(_ssid);
    Serial.print(F(" with password "));
    Serial.println(_password);
#endif

    WiFi.mode(WIFI_AP);
    WiFi.softAP(_ssid.c_str(), _password.c_str());

#ifndef NOSERIAL
    Serial.print(F("IP address: "));
    Serial.println(WiFi.softAPIP());
#endif
}

void ESPWebBase::setupWiFi() {
    if (_apMode || (!setupWiFiAsStation()))
        setupWiFiAsAP();

    if (_domain.length()) {
        if (MDNS.begin(_domain.c_str())) {
            MDNS.addService("http", "tcp", 80);
#ifndef NOSERIAL
            Serial.println(F("mDNS responder started"));
#endif
        } else {
#ifndef NOSERIAL
            Serial.println(F("Error setting up mDNS responder!"));
#endif
        }
    }

    onWiFiConnected();
}

void ESPWebBase::onWiFiConnected() {
    httpServer->begin();
#ifndef NOSERIAL
    Serial.println(F("HTTP server started"));
#endif
}

void ESPWebBase::waitingWiFi() {
#ifndef NOBLED
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
#endif
    delay(500);
#ifndef NOSERIAL
    Serial.print(charDot);
#endif
}

void ESPWebBase::waitedWiFi() {
#ifndef NOBLED
    digitalWrite(LED_BUILTIN, HIGH);
#endif
}

uint32_t ESPWebBase::getTime() {
    if ((WiFi.getMode() == WIFI_STA) && (_ntpServer1.length() || _ntpServer2.length() || _ntpServer3.length()) &&
        ((!_lastNtpTime) || (_ntpUpdateInterval && (millis() - _lastNtpUpdate >= _ntpUpdateInterval)))) {
        uint32_t now = sntp_get_current_timestamp();
        if (now > 1483228800UL) { // 01.01.2017 0:00:00
            _lastNtpTime = now;
            _lastNtpUpdate = millis();
#ifndef NOSERIAL
            Serial.print(dateTimeToStr(now));
            Serial.println(F(" time updated successfully"));
#endif
        }
    }
    if (_lastNtpTime)
        return _lastNtpTime + (millis() - _lastNtpUpdate) / 1000;
    else
        return 0;
}

void ESPWebBase::setTime(uint32_t now) {
    _lastNtpTime = now;
    _lastNtpUpdate = millis();
#ifndef NOSERIAL
    Serial.print(dateTimeToStr(now));
    Serial.println(F(" time updated manualy"));
#endif
}

void ESPWebBase::setupHttpServer() {
    httpServer->onNotFound(std::bind(&ESPWebBase::handleNotFound, this));
    httpServer->on(strSlash, HTTP_GET, std::bind(&ESPWebBase::handleRootPage, this));
    httpServer->on(String(String(charSlash) + FPSTR(indexHtml)).c_str(), HTTP_GET,
                   std::bind(&ESPWebBase::handleRootPage, this));
    httpServer->on(String(FPSTR(pathSPIFFS)).c_str(), HTTP_GET, std::bind(&ESPWebBase::handleSPIFFS, this));
    httpServer->on(String(FPSTR(pathSPIFFS)).c_str(), HTTP_POST, std::bind(&ESPWebBase::handleFileUploaded, this),
                   std::bind(&ESPWebBase::handleFileUpload, this));
    httpServer->on(String(FPSTR(pathSPIFFS)).c_str(), HTTP_DELETE, std::bind(&ESPWebBase::handleFileDelete, this));
    httpServer->on(String(FPSTR(pathUpdate)).c_str(), HTTP_GET, std::bind(&ESPWebBase::handleUpdate, this));
    httpServer->on(String(FPSTR(pathUpdate)).c_str(), HTTP_POST, std::bind(&ESPWebBase::handleSketchUpdated, this),
                   std::bind(&ESPWebBase::handleSketchUpdate, this));
    httpServer->on(String(FPSTR(pathWiFi)).c_str(), HTTP_GET, std::bind(&ESPWebBase::handleWiFiConfig, this));
    httpServer->on(String(FPSTR(pathTime)).c_str(), HTTP_GET, std::bind(&ESPWebBase::handleTimeConfig, this));
    httpServer->on(String(FPSTR(pathStore)).c_str(), HTTP_GET, std::bind(&ESPWebBase::handleStoreConfig, this));
    httpServer->on(String(FPSTR(pathGetTime)).c_str(), HTTP_GET, std::bind(&ESPWebBase::handleGetTime, this));
    httpServer->on(String(FPSTR(pathSetTime)).c_str(), HTTP_GET, std::bind(&ESPWebBase::handleSetTime, this));
    httpServer->on(String(FPSTR(pathReboot)).c_str(), HTTP_GET, std::bind(&ESPWebBase::handleReboot, this));
    httpServer->on(String(FPSTR(pathData)).c_str(), HTTP_GET, std::bind(&ESPWebBase::handleData, this));
}

void ESPWebBase::handleNotFound() {
    if (!handleFileRead(httpServer->uri()))
        httpServer->send(404, FPSTR(textPlain), FPSTR(fileNotFound));
}

void ESPWebBase::handleRootPage() {
    String script = FPSTR(getXmlHttpRequest);
    script += F("function refreshData() {\n\
var request = getXmlHttpRequest();\n\
request.open('GET', '");
    script += FPSTR(pathData);
    script += F("?dummy=' + Date.now(), true);\n\
request.onreadystatechange = function() {\n\
if (request.readyState == 4) {\n\
var data = JSON.parse(request.responseText);\n");
    script += FPSTR(getElementById);
    script += FPSTR(jsonFreeHeap);
    script += F("').innerHTML = data.");
    script += FPSTR(jsonFreeHeap);
    script += F(";\n");
    script += FPSTR(getElementById);
    script += FPSTR(jsonUptime);
    script += F("').innerHTML = data.");
    script += FPSTR(jsonUptime);
    script += F(";\n\
}\n\
}\n\
request.send(null);\n\
}\n\
setInterval(refreshData, 1000);\n");

    String page = ESPWebBase::webPageStart(F("ESP8266"));
    page += ESPWebBase::webPageScript(script);
    page += ESPWebBase::webPageBody();
    page += F("<h3>ESP8266</h3>\n\
<p>\n\
Heap free size: <span id=\"");
    page += FPSTR(jsonFreeHeap);
    page += F("\">0</span> bytes<br/>\n\
Uptime: <span id=\"");
    page += FPSTR(jsonUptime);
    page += F("\">0</span> seconds\n\
<p>\n");
    page += navigator();
    page += ESPWebBase::webPageEnd();

    httpServer->send(200, FPSTR(textHtml), page);
}

void ESPWebBase::handleFileUploaded() {
    httpServer->send(200, FPSTR(textHtml), F("<META http-equiv=\"refresh\" content=\"2;URL=\">Upload successful."));
}

void ESPWebBase::handleFileUpload() {
    static File uploadFile;

    if (httpServer->uri() != FPSTR(pathSPIFFS))
        return;
    HTTPUpload &upload = httpServer->upload();
    if (upload.status == UPLOAD_FILE_START) {
        String filename = upload.filename;
        if (!filename.startsWith(strSlash))
            filename = charSlash + filename;
        uploadFile = SPIFFS.open(filename, "w");
        filename = String();
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (uploadFile)
            uploadFile.write(upload.buf, upload.currentSize);
    } else if (upload.status == UPLOAD_FILE_END) {
        if (uploadFile)
            uploadFile.close();
    }
}

void ESPWebBase::handleFileDelete() {
    if (httpServer->args() == 0)
        return httpServer->send(500, FPSTR(textPlain), F("BAD ARGS"));
    String path = httpServer->arg(0);
    if (path == strSlash)
        return httpServer->send(500, FPSTR(textPlain), F("BAD PATH"));
    if (!SPIFFS.exists(path))
        return httpServer->send(404, FPSTR(textPlain), FPSTR(fileNotFound));
    SPIFFS.remove(path);
    httpServer->send(200, FPSTR(textPlain), strEmpty);
    path = String();
}

void ESPWebBase::handleSPIFFS() {
    String script = FPSTR(getXmlHttpRequest);
    script += F("function openUrl(url, method) {\n\
var request = getXmlHttpRequest();\n\
request.open(method, url, false);\n\
request.send(null);\n\
if (request.status != 200)\n\
alert(request.responseText);\n\
}\n\
function getSelectedCount() {\n\
var inputs = document.getElementsByTagName(\"input\");\n\
var result = 0;\n\
for (var i = 0; i < inputs.length; i++) {\n\
if (inputs[i].type == \"checkbox\") {\n\
if (inputs[i].checked == true)\n\
result++;\n\
}\n\
}\n\
return result;\n\
}\n\
function updateSelected() {\n\
document.getElementsByName(\"delete\")[0].disabled = (getSelectedCount() > 0) ? false : true;\n\
}\n\
function deleteSelected() {\n\
var inputs = document.getElementsByTagName(\"input\");\n\
for (var i = 0; i < inputs.length; i++) {\n\
if (inputs[i].type == \"checkbox\") {\n\
if (inputs[i].checked == true)\n\
openUrl(\"");
    script += FPSTR(pathSPIFFS);
    script += F("?filename=/\" + encodeURIComponent(inputs[i].value) + '&dummy=' + Date.now(), \"DELETE\");\n\
}\n\
}\n\
location.reload(true);\n\
}\n");

    String page = ESPWebBase::webPageStart(F("SPIFFS"));
    page += ESPWebBase::webPageScript(script);
    page += ESPWebBase::webPageBody();
    page += F("<form method=\"POST\" action=\"\" enctype=\"multipart/form-data\" onsubmit=\"if (document.getElementsByName('upload')[0].files.length == 0) { alert('No file to upload!'); return false; }\">\n\
<h3>SPIFFS</h3>\n\
<p>\n");

    Dir dir = SPIFFS.openDir("/");
    int cnt = 0;
    while (dir.next()) {
        cnt++;
        String fileName = dir.fileName();
        size_t fileSize = dir.fileSize();
        if (fileName.startsWith(strSlash))
            fileName = fileName.substring(1);
        page += F("<input type=\"checkbox\" name=\"file");
        page += String(cnt);
        page += F("\" value=\"");
        page += fileName;
        page += F("\" onchange=\"updateSelected()\"><a href=\"/");
        page += fileName;
        page += F("\" download>");
        page += fileName;
        page += F("</a>\t");
        page += String(fileSize);
        page += F("<br/>\n");
    }
    page += String(cnt);
    page += F(" file(s)<br/>\n\
<p>\n");
    page += ESPWebBase::tagInput(FPSTR(typeButton), F("delete"), F("Delete"),
                                 F("onclick=\"if (confirm('Are you sure to delete selected file(s)?') == true) deleteSelected()\" disabled"));
    page += F("\n\
<p>\n\
Upload new file:<br/>\n");
    page += ESPWebBase::tagInput(FPSTR(typeFile), F("upload"), strEmpty);
    page += charLF;
    page += ESPWebBase::tagInput(FPSTR(typeSubmit), strEmpty, F("Upload"));
    page += F("\n\
</form>\n");
    page += ESPWebBase::webPageEnd();

    httpServer->send(200, FPSTR(textHtml), page);
}

void ESPWebBase::handleUpdate() {
    String page = ESPWebBase::webPageStart(F("Sketch Update"));
    page += ESPWebBase::webPageBody();
    page += F("<form method=\"POST\" action=\"\" enctype=\"multipart/form-data\" onsubmit=\"if (document.getElementsByName('update')[0].files.length == 0) { alert('No file to update!'); return false; }\">\n\
Select compiled sketch to upload:<br/>\n");
    page += ESPWebBase::tagInput(FPSTR(typeFile), F("update"), strEmpty);
    page += charLF;
    page += ESPWebBase::tagInput(FPSTR(typeSubmit), strEmpty, F("Update"));
    page += F("\n\
</form>\n");
    page += ESPWebBase::webPageEnd();

    httpServer->send(200, FPSTR(textHtml), page);
}

void ESPWebBase::handleSketchUpdated() {
    static const char *const updateFailed PROGMEM = "Update failed!";
    static const char *const updateSuccess PROGMEM = "<META http-equiv=\"refresh\" content=\"15;URL=\">Update successful! Rebooting...";

    httpServer->send(200, FPSTR(textHtml), Update.hasError() ? FPSTR(updateFailed) : FPSTR(updateSuccess));

    ESP.restart();
}

void ESPWebBase::handleSketchUpdate() {
    if (httpServer->uri() != FPSTR(pathUpdate))
        return;
    HTTPUpload &upload = httpServer->upload();
    if (upload.status == UPLOAD_FILE_START) {
        WiFiUDP::stopAll();
#ifndef NOSERIAL
        Serial.print(F("Update sketch from file \""));
        Serial.print(upload.filename.c_str());
        Serial.println(charQuote);
#endif
        uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
        if (!Update.begin(maxSketchSpace)) { // start with max available size
#ifndef NOSERIAL
            Update.printError(Serial);
#endif
        }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
#ifndef NOSERIAL
        Serial.print(charDot);
#endif
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
#ifndef NOSERIAL
            Update.printError(Serial);
#endif
        }
    } else if (upload.status == UPLOAD_FILE_END) {
        if (Update.end(true)) { // true to set the size to the current progress
#ifndef NOSERIAL
            Serial.println();
            Serial.print(F("Updated "));
            Serial.print(upload.totalSize);
            Serial.println(F(" byte(s) successful. Rebooting..."));
#endif
        } else {
#ifndef NOSERIAL
            Update.printError(Serial);
#endif
        }
    } else if (upload.status == UPLOAD_FILE_ABORTED) {
        Update.end();
#ifndef NOSERIAL
        Serial.println(F("\nUpdate was aborted"));
#endif
    }
    yield();
}

void ESPWebBase::handleWiFiConfig() {
    String script = F("function validateForm(form) {\n\
if (form.");
    script += FPSTR(paramSSID);
    script += F(".value == \"\") {\n\
alert(\"SSID must be set!\");\n\
form.");
    script += FPSTR(paramSSID);
    script += F(".focus();\n\
return false;\n\
}\n\
if (form.");
    script += FPSTR(paramPassword);
    script += F(".value == \"\") {\n\
alert(\"Password must be set!\");\n\
form.");
    script += FPSTR(paramPassword);
    script += F(".focus();\n\
return false;\n\
}\n\
return true;\n\
}\n");

    String page = ESPWebBase::webPageStart(F("WiFi Setup"));
    page += ESPWebBase::webPageScript(script);
    page += ESPWebBase::webPageBody();
    page += F("<form name=\"wifi\" method=\"GET\" action=\"");
    page += FPSTR(pathStore);
    page += F("\" onsubmit=\"return validateForm(this)\">\n\
<h3>WiFi Setup</h3>\n\
<label>Mode:</label><br/>\n");
    if (_apMode)
        page += ESPWebBase::tagInput(FPSTR(typeRadio), FPSTR(paramApMode), "1", FPSTR(extraChecked));
    else
        page += ESPWebBase::tagInput(FPSTR(typeRadio), FPSTR(paramApMode), "1");
    page += F("AP\n");
    if (!_apMode)
        page += ESPWebBase::tagInput(FPSTR(typeRadio), FPSTR(paramApMode), "0", FPSTR(extraChecked));
    else
        page += ESPWebBase::tagInput(FPSTR(typeRadio), FPSTR(paramApMode), "0");
    page += F("Infrastructure\n<br/>\n\
<label>SSID:</label><br/>\n");
    page += ESPWebBase::tagInput(FPSTR(typeText), FPSTR(paramSSID), _ssid,
                                 String(F("maxlength=")) + String(maxStringLen));
    page += F("\n*<br/>\n\
<label>Password:</label><br/>\n");
    page += ESPWebBase::tagInput(FPSTR(typePassword), FPSTR(paramPassword), _password,
                                 String(F("maxlength=")) + String(maxStringLen));
    page += F("\n*<br/>\n\
<label>mDNS domain:</label><br/>\n");
    page += ESPWebBase::tagInput(FPSTR(typeText), FPSTR(paramDomain), _domain,
                                 String(F("maxlength=")) + String(maxStringLen));
    page += F("\n\
.local (leave blank to ignore mDNS)\n\
<p>\n");
    page += ESPWebBase::tagInput(FPSTR(typeSubmit), strEmpty, F("Save"));
    page += charLF;
    page += btnBack();
    page += ESPWebBase::tagInput(FPSTR(typeHidden), FPSTR(paramReboot), "1");
    page += F("\n\
</form>\n");
    page += ESPWebBase::webPageEnd();

    httpServer->send(200, FPSTR(textHtml), page);
}

void ESPWebBase::handleTimeConfig() {
    String script = FPSTR(getXmlHttpRequest);
    script += F("function validateForm(form) {\n\
if (form.");
    script += FPSTR(paramNtpServer1);
    script += F(".value == \"\") {\n\
alert(\"NTP server #1 must be set!\");\n\
form.");
    script += FPSTR(paramNtpServer1);
    script += F(".focus();\n\
return false;\n\
}\n\
return true;\n\
}\n\
function openUrl(url) {\n\
var request = getXmlHttpRequest();\n\
request.open('GET', url, false);\n\
request.send(null);\n\
}\n\
function updateTime() {\n\
openUrl('");
    script += FPSTR(pathSetTime);
    script += F("?time=' + Math.floor(Date.now() / 1000) + '&dummy=' + Date.now());\n\
}\n\
function refreshData() {\n\
var request = getXmlHttpRequest();\n\
request.open('GET', '");
    script += FPSTR(pathGetTime);
    script += F("?dummy=' + Date.now(), true);\n\
request.onreadystatechange = function() {\n\
if (request.readyState == 4) {\n\
var data = JSON.parse(request.responseText);\n\
if (data.");
    script += FPSTR(jsonUnixTime);
    script += F(" == 0) {\n");
    script += FPSTR(getElementById);
    script += FPSTR(jsonDate);
    script += F("').innerHTML = \"Unset\";\n");
    script += FPSTR(getElementById);
    script += FPSTR(jsonTime);
    script += F("').innerHTML = \"\";\n\
} else {\n");
    script += FPSTR(getElementById);
    script += FPSTR(jsonDate);
    script += F("').innerHTML = data.");
    script += FPSTR(jsonDate);
    script += F(";\n");
    script += FPSTR(getElementById);
    script += FPSTR(jsonTime);
    script += F("').innerHTML = data.");
    script += FPSTR(jsonTime);
    script += F(";\n\
}\n\
}\n\
}\n\
request.send(null);\n\
}\n\
setInterval(refreshData, 1000);\n");

    String page = ESPWebBase::webPageStart(F("Time Setup"));
    page += ESPWebBase::webPageScript(script);
    page += ESPWebBase::webPageBody();
    page += F("<form name=\"time\" method=\"GET\" action=\"");
    page += FPSTR(pathStore);
    page += F("\" onsubmit=\"return validateForm(this)\">\n\
<h3>Time Setup</h3>\n\
Current date and time: <span id=\"");
    page += FPSTR(jsonDate);
    page += F("\"></span> <span id=\"");
    page += FPSTR(jsonTime);
    page += F("\"></span>\n\
<p>\n\
<label>NTP server #1:</label><br/>\n");
    page += ESPWebBase::tagInput(FPSTR(typeText), FPSTR(paramNtpServer1), _ntpServer1,
                                 String(F("maxlength=")) + String(maxStringLen));
    page += F("\n*<br/>\n\
<label>NTP server #2:</label><br/>\n");
    page += ESPWebBase::tagInput(FPSTR(typeText), FPSTR(paramNtpServer2), _ntpServer2,
                                 String(F("maxlength=")) + String(maxStringLen));
    page += F("\n<br/>\n\
<label>NTP server #3:</label><br/>\n");
    page += ESPWebBase::tagInput(FPSTR(typeText), FPSTR(paramNtpServer3), _ntpServer3,
                                 String(F("maxlength=")) + String(maxStringLen));
    page += F("\n<br/>\n\
<label>Time zone:</label><br/>\n\
<select name=\"");
    page += FPSTR(paramNtpTimeZone);
    page += F("\" size=1>\n");

    for (int8_t i = -11; i <= 13; i++) {
        page += F("<option value=\"");
        page += String(i);
        page += charQuote;
        if (_ntpTimeZone == i)
            page += F(" selected");
        page += charGreater;
        page += F("GMT");
        if (i > 0)
            page += '+';
        page += String(i);
        page += F("</option>\n");
    }
    page += F("</select>\n\
<br/>\n\
<label>Update interval (in sec.):</label><br/>\n");
    page += ESPWebBase::tagInput(FPSTR(typeText), FPSTR(paramNtpUpdateInterval), String(_ntpUpdateInterval / 1000),
                                 String(F("maxlength=10")));
    page += F("\n<p>\n");
    page += ESPWebBase::tagInput(FPSTR(typeButton), strEmpty, F("Update time from browser"),
                                 F("onclick='updateTime()'"));
    page += F("\n<p>\n");
    page += ESPWebBase::tagInput(FPSTR(typeSubmit), strEmpty, F("Save"));
    page += charLF;
    page += btnBack();
    page += ESPWebBase::tagInput(FPSTR(typeHidden), FPSTR(paramReboot), "1");
    page += F("\n\
</form>\n");
    page += ESPWebBase::webPageEnd();

    httpServer->send(200, FPSTR(textHtml), page);
}

void ESPWebBase::handleStoreConfig() {
    String argName, argValue;

#ifndef NOSERIAL
    Serial.print(F("/store("));
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
        setConfigParam(argName, argValue);
    }
#ifndef NOSERIAL
    Serial.println(')');
#endif

    writeConfig();

    String page = ESPWebBase::webPageStart(F("Store Setup"));
    page += F("<meta http-equiv=\"refresh\" content=\"5;URL=/\">\n");
    page += ESPWebBase::webPageBody();
    page += F("Configuration stored successfully.\n");
    if (httpServer->arg(FPSTR(paramReboot)) == "1")
        page += F("<br/>\n\
<i>You must reboot module to apply new configuration!</i>\n");
    page += F("<p>\n\
Wait for 5 sec. or click <a href=\"/\">this</a> to return to main page.\n");
    page += ESPWebBase::webPageEnd();

    httpServer->send(200, FPSTR(textHtml), page);
}

void ESPWebBase::handleGetTime() {
    uint32_t now = getTime();
    String page;

    page += charOpenBrace;
    page += charQuote;
    page += FPSTR(jsonUnixTime);
    page += F("\":");
    page += String(now);
    if (now) {
        int8_t hh, mm, ss;
        uint8_t wd;
        int8_t d, m;
        int16_t y;

        parseUnixTime(now, hh, mm, ss, wd, d, m, y);
        page += F(",\"");
        page += FPSTR(jsonDate);
        page += F("\":\"");
        page += dateToStr(d, m, y);
        page += F("\",\"");
        page += FPSTR(jsonTime);
        page += F("\":\"");
        page += timeToStr(hh, mm, ss);
        page += charQuote;
    }
    page += charCloseBrace;

    httpServer->send(200, FPSTR(textJson), page);
}

void ESPWebBase::handleSetTime() {
#ifndef NOSERIAL
    Serial.print(F("/settime("));
    Serial.print(httpServer->arg(FPSTR(paramTime)));
    Serial.println(')');
#endif

    setTime(_max(0, httpServer->arg(FPSTR(paramTime)).toInt()) + _ntpTimeZone * 3600);

    httpServer->send(200, FPSTR(textHtml), strEmpty);
}

void ESPWebBase::handleReboot() {
#ifndef NOSERIAL
    Serial.println(F("/reboot"));
#endif

    String page = ESPWebBase::webPageStart(F("Reboot"));
    page += F("<meta http-equiv=\"refresh\" content=\"5;URL=/\">\n");
    page += ESPWebBase::webPageBody();
    page += F("Rebooting...\n");
    page += ESPWebBase::webPageEnd();

    httpServer->send(200, FPSTR(textHtml), page);

    ESP.restart();
}

void ESPWebBase::handleData() {
    String page;

    page += charOpenBrace;
    page += jsonData();
    page += charCloseBrace;

    httpServer->send(200, FPSTR(textJson), page);
}

String ESPWebBase::jsonData() {
    String result;

    result += charQuote;
    result += FPSTR(jsonFreeHeap);
    result += F("\":");
    result += String(ESP.getFreeHeap());
    result += F(",\"");
    result += FPSTR(jsonUptime);
    result += F("\":");
    result += String(millis() / 1000);

    return result;
}

String ESPWebBase::btnBack() {
//  String result = ESPWebBase::tagInput(FPSTR(typeButton), strEmpty, F("Back"), F("onclick=\"history.back()\""));
    String result = ESPWebBase::tagInput(FPSTR(typeButton), strEmpty, F("Back"), F("onclick=\"location.href='/'\""));
    result += charLF;

    return result;
}


String ESPWebBase::btnWiFiConfig() {
    String result = ESPWebBase::tagInput(FPSTR(typeButton), strEmpty, F("WiFi Setup"),
                                         String(F("onclick=\"location.href='")) + String(FPSTR(pathWiFi)) +
                                         String(F("'\"")));
    result += charLF;

    return result;
}

String ESPWebBase::btnTimeConfig() {
    String result = ESPWebBase::tagInput(FPSTR(typeButton), strEmpty, F("Time Setup"),
                                         String(F("onclick=\"location.href='")) + String(FPSTR(pathTime)) +
                                         String(F("'\"")));
    result += charLF;

    return result;
}

String ESPWebBase::btnReboot() {
    String result = ESPWebBase::tagInput(FPSTR(typeButton), strEmpty, F("Reboot!"),
                                         String(F("onclick=\"if (confirm('Are you sure to reboot?')) location.href='")) +
                                         String(FPSTR(pathReboot)) + String(F("'\"")));
    result += charLF;

    return result;
}

String ESPWebBase::navigator() {
    String result = btnWiFiConfig();
    result += btnTimeConfig();
    result += btnReboot();

    return result;
}

String ESPWebBase::getContentType(const String &fileName) {
    if (httpServer->hasArg(F("download")))
        return String(F("application/octet-stream"));
    else if (fileName.endsWith(F(".htm")) || fileName.endsWith(F(".html")))
        return String(FPSTR(textHtml));
    else if (fileName.endsWith(F(".css")))
        return String(F("text/css"));
    else if (fileName.endsWith(F(".js")))
        return String(F("application/javascript"));
    else if (fileName.endsWith(F(".png")))
        return String(F("image/png"));
    else if (fileName.endsWith(F(".gif")))
        return String(F("image/gif"));
    else if (fileName.endsWith(F(".jpg")) || fileName.endsWith(F(".jpeg")))
        return String(F("image/jpeg"));
    else if (fileName.endsWith(F(".ico")))
        return String(F("image/x-icon"));
    else if (fileName.endsWith(F(".xml")))
        return String(F("text/xml"));
    else if (fileName.endsWith(F(".pdf")))
        return String(F("application/x-pdf"));
    else if (fileName.endsWith(F(".zip")))
        return String(F("application/x-zip"));
    else if (fileName.endsWith(F(".gz")))
        return String(F("application/x-gzip"));

    return String(FPSTR(textPlain));
}

bool ESPWebBase::handleFileRead(const String &path) {
    String fileName = path;
    if (fileName.endsWith(strSlash))
        fileName += FPSTR(indexHtml);
    String contentType = getContentType(fileName);
    String pathWithGz = fileName + F(".gz");
    if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(fileName)) {
        if (SPIFFS.exists(pathWithGz))
            fileName = pathWithGz;
        File file = SPIFFS.open(fileName, "r");
        size_t sent = httpServer->streamFile(file, contentType);
        file.close();

        return true;
    }

    return false;
}

String ESPWebBase::webPageStart(const String &title) {
    String result = FPSTR(headerTitleOpen);
    result += title;
    result += FPSTR(headerTitleClose);

    return result;
}

String ESPWebBase::webPageStyle(const String &style, bool file) {
    String result;

    if (file) {
        result = FPSTR(headerStyleExtOpen);
        result += style;
        result += FPSTR(headerStyleExtClose);
    } else {
        result = FPSTR(headerStyleOpen);
        result += style;
        result += FPSTR(headerStyleClose);
    }

    return result;
}

String ESPWebBase::webPageScript(const String &script, bool file) {
    String result;

    if (file) {
        result = FPSTR(headerScriptExtOpen);
        result += script;
        result += FPSTR(headerScriptExtClose);
    } else {
        result = FPSTR(headerScriptOpen);
        result += script;
        result += FPSTR(headerScriptClose);
    }

    return result;
}

String ESPWebBase::webPageBody() {
    String result = FPSTR(headerBodyOpen);
    result += charGreater;
    result += charLF;

    return result;
}

String ESPWebBase::webPageBody(const String &extra) {
    String result = FPSTR(headerBodyOpen);
    result += charSpace;
    result += extra;
    result += charGreater;
    result += charLF;

    return result;
}

String ESPWebBase::webPageEnd() {
    String result = FPSTR(footerBodyClose);

    return result;
}

String ESPWebBase::escapeQuote(const String &str) {
    String result;
    int start = 0, pos;

    while (start < str.length()) {
        pos = str.indexOf(charQuote, start);
        if (pos != -1) {
            result += str.substring(start, pos) + F("&quot;");
            start = pos + 1;
        } else {
            result += str.substring(start);
            break;
        }
    }

    return result;
}

String ESPWebBase::tagInput(const String &type, const String &name, const String &value) {
    String result = FPSTR(inputTypeOpen);

    result += type;
    result += charQuote;
    if (name != strEmpty) {
        result += FPSTR(inputNameOpen);
        result += name;
        result += charQuote;
    }
    if (value != strEmpty) {
        result += FPSTR(inputValueOpen);
        result += ESPWebBase::escapeQuote(value);
        result += charQuote;
    }
    result += charGreater;

    return result;
}

String ESPWebBase::tagInput(const String &type, const String &name, const String &value, const String &extra) {
    String result = FPSTR(inputTypeOpen);

    result += type;
    result += charQuote;
    if (name != strEmpty) {
        result += FPSTR(inputNameOpen);
        result += name;
        result += charQuote;
    }
    if (value != strEmpty) {
        result += FPSTR(inputValueOpen);
        result += ESPWebBase::escapeQuote(value);
        result += charQuote;
    }
    result += charSpace;
    result += extra;
    result += charGreater;

    return result;
}

const char ESPWebBase::_signEEPROM[4] PROGMEM = {'#', 'E', 'S', 'P'};

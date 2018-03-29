#ifndef __ESPWEB_H
#define __ESPWEB_H

#include <ESP8266WebServer.h>

//#define NOSERIAL // Раскомментируйте это макроопределение, чтобы не использовать отладочный вывод в Serial (можно будет использовать пины RX и TX после загрузки скетча для полезной нагрузки)
//#define NOBLED // Раскомментируйте это макроопределение, чтобы не использовать мигание встроенного светодиода (можно будет использовать пин LED_BUILTIN для полезной нагрузки)
//#define NOEEPROMERASE // Раскомментируйте это макроопределение, чтобы не использовать возможность стирания EEPROM замыканием A0 на VCC при старте

// Односимвольные константы
const char charCR = '\r';
const char charLF = '\n';
const char charSlash = '/';
const char charHash = '#';
const char charSpace = ' ';
const char charDot = '.';
const char charComma = ',';
const char charColon = ':';
const char charSemicolon = ';';
const char charQuote = '"';
const char charApostroph = '\'';
const char charOpenBrace = '{';
const char charCloseBrace = '}';
const char charEqual = '=';
const char charLess = '<';
const char charGreater = '>';

const char *const strEmpty = "";
const char *const strSlash = "/";

const char *const defSSID PROGMEM = "ESP8266"; // Имя точки доступа по умолчанию
const char *const defPassword PROGMEM = "Pa$$w0rd"; // Пароль точки доступа по умолчанию
const char *const defNtpServer PROGMEM = "pool.ntp.org"; // NTP-сервер по умолчанию
const int8_t defNtpTimeZone = 3; // Временная зона по умолчанию (-11..13, +3 - Москва)
const uint32_t defNtpUpdateInterval = 3600000; // Интервал в миллисекундах для обновления времени с NTP-серверов (по умолчанию 1 час)

const char *const pathSPIFFS PROGMEM = "/spiffs"; // Путь до страницы просмотра содержимого SPIFFS
const char *const pathUpdate PROGMEM = "/update"; // Путь до страницы OTA-обновления
const char *const pathWiFi PROGMEM = "/wifi"; // Путь до страницы конфигурации параметров беспроводной сети
const char *const pathTime PROGMEM = "/time"; // Путь до страницы конфигурации параметров времени
const char *const pathGetTime PROGMEM = "/gettime"; // Путь до страницы получения JSON-пакета времени
const char *const pathSetTime PROGMEM = "/settime"; // Путь до страницы ручной установки времени
const char *const pathStore PROGMEM = "/store"; // Путь до страницы сохранения параметров
const char *const pathReboot PROGMEM = "/reboot"; // Путь до страницы перезагрузки
const char *const pathData PROGMEM = "/data"; // Путь до страницы получения JSON-пакета данных

const char *const textPlain PROGMEM = "text/plain";
const char *const textHtml PROGMEM = "text/html";
const char *const textJson PROGMEM = "text/json";

const char *const fileNotFound PROGMEM = "FileNotFound";
const char *const indexHtml PROGMEM = "index.html";

const char *const headerTitleOpen PROGMEM = "<!DOCTYPE html>\n"
        "<html>\n"
        "<head>\n"
        "<title>";
const char *const headerTitleClose PROGMEM = "</title>\n";
const char *const headerStyleOpen PROGMEM = "<style type=\"text/css\">\n";
const char *const headerStyleClose PROGMEM = "</style>\n";
const char *const headerStyleExtOpen PROGMEM = "<link rel=\"stylesheet\" href=\"";
const char *const headerStyleExtClose PROGMEM = "\">\n";
const char *const headerScriptOpen PROGMEM = "<script type=\"text/javascript\">\n";
const char *const headerScriptClose PROGMEM = "</script>\n";
const char *const headerScriptExtOpen PROGMEM = "<script type=\"text/javascript\" src=\"";
const char *const headerScriptExtClose PROGMEM = "\"></script>\n";
const char *const headerBodyOpen PROGMEM = "</head>\n"
        "<body";
const char *const footerBodyClose PROGMEM = "</body>\n"
        "</html>";
const char *const getXmlHttpRequest PROGMEM = "function getXmlHttpRequest() {\n"
        "var xmlhttp;\n"
        "try {\n"
        "xmlhttp = new ActiveXObject(\"Msxml2.XMLHTTP\");\n"
        "} catch (e) {\n"
        "try {\n"
        "xmlhttp = new ActiveXObject(\"Microsoft.XMLHTTP\");\n"
        "} catch (E) {\n"
        "xmlhttp = false;\n"
        "}\n"
        "}\n"
        "if ((! xmlhttp) && (typeof XMLHttpRequest != 'undefined')) {\n"
        "xmlhttp = new XMLHttpRequest();\n"
        "}\n"
        "return xmlhttp;\n"
        "}\n";
const char *const inputTypeOpen PROGMEM = "<input type=\"";
const char *const inputNameOpen PROGMEM = " name=\"";
const char *const inputValueOpen PROGMEM = " value=\"";
const char *const simpleTagClose PROGMEM = " />";
const char *const typeText PROGMEM = "text";
const char *const typePassword PROGMEM = "password";
const char *const typeRadio PROGMEM = "radio";
const char *const typeCheckbox PROGMEM = "checkbox";
const char *const typeButton PROGMEM = "button";
const char *const typeSubmit PROGMEM = "submit";
const char *const typeReset PROGMEM = "reset";
const char *const typeHidden PROGMEM = "hidden";
const char *const typeFile PROGMEM = "file";
const char *const extraChecked PROGMEM = "checked";
const char *const getElementById PROGMEM = "document.getElementById('";

// Имена JSON-переменных
const char *const jsonFreeHeap PROGMEM = "freeheap";
const char *const jsonUptime PROGMEM = "uptime";
const char *const jsonUnixTime PROGMEM = "unixtime";
const char *const jsonDate PROGMEM = "date";
const char *const jsonTime PROGMEM = "time";
const char *const jsonTemp PROGMEM = "temp";
const char *const jsonSWver PROGMEM = "swver";
const char *const jsonUid PROGMEM = "uid";

const char bools[2][6] PROGMEM = {"false", "true"};

// Имена параметров для Web-форм
const char *const paramApMode PROGMEM = "apmode";
const char *const paramSSID PROGMEM = "ssid";
const char *const paramPassword PROGMEM = "password";
const char *const paramDomain PROGMEM = "domain";
const char *const paramNtpServer1 PROGMEM = "ntpserver1";
const char *const paramNtpServer2 PROGMEM = "ntpserver2";
const char *const paramNtpServer3 PROGMEM = "ntpserver3";
const char *const paramNtpTimeZone PROGMEM = "ntptimezone";
const char *const paramNtpUpdateInterval PROGMEM = "ntpupdateinterval";
const char *const paramTime PROGMEM = "time";
const char *const paramReboot PROGMEM = "reboot";

const uint16_t maxStringLen = 32; // Максимальная длина строковых параметров в Web-интерфейсе

class ESPWebBase { // Базовый класс
public:
    ESPWebBase();

    virtual void setup(); // Метод должен быть вызван из функции setup() скетча
    virtual void loop(); // Метод должен быть вызван из функции loop() скетча

    ESP8266WebServer *httpServer; // Web-сервер
protected:
    virtual void setupExtra(); // Дополнительный код инициализации
    virtual void loopExtra(); // Дополнительный код главного цикла

    static uint16_t readEEPROMString(uint16_t offset, String &str,
                                     uint16_t len); // Чтение строкового параметра из EEPROM, при успехе возвращает смещение следующего параметра
    static uint16_t writeEEPROMString(uint16_t offset, const String &str,
                                      uint16_t len); // Запись строкового параметра в EEPROM, возвращает смещение следующего параметра
    static uint8_t crc8EEPROM(uint16_t start, uint16_t end); // Вычисление 8-ми битной контрольной суммы участка EEPROM

    virtual uint16_t readConfig(); // Чтение конфигурационных параметров из EEPROM
    virtual uint16_t writeConfig(bool commit = true); // Запись конфигурационных параметров в EEPROM
    virtual void commitConfig(); // Подтверждение сохранения EEPROM
    virtual void defaultConfig(uint8_t level = 0); // Установление параметров в значения по умолчанию
    virtual bool setConfigParam(const String &name, const String &value); // Присвоение значений параметрам по их имени

    virtual bool setupWiFiAsStation(); // Настройка модуля в режиме инфраструктуры
    virtual void setupWiFiAsAP(); // Настройка модуля в режиме точки доступа
    virtual void
    setupWiFi(); // Попытка настройки модуля в заданный параметрами режим, при неудаче принудительный переход в режим точки доступа
    virtual void onWiFiConnected(); // Вызывается после активации беспроводной сети

    virtual void
    waitingWiFi(); // Ожидание подключения к инфраструктуре (можно использовать для визуализации процесса ожидания)
    virtual void waitedWiFi(); // Окончание ожидания подключения к инфраструктуре

    virtual uint32_t
    getTime(); // Возвращает время в формате UNIX-time с учетом часового пояса или 0, если ни разу не удалось получить точное время
    virtual void setTime(uint32_t now); // Ручная установка времени в формате UNIX-time

    virtual void
    setupHttpServer(); // Настройка Web-сервера (переопределяется для добавления обработчиков новых страниц)
    virtual void handleNotFound(); // Обработчик несуществующей страницы
    virtual void handleRootPage(); // Обработчик главной страницы
    virtual void handleFileUploaded(); // Обработчик страницы окончания загрузки файла в SPIFFS
    virtual void handleFileUpload(); // Обработчик страницы загрузки файла в SPIFFS
    virtual void handleFileDelete(); // Обработчик страницы удаления файла из SPIFFS
    virtual void handleSPIFFS(); // Обработчик страницы просмотра списка файлов в SPIFFS
    virtual void handleUpdate(); // Обработчик страницы выбора файла для OTA-обновления скетча
    virtual void handleSketchUpdated(); // Обработчик страницы окончания OTA-обновления скетча
    virtual void handleSketchUpdate(); // Обработчик страницы OTA-обновления скетча
    virtual void handleWiFiConfig(); // Обработчик страницы настройки параметров беспроводной сети
    virtual void handleTimeConfig(); // Обработчик страницы настройки параметров времени
    virtual void handleStoreConfig(); // Обработчик страницы сохранения параметров
    virtual void handleReboot(); // Обработчик страницы перезагрузки модуля
    virtual void handleGetTime(); // Обработчик страницы, возвращающей JSON-пакет времени
    virtual void handleSetTime(); // Обработчик страницы ручной установки времени
    virtual void handleData(); // Обработчик страницы, возвращающей JSON-пакет данных
    virtual String jsonData(); // Формирование JSON-пакета данных

    virtual String btnBack(); // HTML-код кнопки "назад" для интерфейса
    virtual String btnWiFiConfig(); // HTML-код кнопки настройки параметров беспроводной сети
    virtual String btnTimeConfig(); // HTML-код кнопки настройки параметров времени
    virtual String btnReboot(); // HTML-код кнопки перезагрузки
    virtual String navigator(); // HTML-код кнопок интерфейса главной страницы

    virtual String getContentType(const String &fileName); // MIME-тип фала по его расширению
    virtual bool handleFileRead(const String &path); // Чтение файла из SPIFFS

    static String webPageStart(const String &title); // HTML-код заголовка Web-страницы
    static String webPageStyle(const String &style, bool file = false); // HTML-код стилевого блока или файла
    static String webPageScript(const String &script, bool file = false); // HTML-код скриптового блока или файла
    static String webPageBody(); // HTML-код заголовка тела страницы
    static String webPageBody(const String &extra); // HTML-код заголовка тела страницы с дополнительными параметрами
    static String webPageEnd(); // HTML-код завершения Web-страницы
    static String escapeQuote(const String &str); // Экранирование двойных кавычек для строковых значений в Web-формах
    static String tagInput(const String &type, const String &name, const String &value); // HTML-код для тэга INPUT
    static String tagInput(const String &type, const String &name, const String &value,
                           const String &extra); // HTML-код для тэга INPUT с дополнительными параметрами

    bool _apMode; // Режим точки доступа (true) или инфраструктуры (false)
    String _ssid; // Имя сети или точки доступа
    String _password; // Пароль сети
    String _domain; // mDNS домен
    String _ntpServer1; // NTP-серверы
    String _ntpServer2;
    String _ntpServer3;
    int8_t _ntpTimeZone; // Временная зона (в часах от UTC)
    uint32_t _ntpUpdateInterval; // Период в миллисекундах для обновления времени с NTP-серверов
    static const char _signEEPROM[4] PROGMEM; // Сигнатура в начале EEPROM для определения, что параметры имеет смысл пытаться прочитать
private:
    uint32_t _lastNtpTime; // Последнее полученное от NTP-серверов время в формате UNIX-time
    uint32_t _lastNtpUpdate; // Значение millis() в момент последней синхронизации времени
};

#endif

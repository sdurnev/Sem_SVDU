#ifndef __ESPWEBMQTT_H
#define __ESPWEBMQTT_H

#include "ESPWeb.h"
#include <PubSubClient.h>
#include <ESP8266WiFi.h>

const char *const defMQTTClient PROGMEM = "ESP8266"; // Имя клиента для MQTT-брокера по умолчанию
const uint16_t defMQTTPort = 1883; // Порт MQTT-брокера по умолчанию

const char *const pathMQTT PROGMEM = "/mqtt"; // Путь до страницы конфигурации параметров MQTT

// Имена JSON-переменных
const char *const jsonMQTTConnected PROGMEM = "mqttconnected";

// Имена параметров для Web-форм
const char *const paramMQTTServer PROGMEM = "mqttserver";
const char *const paramMQTTPort PROGMEM = "mqttport";
const char *const paramMQTTUser PROGMEM = "mqttuser";
const char *const paramMQTTPassword PROGMEM = "mqttpswd";
const char *const paramMQTTClient PROGMEM = "mqttclient";

class ESPWebMQTTBase : public ESPWebBase { // Расширение базового класса с поддержкой MQTT
public:
    ESPWebMQTTBase();

    PubSubClient *pubSubClient; // Клиент MQTT-брокера
protected:
    void setupExtra();

    void loopExtra();

    uint16_t readConfig();

    uint16_t writeConfig(bool commit = true);

    void defaultConfig(uint8_t level = 0);

    bool setConfigParam(const String &name, const String &value);

    void setupHttpServer();

    void handleRootPage();

    virtual void handleMQTTConfig(); // Обработчик страницы настройки параметров MQTT
    String jsonData(); // Формирование JSON-пакета данных

    virtual String btnMQTTConfig(); // HTML-код кнопки параметров MQTT
    String navigator();

    virtual bool mqttReconnect(); // Восстановление соединения с MQTT-брокером
    virtual void mqttCallback(char *topic, byte *payload,
                              unsigned int length); // Callback-функция, вызываемая MQTT-брокером при получении топика, на которое оформлена подписка
    virtual void mqttResubscribe(); // Осуществление подписки на топики
    bool mqttSubscribe(const String &topic); // Хэлпер для подписки на топик
    bool mqttPublish(const String &topic, const String &value); // Хэлпер для публикации топика

    virtual void
    waitingMQTT(); // Ожидание подключения к MQTT-брокеру (можно использовать для визуализации процесса ожидания)
    virtual void waitedMQTT(); // Окончание ожидания подключения к MQTT-брокеру

    String _mqttServer; // MQTT-брокер
    uint16_t _mqttPort; // Порт MQTT-брокера
    String _mqttUser; // Имя пользователя для авторизации
    String _mqttPassword; // Пароль для авторизации
    String _mqttClient; // Имя клиента для MQTT-брокера (используется при формировании имени топика для публикации в целях различия между несколькими клиентами с идентичным скетчем)
    WiFiClient *_espClient;
};

#endif

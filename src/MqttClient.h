#ifndef CPP_MQTT_CLIENT_HPP
#define CPP_MQTT_CLIENT_HPP

#include <functional>
#include <vector>
#include <string>
#include <map>
#include "mqtt_client.h"
#include <memory>

class MqttSubscribeData
{
public:
    std::string topic;
    std::string data;

    MqttSubscribeData(const std::string &topic, const std::string &data)
        : topic(topic), data(data)
    {
    }

    MqttSubscribeData(std::string &&topic, std::string &&data)
        : topic(std::move(topic)), data(std::move(data))
    {
    }
};

using MqttSubscribeCallback = std::function<void(std::shared_ptr<MqttSubscribeData>)>;


class SubscribeMapData
{
public:
    std::string topic;
    int qos;
    MqttSubscribeCallback callback;

    SubscribeMapData(const std::string &topic, int qos, MqttSubscribeCallback callback)
        : topic(topic), qos(qos), callback(callback)
    {
    }

    SubscribeMapData(std::string &&topic, int qos, MqttSubscribeCallback &&callback)
        : topic(std::move(topic)), qos(qos), callback(std::move(callback))
    {
    }
};

class MqttClient
{
public:
    MqttClient();
    void begin(const std::string &host, int port, const std::string &clientId);
    void subscribe(std::string topic, int qos, MqttSubscribeCallback callback);
    void publish(const std::string topic, const std::string message, int qos, bool retain);

    void handleSubscriptionData(const esp_mqtt_event_handle_t event);

    using topic_callback_map_t = std::map<std::string, SubscribeMapData>;
    topic_callback_map_t* getTopicCallbackMap()
    {
        return &topic_cb_map_;
    }

private:
    std::string host_;
    int port_;
    std::string clientId_;

    topic_callback_map_t topic_cb_map_;

    esp_mqtt_client_handle_t client_;
};

extern MqttClient Mqtt;

#endif
#include "MqttClient.h"
#include "esp_event.h"
#include "esp_system.h"
#include "mqtt_client.h"

#if defined(ARDUINO)
#include <Arduino.h>

#define ESP_MQTT_CLIENT_LOGE(...) log_e(__VA_ARGS__)
#define ESP_MQTT_CLIENT_LOGI(...) log_i(__VA_ARGS__)
#define ESP_MQTT_CLIENT_LOGD(...) log_d(__VA_ARGS__)
#define ESP_MQTT_CLIENT_LOGW(...) log_w(__VA_ARGS__)

#else

#include "esp_log.h"

#define TAG "DXESPMQTT"
#define ESP_MQTT_CLIENT_LOGE(...) ESP_LOGE(TAG, __VA_ARGS__)
#define ESP_MQTT_CLIENT_LOGI(...) ESP_LOGI(TAG, __VA_ARGS__)
#define ESP_MQTT_CLIENT_LOGD(...) ESP_LOGD(TAG, __VA_ARGS__)
#define ESP_MQTT_CLIENT_LOGW(...) ESP_LOGW(TAG, __VA_ARGS__)

#endif

static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0)
    {
        ESP_MQTT_CLIENT_LOGE("Last error %s: 0x%x", message, error_code);
    }
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    MqttClient *mqtt = static_cast<MqttClient *>(handler_args);

    ESP_MQTT_CLIENT_LOGD("Event dispatched from event loop base=%s, event_id=%d", base, event_id);

    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    esp_mqtt_client_handle_t client = event->client;

    int msg_id;

    switch ((esp_mqtt_event_id_t)event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_MQTT_CLIENT_LOGI("MQTT_EVENT_CONNECTED");

        for (auto &[topic, subscribeData] : *mqtt->getTopicCallbackMap())
        {
            esp_mqtt_client_subscribe(client, topic.c_str(), subscribeData.qos);
        }

        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_MQTT_CLIENT_LOGI("MQTT_EVENT_DISCONNECTED");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_MQTT_CLIENT_LOGI("MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_MQTT_CLIENT_LOGI("MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_PUBLISHED:
        ESP_MQTT_CLIENT_LOGI("MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_DATA:
        ESP_MQTT_CLIENT_LOGI("MQTT_EVENT_DATA");

        mqtt->handleSubscriptionData(event);

        break;

    case MQTT_EVENT_ERROR:
        ESP_MQTT_CLIENT_LOGI("MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT)
        {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno", event->error_handle->esp_transport_sock_errno);
            ESP_MQTT_CLIENT_LOGI("Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
        }
        break;
    default:
        ESP_MQTT_CLIENT_LOGI("Other event id: %d", event_id);
        break;
    }
}

MqttClient::MqttClient() {}

void MqttClient::subscribe(std::string topic, int qos, MqttSubscribeCallback callback)
{
    SubscribeMapData data(topic, qos, callback);
    topic_cb_map_.insert(std::make_pair(topic, data));
}

void MqttClient::publish(const std::string topic, const std::string message, int qos, bool retain)
{
    esp_mqtt_client_publish(client_, topic.c_str(), message.c_str(), message.length(), qos, retain);
}

void MqttClient::begin(const std::string &host, int port, const std::string &clientId)
{
    host_ = host;
    port_ = port;
    clientId_ = clientId;

    esp_mqtt_client_config_t mqtt_cfg = {};
    mqtt_cfg.host = host_.c_str();
    mqtt_cfg.port = port_;
    mqtt_cfg.client_id = clientId_.c_str();
    mqtt_cfg.buffer_size = 4096;

    client_ = esp_mqtt_client_init(&mqtt_cfg);
    start();
}

void MqttClient::begin(const std::string &uri, const std::string &clientId, const char *cert)
{
    uri_ = uri;
    esp_mqtt_client_config_t mqtt_cfg = {};

    if (clientId.length() < 1)
    {
        mqtt_cfg.set_null_client_id = true;
    }
    else
    {
        mqtt_cfg.set_null_client_id = false;
        mqtt_cfg.client_id = clientId.c_str();
    }

    mqtt_cfg.uri = uri.c_str();

    if (cert != nullptr)
    {
        mqtt_cfg.cert_pem = cert;
    }

    client_ = esp_mqtt_client_init(&mqtt_cfg);
    start();
}

void MqttClient::start()
{
    esp_mqtt_client_register_event(client_, static_cast<esp_mqtt_event_id_t>(ESP_EVENT_ANY_ID), mqtt_event_handler, this);
    esp_mqtt_client_start(client_);
}

void MqttClient::handleSubscriptionData(const esp_mqtt_event_handle_t event)
{
    std::string topic(event->topic, event->topic_len);
    auto it = topic_cb_map_.find(topic);

    if (it == topic_cb_map_.end())
    {
        ESP_MQTT_CLIENT_LOGW("No callback for topic %s", topic.c_str());

        return;
    }

    SubscribeMapData subscribeMap = it->second;
    MqttSubscribeCallback callback = subscribeMap.callback;

    std::string data(event->data, event->data_len);
    auto subData = std::make_shared<MqttSubscribeData>(std::move(topic), std::move(data));

    callback(subData);
}

MqttClient Mqtt;
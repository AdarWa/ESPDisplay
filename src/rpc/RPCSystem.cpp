#include "RPCSystem.hpp"

// ---------------- RPCSystem ----------------

RPCSystem::RPCSystem(
    const char* ssid,
    const char* password,
    const char* mqtt_server,
    int mqtt_port,
    const char* mqtt_user,
    const char* mqtt_pass
)
  : ssid(ssid), password(password),
    mqtt_server(mqtt_server), mqtt_port(mqtt_port),
    mqtt_user(mqtt_user), mqtt_pass(mqtt_pass),
    mqtt(client), rpc(mqtt) {}

bool RPCSystem::initSPIFFS() {
  if (!SPIFFS.begin(true)) {
    Serial.println("Failed to mount SPIFFS");
    return false;
  }
  return true;
}

bool RPCSystem::connectWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (millis() - start > 20000) {
      Serial.println("WiFi connection failed");
      return false;
    }
  }
  Serial.println();
  Serial.print("Connected, IP: ");
  Serial.println(WiFi.localIP());
  return true;
}

bool RPCSystem::connectMQTT() {
  mqtt.setServer(mqtt_server, mqtt_port);
  Serial.print("Connecting to MQTT");
  unsigned long start = millis();
  while (!mqtt.connected()) {
    bool ok;
    if (mqtt_user && mqtt_pass) ok = mqtt.connect("esp32client", mqtt_user, mqtt_pass);
    else ok = mqtt.connect("esp32client");
    if (ok) {
      Serial.println("MQTT connected");
      break;
    }
    delay(500);
    Serial.print(".");
    if (millis() - start > 10000) {
      Serial.println("MQTT connection failed");
      return false;
    }
  }
  return true;
}

bool RPCSystem::begin() {
  if (!initSPIFFS()) return false;
  if (!connectWiFi()) return false;
  if (!connectMQTT()) return false;
  return rpc.begin();
}

// ---------------- ESP32RPC ----------------

static ESP32RPC* s_rpc_instance = nullptr;

ESP32RPC::ESP32RPC(PubSubClient &client, const String &uuid_file)
  : mqtt(client), uuid_file(uuid_file) {
  s_rpc_instance = this;
}

bool ESP32RPC::begin() {
  mqtt.setCallback(&ESP32RPC::mqttThunk);
  if (!loadUUID()) {
    if (!requestUUID()) {
      Serial.println("subscribe handshake failed");
      return false;
    }
    if (!saveUUID()) {
      Serial.println("failed to save UUID to SPIFFS");
    }
  }

  mqtt.subscribe(topicServer().c_str());

  Serial.print("ESP32RPC ready, uuid=");
  Serial.println(uuid);
  return true;
}

void ESP32RPC::loop() {
  if (!mqtt.connected()) return;
  mqtt.loop();
}

// --------- UUID storage ---------

bool ESP32RPC::loadUUID() {
  File f = SPIFFS.open(uuid_file, "r");
  if (!f) return false;
  String s = f.readString();
  f.close();
  s.trim();
  if (s.length() == 0) return false;
  uuid = s.toInt();
  return uuid >= 0;
}

bool ESP32RPC::saveUUID() {
  File f = SPIFFS.open(uuid_file, "w");
  if (!f) return false;
  f.printf("%d", uuid);
  f.close();
  return true;
}

// --------- handshake with server ---------

bool ESP32RPC::requestUUID() {
  String req_id = newId();

  JsonDocument doc;
  doc["request_id"] = req_id;
  doc["request_type"] = "subscribe";

  String payload;
  serializeJson(doc, payload);
  String rx_topic = "espdisplay/broadcast";
  mqtt.subscribe(rx_topic.c_str());
  mqtt.loop(); // process any pending messages
  mqtt.publish("espdisplay/subscribe", payload.c_str());


  unsigned long start = millis();
  while (millis() - start < 5000) {
    mqtt.loop();
    if (uuid >= 0) {
      mqtt.unsubscribe(rx_topic.c_str());
      return true;
    }
    delay(5);
  }
  mqtt.unsubscribe(rx_topic.c_str());
  return false;
}

// --------- MQTT and topics ---------

String ESP32RPC::topicServer() const {
  String s = "espdisplay/";
  s += String(uuid);
  s += "/server";
  return s;
}

String ESP32RPC::topicClient() const {
  String s = "espdisplay/";
  s += String(uuid);
  s += "/client";
  return s;
}

void ESP32RPC::mqttThunk(char* topic, byte* payload, unsigned int length) {
  Serial.println("MQTT message received");
  if (!s_rpc_instance) return;
  s_rpc_instance->onMQTT(topic, payload, length);
}

void ESP32RPC::onMQTT(char* topic, byte* payload, unsigned int length) {
  String t(topic);
  if (t == topicClient()) {
    return;
  }
  Serial.print("Got MQTT message on topic: ");
  Serial.println(t);
  String s;
  s.reserve(length + 1);
  for (unsigned int i = 0; i < length; i++) s += (char)payload[i];

  if (t == "espdisplay/broadcast") {
    Serial.print("Got broadcast message: ");
    Serial.println(s);
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, s);
    if (err) return;
    const char* type = doc["request_type"] | "";
    const char* rid  = doc["request_id"] | "";
    // print request_type and request_id for debugging
    Serial.print("request_type: ");
    Serial.println(type);
    Serial.print("request_id: ");
    Serial.println(rid);
    if (strcmp(type, "subscribe_reply") == 0 && strlen(rid) > 0) {
      uuid = doc["uuid"] | -1;
      if (uuid >= 0) {
        Serial.print("Got UUID from server: ");
        Serial.println(uuid);
      }
    }
    return;
  }

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, s);
  if (err) return;
  handleIncomingJSON(t, doc);
}

// --------- JSON-RPC helpers ---------

String ESP32RPC::newId() {
  char buf[37];
  for (int i = 0; i < 36; i++) buf[i] = "abcdef0123456789"[esp_random() % 16];
  buf[36] = 0;
  return String(buf);
}

void ESP32RPC::sendJSON(const String &topic, JsonDocument const &doc) {
  String out;
  serializeJson(doc, out);
  mqtt.publish(topic.c_str(), out.c_str());
}

void ESP32RPC::handleIncomingJSON(const String &topic, JsonDocument const &doc) {
  if (doc["method"].is<JsonVariantConst>()) {
    handleIncomingRequest(doc);
  } else if (doc["result"].is<JsonVariantConst>()) {
    handleIncomingResponse(doc);
  } else if (doc["error"].is<JsonVariantConst>()) {
    handleIncomingResponse(doc);
  }
}

void ESP32RPC::handleIncomingRequest(JsonDocument const &doc) {
  String method = doc["method"] | "";
  JsonVariantConst params = doc["params"];
  String id = doc["id"] | "";

  auto it = methods.find(method);
  JsonDocument reply;

  reply["jsonrpc"] = "2.0";
  if (it == methods.end()) {
    JsonObject err = reply["error"].to<JsonObject>();
    err["code"] = -32601;
    err["message"] = "Unknown method";
    reply["id"] = id;
  } else {
    JsonVariant result = it->second(params);
    reply["result"] = result;
    reply["id"] = id;
  }

  sendJSON(topicClient(), reply);
}

void ESP32RPC::handleIncomingResponse(JsonDocument const &doc) {
  String id = doc["id"] | "";
  auto it = pending.find(id);
  if (it == pending.end()) return;
  Pending* p = it->second;
  p->doc.clear();
  for (JsonPairConst kv : doc.as<JsonObjectConst>()) {
    p->doc[kv.key()] = kv.value();
  }
  p->done = true;
}

// --------- device makes JSON-RPC call to server ---------

JsonDocument ESP32RPC::call(const String &method, JsonVariantConst params, unsigned long timeout) {
  String id = newId();
  JsonDocument req;
  req["jsonrpc"] = "2.0";
  req["method"] = method;
  if (!params.isNull()) req["params"] = params;
  req["id"] = id;

  Pending* p = new Pending();
  pending[id] = p;

  sendJSON(topicClient(), req);

  unsigned long start = millis();
  while (millis() - start < timeout) {
    mqtt.loop();
    if (p->done) break;
    delay(5);
  }

  JsonDocument out; // make a deep copy of result
  if (p->done) {
    if (p->doc["result"].is<JsonVariantConst>()) {
      out.set(p->doc["result"]);
    } else if (p->doc["error"].is<JsonVariantConst>()) {
      // leave out empty to signal error
    }
  } else {
    // timeout: leave out empty
  }

  pending.erase(id);
  delete p;
  return out;
}

void ESP32RPC::registerMethod(const String &name, Callback cb) {
  methods[name] = cb;
}

#pragma once
#include "base/StateModule.h"
#include "ha_helper.h"  // For MQTT fan_light, fan_reverse, fan_speed, fan_timer

class FanStateModule : public StateModule {
public:
    FanStateModule(const char* filepath = "/fan.json")
        : StateModule(filepath),
          current_level(-1),
          light_power(false),
          fan_dir(false),
          timer_epoch(0) {}

    // Serialize fan state to JSON
    void serialize(StaticJsonDocument<512>& doc) override {
        doc["current_level"] = current_level;
        doc["light_power"] = light_power;
        doc["fan_dir"] = fan_dir;
        doc["timer_epoch"] = timer_epoch;
    }

    // Deserialize fan state from JSON
    void deserialize(StaticJsonDocument<512>& doc) override {
        current_level = doc["current_level"] | current_level;
        light_power = doc["light_power"] | light_power;
        fan_dir = doc["fan_dir"] | fan_dir;
        timer_epoch = doc["timer_epoch"] | timer_epoch;
    }

    // Publish state to MQTT & save
    void publish_update() override {
        save();
        fan_light.setState(light_power);
        fan_reverse.setState(fan_dir);
        fan_speed.setValue((float)current_level + 1);
        fan_timer.setValue((float)timer_epoch);
    }

    // Fan state getters/setters that publish on change
    void set_light_power(bool val) {
        if (light_power != val) {
            light_power = val;
            publish_update();
        }
    }

    void set_fan_dir(bool val) {
        if (fan_dir != val) {
            fan_dir = val;
            publish_update();
        }
    }

    void set_current_level(int16_t val) {
        if (current_level != val) {
            current_level = val;
            publish_update();
        }
    }

    void set_timer_epoch(unsigned long val) {
        if (timer_epoch != val) {
            timer_epoch = val;
            publish_update();
        }
    }

    // Getters
    int16_t get_current_level() const { return current_level; }
    bool get_light_power() const { return light_power; }
    bool get_fan_dir() const { return fan_dir; }
    unsigned long get_timer_epoch() const { return timer_epoch; }

    static constexpr unsigned long MAX_TIMER = 60*60*23 + 55*60;

    static bool is_timer_running(unsigned long timer_epoch) {
        return timer_epoch - getCurrentEpochTime() < MAX_TIMER;
    }

private:
    int16_t current_level;
    bool light_power;
    bool fan_dir;
    unsigned long timer_epoch;
};

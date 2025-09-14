#pragma once
#include <ArduinoJson.h>
#include <lvgl.h>
#include <map>
#include <vector>
#include <functional>
#include <memory>
// Avoid including component implementations here to prevent circular dependencies.
// Components should include this header to access interfaces and context types.

// Forward declaration so it can be used in pointers below
class ScreenRenderer;

struct CompCtx {
    String comp_id;
    String type;
    JsonVariantConst params; // read-only JSON input
};

struct BackCbData {
    ScreenRenderer* self;
    char* target;
};

class IComponent {
public:
    virtual ~IComponent() = default;
    virtual lv_obj_t* build(lv_obj_t* parent, const CompCtx& ctx) = 0;
};

class ComponentRegistry {
public:
    using Factory = std::function<IComponent*()>;

    static ComponentRegistry& instance() {
        static ComponentRegistry r;
        return r;
    }

    void registerType(const String& type, Factory f) { map_[type] = f; }

    IComponent* create(const String& type) {
        auto it = map_.find(type);
        if (it == map_.end()) return nullptr;
        return it->second();
    }

private:
    std::map<String, Factory> map_;
};

class ScreenRenderer {
public:
    void buildFromConfig(JsonVariantConst cfg); // read-only JSON
    void showScreenById(const String& scr_id);

private:
    struct ScreenInfo {
        String scr_id;
        String name;
        String back_screen;
        lv_obj_t* root = nullptr;
    };

    std::map<String, ScreenInfo> screens;

    void ensureRegistrySetup();
};

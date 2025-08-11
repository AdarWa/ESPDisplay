#pragma once
#include "Entity.h"
#include "component/Component.h"
#include "component/Module.h"
#include <vector>
#include <memory>
#include <string>
#include <lvgl.h>

class Screen : public Entity {
protected:
    std::string name_;
    std::string backScreen_; // optional back screen
    std::vector<std::shared_ptr<Component>> components_;
    std::vector<std::shared_ptr<Module>> modules_;
    lv_obj_t* parent;

public:
    Screen(const std::string& name, std::string backScreen = "");
    virtual ~Screen() = default;

    const std::string& getName() const;
    std::string getBackScreen() const;
    void setBackScreen(std::string backName);

    void addComponent(std::shared_ptr<Component> comp);
    void addModule(std::shared_ptr<Module> mod);

    const std::vector<std::shared_ptr<Component>>& getComponents() const;
    const std::vector<std::shared_ptr<Module>>& getModules() const;

    void onLoad() override;
    void onUpdate() override;
    void onUnload() override;

    virtual lv_obj_t* render();
};
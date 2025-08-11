#pragma once
#include "../Entity.h"
#include "../states/States.h"
#include "../states/Actions.h"
#include <memory>
#include <string>
#include "utils/stdutils.h"
#include <lvgl.h>

class Component : public Entity {
protected:
    std::string uuid_;
    std::unique_ptr<States> states_;
    std::unique_ptr<Actions> actions_;

public:
    Component(const std::string& uuid);
    virtual ~Component() = default;

    const std::string& getUUID() const;
    States* getStates() const;
    Actions* getActions() const;
    
    virtual lv_obj_t* render(lv_obj_t* parent);
    
    void onLoad() override;
    void onUpdate() override;
    void onUnload() override;
};

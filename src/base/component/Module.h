#pragma once
#include "../Entity.h"
#include "../states/States.h"
#include "../states/Actions.h"
#include "utils/stdutils.h"
#include <memory>

class Module : public Entity {
protected:
    std::unique_ptr<States> states_;
    std::unique_ptr<Actions> actions_;

public:
    Module();
    virtual ~Module() = default;

    States* getStates() const;
    Actions* getActions() const;

    virtual void config();

    void onLoad() override;
    void onUpdate() override;
    void onUnload() override;
};

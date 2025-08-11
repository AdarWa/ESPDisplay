#pragma once

class Entity {
public:
    virtual ~Entity() = default;

    virtual void onLoad() = 0;
    virtual void onUpdate() = 0;
    virtual void onUnload() = 0;
};

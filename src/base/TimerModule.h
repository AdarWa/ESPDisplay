/*
#pragma once
#include <functional>
#include <ha_helper.h>

class TimerModule {
public:
    TimerModule(unsigned long max_duration_sec)
        : _timer_epoch(0), _max_duration(max_duration_sec), _callback(nullptr) {}

    void set_timer_epoch(unsigned long epoch) {
        _timer_epoch = epoch;
        notify_change();
    }

    unsigned long get_timer_epoch() const {
        return _timer_epoch;
    }

    bool is_running() const {
        unsigned long now = getCurrentEpochTime();
        return _timer_epoch > now && (_timer_epoch - now) < _max_duration;
    }

    unsigned long time_left_sec() const {
        unsigned long now = getCurrentEpochTime();
        if (!is_running()) return 0;
        return _timer_epoch - now;
    }

    void cancel() {
        _timer_epoch = 0;
        notify_change();
    }

    // Set a callback for when timer changes (e.g. UI update)
    void set_change_callback(std::function<void()> cb) {
        _callback = cb;
    }

private:
    void notify_change() {
        if (_callback) {
            _callback();
        }
    }

    unsigned long _timer_epoch;
    unsigned long _max_duration;
    std::function<void()> _callback;
};
*/
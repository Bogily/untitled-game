#pragma once
#include <RmlUi/Core.h>
#include <functional>

class GameEventListener : public Rml::EventListener
{
public:
    using Callback = std::function<void(Rml::Event &)>;

    GameEventListener(Callback callback) : callback(callback) {}

    void ProcessEvent(Rml::Event &event) override
    {
        if (callback)
            callback(event);
    }

    void OnDetach(Rml::Element * /*element*/) override
    {
        delete this;
    }

private:
    Callback callback;
};

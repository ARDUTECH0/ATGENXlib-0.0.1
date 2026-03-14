#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>

namespace atg
{

    struct UiWidget
    {
        enum Type
        {
            Switch,
            Indicator,
            Gauge,
            Button
        } type;

        String label;

        static UiWidget SwitchW(const String &l) { return {Switch, l}; }
        static UiWidget IndicatorW(const String &l) { return {Indicator, l}; }
        static UiWidget GaugeW(const String &l) { return {Gauge, l}; }
        static UiWidget ButtonW(const String &l) { return {Button, l}; }
    };

    // ✅ generic callback for automatic state broadcasting
    using DeviceStateChangedCb = void (*)(const char *deviceId, void *user);

    class IDevice
    {
    public:
        virtual ~IDevice() {}
        virtual const char *id() const = 0;
        virtual const char *type() const = 0;

        // Called periodically (read sensors, update state, etc.)
        virtual void loop() = 0;

        // Fill JSON with current state. Example: {"isOn":true} / {"motion":false} / {"temp":25.2}
        virtual void getStateJson(JsonObject out) = 0;

        // Handle command JSON. Example: {"cmd":"set","value":true}
        virtual void handleCmdJson(JsonObject cmd, bool &handled) = 0;

        // Capabilities keys (manifest) – optional but recommended
        virtual void getCapsJson(JsonArray out) = 0;

        // ✅ set internal state-change callback (used by hub/library only)
        void setStateChangedCallback(DeviceStateChangedCb cb, void *user = nullptr)
        {
            _stateChangedCb = cb;
            _stateChangedUser = user;
        }

    protected:
        // ✅ call this from any device when its state changes
        void notifyStateChanged()
        {
            if (_stateChangedCb)
            {
                _stateChangedCb(id(), _stateChangedUser);
            }
        }

    private:
        DeviceStateChangedCb _stateChangedCb = nullptr;
        void *_stateChangedUser = nullptr;
    };

    class DeviceRegistry
    {
    public:
        struct Entry
        {
            IDevice *dev;
            UiWidget widget;
        };

        DeviceRegistry();

        bool add(IDevice &dev, const UiWidget &widget);
        size_t count() const;
        const Entry *at(size_t i) const;
        IDevice *findById(const String &id);

    private:
        static constexpr size_t MAX_DEVICES = 16;
        Entry _items[MAX_DEVICES];
        size_t _count = 0;
    };

} // namespace atg
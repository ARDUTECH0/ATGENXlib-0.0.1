#include "ATG_DeviceRegistry.h"

namespace atg {

DeviceRegistry::DeviceRegistry() {}

bool DeviceRegistry::add(IDevice& dev, const UiWidget& widget) {
  if (_count >= MAX_DEVICES) return false;
  _items[_count++] = { &dev, widget };
  return true;
}

size_t DeviceRegistry::count() const { return _count; }

const DeviceRegistry::Entry* DeviceRegistry::at(size_t i) const {
  if (i >= _count) return nullptr;
  return &_items[i];
}

IDevice* DeviceRegistry::findById(const String& id) {
  for (size_t i = 0; i < _count; i++) {
    if (String(_items[i].dev->id()) == id) return _items[i].dev;
  }
  return nullptr;
}

} // namespace atg
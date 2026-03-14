#pragma once
namespace atg {

using StateChangedCb = void (*)(const char* deviceId);

class IStateEmitter {
public:
  virtual ~IStateEmitter() = default;
  virtual void setStateChangedCb(StateChangedCb cb) = 0;
};

} // namespace atg
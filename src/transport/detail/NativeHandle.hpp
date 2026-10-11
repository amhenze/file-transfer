#pragma once

class NativeHandle {
public:
  virtual ~NativeHandle() = default;
  virtual int getNativeHandle() const noexcept = 0;
};

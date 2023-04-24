// Copyright 2022 The Khronos Group
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "BaseObject.h"

namespace helium {

struct BaseArray : public BaseObject
{
  BaseArray(ANARIDataType type, BaseGlobalDeviceState *s);

  // Implement anariMapArray()
  virtual void *map() = 0;

  // Implement anariUnmapArray()
  virtual void unmap() = 0;

  // This is invoked when this object's public ref count is 0, but still has a
  // non-zero internal ref count. See README for additional explanation.
  virtual void privatize() = 0;
};

} // namespace helium

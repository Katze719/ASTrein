#pragma once

enum class SharedMode : int {
  kDefault = 0,
  kStrict = 1,
};

struct SharedConfig {
  SharedMode mode;
  int timeout_ms;
};

extern "C" int sharedTypeFirst(SharedConfig config, SharedMode mode);
extern "C" int sharedTypeSecond(SharedConfig config, SharedMode mode);

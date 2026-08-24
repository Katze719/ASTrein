#pragma once

using ErrorCallback = void (*)(int error_code, const char *message);

/// @brief Invoke a named callback.
/// @param[in] callback Callback to invoke.
/// @return Zero on success.
extern "C" int invoke(ErrorCallback callback = nullptr);

/// Invoke a callback declared directly on the function.
/// @param[in] callback Callback to invoke.
extern "C" void invoke_direct(void (*callback)(int status,
                                               const char *details));

using PartialCallback = void (*)(int, const char *message);

/// Invoke a callback whose first parameter is intentionally unnamed.
extern "C" void invoke_partial(PartialCallback callback);

typedef void (*LegacyCallback)(long event_id, void *user_data);

/// Invoke a callback declared with a C-style typedef.
extern "C" void invoke_legacy(LegacyCallback callback);

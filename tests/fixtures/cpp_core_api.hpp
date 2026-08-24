#pragma once

#include <string>

#define MODULE_API __attribute__((visibility("default")))

using ErrorCallbackT = void (*)(int error_code, const char *message);

extern "C" {

/**
 * @brief Write raw bytes to the serial port.
 *
 * The timeout for later bytes is @p timeout_ms multiplied by @p multiplier.
 *
 * @param[in] handle Port handle.
 * @param[in] buffer Data to transmit.
 * @param[in] buffer_size Number of bytes in @p buffer.
 * @param[in] timeout_ms Base timeout in milliseconds.
 * @param[in] multiplier Timeout multiplier.
 * @param[in] error_callback Optional callback invoked on error.
 * @return Bytes written or a negative error code.
 *
 * @code{.c}
 * serialWrite(handle, data, size, 50, 1);
 * @endcode
 */
MODULE_API int serialWrite(int handle, const void *buffer, int buffer_size,
                           int timeout_ms, int multiplier,
                           ErrorCallbackT error_callback = nullptr);

/**
 * @brief Register a callback invoked after bytes are read.
 * @param[in] callback_fn Function receiving the byte count.
 */
MODULE_API void serialSetReadCallback(void (*callback_fn)(int bytes_read));

void serialWithoutVisibility();
__attribute__((visibility("hidden"))) void serialHidden();

} // extern "C"

MODULE_API void cppLinkageHelper();

#pragma once

#include <stddef.h>

#ifdef _WIN32
#define MODULE_API __declspec(dllexport)
#else
#define MODULE_API __attribute__((visibility("default")))
#endif

using ErrorCallbackT = void (*)(int error_code, const char *message);

/** Default serial-port configuration. */
struct SerialDefaultConfig {
  int baud_rate = 115200; ///< Baud rate in bits per second.
};

struct SerialOpenConfig {
  /** Device path used to open the port. */
  const char *device;
};

/** Physical serial-line configuration. */
struct SerialLineConfig {
  int data_bits; ///< Number of data bits per frame.
  int stop_bits; ///< Number of stop bits per frame.
};

struct SerialValidationConfig {
  int baud_rate;         ///< Baud rate to validate.
  SerialLineConfig line; ///< Line configuration to validate.
};

extern "C" {

/** Return the default serial port configuration. */
MODULE_API SerialDefaultConfig serialDefaultConfig();

/** Open a serial port using @p config. */
MODULE_API int serialOpen(const SerialOpenConfig *config);

/** Validate @p config passed by value. */
MODULE_API int serialValidateConfig(SerialValidationConfig config);

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
#ifdef _WIN32
__declspec(dllimport) void serialImported();
#endif

} // extern "C"

MODULE_API void cppLinkageHelper();

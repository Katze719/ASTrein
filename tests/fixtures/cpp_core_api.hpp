#pragma once

#include <stddef.h>

#ifdef _WIN32
#define MODULE_API __declspec(dllexport)
#else
#define MODULE_API __attribute__((visibility("default")))
#endif

using ErrorCallbackT = void (*)(int error_code, const char *message);

/** Parity mode. */
enum class Parity : int {
  kNone = 0, ///< Disable parity.
  kEven = 1, ///< Use even parity.
  kOdd = 2,  ///< Use odd parity.
};

/** Stop-bit mode. */
enum class StopBits : int {
  kOne = 0, ///< Use one stop bit.
  kTwo = 2, ///< Use two stop bits.
};

/** Flow-control mode. */
enum class FlowControl : int {
  kNone = 0,    ///< Disable flow control.
  kRtsCts = 1,  ///< Use hardware RTS/CTS flow control.
  kXonXoff = 2, ///< Use software XON/XOFF flow control.
};

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
  Parity parity;         ///< Parity mode.
  StopBits stop_bits;    ///< Stop-bit mode.
  FlowControl flow_mode; ///< Flow-control mode.
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

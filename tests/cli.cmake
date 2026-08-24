execute_process(
  COMMAND "${ASTREIN}" --help
  RESULT_VARIABLE HELP_RESULT
  OUTPUT_VARIABLE HELP_OUTPUT
  ERROR_VARIABLE HELP_ERROR)
if(NOT HELP_RESULT EQUAL 0 OR NOT HELP_ERROR STREQUAL "")
  message(FATAL_ERROR
          "--help failed (${HELP_RESULT}); stderr: ${HELP_ERROR}")
endif()
foreach(EXPECTED
    "Usage: astrein [options] <translation-unit>"
    "-h, --help"
    "-V, --version"
    "--mode <mode>"
    "--require-c-linkage"
    "--require-default-visibility"
    "-p, --build-path <dir>"
    "Examples:")
  string(FIND "${HELP_OUTPUT}" "${EXPECTED}" FOUND)
  if(FOUND EQUAL -1)
    message(FATAL_ERROR "--help output is missing: ${EXPECTED}")
  endif()
endforeach()

execute_process(
  COMMAND "${ASTREIN}" --version
  RESULT_VARIABLE VERSION_RESULT
  OUTPUT_VARIABLE VERSION_OUTPUT
  ERROR_VARIABLE VERSION_ERROR)
if(NOT VERSION_RESULT EQUAL 0 OR NOT VERSION_ERROR STREQUAL "")
  message(FATAL_ERROR
          "--version failed (${VERSION_RESULT}); stderr: ${VERSION_ERROR}")
endif()
foreach(EXPECTED "ASTrein " "Clang ")
  string(FIND "${VERSION_OUTPUT}" "${EXPECTED}" FOUND)
  if(FOUND EQUAL -1)
    message(FATAL_ERROR "--version output is missing: ${EXPECTED}")
  endif()
endforeach()

execute_process(
  COMMAND "${ASTREIN}" --unknown-option
  RESULT_VARIABLE UNKNOWN_RESULT
  OUTPUT_VARIABLE UNKNOWN_OUTPUT
  ERROR_VARIABLE UNKNOWN_ERROR)
if(NOT UNKNOWN_RESULT EQUAL 2)
  message(FATAL_ERROR
          "unknown option returned ${UNKNOWN_RESULT} instead of 2")
endif()
foreach(EXPECTED
    "astrein: error: unknown option '--unknown-option'"
    "Usage: astrein [options] <translation-unit>")
  string(FIND "${UNKNOWN_ERROR}" "${EXPECTED}" FOUND)
  if(FOUND EQUAL -1)
    message(FATAL_ERROR "unknown-option error is missing: ${EXPECTED}")
  endif()
endforeach()

execute_process(
  COMMAND "${ASTREIN}"
  RESULT_VARIABLE MISSING_RESULT
  OUTPUT_VARIABLE MISSING_OUTPUT
  ERROR_VARIABLE MISSING_ERROR)
if(NOT MISSING_RESULT EQUAL 2)
  message(FATAL_ERROR
          "missing input returned ${MISSING_RESULT} instead of 2")
endif()
foreach(EXPECTED
    "astrein: error: missing translation unit"
    "Usage: astrein [options] <translation-unit>")
  string(FIND "${MISSING_ERROR}" "${EXPECTED}" FOUND)
  if(FOUND EQUAL -1)
    message(FATAL_ERROR "missing-input error is missing: ${EXPECTED}")
  endif()
endforeach()

execute_process(
  COMMAND "${ASTREIN}" --mode=invalid input.cpp
  RESULT_VARIABLE MODE_RESULT
  OUTPUT_VARIABLE MODE_OUTPUT
  ERROR_VARIABLE MODE_ERROR)
if(NOT MODE_RESULT EQUAL 2)
  message(FATAL_ERROR "invalid mode returned ${MODE_RESULT} instead of 2")
endif()
string(FIND "${MODE_ERROR}" "expected 'full' or 'reduced'" FOUND)
if(FOUND EQUAL -1)
  message(FATAL_ERROR "invalid-mode error does not explain valid modes")
endif()

execute_process(
  COMMAND "${ASTREIN}" --output
  RESULT_VARIABLE VALUE_RESULT
  OUTPUT_VARIABLE VALUE_OUTPUT
  ERROR_VARIABLE VALUE_ERROR)
if(NOT VALUE_RESULT EQUAL 2)
  message(FATAL_ERROR "missing value returned ${VALUE_RESULT} instead of 2")
endif()
string(FIND "${VALUE_ERROR}" "option '--output' requires a value" FOUND)
if(FOUND EQUAL -1)
  message(FATAL_ERROR "missing-value error is not actionable")
endif()

execute_process(
  COMMAND "${ASTREIN}" first.cpp second.cpp
  RESULT_VARIABLE EXTRA_RESULT
  OUTPUT_VARIABLE EXTRA_OUTPUT
  ERROR_VARIABLE EXTRA_ERROR)
if(NOT EXTRA_RESULT EQUAL 2)
  message(FATAL_ERROR
          "extra input returned ${EXTRA_RESULT} instead of 2")
endif()
foreach(EXPECTED
    "unexpected positional argument 'second.cpp'"
    "exactly one translation unit is required"
    "Usage: astrein [options] <translation-unit>")
  string(FIND "${EXTRA_ERROR}" "${EXPECTED}" FOUND)
  if(FOUND EQUAL -1)
    message(FATAL_ERROR "extra-input error is missing: ${EXPECTED}")
  endif()
endforeach()

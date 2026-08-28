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
    "Usage: astrein [options] <input-file>"
    "-h, --help"
    "-V, --version"
    "--ffi"
    "--mode <mode>"
    "--minify"
    "--require-c-linkage"
    "--require-default-visibility"
    "-p, --compile-commands <path>"
    "does not select the input file"
    "A header is a valid input file")
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
    "Usage: astrein [options] <input-file>")
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
    "astrein: error: missing input file"
    "Usage: astrein [options] <input-file>")
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
    "exactly one input file is required"
    "Usage: astrein [options] <input-file>")
  string(FIND "${EXTRA_ERROR}" "${EXPECTED}" FOUND)
  if(FOUND EQUAL -1)
    message(FATAL_ERROR "extra-input error is missing: ${EXPECTED}")
  endif()
endforeach()

execute_process(
  COMMAND "${ASTREIN}" file-that-does-not-exist.cpp
  RESULT_VARIABLE NOT_FOUND_RESULT
  OUTPUT_VARIABLE NOT_FOUND_OUTPUT
  ERROR_VARIABLE NOT_FOUND_ERROR)
if(NOT NOT_FOUND_RESULT EQUAL 2)
  message(FATAL_ERROR
          "missing file returned ${NOT_FOUND_RESULT} instead of 2")
endif()
foreach(EXPECTED
    "input file 'file-that-does-not-exist.cpp' does not exist"
    "pass a C or C++ header/source file as the positional input")
  string(FIND "${NOT_FOUND_ERROR}" "${EXPECTED}" FOUND)
  if(FOUND EQUAL -1)
    message(FATAL_ERROR "missing-file error is missing: ${EXPECTED}")
  endif()
endforeach()

execute_process(
  COMMAND "${ASTREIN}" --compile-commands missing-compilation-database
          "${CMAKE_CURRENT_LIST_FILE}"
  RESULT_VARIABLE COMPDB_RESULT
  OUTPUT_VARIABLE COMPDB_OUTPUT
  ERROR_VARIABLE COMPDB_ERROR)
if(NOT COMPDB_RESULT EQUAL 2)
  message(FATAL_ERROR
          "missing compilation database returned ${COMPDB_RESULT} instead of 2")
endif()
foreach(EXPECTED
    "cannot load compile_commands.json from 'missing-compilation-database'"
    "cmake -S . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON")
  string(FIND "${COMPDB_ERROR}" "${EXPECTED}" FOUND)
  if(FOUND EQUAL -1)
    message(FATAL_ERROR
            "missing-compilation-database error is missing: ${EXPECTED}")
  endif()
endforeach()

set(EMPTY_COMPDB_DIR "${CMAKE_CURRENT_BINARY_DIR}/empty-compdb")
file(MAKE_DIRECTORY "${EMPTY_COMPDB_DIR}")
file(WRITE "${EMPTY_COMPDB_DIR}/compile_commands.json" "[]\n")
execute_process(
  COMMAND "${ASTREIN}"
          --compile-commands "${EMPTY_COMPDB_DIR}/compile_commands.json"
          "${CMAKE_CURRENT_LIST_FILE}"
  RESULT_VARIABLE NO_COMMAND_RESULT
  OUTPUT_VARIABLE NO_COMMAND_OUTPUT
  ERROR_VARIABLE NO_COMMAND_ERROR)
if(NOT NO_COMMAND_RESULT EQUAL 2)
  message(FATAL_ERROR
          "empty compilation database returned ${NO_COMMAND_RESULT} instead of 2")
endif()
foreach(EXPECTED
    "no compile command is available for input file"
    "make sure compile_commands.json contains at least one C/C++ source file"
    "pass Clang arguments after '--'")
  string(FIND "${NO_COMMAND_ERROR}" "${EXPECTED}" FOUND)
  if(FOUND EQUAL -1)
    message(FATAL_ERROR "no-command error is missing: ${EXPECTED}")
  endif()
endforeach()

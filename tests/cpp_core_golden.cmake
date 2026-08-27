file(MAKE_DIRECTORY "${TEST_DIR}")
get_filename_component(FIXTURE_DIR "${FIXTURE}" DIRECTORY)
set(FILTERED_JSON "${TEST_DIR}/cpp-core-filtered.json")

execute_process(
  COMMAND "${ASTREIN}" --mode=reduced --require-c-linkage
          --require-default-visibility --public-header=cpp_core/serial.h
          --api-root "${FIXTURE_DIR}" --output "${FILTERED_JSON}" "${FIXTURE}"
          -- --target=x86_64-unknown-linux-gnu -std=c++2c -xc++
  RESULT_VARIABLE FILTERED_RESULT
  ERROR_VARIABLE FILTERED_ERROR)
if(NOT FILTERED_RESULT EQUAL 0)
  message(FATAL_ERROR
          "filtered cpp-core export failed (${FILTERED_RESULT}): "
          "${FILTERED_ERROR}")
endif()

file(READ "${FILTERED_JSON}" ACTUAL)
file(READ "${GOLDEN}" EXPECTED)
string(REPLACE "\r\n" "\n" ACTUAL "${ACTUAL}")
string(REPLACE "\r\n" "\n" EXPECTED "${EXPECTED}")
if(NOT ACTUAL STREQUAL EXPECTED)
  message(FATAL_ERROR
          "cpp-core export differs from ${GOLDEN}; actual output: "
          "${FILTERED_JSON}")
endif()

set(WINDOWS_FILTERED_JSON "${TEST_DIR}/cpp-core-windows-filtered.json")
execute_process(
  COMMAND "${ASTREIN}" --mode=reduced --require-c-linkage
          --require-default-visibility --api-root "${FIXTURE_DIR}"
          --output "${WINDOWS_FILTERED_JSON}" "${FIXTURE}"
          -- --target=x86_64-pc-windows-msvc -std=c++2c -xc++
  RESULT_VARIABLE WINDOWS_FILTERED_RESULT
  ERROR_VARIABLE WINDOWS_FILTERED_ERROR)
if(NOT WINDOWS_FILTERED_RESULT EQUAL 0)
  message(FATAL_ERROR
          "filtered Windows export failed (${WINDOWS_FILTERED_RESULT}): "
          "${WINDOWS_FILTERED_ERROR}")
endif()
file(READ "${WINDOWS_FILTERED_JSON}" WINDOWS_FILTERED_CONTENT)
string(JSON WINDOWS_FUNCTION_COUNT LENGTH
       "${WINDOWS_FILTERED_CONTENT}" functions)
if(NOT WINDOWS_FUNCTION_COUNT EQUAL 2)
  message(FATAL_ERROR
          "Windows visibility filter emitted ${WINDOWS_FUNCTION_COUNT} "
          "functions; expected 2")
endif()
foreach(INDEX RANGE 0 1)
  string(JSON WINDOWS_FUNCTION_NAME GET
         "${WINDOWS_FILTERED_CONTENT}" functions ${INDEX} name)
  if(INDEX EQUAL 0)
    set(EXPECTED_WINDOWS_FUNCTION_NAME serialSetReadCallback)
  else()
    set(EXPECTED_WINDOWS_FUNCTION_NAME serialWrite)
  endif()
  if(NOT WINDOWS_FUNCTION_NAME STREQUAL EXPECTED_WINDOWS_FUNCTION_NAME)
    message(FATAL_ERROR
            "Windows function ${INDEX} is ${WINDOWS_FUNCTION_NAME}; "
            "expected ${EXPECTED_WINDOWS_FUNCTION_NAME}")
  endif()
endforeach()

foreach(FILTER IN ITEMS c-linkage default-visibility none)
  set(OUTPUT "${TEST_DIR}/cpp-core-${FILTER}.json")
  if(FILTER STREQUAL "c-linkage")
    set(FILTER_ARGUMENT --require-c-linkage)
    set(EXPECTED_COUNT 4)
  elseif(FILTER STREQUAL "default-visibility")
    set(FILTER_ARGUMENT --require-default-visibility)
    set(EXPECTED_COUNT 3)
  else()
    set(FILTER_ARGUMENT)
    set(EXPECTED_COUNT 5)
  endif()

  execute_process(
    COMMAND "${ASTREIN}" --mode=reduced ${FILTER_ARGUMENT}
            --api-root "${FIXTURE_DIR}" --output "${OUTPUT}" "${FIXTURE}"
            -- --target=x86_64-unknown-linux-gnu -std=c++2c -xc++
    RESULT_VARIABLE RESULT
    ERROR_VARIABLE ERROR)
  if(NOT RESULT EQUAL 0)
    message(FATAL_ERROR "${FILTER} export failed (${RESULT}): ${ERROR}")
  endif()
  file(READ "${OUTPUT}" CONTENT)
  string(JSON FUNCTION_COUNT LENGTH "${CONTENT}" functions)
  if(NOT FUNCTION_COUNT EQUAL EXPECTED_COUNT)
    message(FATAL_ERROR
            "${FILTER} filter emitted ${FUNCTION_COUNT} functions; "
            "expected ${EXPECTED_COUNT}")
  endif()
endforeach()

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
if(NOT WINDOWS_FUNCTION_COUNT EQUAL 5)
  message(FATAL_ERROR
          "Windows visibility filter emitted ${WINDOWS_FUNCTION_COUNT} "
          "functions; expected 5")
endif()
foreach(INDEX RANGE 0 4)
  string(JSON WINDOWS_FUNCTION_NAME GET
         "${WINDOWS_FILTERED_CONTENT}" functions ${INDEX} name)
  if(INDEX EQUAL 0)
    set(EXPECTED_WINDOWS_FUNCTION_NAME serialDefaultConfig)
  elseif(INDEX EQUAL 1)
    set(EXPECTED_WINDOWS_FUNCTION_NAME serialOpen)
  elseif(INDEX EQUAL 2)
    set(EXPECTED_WINDOWS_FUNCTION_NAME serialSetReadCallback)
  elseif(INDEX EQUAL 3)
    set(EXPECTED_WINDOWS_FUNCTION_NAME serialValidateConfig)
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
    set(EXPECTED_COUNT 7)
  elseif(FILTER STREQUAL "default-visibility")
    set(FILTER_ARGUMENT --require-default-visibility)
    set(EXPECTED_COUNT 6)
  else()
    set(FILTER_ARGUMENT)
    set(EXPECTED_COUNT 8)
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

string(JSON STRUCT_COUNT LENGTH "${ACTUAL}" structs)
string(JSON RETURN_STRUCT_NAME GET "${ACTUAL}" structs 0 name)
string(JSON RETURN_STRUCT_SIZE GET "${ACTUAL}" structs 0 size)
string(JSON RETURN_STRUCT_ALIGNMENT GET "${ACTUAL}" structs 0 alignment)
string(JSON RETURN_STRUCT_FIELD_TYPE GET "${ACTUAL}" structs 0 fields 0 type)
string(JSON POINTER_STRUCT_NAME GET "${ACTUAL}" structs 2 name)
string(JSON POINTER_STRUCT_SIZE GET "${ACTUAL}" structs 2 size)
string(JSON POINTER_STRUCT_ALIGNMENT GET "${ACTUAL}" structs 2 alignment)
string(JSON POINTER_STRUCT_FIELD_TYPE GET "${ACTUAL}" structs 2 fields 0 type)
string(JSON POINTER_STRUCT_FIELD_OFFSET GET "${ACTUAL}" structs 2 fields 0 offset)
string(JSON POINTER_STRUCT_FIELD_SIZE GET "${ACTUAL}" structs 2 fields 0 size)
string(JSON NESTED_STRUCT_NAME GET "${ACTUAL}" structs 1 name)
string(JSON NESTED_STRUCT_SIZE GET "${ACTUAL}" structs 1 size)
string(JSON NESTED_STRUCT_ALIGNMENT GET "${ACTUAL}" structs 1 alignment)
string(JSON VALUE_STRUCT_NAME GET "${ACTUAL}" structs 3 name)
string(JSON VALUE_STRUCT_SIZE GET "${ACTUAL}" structs 3 size)
string(JSON VALUE_STRUCT_ALIGNMENT GET "${ACTUAL}" structs 3 alignment)
string(JSON VALUE_STRUCT_NESTED_FIELD_TYPE GET
       "${ACTUAL}" structs 3 fields 1 type)
string(JSON VALUE_STRUCT_SECOND_FIELD_OFFSET GET
       "${ACTUAL}" structs 3 fields 1 offset)
string(JSON VALUE_STRUCT_SECOND_FIELD_SIZE GET
       "${ACTUAL}" structs 3 fields 1 size)
string(JSON STRUCT_POINTER_TYPE GET "${ACTUAL}"
       functions 1 parameters 0 type)
string(JSON STRUCT_POINTER_SIZE GET "${ACTUAL}"
       functions 1 parameters 0 size)
string(JSON STRUCT_POINTER_ALIGNMENT GET "${ACTUAL}"
       functions 1 parameters 0 alignment)
string(JSON STRUCT_VALUE_PARAMETER_TYPE GET "${ACTUAL}"
       functions 3 parameters 0 type)
string(JSON STRUCT_VALUE_PARAMETER_SIZE GET "${ACTUAL}"
       functions 3 parameters 0 size)
string(JSON STRUCT_VALUE_PARAMETER_ALIGNMENT GET "${ACTUAL}"
       functions 3 parameters 0 alignment)
string(JSON STRUCT_VALUE_RETURN_TYPE GET "${ACTUAL}"
       functions 0 returnType)
string(JSON STRUCT_VALUE_RETURN_SIZE GET "${ACTUAL}"
       functions 0 returnSize)
string(JSON STRUCT_VALUE_RETURN_ALIGNMENT GET "${ACTUAL}"
       functions 0 returnAlignment)
if(NOT STRUCT_COUNT EQUAL 4 OR
   NOT RETURN_STRUCT_NAME STREQUAL "SerialDefaultConfig" OR
   NOT RETURN_STRUCT_SIZE EQUAL 4 OR
   NOT RETURN_STRUCT_ALIGNMENT EQUAL 4 OR
   NOT RETURN_STRUCT_FIELD_TYPE STREQUAL "int" OR
   NOT NESTED_STRUCT_NAME STREQUAL "SerialLineConfig" OR
   NOT NESTED_STRUCT_SIZE EQUAL 8 OR
   NOT NESTED_STRUCT_ALIGNMENT EQUAL 4 OR
   NOT POINTER_STRUCT_NAME STREQUAL "SerialOpenConfig" OR
   NOT POINTER_STRUCT_SIZE EQUAL 8 OR
   NOT POINTER_STRUCT_ALIGNMENT EQUAL 8 OR
   NOT POINTER_STRUCT_FIELD_TYPE STREQUAL "const char *" OR
   NOT POINTER_STRUCT_FIELD_OFFSET EQUAL 0 OR
   NOT POINTER_STRUCT_FIELD_SIZE EQUAL 8 OR
   NOT VALUE_STRUCT_NAME STREQUAL "SerialValidationConfig" OR
   NOT VALUE_STRUCT_SIZE EQUAL 12 OR
   NOT VALUE_STRUCT_ALIGNMENT EQUAL 4 OR
   NOT VALUE_STRUCT_NESTED_FIELD_TYPE STREQUAL "SerialLineConfig" OR
   NOT VALUE_STRUCT_SECOND_FIELD_OFFSET EQUAL 4 OR
   NOT VALUE_STRUCT_SECOND_FIELD_SIZE EQUAL 8 OR
   NOT STRUCT_POINTER_TYPE STREQUAL "const SerialOpenConfig *" OR
   NOT STRUCT_POINTER_SIZE EQUAL 8 OR
   NOT STRUCT_POINTER_ALIGNMENT EQUAL 8 OR
   NOT STRUCT_VALUE_PARAMETER_TYPE STREQUAL "SerialValidationConfig" OR
   NOT STRUCT_VALUE_PARAMETER_SIZE EQUAL 12 OR
   NOT STRUCT_VALUE_PARAMETER_ALIGNMENT EQUAL 4 OR
   NOT STRUCT_VALUE_RETURN_TYPE STREQUAL "SerialDefaultConfig" OR
   NOT STRUCT_VALUE_RETURN_SIZE EQUAL 4 OR
   NOT STRUCT_VALUE_RETURN_ALIGNMENT EQUAL 4)
  message(FATAL_ERROR
          "struct function signatures or emitted definition are unexpected")
endif()

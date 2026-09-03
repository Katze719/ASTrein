file(MAKE_DIRECTORY "${TEST_DIR}")
set(OUTPUT "${TEST_DIR}/shared-types.json")

execute_process(
  COMMAND "${ASTREIN}" --mode=reduced --minify --output "${OUTPUT}"
          "${FIXTURE}" -- -std=c++2c -xc++
  RESULT_VARIABLE RESULT
  ERROR_VARIABLE ERROR)
if(NOT RESULT EQUAL 0)
  message(FATAL_ERROR
          "shared-type export failed (${RESULT}): ${ERROR}")
endif()

file(READ "${OUTPUT}" CONTENT)
string(JSON FUNCTION_COUNT LENGTH "${CONTENT}" functions)
string(JSON STRUCT_COUNT LENGTH "${CONTENT}" structs)
string(JSON ENUM_COUNT LENGTH "${CONTENT}" enums)
string(JSON STRUCT_NAME GET "${CONTENT}" structs 0 name)
string(JSON ENUM_NAME GET "${CONTENT}" enums 0 name)

if(NOT FUNCTION_COUNT EQUAL 2 OR
   NOT STRUCT_COUNT EQUAL 1 OR
   NOT ENUM_COUNT EQUAL 1 OR
   NOT STRUCT_NAME STREQUAL "SharedConfig" OR
   NOT ENUM_NAME STREQUAL "SharedMode")
  message(FATAL_ERROR
          "shared struct or enum was not emitted exactly once")
endif()

foreach(INDEX RANGE 0 1)
  string(JSON STRUCT_PARAMETER_TYPE GET
         "${CONTENT}" functions ${INDEX} parameters 0 type)
  string(JSON ENUM_PARAMETER_TYPE GET
         "${CONTENT}" functions ${INDEX} parameters 1 type)
  if(NOT STRUCT_PARAMETER_TYPE STREQUAL "SharedConfig" OR
     NOT ENUM_PARAMETER_TYPE STREQUAL "SharedMode")
    message(FATAL_ERROR
            "function ${INDEX} does not reference both shared types")
  endif()
endforeach()

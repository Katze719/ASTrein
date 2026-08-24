file(MAKE_DIRECTORY "${TEST_DIR}")
set(FULL_JSON "${TEST_DIR}/full.json")
set(REDUCED_JSON "${TEST_DIR}/reduced.json")
get_filename_component(FIXTURE_DIR "${FIXTURE}" DIRECTORY)

execute_process(
  COMMAND "${ASTREIN}" --output "${FULL_JSON}" "${FIXTURE}" -- -std=c++2c -xc++
  RESULT_VARIABLE FULL_RESULT
  ERROR_VARIABLE FULL_ERROR)
if(NOT FULL_RESULT EQUAL 0)
  message(FATAL_ERROR "full mode failed (${FULL_RESULT}): ${FULL_ERROR}")
endif()

execute_process(
  COMMAND "${ASTREIN}" --mode=reduced --public-header=callbacks.hpp
          "--api-root=${FIXTURE_DIR}"
          --output "${REDUCED_JSON}" "${FIXTURE}" -- -std=c++2c -xc++
  RESULT_VARIABLE REDUCED_RESULT
  ERROR_VARIABLE REDUCED_ERROR)
if(NOT REDUCED_RESULT EQUAL 0)
  message(FATAL_ERROR
          "reduced mode failed (${REDUCED_RESULT}): ${REDUCED_ERROR}")
endif()

file(READ "${FULL_JSON}" FULL_CONTENT)
file(READ "${REDUCED_JSON}" REDUCED_CONTENT)

foreach(EXPECTED
    "\"error_code\""
    "\"message\""
    "\"status\""
    "\"details\""
    "\"event_id\""
    "\"user_data\"")
  string(FIND "${FULL_CONTENT}" "${EXPECTED}" FOUND)
  if(FOUND EQUAL -1)
    message(FATAL_ERROR "full JSON is missing ${EXPECTED}")
  endif()
  string(FIND "${REDUCED_CONTENT}" "${EXPECTED}" FOUND)
  if(FOUND EQUAL -1)
    message(FATAL_ERROR "reduced JSON is missing ${EXPECTED}")
  endif()
endforeach()

string(FIND "${FULL_CONTENT}" "\"callbackParameters\"" FULL_PARAMETERS)
if(FULL_PARAMETERS EQUAL -1)
  message(FATAL_ERROR "full JSON is missing callbackParameters objects")
endif()

string(FIND "${REDUCED_CONTENT}" "\"callback\"" REDUCED_CALLBACK)
if(REDUCED_CALLBACK EQUAL -1)
  message(FATAL_ERROR "reduced JSON is missing callback objects")
endif()

string(FIND "${REDUCED_CONTENT}" "\"parameterNames\"" LEGACY_NAMES)
if(NOT LEGACY_NAMES EQUAL -1)
  message(FATAL_ERROR
          "reduced JSON must model callback parameters as objects")
endif()

string(JSON SCHEMA GET "${REDUCED_CONTENT}" schema)
string(JSON SCHEMA_VERSION GET "${REDUCED_CONTENT}" schemaVersion)
string(JSON FUNCTION_COUNT LENGTH "${REDUCED_CONTENT}" functions)
if(NOT SCHEMA STREQUAL "cpp_core_ffi_api" OR
   NOT SCHEMA_VERSION EQUAL 1 OR
   NOT FUNCTION_COUNT EQUAL 4)
  message(FATAL_ERROR "unexpected reduced schema header or function count")
endif()

string(JSON ALIAS_CALLBACK_NAME GET "${REDUCED_CONTENT}"
       functions 0 parameters 0 callback parameters 0 name)
string(JSON ALIAS_CALLBACK_TYPE GET "${REDUCED_CONTENT}"
       functions 0 parameters 0 callback parameters 0 type)
string(JSON DIRECT_CALLBACK_NAME GET "${REDUCED_CONTENT}"
       functions 1 parameters 0 callback parameters 0 name)
string(JSON DIRECT_CALLBACK_TYPE GET "${REDUCED_CONTENT}"
       functions 1 parameters 0 callback parameters 1 type)
string(JSON PARTIAL_CALLBACK_SECOND_NAME GET "${REDUCED_CONTENT}"
       functions 2 parameters 0 callback parameters 1 name)
string(JSON LEGACY_CALLBACK_FIRST_NAME GET "${REDUCED_CONTENT}"
       functions 3 parameters 0 callback parameters 0 name)
string(JSON LEGACY_CALLBACK_SECOND_TYPE GET "${REDUCED_CONTENT}"
       functions 3 parameters 0 callback parameters 1 type)
if(NOT ALIAS_CALLBACK_NAME STREQUAL "error_code" OR
   NOT ALIAS_CALLBACK_TYPE STREQUAL "int" OR
   NOT DIRECT_CALLBACK_NAME STREQUAL "status" OR
   NOT DIRECT_CALLBACK_TYPE STREQUAL "const char *" OR
   NOT PARTIAL_CALLBACK_SECOND_NAME STREQUAL "message" OR
   NOT LEGACY_CALLBACK_FIRST_NAME STREQUAL "event_id" OR
   NOT LEGACY_CALLBACK_SECOND_TYPE STREQUAL "void *")
  message(FATAL_ERROR "callback parameter objects have unexpected values")
endif()

string(JSON UNUSED ERROR_VARIABLE UNNAMED_ERROR GET "${REDUCED_CONTENT}"
       functions 2 parameters 0 callback parameters 0 name)
if(UNNAMED_ERROR STREQUAL "NOTFOUND")
  message(FATAL_ERROR "unnamed callback parameter unexpectedly has a name")
endif()

foreach(EXPECTED
    "\"schema\": \"cpp_core_ffi_api\""
    "\"declaredIn\": \"callbacks.hpp\""
    "\"name\": \"invoke\""
    "\"brief\": \"Invoke a named callback.\""
    "\"returns\": \"Zero on success.\"")
  string(FIND "${REDUCED_CONTENT}" "${EXPECTED}" FOUND)
  if(FOUND EQUAL -1)
    message(FATAL_ERROR "reduced JSON is missing ${EXPECTED}")
  endif()
endforeach()

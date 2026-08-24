file(MAKE_DIRECTORY "${TEST_DIR}")
set(FULL_JSON "${TEST_DIR}/full.json")
set(REDUCED_JSON "${TEST_DIR}/reduced.json")
set(COMPDB_JSON "${TEST_DIR}/compdb.json")
set(COMPDB_DIR "${TEST_DIR}/compdb")
get_filename_component(FIXTURE_DIR "${FIXTURE}" DIRECTORY)

execute_process(
  COMMAND "${ASTREIN}" --output "${FULL_JSON}" "${FIXTURE}" -- -std=c++2c -xc++
  RESULT_VARIABLE FULL_RESULT
  ERROR_VARIABLE FULL_ERROR)
if(NOT FULL_RESULT EQUAL 0)
  message(FATAL_ERROR "full mode failed (${FULL_RESULT}): ${FULL_ERROR}")
endif()

execute_process(
  COMMAND "${ASTREIN}" --mode reduced --public-header callbacks.hpp
          --api-root "${FIXTURE_DIR}"
          -o "${REDUCED_JSON}" "${FIXTURE}" -- -std=c++2c -xc++
  RESULT_VARIABLE REDUCED_RESULT
  ERROR_VARIABLE REDUCED_ERROR)
if(NOT REDUCED_RESULT EQUAL 0)
  message(FATAL_ERROR
          "reduced mode failed (${REDUCED_RESULT}): ${REDUCED_ERROR}")
endif()

file(MAKE_DIRECTORY "${COMPDB_DIR}")
file(TO_CMAKE_PATH "${FIXTURE}" FIXTURE_JSON_PATH)
file(TO_CMAKE_PATH "${FIXTURE_DIR}" FIXTURE_DIR_JSON_PATH)
file(WRITE "${COMPDB_DIR}/compile_commands.json"
  "[\n"
  "  {\n"
  "    \"directory\": \"${FIXTURE_DIR_JSON_PATH}\",\n"
  "    \"arguments\": [\"c++\", \"-std=c++2c\", \"-xc++\", "
  "\"-fmodules-ts\", \"-fmodule-mapper=dummy.modmap\", "
  "\"-fdeps-format=p1689r5\", \"-MD\", \"-MF\", \"fixture.d\", "
  "\"-MT\", \"fixture.o\", \"-c\", \"${FIXTURE_JSON_PATH}\"],\n"
  "    \"file\": \"${FIXTURE_JSON_PATH}\"\n"
  "  }\n"
  "]\n")
execute_process(
  COMMAND "${ASTREIN}" --mode=reduced -p "${COMPDB_DIR}"
          --output "${COMPDB_JSON}" "${FIXTURE}"
  RESULT_VARIABLE COMPDB_RESULT
  ERROR_VARIABLE COMPDB_ERROR)
if(NOT COMPDB_RESULT EQUAL 0)
  message(FATAL_ERROR
          "compilation-database mode failed (${COMPDB_RESULT}): "
          "${COMPDB_ERROR}")
endif()
string(FIND "${COMPDB_ERROR}" "unknown argument" UNKNOWN_COMPDB_ARGUMENT)
if(NOT UNKNOWN_COMPDB_ARGUMENT EQUAL -1)
  message(FATAL_ERROR
          "GCC compilation-database flags leaked into Clang: ${COMPDB_ERROR}")
endif()

file(READ "${FULL_JSON}" FULL_CONTENT)
file(READ "${REDUCED_JSON}" REDUCED_CONTENT)
file(READ "${COMPDB_JSON}" COMPDB_CONTENT)

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
  string(FIND "${COMPDB_CONTENT}" "${EXPECTED}" FOUND)
  if(FOUND EQUAL -1)
    message(FATAL_ERROR "compilation-database JSON is missing ${EXPECTED}")
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

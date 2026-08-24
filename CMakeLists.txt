cmake_minimum_required(VERSION 3.28)

project(ASTrein VERSION 0.1.0 LANGUAGES C CXX)

include(CTest)
include(FetchContent)

option(ASTREIN_BUNDLED_LLVM
       "Download the pinned LLVM source and link ASTrein against static LLVM/Clang libraries"
       ON)
option(ASTREIN_ALLOW_SHARED_LLVM
       "Allow a system clang-cpp shared library (development only)" OFF)
set(ASTREIN_LLVM_VERSION "22.1.8" CACHE STRING
    "Pinned LLVM release used by the bundled static build")
set(ASTREIN_LLVM_SHA256
    "922f1817a0df7b1489272d18134ee0087a8b068828f87ac63b9861b1a9965888"
    CACHE STRING "SHA-256 of the pinned llvm-project source archive")

if(ASTREIN_BUNDLED_LLVM)
  set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
  set(LLVM_BUILD_LLVM_DYLIB OFF CACHE BOOL "" FORCE)
  set(LLVM_LINK_LLVM_DYLIB OFF CACHE BOOL "" FORCE)
  set(CLANG_LINK_CLANG_DYLIB OFF CACHE BOOL "" FORCE)
  set(LLVM_ENABLE_PROJECTS "clang" CACHE STRING "" FORCE)
  # ASTrein parses source only; no LLVM code-generation backend is required.
  set(LLVM_TARGETS_TO_BUILD "" CACHE STRING "" FORCE)
  set(LLVM_INCLUDE_BENCHMARKS OFF CACHE BOOL "" FORCE)
  set(LLVM_INCLUDE_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(LLVM_INCLUDE_TESTS OFF CACHE BOOL "" FORCE)
  set(LLVM_INCLUDE_TOOLS ON CACHE BOOL "" FORCE)
  set(LLVM_ENABLE_TERMINFO OFF CACHE BOOL "" FORCE)
  set(LLVM_ENABLE_LIBEDIT OFF CACHE BOOL "" FORCE)
  set(LLVM_ENABLE_LIBXML2 OFF CACHE BOOL "" FORCE)
  set(LLVM_ENABLE_ZLIB OFF CACHE BOOL "" FORCE)
  set(LLVM_ENABLE_ZSTD OFF CACHE BOOL "" FORCE)
  set(CLANG_INCLUDE_TESTS OFF CACHE BOOL "" FORCE)
  set(CLANG_ENABLE_STATIC_ANALYZER OFF CACHE BOOL "" FORCE)
  set(CLANG_ENABLE_OBJC_REWRITER OFF CACHE BOOL "" FORCE)

  FetchContent_Declare(
    llvm_project
    URL
      https://github.com/llvm/llvm-project/releases/download/llvmorg-${ASTREIN_LLVM_VERSION}/llvm-project-${ASTREIN_LLVM_VERSION}.src.tar.xz
    URL_HASH SHA256=${ASTREIN_LLVM_SHA256}
    DOWNLOAD_EXTRACT_TIMESTAMP FALSE
    SOURCE_SUBDIR llvm
    EXCLUDE_FROM_ALL)
  FetchContent_MakeAvailable(llvm_project)

  set(ASTREIN_LLVM_INCLUDE_DIRS
    "${llvm_project_SOURCE_DIR}/llvm/include"
    "${llvm_project_BINARY_DIR}/include"
    "${llvm_project_SOURCE_DIR}/clang/include"
    "${llvm_project_BINARY_DIR}/tools/clang/include")
else()
  find_package(LLVM CONFIG REQUIRED)
  find_package(Clang CONFIG REQUIRED)
  set(ASTREIN_LLVM_INCLUDE_DIRS
    ${LLVM_INCLUDE_DIRS}
    ${CLANG_INCLUDE_DIRS})
endif()

add_executable(astrein
  src/main.cpp
  src/app/run.cpp
  src/ast/ast_action_factory.cpp
  src/ast/ast_consumer.cpp
  src/ast/ast_frontend_action.cpp
  src/ast/ast_visitor.cpp
  src/ast/signature_catalog.cpp
  src/cli/argument_parser.cpp
  src/functions/api_relative_path.cpp
  src/functions/append_words.cpp
  src/functions/callback_json.cpp
  src/functions/callback_parameters_json.cpp
  src/functions/declared_in.cpp
  src/functions/documentation_json.cpp
  src/functions/expression_text.cpp
  src/functions/extract_documentation.cpp
  src/functions/flush_comment_section.cpp
  src/functions/function_prototype.cpp
  src/functions/normalized_path.cpp
  src/functions/patch_full_ast.cpp
  src/functions/path_is_below.cpp
  src/functions/pointer_id.cpp
  src/functions/print_type.cpp
  src/functions/reduced_function_json.cpp
  src/functions/start_comment_command.cpp
  src/functions/strip_gcc_module_arguments.cpp
  src/model/callback_signature.cpp)

target_compile_features(astrein PRIVATE cxx_std_26)
set_target_properties(astrein PROPERTIES
  CXX_EXTENSIONS OFF
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

target_include_directories(astrein PRIVATE src)
target_include_directories(astrein SYSTEM PRIVATE ${ASTREIN_LLVM_INCLUDE_DIRS})
target_compile_definitions(astrein PRIVATE
  ASTREIN_VERSION="${PROJECT_VERSION}")

if(MSVC OR CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
  target_compile_options(astrein PRIVATE
    /W4 /permissive- /Zc:__cplusplus /EHsc)
  target_compile_definitions(astrein PRIVATE
    _CRT_SECURE_NO_WARNINGS
    _SILENCE_ALL_CXX23_DEPRECATION_WARNINGS)
else()
  target_compile_options(astrein PRIVATE -Wall -Wextra -Wpedantic)
endif()

# LLVM disables C++ RTTI by default. A consumer that instantiates LLVM command
# line templates must use the same setting or it will reference typeinfo that
# intentionally does not exist in the static archives.
if(NOT LLVM_ENABLE_RTTI)
  if(MSVC OR CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    target_compile_options(astrein PRIVATE /GR-)
  else()
    target_compile_options(astrein PRIVATE -fno-rtti)
  endif()
endif()

set(ASTREIN_STATIC_CLANG_LIBS
  clangTooling
  clangFrontend
  clangSerialization
  clangDriver
  clangParse
  clangSema
  clangAnalysis
  clangASTMatchers
  clangAST
  clangLex
  clangBasic)

if(TARGET clangTooling)
  get_target_property(ASTREIN_CLANG_TOOLING_TYPE clangTooling TYPE)
  if(NOT ASTREIN_CLANG_TOOLING_TYPE STREQUAL "STATIC_LIBRARY" AND
     NOT ASTREIN_ALLOW_SHARED_LLVM)
    message(FATAL_ERROR
      "clangTooling is not static. Use -DASTREIN_BUNDLED_LLVM=ON for the "
      "self-contained release build, or explicitly allow a development build "
      "with -DASTREIN_ALLOW_SHARED_LLVM=ON.")
  endif()
  target_link_libraries(astrein PRIVATE ${ASTREIN_STATIC_CLANG_LIBS})
elseif(TARGET clang-cpp AND ASTREIN_ALLOW_SHARED_LLVM)
  message(WARNING
    "Building against shared clang-cpp. This development binary is not the "
    "self-contained release artifact.")
  target_link_libraries(astrein PRIVATE clang-cpp LLVM)
else()
  message(FATAL_ERROR
    "Static Clang component libraries were not found. Configure with "
    "-DASTREIN_BUNDLED_LLVM=ON.")
endif()

if(BUILD_TESTING)
  add_test(
    NAME astrein.callback-parameter-names
    COMMAND ${CMAKE_COMMAND}
      -DASTREIN=$<TARGET_FILE:astrein>
      -DFIXTURE=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/callbacks.hpp
      -DTEST_DIR=${CMAKE_CURRENT_BINARY_DIR}/tests
      -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/integration.cmake)
  add_test(
    NAME astrein.command-line-interface
    COMMAND ${CMAKE_COMMAND}
      -DASTREIN=$<TARGET_FILE:astrein>
      -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/cli.cmake)
endif()

install(TARGETS astrein RUNTIME DESTINATION bin)
install(FILES LICENSE THIRD_PARTY_NOTICES.md DESTINATION share/astrein)
if(ASTREIN_BUNDLED_LLVM)
  install(FILES "${llvm_project_SOURCE_DIR}/LICENSE.TXT"
    DESTINATION share/astrein
    RENAME LLVM-LICENSE.txt)
endif()

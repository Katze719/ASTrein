# ASTrein

ASTrein turns a C or C++ header/source file into JSON by parsing it with Clang.
It is primarily intended for tools that generate FFI bindings or inspect a
public C API.

For most users, the compact and stable FFI output selected by `--ffi` is the
right starting point. The complete Clang JSON AST remains available for
advanced consumers.

## See the result first

Given this public header:

```cpp
// include/api.hpp
#pragma once

/**
 * @brief Add two numbers.
 * @param[in] left First number.
 * @param[in] right Second number.
 * @return The sum of both numbers.
 */
extern "C" int add(int left, int right);
```

Run:

```sh
astrein --ffi \
  --api-root include \
  --output ffi-api.json \
  include/api.hpp -- -xc++ -std=c++20 -Iinclude
```

`ffi-api.json` contains a compact description suitable for a binding
generator:

```json
{
  "$schema": "https://raw.githubusercontent.com/Katze719/ASTrein/main/schema/astrein-ffi-api-v3.schema.json",
  "functions": [
    {
      "declaredIn": "api.hpp",
      "doc": {
        "brief": "Add two numbers.",
        "returns": "The sum of both numbers."
      },
      "name": "add",
      "parameters": [
        {
          "alignment": 4,
          "doc": {
            "description": "First number.",
            "direction": "in"
          },
          "name": "left",
          "size": 4,
          "type": "int"
        },
        {
          "alignment": 4,
          "doc": {
            "description": "Second number.",
            "direction": "in"
          },
          "name": "right",
          "size": 4,
          "type": "int"
        }
      ],
      "returnAlignment": 4,
      "returnSize": 4,
      "returnType": "int",
      "symbol": "add"
    }
  ],
  "publicHeader": "api.hpp",
  "schema": "astrein_ffi_api",
  "schemaVersion": 3,
  "structs": []
}
```

ASTrein retains function names, parameter names and types, callback signatures,
struct definitions used by exported functions, default arguments, and
supported Doxygen documentation.

## Get started

Download the archive for Linux or Windows from the
[latest release](https://github.com/Katze719/ASTrein/releases/latest), unpack
it, and check the executable:

```sh
astrein --version
```

Then choose one of the following two ways to give ASTrein the compiler settings
needed to understand your code.

### Option A: use an existing build

This is the recommended option for a real project. Generate
`compile_commands.json` with CMake:

```sh
cmake -S . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build
```

Point ASTrein at either the build directory or the JSON file:

```sh
astrein --ffi \
  --compile-commands build \
  --api-root include \
  --output ffi-api.json \
  include/my_library/api.hpp
```

ASTrein uses the matching compile command for a source file. If the header
itself is not listed, Clang infers a suitable command from nearby project
sources. This supplies the same language standard, include paths, defines, and
target settings as the real build.

The following form is equivalent:

```sh
astrein --ffi \
  --compile-commands build/compile_commands.json \
  --api-root include \
  --output ffi-api.json \
  include/my_library/api.hpp
```

### Parse a cross-compiled target

ASTrein runs on the host and does not execute code for the parsed target. An
x86-64 ASTrein binary can therefore inspect an AArch64 build when its
compilation database contains the cross-compiler settings:

```sh
cmake -S . -B build-aarch64 -G Ninja \
  -DCMAKE_SYSTEM_NAME=Linux \
  -DCMAKE_SYSTEM_PROCESSOR=aarch64 \
  -DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc \
  -DCMAKE_CXX_COMPILER=aarch64-linux-gnu-g++ \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

cmake --build build-aarch64

astrein --ffi \
  --compile-commands build-aarch64 \
  --api-root include \
  --output ffi-api-aarch64.json \
  include/my_library/api.hpp
```

The cross-toolchain and target sysroot referenced by `compile_commands.json`
must be installed on the host. This approach preserves target-specific
preprocessor definitions, include paths, type interpretation, and other
compiler settings. Generate separate output for each target when those
settings can change the public API.

For a self-contained header without a compilation database, pass the Clang
target and sysroot explicitly:

```sh
astrein --ffi \
  --api-root include \
  --output ffi-api-aarch64.json \
  include/my_library/api.hpp -- \
  -xc++ -std=c++20 -Iinclude \
  --target=aarch64-linux-gnu \
  --sysroot=/path/to/aarch64-sysroot
```

### Option B: pass compiler settings directly

For a self-contained C++ header, put the required Clang arguments after `--`:

```sh
astrein --ffi \
  --api-root include \
  --output ffi-api.json \
  include/my_library/api.hpp -- -xc++ -std=c++20 -Iinclude
```

For a C header, select C instead:

```sh
astrein --ffi \
  --api-root include \
  --output ffi-api.json \
  include/my_library/api.h -- -xc -std=c17 -Iinclude
```

Pass any required defines in the same place, for example
`-- -xc++ -std=c++20 -Iinclude -DMY_LIBRARY_STATIC`.

## Understand the command

The command has three parts:

```text
astrein [ASTrein options] <input-file> [-- <extra Clang arguments>]
```

| Part | Purpose |
| --- | --- |
| `<input-file>` | Header or source file that Clang parses |
| `--compile-commands` | Reuses compiler settings from the project build |
| Arguments after `--` | Supplies or extends Clang compiler settings |
| `--api-root` | Limits reduced output to declarations below a directory |
| `--public-header` | Overrides only the `publicHeader` text in the JSON |
| `--output` | Selects the output file; the default is standard output |
| `--minify` | Writes compact JSON without indentation |

A header is a valid input file. Internally, Clang parses that input as one
translation unit. You only need a separate `.cpp` input when its includes,
defines, or include order are part of the API's required context.

`--public-header` does not choose what ASTrein parses. It is useful when the
input is a wrapper source file but generated bindings should refer to a public
header:

```sh
astrein --ffi \
  --compile-commands build \
  --api-root include \
  --public-header my_library/api.hpp \
  --output ffi-api.json \
  src/my_library_ffi_ast.cpp
```

## Common recipes

### Include only an exported C ABI

For a C API declared inside a C++ project, require C linkage:

```sh
astrein --ffi \
  --compile-commands build \
  --api-root include \
  --require-c-linkage \
  --output ffi-api.json \
  include/my_library/api.hpp
```

If the API also marks exports with
`__attribute__((visibility("default")))`, add
`--require-default-visibility`. The two filters are independent.

### Emit the complete Clang AST

Full mode is the default and is intended for consumers that need Clang's
complete JSON representation:

```sh
astrein --mode full \
  --compile-commands build \
  --output clang-ast.json \
  include/my_library/api.hpp
```

### Extend a compilation-database command

Arguments after `--` are appended even when `--compile-commands` is used:

```sh
astrein --ffi \
  --compile-commands build \
  --api-root include \
  include/my_library/api.hpp -- -DMY_LIBRARY_STATIC
```

### What `compile_commands.json` looks like

Build systems normally generate this file; you usually do not write it by
hand. A minimal entry looks like this:

```json
[
  {
    "directory": "/absolute/path/to/project",
    "arguments": [
      "clang++",
      "-std=c++20",
      "-Iinclude",
      "-DMY_LIBRARY_STATIC",
      "-c",
      "src/library.cpp"
    ],
    "file": "src/library.cpp"
  }
]
```

With this file at `build/compile_commands.json`, ASTrein can infer the compile
settings for `include/my_library/api.hpp` from the project source entry:

```sh
astrein --ffi --compile-commands build \
  --api-root include include/my_library/api.hpp
```

## Troubleshooting

### Clang cannot find a header or a type

ASTrein must see the same include paths, defines, language standard, and target
as the real compiler. Prefer `--compile-commands build`, or add the missing
settings after `--`.

### No compile command is available

Make sure the database contains at least one C or C++ source entry:

```sh
cmake -S . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
```

If different source files use very different settings, pass a small wrapper
`.cpp` that includes the public header and has its own database entry.

### The output contains declarations from dependencies

Add one or more `--api-root` options. Only declarations located below those
directories are then included in reduced output. System headers are always
excluded.

### A function is missing

Check conditional defines and include paths first. Also verify whether
`--require-c-linkage` or `--require-default-visibility` filtered it out.

### A `.h` file is parsed as the wrong language

Pass `-xc` for C or `-xc++` for C++ after `--`. A compilation database
normally supplies this context automatically.

Command-line usage errors return exit code `2`; Clang parsing failures return
`1`.

## Command-line reference

Run `astrein --help` for the complete reference:

```text
Usage: astrein [options] <input-file> [-- <extra-clang-arguments>...]
```

The most important options are:

| Option | Meaning |
| --- | --- |
| `--ffi` | Emit the compact FFI API |
| `--mode full` | Emit the complete Clang JSON AST |
| `--minify` | Emit compact JSON without indentation |
| `-o, --output <path>` | Write JSON to a file; `-` means standard output |
| `-p, --compile-commands <path>` | Load `compile_commands.json` |
| `--api-root <directory>` | Select API declaration roots; repeatable |
| `--public-header <path>` | Override `publicHeader` output metadata |
| `--require-c-linkage` | Keep only functions with C linkage |
| `--require-default-visibility` | Keep only explicitly visible functions |

The previous `--build-path` name remains available as a compatibility alias
for `--compile-commands`.

In PowerShell, put an example on one line or use PowerShell's backtick line
continuation character instead of `\`.

## Output modes

### Reduced FFI API

Reduced output uses the
[`astrein_ffi_api` JSON Schema](schema/astrein-ffi-api-v3.schema.json) and
identifies it through the top-level `$schema` property.

Schema version 3 is designed for additive evolution. Every object permits
unknown properties, so consumers must read the fields they understand and
ignore the rest. New optional metadata can therefore be added without a schema
version change. Removing or renaming fields, adding required fields, or
changing existing semantics still requires a new schema version. The frozen
version 1 and version 2 schemas remain available for their corresponding
outputs.

The top-level `structs` array contains the named structs reachable from an
exported function's return type or parameter types, including through pointers,
references, arrays, callback signatures, and nested struct fields. Complete
definitions contain their fields in declaration order. A struct for which only
a forward declaration is public has `"opaque": true` and an empty `fields`
array. For complete definitions, `size`, `alignment`, and each field's `offset`
and `size` describe the ABI layout in target bytes. Bitfields additionally
contain their absolute `bitOffset` and source-level `bitWidth`. These values
reflect the target and ABI selected by the Clang arguments or compilation
database. Structs and their transitive field dependencies follow the same
`--api-root` and system-header filtering as functions.

Function parameters and parameters of callback signatures likewise contain
`size` and `alignment`. Non-`void` function returns expose the same information
as `returnSize` and `returnAlignment`. Functions themselves do not have an
object size or offset; calling-convention register and stack placement is
intentionally outside these layout fields.

#### Layout fields

All byte values refer to target bytes and therefore depend on the target triple
and ABI used while parsing.

| Field | Applies to | Unit | Meaning |
| --- | --- | --- | --- |
| `size` | Struct | Bytes | Total object size, including internal and trailing padding. This is the distance required between elements in an array of that struct. |
| `alignment` | Struct | Bytes | Address boundary required by the struct type. For example, alignment `8` means its address must normally be divisible by eight. |
| `offset` | Struct field | Bytes | Position of the field relative to the start of its containing struct. Padding is visible as gaps between offsets. |
| `size` | Struct field | Bytes | Size of the field's type. For a nested struct this includes that nested struct's padding; for a pointer it is the target pointer size. |
| `bitOffset` | Bitfield | Bits | Absolute start of a bitfield relative to the beginning of its containing struct. |
| `bitWidth` | Bitfield | Source expression | Width written in the source, such as `3` or `FLAG_BITS`. For bitfields, byte `offset` identifies the containing byte and byte `size` describes the declared storage type. |
| `size` | Function or callback parameter | Bytes | Size of the declared parameter type after C/C++ parameter adjustment. A pointer parameter therefore reports the pointer size, not the pointee size. |
| `alignment` | Function or callback parameter | Bytes | Natural ABI alignment of the parameter type. This is useful when allocating temporary or marshalling storage, but it does not describe where the argument is passed. |
| `returnSize` | Function return | Bytes | Size of a non-`void` return type. |
| `returnAlignment` | Function return | Bytes | Natural ABI alignment of a non-`void` return type. |

Function parameter order comes from the `parameters` array. Actual register or
stack placement is controlled by the platform calling convention, so function
parameters intentionally have no `offset`. Large by-value values may also be
lowered to hidden pointers even though their JSON `size` still describes the
source-level type.

#### Struct example

Given an exported function that receives a nested struct through a pointer:

```cpp
struct SerialLineConfig {
  int data_bits; ///< Number of data bits per frame.
  int stop_bits; ///< Number of stop bits per frame.
};

struct SerialConfig {
  const char *device;
  SerialLineConfig line;
};

extern "C" int serialOpen(const SerialConfig *config);
```

On an x86-64 target, the relevant reduced output is:

```json
{
  "functions": [
    {
      "name": "serialOpen",
      "parameters": [
        {
          "alignment": 8,
          "name": "config",
          "size": 8,
          "type": "const SerialConfig *"
        }
      ],
      "returnAlignment": 4,
      "returnSize": 4,
      "returnType": "int",
      "symbol": "serialOpen"
    }
  ],
  "structs": [
    {
      "alignment": 8,
      "fields": [
        {
          "name": "device",
          "offset": 0,
          "size": 8,
          "type": "const char *"
        },
        {
          "name": "line",
          "offset": 8,
          "size": 8,
          "type": "SerialLineConfig"
        }
      ],
      "name": "SerialConfig",
      "size": 16
    },
    {
      "alignment": 4,
      "fields": [
        {
          "doc": {
            "brief": "Number of data bits per frame."
          },
          "name": "data_bits",
          "offset": 0,
          "size": 4,
          "type": "int"
        },
        {
          "doc": {
            "brief": "Number of stop bits per frame."
          },
          "name": "stop_bits",
          "offset": 4,
          "size": 4,
          "type": "int"
        }
      ],
      "name": "SerialLineConfig",
      "size": 8
    }
  ]
}
```

Callback parameters are modeled like top-level function parameters. For an
unnamed callback parameter, its object contains `type` and omits `name`.
Doxygen `brief`, `param`, `return`, `details`, `note`, and `remark` commands
are projected into the reduced representation. Documentation attached to a
struct or one of its fields is emitted as a `doc` object with `brief` and
`details`; both leading comments and trailing comments such as `///< ...` are
supported. Inline commands such as `@p` retain their formatting, and `@code`
blocks become fenced Markdown code blocks.

Functions are ordered by qualified name for deterministic output.

### Full Clang AST

Clang interns function types, although parameter names belong to individual
source declarations rather than to a canonical type. ASTrein therefore adds:

- `parameters` objects to `FunctionProtoType` when source spelling is
  available;
- `callbackParameters` objects to the corresponding `TypedefDecl`,
  `TypeAliasDecl`, and callback `ParmVarDecl`.

The declaration-local field is authoritative if multiple callbacks share the
same canonical signature but use different names.

## Build from source

Linux and Windows are first-class targets. Release builds statically link the
required LLVM and Clang libraries, so the resulting executable does not need a
separate LLVM/Clang installation at runtime. System C/C++ runtime libraries
remain platform dependencies.

Building ASTrein requires GCC 16 or newer, or LLVM/Clang 22 or newer. The
reproducible release preset downloads the pinned LLVM source and builds only
the required libraries, so the first build is intentionally large.

### Linux

```sh
cmake --preset release-static
cmake --build --preset release-static --parallel
ctest --preset release-static
```

The executable is `build/release-static/bin/astrein`.

### Windows

Run from an x64 Native Tools Command Prompt for Visual Studio with CMake,
Ninja, and Git available:

```powershell
cmake --preset release-static
cmake --build --preset release-static --parallel
ctest --preset release-static
```

The executable is `build/release-static/bin/astrein.exe`. The same preset
works with MSVC or `clang-cl`; CMake selects the compiler from the active
developer environment.

For a quick development build against an installed LLVM/Clang CMake package,
use `dev-system`. This opt-in preset permits `clang-cpp` to be shared and is
not the self-contained release artifact.

## License

ASTrein is licensed under the [Apache License 2.0 with LLVM Exceptions](LICENSE)
(`Apache-2.0 WITH LLVM-exception`). Third-party notices are listed in
[`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md).

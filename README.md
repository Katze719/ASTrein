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
  "$schema": "https://raw.githubusercontent.com/Katze719/ASTrein/main/schema/astrein-ffi-api-v1.schema.json",
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
          "doc": {
            "description": "First number.",
            "direction": "in"
          },
          "name": "left",
          "type": "int"
        },
        {
          "doc": {
            "description": "Second number.",
            "direction": "in"
          },
          "name": "right",
          "type": "int"
        }
      ],
      "returnType": "int",
      "symbol": "add"
    }
  ],
  "publicHeader": "api.hpp",
  "schema": "astrein_ffi_api",
  "schemaVersion": 1
}
```

ASTrein retains function names, parameter names and types, callback signatures,
default arguments, and supported Doxygen documentation.

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
[`astrein_ffi_api` JSON Schema](schema/astrein-ffi-api-v1.schema.json) and
identifies it through the top-level `$schema` property.

Callback parameters are modeled like top-level function parameters. For an
unnamed callback parameter, its object contains `type` and omits `name`.
Doxygen `brief`, `param`, `return`, `details`, `note`, and `remark`
commands are projected into the reduced representation. Inline commands such
as `@p` retain their formatting, and `@code` blocks become fenced Markdown
code blocks.

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

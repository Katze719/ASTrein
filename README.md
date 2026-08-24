# ASTrein

ASTrein is a C++26 command-line tool that emits Clang's JSON AST without losing
the source-level names of callback parameters. It also provides a smaller,
stable JSON view intended for FFI binding generators.

Linux and Windows are first-class targets. Release builds statically link the
required LLVM and Clang libraries, so the resulting executable does not need a
separate LLVM/Clang installation at runtime. System C/C++ runtime libraries are
still platform dependencies.

Building ASTrein requires GCC 16 or newer, or LLVM/Clang 22 or newer.

## Build

The reproducible release preset downloads the pinned LLVM source and builds
only the libraries needed by ASTrein. The first build is intentionally large.

### Linux

```sh
cmake --preset release-static
cmake --build --preset release-static --parallel
ctest --preset release-static
```

The executable is `build/release-static/bin/astrein`.

### Windows

Run these commands from an x64 Native Tools Command Prompt for Visual Studio
with CMake, Ninja, and Git available:

```powershell
cmake --preset release-static
cmake --build --preset release-static --parallel
ctest --preset release-static
```

The executable is `build/release-static/bin/astrein.exe`. The same preset works
with MSVC or `clang-cl`; CMake selects the compiler from the active developer
environment.

For a quick development build against an already installed LLVM/Clang CMake
package, use `dev-system`. This opt-in preset permits `clang-cpp` to be shared
and is therefore not the release artifact.

## Usage

ASTrein accepts exactly one translation unit. Arguments following `--` are
passed to Clang. Alternatively, use `-p <build-directory>` to load a
`compile_commands.json` entry.

```sh
# Full JSON AST (the default)
astrein --output ast.json include/api.hpp -- -std=c++2c -Iinclude

# Reduced FFI API
astrein --mode=reduced \
  --public-header=my_library/api.hpp \
  --api-root=include \
  --output ffi_api.json \
  include/my_library/api.hpp -- -std=c++2c -Iinclude
```

In PowerShell, use the same arguments on one line or PowerShell's backtick line
continuation character.

### Command-line help

Use the built-in help and version commands to inspect the installed executable:

```sh
astrein --help
astrein --version
```

The general command form is:

```text
astrein [options] <translation-unit> [-- <clang-arguments>...]
```

ASTrein reports invalid options, missing option values, invalid output modes,
and missing or extra translation units together with the full help text. These
command-line errors return exit code `2`; Clang processing failures return `1`.
Use `-p <build-directory>` to load `compile_commands.json`. Any arguments after
`--` are passed to Clang and can also extend a compilation-database command.

### Full mode patch

Clang interns function types, although parameter names belong to individual
source declarations rather than to a canonical type. ASTrein therefore emits:

- `parameters` objects on `FunctionProtoType` when a source spelling is
  available;
- `callbackParameters` objects on the corresponding `TypedefDecl`,
  `TypeAliasDecl`, and callback `ParmVarDecl`.

The declaration-local field is authoritative if multiple callbacks share the
same canonical signature but use different names.

### Reduced mode

Reduced output uses the [`astrein_ffi_api` JSON Schema](schema/astrein-ffi-api-v1.schema.json)
and identifies it through the top-level `$schema` property. Callback parameters
are modeled exactly like top-level function parameters:

```json
{
  "callback": {
    "returnType": "void",
    "parameters": [
      { "name": "error_code", "type": "int" },
      { "name": "message", "type": "const char *" }
    ]
  }
}
```

For an unnamed callback parameter, the object still contains `type` and simply
omits `name`. Doxygen `brief`, `param`, `return`, `details`, `note`, and
`remark` commands are projected into the reduced representation.

Use `--api-root` more than once when a public API spans multiple source roots.
System-header declarations are always excluded from reduced output.

## License

ASTrein is licensed under the [Apache License 2.0 with LLVM Exceptions](LICENSE)
(`Apache-2.0 WITH LLVM-exception`). Third-party notices are listed in
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

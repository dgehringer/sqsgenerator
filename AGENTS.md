# AGENTS.md — sqsgenerator

Guidance for AI coding agents (and humans) working on this repository.
Read this before making changes. When in doubt, prefer the simplest solution that
follows the rules below, and ask before introducing a new dependency.

## Project overview

*sqsgenerator* generates special quasi-random structures (SQS): it searches atomic
configurations whose short-range-order (SRO) parameters match a target, using
Monte-Carlo (random) or systematic iteration, optionally over multiple sublattices.

- Core: C++23, header-only, performance-critical.
- Frontends: Python package (nanobind), WebAssembly app (Emscripten, multithreaded),
  native CLI with optional MPI for HPC.
- Docs: https://sqsgenerator.readthedocs.io

The bond-counting / objective loop is the hot path of the whole program.
Changes there must not regress performance (see "Performance").

## Supported targets

Every change must keep all of these building:

| Target | Notes |
|---|---|
| macOS, Linux (glibc), Windows | native, x86_64 |
| aarch64, armv7, ppc64le | native or cross-compiled |
| musl Linux (e.g. Alpine) | no glibc-only APIs |
| Python wheels | via nanobind, Python 3.12 stable ABI only (see below) |
| WebAssembly (Emscripten) | browser + Node, with pthreads |
| Pyodide | Python-in-WASM; no MPI, no OS-specific code |

Consequences:
- No platform-specific APIs outside a clearly isolated file (e.g. signal handling).
  Guard with `#ifdef _WIN32` etc. only there.
- No assumptions about `sizeof(long)`, endianness, or `char` signedness.
  Use fixed-width types (`std::uint8_t`, `std::int64_t`, `std::size_t`) deliberately.
- No OpenMP (does not work with Emscripten). Use `std::thread` / `std::jthread`.
- No inline assembly, no compiler-specific intrinsics without a portable fallback.

## Language and style

- **C++23.** Use standard library facilities first:
  `std::expected`, `std::ranges`, `std::format`, `std::span`, `std::jthread`,
  `std::stop_token`, `if constexpr`, concepts.
- **Formatting:** `clang-format` with the repository `.clang-format`. Run it on every
  changed file. Do not hand-format.
- **Errors:** return `std::expected<T, E>` (project alias `result_t<T>`), do not throw
  across module boundaries. Use `and_then` for fallible steps, `transform` for
  infallible steps. Reserve `assert` for internal invariants, never for user input.
- **Ownership:** Rule of Zero. Do not hand-write copy/move constructors unless the
  type manages a resource directly.
- **Moves:** `std::move` only named lvalues you are done with. Never wrap a function
  call result (prvalue) in `std::move` — it disables copy elision.
- **Signedness:** loop indices over containers are `std::size_t`. No `int` vs
  `size_t` comparisons. Use `std::cmp_less` etc. for mixed-sign comparisons.
- **Readability over cleverness.** Flat, explicit code is preferred over deep
  template metaprogramming. A plain loop with an early `return` beats a clever fold.
- Each physical/model quantity gets its own documented function; state the
  equation in a comment above it.

## C++ Core Guidelines

Follow the [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines).
The `cpp-coding-standards` skill carries the full reference; the rules below are the ones
this codebase cares about most and that reviews enforce:

- **Express intent, strong types (P.1, P.3, I.4).** Prefer a named type over a bare
  scalar when units/meaning matter. Make interfaces precisely typed.
- **Immutability by default (Con.1–5, ES.25).** Start with `const`/`constexpr`.
  Member functions are `const` unless they mutate (Con.2). Pass `const&` for "in" params.
- **Type & sign safety (P.4, ES.46, ES.100).** No narrowing/lossy conversions; no mixed
  signed/unsigned arithmetic. Use `std::cmp_less`/`std::cmp_not_equal` for mixed-sign
  comparisons and `std::size_t` for container indices (restates the Signedness rule above).
- **Error handling (E.*, and the Errors rule above).** Signal failures with
  `result_t<T>` across module boundaries; never `throw` on user input. Reserve `assert`
  for internal invariants. Use purpose-designed error types, not built-ins or strings.
- **Functions (F.1–F.3, F.8, F.16, F.20/F.21).** One logical operation, short, prefer
  pure functions. Return values/structs instead of out-parameters.
- **Classes (C.2, C.9, C.20).** `struct` when members vary independently with no
  invariant, `class` when an invariant must hold. Minimize member exposure. Rule of Zero.
- **Resources (R.1, R.3, R.11, R.20/R.21).** RAII; no naked `new`/`delete`; raw pointers
  are non-owning; `unique_ptr` over `shared_ptr` unless ownership is shared.
- **Templates (T.10, T.11, T.13).** Constrain template arguments with concepts
  (`std::floating_point`, `std::integral`, …); prefer the standard concepts.
- **Expressions (ES.5, ES.20, ES.23, ES.47, ES.48).** Keep scopes small, always
  initialize, prefer `{}`, use `nullptr`, avoid C-casts.
- **Enums & naming (Enum.3, Enum.5, NL.9, NL.10).** `enum class` with lowercase
  enumerators; `underscore_style`; `ALL_CAPS` for macros only.
- **Headers (SF.7, SF.8, SF.11).** No `using namespace` at global scope in a header;
  include guard; self-contained (restates the Header layout rules below).

## Header layout (header-only)

The library is header-only. Separate interface from implementation by file:

```
include/sqsgen/<module>.h                      # public interface: types + declarations
include/sqsgen/impl/<module>/<module>_impl.hpp # template definitions
include/sqsgen/impl/<module>/<concern>_impl.hpp# further split by concern, if large
```

Rules:
- The public header declares; it includes its `*_impl.hpp` files **at the bottom**,
  after all declarations. Impl files are never included directly by users.
- Split impl files **by concern** (e.g. `configuration_impl.hpp`,
  `configuration_dto_impl.hpp`), not per function. Split further only when a file
  exceeds roughly 300 lines.
- Internal helpers live in `namespace sqsgen::<module>::detail` (or
  `sqsgen::detail`). **Never use anonymous namespaces in headers** (each TU would get
  its own copy).
- Non-template free functions defined in a header must be `inline`.
  Templates need no `inline`.
- Do not add explicit template instantiations; the library is header-only.
- Every header has an include guard (`#ifndef SQSGEN_..._H`) or `#pragma once`,
  consistent with the surrounding files.
- Include what you use; do not rely on transitive includes.

## Python

- Target **Python 3.12 using the stable ABI** (nanobind `STABLE_ABI`). Wheels are built
  once against 3.12 and are forward-compatible with later versions.
- Use only the stable (limited) C API in bindings. Do not add version-specific code
  paths or support for Python < 3.12.

## Dependencies

All dependencies are managed with **vcpkg** (manifest mode, `vcpkg.json`). Do not
vendor libraries, use `FetchContent`, or rely on system-installed packages.
A new dependency must be available as a vcpkg port for every target in the
"Supported targets" table, including the Emscripten triplet.

Policy: **standard library first, then header-only libraries.** Compiled
dependencies make the Emscripten, Pyodide and Python CI much harder. Ask before
adding any dependency, and never add one that requires a compiled library.

Current dependencies:

| Purpose | Library |
|---|---|
| Linear algebra / tensors | Eigen (incl. `unsupported/Eigen/CXX11/Tensor`) |
| JSON / YAML / CBOR / MessagePack / BEVE | glaze |
| Hash maps / sets | ankerl::unordered_dense |
| Python bindings | nanobind |
| WASM bindings | Emscripten (embind) |
| Distributed runs | MPI, **C API only** (no C++ wrapper) |

<!-- TODO: keep this table current; note any remaining abseil usage and its replacement plan. -->

### glaze conventions

- Domain types do not depend on glaze. Serialization goes through **DTO structs**
  (plain aggregates) converted with `to_dto<AMode>()` / `from_dto<AMode>()`.
- Eigen types never reach glaze directly. Tensors/matrices are converted to a plain
  representation chosen by `array_mode`:
  `nested` (human-readable nested arrays) or `packed` (`{shape, data}`, for storage/MPI).
- Use the compile-time format helpers (`read_format<DTO, wire_format::json>`,
  `write_format<...>`) when a type supports only some formats. Use the runtime
  `read_any` / `write_any` only for types that support every format (a runtime
  `switch` instantiates all formats).
- Avoid `std::size_t` / `unsigned long` in typed arrays written to CBOR; use
  `std::int64_t` or `std::uint32_t`.

### MPI conventions

- Use the plain MPI C API. Serialize payloads with glaze (BEVE) and send `MPI_BYTE`
  buffers; probe the size before receiving.
- MPI code lives in its own module. The single-node path must not contain MPI code;
  MPI differs only in a final reconciliation step.

## Performance

- The SRO / bond-counting loop is the hot path. In it: no allocation, no hashing,
  no logging, no virtual calls, no exceptions.
- Precompute everything that does not depend on the configuration (pair/triplet
  lists, shell indices, symmetry patterns) outside the loop.
- Store counts in small dense, contiguous arrays indexed by a canonical key, not in
  maps.
- Per-thread scratch buffers are allocated once and reused.
- Benchmark before and after any change to the hot path and report the numbers.

## Building and testing

All configuration goes through **CMake presets** (`CMakePresets.json`). Do not pass
ad-hoc `-D` flags or create build directories by hand; if a new configuration is
needed, add a preset.

<!-- TODO: fill in the actual preset names used in this repository. -->

- Configure and build: `cmake --preset <name>` then `cmake --build --preset <name>`.
- List available presets: `cmake --list-presets`.
- Prefer a native debug preset for debugging (WASM is hard to debug). It should set
  `CMAKE_BUILD_TYPE=Debug` and `CMAKE_EXPORT_COMPILE_COMMANDS=ON`.
- The vcpkg toolchain is set in the presets; do not override it.
- Build with warnings enabled and clean: at least `-Wall -Wextra`.
- Run the test suite and the Python tests before submitting.
- If you touch WASM- or Python-facing code, build those targets too.

## When working on this repository

- Make the smallest change that solves the task; do not refactor unrelated code.
- Keep the build green on all targets listed above.
- Do not change public APIs, file formats, or configuration keys without saying so
  explicitly.
- Explain non-obvious design decisions in a short comment.

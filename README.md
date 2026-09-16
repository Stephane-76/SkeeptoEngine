# Skeepto Engine

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](./LICENSE)
[![C++](https://img.shields.io/badge/C%2B%2B-20-00599C?logo=cplusplus&logoColor=white)](https://isocpp.org/)
[![CMake](https://img.shields.io/badge/build-CMake-064F8C?logo=cmake&logoColor=white)](https://cmake.org/)
[![WebAssembly](https://img.shields.io/badge/target-WASM-654FF0?logo=webassembly&logoColor=white)](https://webassembly.org/)

**C++20 spreadsheet calculation engine** used by [Skeepto](https://github.com/Stephane-76/Skeepto).
The same sources compile natively and to **WebAssembly** (browser + Node.js).

This repository is the engine only: libraries, unit tests, the WASM React
binding (`SkReactSpreadSheet`), the Excel converter (`SkExcel`), and the
spreadsheet stress tool (`SkPressureSp`). The UI and server live in the
Skeepto application repo.

## Layout

```
skeepto-engine/
├── CMakeLists.txt              # superbuild (configure once, build everything)
├── cmake/                      # Emscripten, third-party, wasm flags
├── Libraries/
│   ├── SkRoot/                 # variants, dates, files, containers
│   ├── SkFormat/               # number/date formats, CSS, styles
│   └── SkSpreadSheet/          # workbook, formulas, calculation path
├── Libraries_test/             # CppUnit suites
├── SkReactSpreadSheet/         # WASM module consumed by Skeepto
├── SkExcel/                    # .xlsx ↔ .sker converter
└── SkPressureSp/               # spreadsheet stress / volume tool
```

Third-party headers and libraries (RapidJSON, RapidXML, CppUnit, pugixml,
libzip) are **cloned and built by CMake** into `third-party/` — they are not
vendored in git. CppUnit is [Ultimaker/CppUnit](https://github.com/Ultimaker/CppUnit)
1.14.2 (CMake port of LibreOffice). The old `dlrdave/cppunit` 1.11 tree is not
C++17-compatible.

## Prerequisites

- **CMake** 3.16 or newer
- A **C++20** compiler (Apple Clang, GCC, or MSVC)
- **Git** (CMake fetches third-party repos on first configure)
- Native Excel converter: **libzip** (Homebrew / `libzip-dev` / vcpkg)
- WebAssembly: [Emscripten](https://emscripten.org/) (`emcc` on `PATH`, or `EMSDK`)

## Build (CMake only)

Use a **separate** `-B` directory per generator.

### Native (macOS / Linux)

```bash
cmake -B build-unix
cmake --build build-unix --parallel
cmake --build build-unix --target run-tests
```

Libraries and binaries land in `unix/lib` and `unix/bin`.

### Xcode

```bash
cmake -G Xcode -B build-xcode -DSK_PLATFORM=xcode
cmake --build build-xcode --parallel
```

### Windows

```bat
cmake -G "Visual Studio 17 2022" -A x64 -B build-windows -DSK_PLATFORM=windows
cmake --build build-windows --parallel
```

### WebAssembly

```bash
cmake -B build-wasm -DSK_PLATFORM=wasm
cmake --build build-wasm --parallel
```

That produces **two** `SkReactSpreadSheet` modules (they must not share a CMake
tree: Node sets `-DSK_NODE` and `ENVIRONMENT=node`):

| Artifact | Host | Copied to Skeepto |
|----------|------|-------------------|
| `wasm/bin/SkReactSpreadSheet.js` | browser (`ENVIRONMENT=web`) | `public/SkReactSpreadSheet.mjs` |
| `wasm/bin/SkReactSpreadSheetNode.js` | Node (`ENVIRONMENT=node`) | `Node/Server/SkReactSpreadSheet.mjs` |

Pass the app tree with `-DSK_SKEEPTO_DIR=/path/to/skeepto`. If it is empty,
the superbuild uses the sibling `../skeepto` when that folder exists.
`-DSK_SKEEPTO_DIR=NONE` skips the copy (target `skeepto-sync` is omitted).

Release vs debug wasm: `-DSK_COMPIL_MODE=RELEASE` (default) or `DEBUG`.
64-bit Memory64: `-DSK_MEMORY64=ON` (artifacts go to `wasm64/`; Node module
is skipped, same as the old `compil2Wasm.sh`).

Useful options:

| Option | Default | Meaning |
|--------|---------|---------|
| `SK_BUILD_TESTS` | `ON` | CppUnit executables |
| `SK_BUILD_APPS` | `ON` | `SkExcel` + `SkPressureSp` + `SkReactSpreadSheet` |
| `SK_BUILD_SKEXCEL_LIB` | `ON` | in-process `SkExcelLib.js` (wasm only) |
| `SK_BUILD_REACT_NODE` | `ON` | Node `SkReactSpreadSheetNode.js` (wasm32 only) |
| `SK_SKEEPTO_DIR` | sibling `../skeepto` | Skeepto app root; `NONE` disables copy |
| `SK_FETCH_THIRD_PARTY` | `ON` | git clone missing deps |
| `SK_PLATFORM` | auto | `unix`, `xcode`, `windows`, `wasm` |

Libraries only (no tests, no apps):

```bash
cmake -B build-unix -DSK_BUILD_TESTS=OFF -DSK_BUILD_APPS=OFF
cmake --build build-unix --parallel
```

## Artifacts for Skeepto

With `-DSK_SKEEPTO_DIR` (or a sibling `../skeepto`), the `skeepto-sync` target
copies the **browser** module into `public/` and the **Node** module into
`Node/Server/` (and `Node/Client/` if that folder exists). It does **not**
touch `skeepto/build/` — from Skeepto, sync `public/` → `build/` (`npm run build`
or copy the three browser files) and hard-refresh (**Cmd+Shift+R**).

Manual copy if you skipped the sync:

```bash
cp wasm/bin/SkReactSpreadSheet.js       ../skeepto/public/SkReactSpreadSheet.mjs
cp wasm/bin/SkReactSpreadSheet.wasm     ../skeepto/public/
cp wasm/bin/SkReactSpreadSheet.wasm.map ../skeepto/public/

cp wasm/bin/SkReactSpreadSheetNode.js       ../skeepto/Node/Server/SkReactSpreadSheet.mjs
cp wasm/bin/SkReactSpreadSheetNode.wasm     ../skeepto/Node/Server/SkReactSpreadSheet.wasm
cp wasm/bin/SkReactSpreadSheetNode.wasm.map ../skeepto/Node/Server/SkReactSpreadSheet.wasm.map
```

Do not copy the browser `.wasm` into `Node/Server/` (or the Node `.wasm` into
`public/`): `-DSK_NODE` changes EM_JS layout, so a mismatched `.mjs`/`.wasm`
pair aborts at runtime.

`SkExcel.js` / `SkExcelLib.js` (xlsx conversion) and `SkPressureSp.js`
(stress tool) land in the same `wasm/bin/` tree.

## License

MIT — see [`LICENSE`](./LICENSE).

Copyright © 2026 Stéphane ALLEZ.

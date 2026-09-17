# Skeepto Engine

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](./LICENSE)
[![C++](https://img.shields.io/badge/C%2B%2B-20-00599C?logo=cplusplus&logoColor=white)](https://isocpp.org/)
[![CMake](https://img.shields.io/badge/build-CMake-064F8C?logo=cmake&logoColor=white)](https://cmake.org/)
[![WebAssembly](https://img.shields.io/badge/target-WASM-654FF0?logo=webassembly&logoColor=white)](https://webassembly.org/)
[![Functions](https://img.shields.io/badge/worksheet%20functions-344-2ea44f)](#344-worksheet-functions)

**A C++20 spreadsheet engine built to outrun Excel.** Same sources, native or
**WebAssembly** — browser and Node.js. Used by [Skeepto](https://github.com/Stephane-76/Skeepto).

Excel is a desktop product. This is a **calculation core**: sparse cells,
index-based rows and columns, shared formulas, incremental recalc. Inserting
or deleting a row does not rewrite every address. Large workbooks stay
responsive where Excel stalls.

Drive it with a **clear API** (`tApi` in C++, `UISpreadSheet` in JavaScript) —
workbooks, cells, formulas, format, undo, JSON. No COM, no VBA, no opaque
add-in. The same 344 worksheet functions run in the grid, in a headless
server, and under an agent.

This repository is the engine only: libraries, unit tests, the WASM React
binding (`SkReactSpreadSheet`), the Excel converter (`SkExcel`), and the
spreadsheet stress tool (`SkPressureSp`). The UI and server live in the
Skeepto application repo.

## Why this engine

| | What you get |
| --- | --- |
| **Faster than Excel** | Native C++20, not a JavaScript grid. Recalc is a **Kahn** work-queue on the dirty graph (not a full-sheet sweep). Cycles go through **Tarjan SCC** then **Gauss–Seidel**. Shared formulas are pooled. Insert/delete is index-based. Same binary in the browser (WASM) and on the server. |
| **344 worksheet functions** | Math, stats, text, logical, lookup, date, financial, and **dynamic arrays** (`FILTER`, `SORT`, `UNIQUE`, `MAP`, `REDUCE`, `SCAN`, `XLOOKUP`, `LET`, …). Excel-compatible names. |
| **Clear API** | One class to create a workbook, write `A1`, compile `=SUM(A1:A2)`, read the result, format, undo, serialize to JSON. Same surface in C++ and in JavaScript. |

```javascript
const ss = new SpreadSheet.UISpreadSheet();
ss.NewWorkBook("demo");
ss.Value("A1", "10", "Sheet1");
ss.Value("A2", "20", "Sheet1");
ss.Value("B1", "=SUM(A1:A2)", "Sheet1");
ss.GetValue("B1", "Sheet1");   // "30"
```

C++ uses the same idea through `tApi` (`CellValue`, sheets, named ranges,
recalc). WASM exposes ~150 methods on `UISpreadSheet`: cells, fill series,
rows/columns, copy/paste, CSS format, conditional format, named formulas,
floating objects, viewport JSON, cooperative recalc.

## 344 worksheet functions

Registered in `Libraries/SkSpreadSheet` (Excel aliases counted separately,
e.g. `STDEV` / `STDEV_S`):

| Family | Count | Highlights |
|--------|------:|------------|
| Math & statistics | 166 | `SUM`, `AVERAGE`, `LINEST`, `FORECAST`, distributions, engineering bases |
| Text | 36 | `TEXTJOIN`, `TEXTSPLIT`, `REGEXEXTRACT`, `REGEXREPLACE` |
| Logical | 35 | `IF`, `IFS`, `SWITCH`, `LET`, `LAMBDA` helpers (`ISOMITTED`) |
| Dynamic arrays | 29 | `FILTER`, `SORT`, `UNIQUE`, `MAP`, `REDUCE`, `SCAN`, `BYROW`, `BYCOL` |
| Lookup / sheet | 29 | `XLOOKUP`, `XMATCH`, `INDEX`, `INDIRECT`, `OFFSET` |
| Date & time | 25 | `NETWORKDAYS_INTL`, `WORKDAY_INTL`, `DATEDIF` |
| Financial | 24 | `PMT`, `XIRR`, `XNPV`, `CUMIPMT` |
| **Total** | **344** | |

Modern Excel is in there: dynamic arrays, `XLOOKUP`, `LET`, higher-order
`MAP` / `REDUCE` / `SCAN`. Import `.xlsx` with `SkExcel`; formulas keep their
names.

## How calculation is optimized

Formulas compile once (Lemon **LALR(1)** parser → **RPN** opcodes) and are
**interned**: identical formulas share one `tSharedFormula`. Recalc never
walks the whole sheet. It builds a sparse **calculation path** (`tPath`) of
dirty cells and their dependents, each node carrying an in-degree
(`m_NbDepend`).

| Step | Algorithm | What it does |
|------|-----------|----------------|
| Ready cells | **Kahn’s algorithm** (work-queue topological sort) | Seed every path with in-degree 0, `Resolve`, decrement dependents. Each cell runs once. Linear in the dirty graph — not a restart-from-head scan. |
| Ranges | Range recovery + implicit intersection | Formulas that cover a written range are pulled onto the path. Single-column refs (`SUMIF` on `G:G`) stay O(1) edges, not a full-column expansion. |
| Blocked leftover | Local adjacency from `VectorRef`, then **Kahn** again | When the queue empties but cells remain (range-covered cycles, `SUMIF` columns), rebuild a small graph and try a full topological order. |
| True cycles | **Tarjan’s algorithm** (strongly connected components) | SCCs of the leftover graph. The **condensation DAG** is sorted with Kahn. Acyclic components evaluate once. |
| Cyclic SCC | **Gauss–Seidel** iteration | In-place re-eval of the component until values stabilize (ε = 1e−12, max 40 passes). Divergence (`|x| > 1e15` or non-finite) → `#RECURSIVE`. |
| UI / WASM | Cooperative **time-sliced** `ReduceStep` | Same Kahn + blocked-subgraph path, yielded every few milliseconds so the grid stays interactive. |
| Named formulas | Per-pass eval cache | A named formula is computed once per caller cell in a recalc, not on every reference. |

Spill / dynamic arrays (`FILTER`, `UNIQUE`, `MAP`, …) wait on the origin cell
before dependents of the spilled range run. Financial solvers (`RATE`,
`IRR`, `XIRR`) use **Newton–Raphson** inside the function, not in the graph
scheduler.

The graph lives in `Libraries/SkSpreadSheet/source/SkCalculationPath.cpp`.

## Layout

```
skeepto-engine/
├── CMakeLists.txt              # superbuild (configure once, build everything)
├── cmake/                      # Emscripten, third-party, wasm flags
├── File/                       # workbook fixtures for a complete native test run
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

Workbook fixtures for a complete native test run live in `File/`
(`Budget.sker`, `Budget-familial.sker`, `AmortBis.sker`, `AmortBis.json`,
`PretBis.sker`, `PretBis.json`, `Calendar.sker`, and
`Calendrier sur 12 mois1.sker`). Override with `SKER_EXCEL_TEST_DIR`.

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
64-bit Memory64: `-DSK_MEMORY64=OFF` (default, wasm32) or `ON` (artifacts go
to `wasm64/`; the Node module is skipped).

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

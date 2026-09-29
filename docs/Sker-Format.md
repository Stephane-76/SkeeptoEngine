# The `.sker` workbook format

A `.sker` file is the native workbook of Skeepto. It is **UTF-8 JSON**, one
object per file. There is no binary container and no compression wrapper: a
text editor, `JSON.parse`, or an MCP client can read it.

The engine writes it from `tWorkBook::Json` and reads it back with
`tWorkBook::Json(const Value&)`:

- [`SkWorkBook.cpp`](../Libraries/SkSpreadSheet/source/SkWorkBook.cpp)
- Key names: [`SkJsonKey.hpp`](../Libraries/SkSpreadSheet/include/SkJsonKey.hpp)

The class model around that JSON is described in
[`Spreadsheet-Structure.md`](./Spreadsheet-Structure.md).

Keys are short on purpose (wire size). A cell address is `"c"`, a value type
is `"t"`, a shared string index is `"si"`. The same document is what the MCP
server returns from `read_workbook`.

---

## 1. Top-level object

```json
{
  "id": 1,
  "uri": "/home/ada/Budget.sker",
  "info": { "author": "", "email": "", "date-creation": "", "date-modification": "", "v-major": 0, "v-minor": 0, "v-patch": 0, "comment": "" },
  "sizecol": 22.0,
  "sizerow": 5.0,
  "defaultfontname": "Calibri",
  "defaultfontsize": 11,
  "printParameters": { },
  "sheets": [ ],
  "namedranges": [ ],
  "formulanamed": [ ],
  "floatingobjects": [ ],
  "si": [ ],
  "fi": [ ],
  "f": { "formats": [ ] },
  "models": { }
}
```

| Key | Required | Role |
|-----|----------|------|
| `id` | yes | Allocator id of the workbook inside the process. Not a stable file identity. |
| `uri` | yes | Path the workbook was saved under. |
| `info` | yes | Author, email, creation and modification dates (`US` date text), version, comment. |
| `sizecol`, `sizerow` | yes | Default column width and row height, in millimeters. |
| `defaultfontname`, `defaultfontsize` | no | Workbook default font. Missing files load as Calibri 11. |
| `printParameters` | yes | Shared page setup (paper, margins, scale). Orientation and fit-to-page live on each sheet. |
| `models` | no | Cell-class schemas used by widgets in this file. Omitted when the workbook has none. |
| `sheets` | yes | Array of sheets, including the hidden host sheets when they exist. |
| `namedranges` | no | Named ranges and structured tables. |
| `formulanamed` | no | Named formulas (`{ "n", "f" }`). |
| `floatingobjects` | no | Charts, images, text boxes. |
| `si` | no | Shared string table. Cell text is an index into this array. |
| `fi` | no | Shared formula table. A formula cell stores an index into this array. |
| `f` | no | Interned CSS formats. A cell stores an index (`fo`) into `f.formats`. |

`si` and `fi` are omitted when empty. Inline lambda helpers (`_INLLMB_…`) are
not written; load rebuilds them from the `LAMBDA(...)` text stored on the cell.

---

## 2. A sheet

Each element of `sheets` is one sheet. Empty rows, columns, and cells are
omitted. Only bands and cells that carry content, a size, a format, or an
outline group are stored.

```json
{
  "name": "Sheet1",
  "rows": [ { "i": 1, "s": 5.0 } ],
  "cols": [ { "i": 1, "s": 22.0 } ],
  "cells": [ ],
  "merged": [ "A1:B2" ],
  "cf": [ ],
  "printParameters": { "orientation": "portrait", "fitToPage": true },
  "sv": 3,
  "sh": 10,
  "viewZoom": 100,
  "showGridLines": false
}
```

| Key | Role |
|-----|------|
| `name` | Sheet tab name. |
| `rows`, `cols` | Sparse row and column bands. |
| `cells` | Non-empty cells. |
| `merged` | Merged rectangles as A1 strings. |
| `cf` | Conditional-format rules. Omitted when the sheet has none. |
| `printParameters` | Per-sheet orientation and fit-to-page. |
| `sv`, `sh` | Frozen panes: split column index and split row index. Omitted when there is no split (`-1`). |
| `viewZoom` | Zoom percent. Omitted at 100%. |
| `showGridLines` | Present and `false` when grid lines are hidden. |

A row or column band:

| Key | Role |
|-----|------|
| `i` | 1-based index. |
| `s` | Size in millimeters. `-1` means the workbook default. |
| `p` | Parent index in the outline. |
| `c` | Child indexes in the outline. |
| `o` | `false` when the outline group is collapsed. Open groups omit the key. |
| `fo` | Index into `f.formats` for a row-wide or column-wide style. |

### Internal sheets

Names that start with `_$$` are engine sheets. They are part of the file and
must round-trip, and they are not user tabs.

| Name | Role |
|------|------|
| `_$$N` | Host cells of named formulas. |
| `_$$A` | Host cells of floating objects (charts, images, text boxes). |

`read_workbook` hides them unless the caller sets `includeInternalSheets`.

---

## 3. A cell

```json
{ "c": "B2", "fi": 0, "t": "d", "v": 12.5, "fo": 3 }
```

| Key | Role |
|-----|------|
| `c` | A1 address (`B2`, `AA10`). |
| `f` | Formula text, used when the shared-formula table is off. |
| `fi` | Index into the workbook `fi` array. This is the normal form. |
| `t`, `v` | Cached value. Present for literals and for formula results, so a load can restore the grid before a full recalculation. |
| `si` | Index into `si` when the value is a shared string. Replaces `t`/`v` for that string. |
| `fo` | Index into `f.formats`. |
| `class`, `cl` | Cell-class widget: factory name and its attribute payload. |
| `extend` | Bounded spill rectangle of an array formula. |
| `ex` | Extension bits (matrix origin, spill, and so on). |
| `spillrange` | A1 rectangle filled by a dynamic array. |

Formulas in `fi` (and in `f` on the cell) are stored in **R1C1**, with
US-style list separators (comma between arguments, dot decimal). Display in
the formula bar converts that back to A1 and to the active locale.

`t` is a one- or two-letter type tag. `v` is the payload.

| `t` | `v` |
|-----|-----|
| `n` | JSON `null` |
| `i` | integer |
| `b` | boolean |
| `d` | number |
| `s` | string |
| `da` | US date text |
| `e` | error object (`#DIV/0!`, `#REF!`, `#N/A`, …) |
| `c` | object value: `{ "n": "ClassName", "v": … }` or `{ "n", "o" }` when the model is saved with the instance |

A widget such as a chart is a class value on its host cell. Its properties
(`Title`, `DataRange`, …) are real cells addressed as `A1.Title` in formulas;
in the file they travel inside `cl`.

---

## 4. Shared tables and formats

`si` and `fi` are flat string arrays at the workbook root. A repeated label
or a copied formula is stored once. The cell keeps the integer index.

```json
"si": ["Revenue", "Cost"],
"fi": ["RC[-1]+1", "SUM(R[-2]C:R[-1]C)"]
```

`f.formats` is the style pool. Each entry is `{ "f": "<css>" }`. Cells, rows,
and columns point at an entry with `fo`. The same style used by a hundred
cells is one string in the pool.

```json
"f": {
  "formats": [
    { "f": "font-weight:700;background-color:#1F4E79;color:#FFFFFF" },
    { "f": "text-align:right;format:'#,##0.00'" }
  ]
}
```

Number and date pictures use Excel syntax inside that CSS (`#,##0.00`,
`dd/mm/yyyy`).

---

## 5. Names, tables, floating objects

Named range:

```json
{ "n": "Rates", "s": "Sheet1", "r": "B2:B20" }
```

A multi-area name joins rectangles with `;` (`"A1:B2;D5:E6"`). A structured
table is the same object plus a `data` member: columns, filters, sort, header
row, totals row, and `tableStyleName`.

Named formula:

```json
{ "n": "TaxRate", "f": "0.2" }
```

Floating object (chart, image, text box):

| Key | Role |
|-----|------|
| `n` | Instance name. |
| `c` | Cell-class name (`SkCellClassLineChart`, …). |
| `t` | Sheet the object is drawn on. |
| `r` | Host row on `_$$A`. |
| `anchorCell` | Visual anchor, for example `Sheet1!B5`. |
| `diffX`, `diffY`, `width`, `height`, `opacity`, `zIndex` | Layout. |

The widget’s data stays on the host cell in `_$$A`. The floating-object entry
is only the registry and the geometry.

---

## 6. What the file does not contain

- The undo stack. Collaboration messages use the same key vocabulary, but they
  are not stored inside the `.sker`.
- VBA or macros. An `.xlsx` import drops them.
- Empty cells. A blank `A1` leaves no entry in `cells`.
- The React viewport (`JsonView`). That JSON is a paint snapshot of the
  visible window. The file is the full workbook.

Load order matters and is fixed in `tWorkBook::Json`: shared strings and
formulas, then formats, then sheet shells, then names, then cell contents,
then floating objects. A hand-edited file has to keep `fi` / `si` / `f`
indexes consistent with the cells that point at them.

---

## 7. Readable by an MCP

[MCP](https://modelcontextprotocol.io/) (Model Context Protocol) is a way for
an agent to call tools over HTTP. Because a `.sker` file is JSON, the agent
does not need a private binary parser. The Skeepto server exposes the workbook
as MCP tools. The server name is `sker-spreadsheet`, on `POST /mcp`
([`SkAiFastify.mjs`](../../skeepto/Node/Server/SkAI/SkAiFastify.mjs) in the
skeepto app).

`read_workbook` is the direct read:

1. The server loads the `.sker` into the same WASM engine that edits the grid.
2. `WriteJson` emits the document described above.
3. If that call fails, the server falls back to the JSON stored in MongoDB /
   GridFS, which is the same document.
4. The tool returns that object. Internal `_$$` sheets are left out unless
   `includeInternalSheets` is true. A `sheet` argument keeps a single tab.

The response (`buildReadWorkbookResponse` in
`skeepto/Node/Server/SkSpreadSheet/SkReadWorkbook.mjs`) wraps the file:

| Field | Role |
|-------|------|
| `path` | Virtual-disk path, for example `/share/Budget.sker`. |
| `scope` | `workbook` or `sheet`. |
| `source` | `wasm` (live engine) or `storage` (persisted file). |
| `workbook` | The `.sker` object. |
| `_hint` | Reminder that `si` is strings, `fi` is R1C1 formulas, and `fo` indexes `f.formats`. |

Other tools on the same server read or write that workbook and save it back
as `.sker`: `list_files`, `open_workbook`, `list_sheets`, `read_cell`,
`read_range`, `write_cell`, `paste_grid`, `apply_format`, `merge_cells`,
`create_table`, `list_tables`, `save_workbook`. They are registered in
[`SkMcpToolRegistrar.mjs`](../../skeepto/Node/Server/SkAI/SkMcpToolRegistrar.mjs).

The C++ engine does not speak MCP. It only reads and writes the JSON. The
Node server is the MCP endpoint, and the file format is the language the
agent sees.

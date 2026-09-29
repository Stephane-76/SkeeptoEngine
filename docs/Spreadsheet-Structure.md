# Spreadsheet structure

This document describes the **C++ model** of the Skeepto spreadsheet: what owns
what, and the role of each structural class, with the header where it is
declared.

Paths below are relative to `skeepto-engine/`. Classes live in
`namespace SkSpreadSheet`, except CSS formatting (`SkFormat`) and Excel import
(`SkExcel` / `SkRoot`).

The `.sker` file this model is saved as is described in
[`Sker-Format.md`](./Sker-Format.md). It is UTF-8 JSON and is what an MCP
client receives from `read_workbook`.

The React chrome (ribbon, canvas, panels) and the cell widgets
(`SkCellClassCheck`, charts) are documented in the skeepto app repository:
`skeepto/docs/Spreadsheet-React-Classes.md` and
`skeepto/docs/Spreadsheet-CellClass.md`. Units (`tCellUnit`) are in the Unit
section of `Spreadsheet-React-Classes.md`.

---

## 1. Core idea

A cell stores allocator references to its row `tColRow` and its column
`tColRow`. The numeric index (`m_Index`) lives on `tColRow`. Inserting or
deleting a row shifts those indexes, then rebases formulas. Cells themselves
are not rewritten one by one.

A formula does not copy its text into every cell. The bytecode (`tFormula`)
is interned in `tSharedFormulaPool`. The cell keeps a handle
(`tSharedFormula`) and the list of what it reads (`m_VectorRef`). Each source
keeps the inverse list of formulas that depend on it
(`m_ContainerCellDepend` on `tItem`). Recalculation walks that graph.

```
tSpreadSheetContainer          open workbooks, function dictionary, parser
└─ tWorkBook                   one .sker file
   ├─ tSheet                   visible sheets
   │  └─ tColRowCellRange      sparse grid of the sheet
   │     ├─ tColRow            one row or one column (index, size, outline)
   │     ├─ tCell              value + formula + format
   │     ├─ tRange             rectangle referenced by a formula (A1:B10)
   │     └─ tCellAttribute     a widget property (A1.Title)
   ├─ sheet _$$N               named formulas (hidden)
   ├─ sheet _$$A               floating-object anchors (hidden)
   ├─ tRangeNamedContainer     names and structured tables
   ├─ tFloatingObjectContainer charts, images, text boxes
   └─ tPrintParameters         shared page setup
```

Sheets whose name starts with `_$$` are internal (`IsSystemSheetName` in
[`SkColRow.hpp`](../Libraries/SkSpreadSheet/include/SkColRow.hpp)). They never
become the active sheet of the UI.

---

## 2. From the browser to the engine

```
React  SkSpreadSheet / SkSpInterface
         │
         ▼
JS     SkUISpreadSheet          skeepto/src/spreadsheet/SkUISpreadSheet.js
         │  window.SpreadSheet.UISpreadSheet
         ▼
WASM   tUISpreadSheet           SkReactSpreadSheet/include/SkUISpreadSheetApi.hpp
         │
         ▼
       tInterfaceWeb            Libraries/SkSpreadSheet/include/SkInterfaceWeb.hpp
         │  messages, user, web undo
         ▼
       tApi                      Libraries/SkSpreadSheet/include/SkApi.hpp
         │  edit, format, copy-paste, JsonView
         ▼
       tWorkBook / tSheet / tCell
```

`tApi` is the low-level API. `tInterfaceWeb` adds the user identity, the undo
stack, and collaboration messages. `tUISpreadSheet` is the facade exposed to
JavaScript (Emscripten): every ribbon action lands here, then goes down to
`tApi`.

After a mutation, the canvas does not read cells one by one. `tJsonView`
serializes the **visible window** (plus a margin) as JSON. React paints that
snapshot.

---

## 3. Container and workbook

| Class | File | Role |
|-------|------|------|
| `tSpreadSheetContainer` | [`SkSpreadSheet.hpp`](../Libraries/SkSpreadSheet/include/SkSpreadSheet.hpp) | Process root. Allocates workbooks, holds the active workbook, owns the function dictionary, the Lemon parser, and the shared-formula pool. Loads and saves `.sker` JSON. |
| `tWorkBook` | [`SkWorkBook.hpp`](../Libraries/SkSpreadSheet/include/SkWorkBook.hpp) | One file. Sheet list, active sheet, names, floating objects, table styles, print setup, default font, collaborative rebase log. Runs recalculation, including cooperative step-by-step recalc. |
| `tWorkBookInfo` | [`SkWorkBook.hpp`](../Libraries/SkSpreadSheet/include/SkWorkBook.hpp) | Metadata: author, dates, version, comment. |
| `tSheet` | [`SkSheet.hpp`](../Libraries/SkSpreadSheet/include/SkSheet.hpp) | One sheet: name, pointer to its grid, sheet CSS, frozen panes (`m_Splitter`), print orientation and zoom, grid lines. |
| `tPrintParameters` | [`SkPrintParameters.hpp`](../Libraries/SkSpreadSheet/include/SkPrintParameters.hpp) | Paper, margins, scale, page order. Shared by the workbook. Orientation and fit-to-page stay on `tSheet`. |

`tWorkBook` and `tSheet` derive from `SkSpAncestor`, which is an alias of
`tClass` ([`SkColRow.hpp`](../Libraries/SkSpreadSheet/include/SkColRow.hpp)).

---

## 4. The grid

Everything on a sheet lives in **one** `tColRowCellRange`. Empty cells are
not allocated: allocators and sparse arrays (`tSparseArray`) materialize only
the rows, columns, and cells that are actually used.

| Class | File | Role |
|-------|------|------|
| `tColRowCellRange` | [`SkColRowCellRange.hpp`](../Libraries/SkSpreadSheet/include/SkColRowCellRange.hpp) | Grid of one sheet. Allocators for rows, columns, cells, ranges, and attributes. Search, insert, dependencies, graph checks. |
| `tStaticColRowCellRange` | [`SkColRowCellRange.hpp`](../Libraries/SkSpreadSheet/include/SkColRowCellRange.hpp) | Static access to the current grid during parse and evaluation. |
| `tColRow` | [`SkColRow.hpp`](../Libraries/SkSpreadSheet/include/SkColRow.hpp) | One row **or** one column. Holds the index, the size (mm), the format, the cell count, and the outline (parent, children, open / closed). Also lists the ranges that cross this row or column. |
| `tItem` | [`SkItem.hpp`](../Libraries/SkSpreadSheet/include/SkItem.hpp) | Ancestor of `tCell` and `tRange`. Type (`t_Cell`, `t_Range`, `t_Attribute`), position by allocator reference, and the inverse list of formulas that depend on this item. |
| `tCell` | [`SkCell.hpp`](../Libraries/SkSpreadSheet/include/SkCell.hpp) | A cell. Value (`tVariant`), formula handle, format (`tFormatRef`), references read by the formula, calculation-path node. Implements `tInterfaceCompil` so the parser can attach a formula. |
| `tCellExtend` | [`SkCell.hpp`](../Libraries/SkSpreadSheet/include/SkCell.hpp) | Output rectangle of an array formula (CSE / bounded spill area). |
| `tRange` | [`SkRange.hpp`](../Libraries/SkSpreadSheet/include/SkRange.hpp) | Rectangle (top, left, bottom, right) pointed at by a formula (`SUM(A1:B10)`). Formulas that use it are its dependents. |
| `tCellAttribute` | [`SkCellAttribute.hpp`](../Libraries/SkSpreadSheet/include/SkCellAttribute.hpp) | Property cell of a widget, addressed as `A1.Title`. It is a real calculation cell, derived from `tCell`. |
| `tCallBackFindCell` | [`SkColRowCellRange.hpp`](../Libraries/SkSpreadSheet/include/SkColRowCellRange.hpp) | Grid walk for Find (match case, entire cell). |
| `tCallBackFindUniqueValue` | [`SkColRowCellRange.hpp`](../Libraries/SkSpreadSheet/include/SkColRowCellRange.hpp) | Collects distinct values (table filters). |

`tTypeItem` and the `tExtension` bits (`t_Named`, `t_Merged`, `t_Data`, spill,
conditional format) are declared in
[`SkStackElem.hpp`](../Libraries/SkSpreadSheet/include/SkStackElem.hpp).

---

## 5. Formulas

### Compilation

The text `=A1+SUM(B1:B10)` goes through three stages.

| Class | File | Role |
|-------|------|------|
| `tLexer` | [`SkLexerSpreadSheet.hpp`](../Libraries/SkSpreadSheet/include/SkLexerSpreadSheet.hpp) | Splits the text into tokens (cell, operator, name, number). |
| `tLexerToken` | [`SkLexerSpreadSheet.hpp`](../Libraries/SkSpreadSheet/include/SkLexerSpreadSheet.hpp) | One token: kind (`tKind`) and value. |
| `tKind` | [`SkLexerSpreadSheet.hpp`](../Libraries/SkSpreadSheet/include/SkLexerSpreadSheet.hpp) | Enumeration of token kinds. |
| `tLexerData` | [`SkLexerData.hpp`](../Libraries/SkSpreadSheet/include/SkLexerData.hpp) | Structured table references (`Table[[#This Row],[Col]]`). |
| `tLemonReserved` | [`SkLemonReserved.hpp`](../Libraries/SkSpreadSheet/include/SkLemonReserved.hpp) | Reserved words and function names known to the parser. |
| `tLemonIdRef` | [`SkLemonReserved.hpp`](../Libraries/SkSpreadSheet/include/SkLemonReserved.hpp) | Resolves an identifier (name, sheet, class). |
| `tLemonInterface` | [`SkLemonInterface.hpp`](../Libraries/SkSpreadSheet/include/SkLemonInterface.hpp) | Bridge between the Lemon grammar and the workbook. Creates cells, ranges, attributes (`A1.attr`), and formula errors. |
| `tLemonFunctionMethod` | [`SkLemonInterface.hpp`](../Libraries/SkSpreadSheet/include/SkLemonInterface.hpp) | Descriptor of a function call as seen by the parser (name, arity, by-reference). |
| `tErrorFormula` | [`SkLemonInterface.hpp`](../Libraries/SkSpreadSheet/include/SkLemonInterface.hpp) | Compile-time error family: syntax, function, data, reference, attribute. |
| `tInterfaceCompil` | [`SkInterfaceCompil.hpp`](../Libraries/SkSpreadSheet/include/SkInterfaceCompil.hpp) | Contract “I can carry a formula”. Implemented by `tCell` and `tConditionalFormat`. |
| grammar | [`SkLemonSpreadSheet.y`](../Libraries/SkSpreadSheet/lemon/SkLemonSpreadSheet.y) | Lemon grammar. Generated code is `SkLemonSpreadSheet.cpp` / `.h`. |

### Representation and sharing

| Class | File | Role |
|-------|------|------|
| `tItemFormula` | [`SkFormula.hpp`](../Libraries/SkSpreadSheet/include/SkFormula.hpp) | One bytecode instruction (operator, call, constant, reference). |
| `tFormula` | [`SkFormula.hpp`](../Libraries/SkSpreadSheet/include/SkFormula.hpp) | Sequence of instructions. Can render itself as A1 or R1C1. |
| `tVolatile` | [`SkFormula.hpp`](../Libraries/SkSpreadSheet/include/SkFormula.hpp) | Volatility: none, whole sheet, row, or column (`TODAY`, `ROW`, …). |
| `tSharedFormulaItem` | [`SkSharedFormula.hpp`](../Libraries/SkSpreadSheet/include/SkSharedFormula.hpp) | One unique formula in the pool, with a user count. |
| `tSharedFormulaPool` | [`SkSharedFormula.hpp`](../Libraries/SkSpreadSheet/include/SkSharedFormula.hpp) | Pool owned by the container. Two cells with the same bytecode share one item. Lives on `tSpreadSheetContainer`. |
| `tSharedFormula` | [`SkSharedFormula.hpp`](../Libraries/SkSpreadSheet/include/SkSharedFormula.hpp) | Lightweight handle stored on `tCell`. Increments and decrements the pool count. |

### Evaluation

| Class | File | Role |
|-------|------|------|
| `tStackElem` | [`SkStackElem.hpp`](../Libraries/SkSpreadSheet/include/SkStackElem.hpp) | One element of the evaluation stack: scalar, range, or error. |
| `tFunction` | [`SkFunction.hpp`](../Libraries/SkSpreadSheet/include/SkFunction.hpp) | Ancestor of a worksheet function. `Call` consumes the stack and pushes the result. |
| `tFunctionRef` | [`SkFunction.hpp`](../Libraries/SkSpreadSheet/include/SkFunction.hpp) | Dictionary entry: canonical name and pointer to the implementation. |
| `tFunctionDictionary` | [`SkFunction.hpp`](../Libraries/SkSpreadSheet/include/SkFunction.hpp) | Name → function registry. Filled when the workbook starts. Shared by `tSpreadSheetContainer`. |
| `tCallBackRangeFunction` | [`SkFunction.hpp`](../Libraries/SkSpreadSheet/include/SkFunction.hpp) | Walks a sparse range for `SUM`, `AVERAGE`, `MIN`, `MAX`. |
| `tPath` | [`SkCalculationPath.hpp`](../Libraries/SkSpreadSheet/include/SkCalculationPath.hpp) | Node of the recalc graph, anchored on a cell. `m_NbDepend` counts precedents that are still uncalculated. |
| `tContainerPath` | [`SkCalculationPath.hpp`](../Libraries/SkSpreadSheet/include/SkCalculationPath.hpp) | Builds the topological order and runs recalculation. At zero remaining dependencies, the cell is pushed and evaluated. |
| `tMatrix` | [`SkMatrix.hpp`](../Libraries/SkSpreadSheet/include/SkMatrix.hpp) | Array result (spill). Writes the rectangle to the right and below the formula cell, or into a named-formula buffer. |

Concrete functions (`tFunctionSum`, `tFunctionIf`, `tFunctionVLookup`, …) are
one class per Excel function. They are declared in:

| File | Family |
|------|--------|
| [`SkFunctionMath.hpp`](../Libraries/SkSpreadSheet/include/SkFunctionMath.hpp) | Sum, statistics, rounding, trigonometry |
| [`SkFunctionLogical.hpp`](../Libraries/SkSpreadSheet/include/SkFunctionLogical.hpp) | IF, AND, OR, IS…, LET |
| [`SkFunctionText.hpp`](../Libraries/SkSpreadSheet/include/SkFunctionText.hpp) | Text |
| [`SkFunctionDate.hpp`](../Libraries/SkSpreadSheet/include/SkFunctionDate.hpp) | Dates and times |
| [`SkFunctionFinancial.hpp`](../Libraries/SkSpreadSheet/include/SkFunctionFinancial.hpp) | Financial |
| [`SkFunctionSpreadSheet.hpp`](../Libraries/SkSpreadSheet/include/SkFunctionSpreadSheet.hpp) | Lookup, INDEX, MATCH, ROW, COUNT, JSON |
| [`SkFunctionArray.hpp`](../Libraries/SkSpreadSheet/include/SkFunctionArray.hpp) | Dynamic arrays: SORT, UNIQUE, FILTER, SEQUENCE, … |
| [`SkJavascriptFunction.hpp`](../SkReactSpreadSheet/include/SkJavascriptFunction.hpp) | `tFunctionJavascript`: a function registered from JavaScript |

---

## 6. Names, tables, floating objects

| Class | File | Role |
|-------|------|------|
| `tFormulaNamed` | [`SkRangeNamed.hpp`](../Libraries/SkSpreadSheet/include/SkRangeNamed.hpp) | A named formula. The host cell lives on the system sheet `_$$N`. `ROW()` / `COLUMN()` are evaluated in the calling cell. |
| `tRangeNamedContainer` | [`SkRangeNamed.hpp`](../Libraries/SkSpreadSheet/include/SkRangeNamed.hpp) | Workbook name registry: named ranges and named formulas. |
| `tColumnData` | [`SkRangeData.hpp`](../Libraries/SkSpreadSheet/include/SkRangeData.hpp) | One column of a structured table: type, filter, sort, calculated-column formula, totals row. |
| `tRangeData` | [`SkRangeData.hpp`](../Libraries/SkSpreadSheet/include/SkRangeData.hpp) | A structured table (Excel ListObject): range, headers, totals, columns. |
| `tRangeFilter` | [`SkRangeFilter.hpp`](../Libraries/SkSpreadSheet/include/SkRangeFilter.hpp) | Applies column filters and hides rows (`m_DataVisible` on `tColRow`). |
| `tFilterOptions` | [`SkRangeFilter.hpp`](../Libraries/SkSpreadSheet/include/SkRangeFilter.hpp) | Filter options (operator, values, AND/OR logic). |
| `tRangeSort` | [`SkRangeSort.hpp`](../Libraries/SkSpreadSheet/include/SkRangeSort.hpp) | Sorts a range or a table. |
| `tSortOptions` | [`SkRangeSort.hpp`](../Libraries/SkSpreadSheet/include/SkRangeSort.hpp) | Keys, order, reorganization method. |
| `tTableStyle` | [`SkTableStyle.hpp`](../Libraries/SkSpreadSheet/include/SkTableStyle.hpp) | Table style (banding, header row, totals row) as `tFormatRef` overlays. |
| `tTableStyleContainer` | [`SkTableStyle.hpp`](../Libraries/SkSpreadSheet/include/SkTableStyle.hpp) | Catalog of the workbook’s table styles. |
| `tFloatingObjectLayout` | [`SkFloatingObject.hpp`](../Libraries/SkSpreadSheet/include/SkFloatingObject.hpp) | Geometry: anchor sheet and cell, offset, size, opacity, z-order. Rebased when rows or columns are inserted. |
| `tFloatingObject` | [`SkFloatingObject.hpp`](../Libraries/SkSpreadSheet/include/SkFloatingObject.hpp) | One floating object: link to the host cell (the widget) and its layout. |
| `tFloatingObjectContainer` | [`SkFloatingObject.hpp`](../Libraries/SkSpreadSheet/include/SkFloatingObject.hpp) | Workbook registry. The widget host is on `_$$A`; the visual anchor is on the displayed sheet. |
| `QualifyRefsForSheet` / `StripTargetSheetFromRefs` | [`SkRangeRefTransform.hpp`](../Libraries/SkSpreadSheet/include/SkRangeRefTransform.hpp) | Prefix or strip `Sheet!` on A1 references of a floating object. Free functions, not classes. |

---

## 7. Cell classes (engine side)

The engine does not have one C++ type per React widget. It registers a
**model** (name + properties). Rendering is documented in
`skeepto/docs/Spreadsheet-CellClass.md`.

| Class | File | Role |
|-------|------|------|
| `tCellModelClass` | [`SkCellClass.hpp`](../Libraries/SkSpreadSheet/include/SkCellClass.hpp) | Registrable model: name, label, family. Decides whether the model and the data go into JSON. |
| `tCellClass` | [`SkCellClass.hpp`](../Libraries/SkSpreadSheet/include/SkCellClass.hpp) | Object cell value, stored in the `tVariant`. Ancestor of every widget and of `tCellUnit`. |
| `tCellModelClassAttribute` | [`SkCellClassAttribute.hpp`](../Libraries/SkSpreadSheet/include/SkCellClassAttribute.hpp) | Model whose every property is a `tCellAttribute` (therefore a formula cell). |
| `tCellClassAttribute` | [`SkCellClassAttribute.hpp`](../Libraries/SkSpreadSheet/include/SkCellClassAttribute.hpp) | Instance placed on a cell: class name, reference name (`MyClass`), host cell, attribute container. |
| `tAttributeElem` | [`SkCellClassAttribute.hpp`](../Libraries/SkSpreadSheet/include/SkCellClassAttribute.hpp) | One property: name plus the allocator reference of its `tCellAttribute`. |
| `tAttributeContainer` | [`SkCellClassAttribute.hpp`](../Libraries/SkSpreadSheet/include/SkCellClassAttribute.hpp) | Property list of an instance. Shared across copies of the same class. |
| `tCellClassContainer` | [`SkCellClassContainer.hpp`](../Libraries/SkSpreadSheet/include/SkCellClassContainer.hpp) | Name → instance registry, plus external JSON payload attached to cells. |
| `tCellClassUnit` | [`SkCellClassUnit.hpp`](../Libraries/SkSpreadSheet/include/SkCellClassUnit.hpp) | A quantity with a unit (length, mass, time, currency). Details in `skeepto/docs/Spreadsheet-React-Classes.md`. |
| `tCellModelClassUnit` | [`SkCellClassUnit.hpp`](../Libraries/SkSpreadSheet/include/SkCellClassUnit.hpp) | Model of `tCellUnit`: data is saved, the model is not. |

`tClassUnit` (the unit descriptor, not a cell) is in
[`SkUnit.hpp`](../Libraries/SkRoot/include/SkUnit.hpp).

---

## 8. Selection, editing, view

| Class | File | Role |
|-------|------|------|
| `tSelect` | [`SkSelect.hpp`](../Libraries/SkSpreadSheet/include/SkSelect.hpp) | Current selection: temporary points and rectangles. Parses a reference (`A1:B10,D4`). The selection does not outlive the operation; it uses circular memory. |
| `tCopy` | [`SkCopyPaste.hpp`](../Libraries/SkSpreadSheet/include/SkCopyPaste.hpp) | Serializes the selection to JSON for paste. |
| `tFillSeries` | [`SkFillSeries.hpp`](../Libraries/SkSpreadSheet/include/SkFillSeries.hpp) | Fill handle: numeric sequences, months, weekdays, “prefix + number” text. |
| `tJsonView` | [`SkJsonView.hpp`](../Libraries/SkSpreadSheet/include/SkJsonView.hpp) | Builds the JSON of the visible window: cells, deduplicated formats, conditional formats, outline. This is what the canvas paints. |
| `tJsonSelect` | [`SkJsonSelect.hpp`](../SkReactSpreadSheet/include/SkJsonSelect.hpp) | Selection, cursor, and row / column headers serialized for React. |
| `tJsonSelection`, `tJsonSelectCol`, `tJsonSelectRow`, `tJsonCursor` | [`SkJsonSelect.hpp`](../SkReactSpreadSheet/include/SkJsonSelect.hpp) | Structures of that serialization. |
| `tUIExtraUndo` | [`SkUISpreadSheetApi.hpp`](../SkReactSpreadSheet/include/SkUISpreadSheetApi.hpp) | View snapshot (corner row / column) attached to an undo operation, so scroll position can be restored. |
| JSON keys | [`SkJsonKey.hpp`](../Libraries/SkSpreadSheet/include/SkJsonKey.hpp) | Short field names (`si`, `fi`, `op`, …). Constants, not a class. |

---

## 9. Conditional formatting

| Class | File | Role |
|-------|------|------|
| `tConditionalFormat` | [`SkConditionalFormat.hpp`](../Libraries/SkSpreadSheet/include/SkConditionalFormat.hpp) | One rule: highlight, data bar, color scale, icon set, or custom formula. Carries its own formula through `tInterfaceCompil`. |
| `tConditionalRanges` | [`SkConditionalFormat.hpp`](../Libraries/SkSpreadSheet/include/SkConditionalFormat.hpp) | Ranges the rule applies to. |
| `tConditionalFormatContainer` | [`SkConditionalFormat.hpp`](../Libraries/SkSpreadSheet/include/SkConditionalFormat.hpp) | Ordered list of a sheet’s rules. Several passes: init, sum, calculate, apply. |
| `tConditionalFormatType` | [`SkConditionalFormat.hpp`](../Libraries/SkSpreadSheet/include/SkConditionalFormat.hpp) | Rule family. |

---

## 10. Undo, redo, collaboration

The generic stack lives in SkRoot. The spreadsheet specializes each gesture.

| Class | File | Role |
|-------|------|------|
| `tUndo` | [`SkUndoRedo.hpp`](../Libraries/SkRoot/include/SkUndoRedo.hpp) | Ancestor: `Do` / `Undo`, state, extra. |
| `tUndoExtra` | [`SkUndoRedo.hpp`](../Libraries/SkRoot/include/SkUndoRedo.hpp) | Accessory data attached to an operation (the view, on the UI side). |
| `tUndoRedoContainer` | [`SkUndoRedo.hpp`](../Libraries/SkRoot/include/SkUndoRedo.hpp) | Undo / redo stack (depth `SkMaxUndoRedo`). |
| `tUndoSpreadSheet` | [`SkUndoRedoSp.hpp`](../Libraries/SkSpreadSheet/include/SkUndoRedoSp.hpp) | Ancestor of spreadsheet operations. Knows the sheet and the mode (client / server, undo active). |
| `tUndoState`, `tMode` | [`SkUndoRedoSp.hpp`](../Libraries/SkSpreadSheet/include/SkUndoRedoSp.hpp) | Phase (`BeforeDo`, `Do`, `Undo`) and mode bits. |
| `tSave` and subclasses | [`SkUndoRedoSaveSp.hpp`](../Libraries/SkSpreadSheet/include/SkUndoRedoSaveSp.hpp) | Snapshots taken **before** a mutation: cell, range, row / column, selection, conditional format, name. Undo restores from these snapshots. |
| `tUndoRebaseLog` | [`SkUndoRedoRebase.hpp`](../Libraries/SkSpreadSheet/include/SkUndoRedoRebase.hpp) | Log of the workbook’s structural operations, so a remote undo can be replayed after concurrent inserts. |
| `tStructuralOp`, `tRebasePlan` | [`SkUndoRedoRebase.hpp`](../Libraries/SkSpreadSheet/include/SkUndoRedoRebase.hpp) | One structural operation (insert / delete row or column) and the rebase plan that follows. |
| `tUndoRedoJsonCallBack` | [`SkUndoRedoJsonCallBack.hpp`](../Libraries/SkSpreadSheet/include/SkUndoRedoJsonCallBack.hpp) | Writes the JSON of cells touched by an operation, for the collaboration message. |
| `tMessage` | [`SkMessage.hpp`](../Libraries/SkSpreadSheet/include/SkMessage.hpp) | Collaboration message: who, which operation, which undo payload. |
| `tInterfaceWeb` | [`SkInterfaceWeb.hpp`](../Libraries/SkSpreadSheet/include/SkInterfaceWeb.hpp) | Web entry point: current user, workbook URI, send / receive of messages. |
| `kCollaborationPasteCopyMaxBytes` | [`SkCollaborationLimits.hpp`](../Libraries/SkSpreadSheet/include/SkCollaborationLimits.hpp) | Cap (384 KB) on a collaborative paste payload. |

`tSave` specializes into `tSaveFormulaCell`, `tSaveCell`, `tSaveRange`,
`tSaveConditionalFormat`, `tSaveColRow`, `tSaveSelect`, `tSaveSelectColRow`,
`tSaveCoveredRange`, `tSaveSelectErase`, `tSaveRangeNamed`
([`SkUndoRedoSaveSp.hpp`](../Libraries/SkSpreadSheet/include/SkUndoRedoSaveSp.hpp)).

Operations, all in [`SkUndoRedoSp.hpp`](../Libraries/SkSpreadSheet/include/SkUndoRedoSp.hpp),
follow the user gesture:

| Classes | Gesture |
|---------|---------|
| `tUndoCellValue`, `tUndoRaz` | Edit, clear |
| `tUndoFillSeries` | Fill handle |
| `tUndoFormat`, `tUndoPrecision`, `tUndoBorder` | Format, decimals, borders |
| `tUndoCellClass`, `tUndoCellAttribute`, `tUndoCellClassAttributes`, `tUndoCellClassCalculable` | Widget and its properties |
| `tUndoPaste`, `tUndoCut`, `tUndoMove` | Paste, cut, move |
| `tUndoApplyMerge` | Merge cells |
| `tUndoInsertCol`, `tUndoInsertRow`, `tUndoInsertRowWithLabel`, `tUndoDeleteCol`, `tUndoDeleteRow` | Grid structure. Ancestor: `tUndoAncestorDeleteInsert` |
| `tUndoAddSheet`, `tUndoDeleteSheet`, `tUndoRenameSheet`, `tUndoSwapSheet` | Sheets |
| `tUndoAddRangeNamed`, `tUndoAddRangeData`, `tUndoApplyRangeData`, `tUndoDeleteRangeNamed`, `tUndoUpdateRangeNamed` | Names and tables |
| `tUndoInsertFormulaNamed`, `tUndoDeleteFormulaNamed` | Named formulas |
| `tUndoConditionaFormat`, `tUndoDeleteConditionaFormat` | Conditional formats |
| `tUndoInsertFloatingObject`, `tUndoDeleteFloatingObject`, `tUndoFloatingObjectLayout` | Floating objects |
| `tUndoOpenCloseTree`, `tUndoChangeTree` | Outline (row / column groups) |
| `tUndoSplitView` | Frozen panes |
| `tUndoChangeSize` | Row height, column width |
| `tUndoPrintParameters` | Page setup |
| `tUndoJsonPayload` | Operation whose state is a JSON blob |
| `tUndoSpreadSheetCallBack` | Ancestor of operations that notify the UI at the end of Do / Undo |

In collaborative mode, renaming a sheet or a name that is already cited in a
formula is refused: textual rebase of those identifiers is not implemented
(`tApi`, multi-user flag).

---

## 11. Format (CSS)

A format is not copied onto every cell. A cell, a row, a column, or a sheet
holds a `tFormatRef` to an **interned** style. `tFormatApi` is the contract;
`tFormatCssApi` is the spreadsheet implementation. The workbook receives that
pointer through `tSpreadSheetContainer::FormatApi`.

Namespace `SkFormat`, except `tFormatApi`, which is in `SkRoot`.

| Class | File | Role |
|-------|------|------|
| `tFormatApi` | [`SkFormatApi.hpp`](../Libraries/SkRoot/include/SkFormatApi.hpp) | Abstract API: apply font, fill, border, alignment, number format. |
| `tFormatCssApi` | [`SkFormatCssApi.hpp`](../Libraries/SkFormat/include/SkFormatCssApi.hpp) | CSS implementation used by the spreadsheet. |
| `tFormatRoot` | [`SkFormatRoot.hpp`](../Libraries/SkFormat/include/SkFormatRoot.hpp) | Interned pools: formats, fonts, texts, borders, shadows, margins. |
| `tFormatCss` | [`SkFormatCss.hpp`](../Libraries/SkFormat/include/SkFormatCss.hpp) | One complete style: colors, font, text, border, shadow, margin. |
| `tFormatShare` | [`SkFormatShare.hpp`](../Libraries/SkFormat/include/SkFormatShare.hpp) | Ancestor of style fragments (refcount, intern key). |
| `tItemCss` | [`SkItemCss.hpp`](../Libraries/SkFormat/include/SkItemCss.hpp) | Generic allocator of a style fragment. |
| `tFontCss` | [`SkFont.hpp`](../Libraries/SkFormat/include/SkFont.hpp) | Font: family, size, weight, style. |
| `tTextCss` | [`SkText.hpp`](../Libraries/SkFormat/include/SkText.hpp) | Alignment, wrap, decoration. |
| `tBorderRectCss` | [`SkBorder.hpp`](../Libraries/SkFormat/include/SkBorder.hpp) | Borders of the four sides. |
| `tColorCss` | [`SkColor.hpp`](../Libraries/SkFormat/include/SkColor.hpp) | Named, hexadecimal, or RGB color. |
| `tShadowCss` | [`SkShadow.hpp`](../Libraries/SkFormat/include/SkShadow.hpp) | Drop shadow. |
| `tUnitCss`, `tUnitRectCss` | [`SkUnitMetrics.hpp`](../Libraries/SkFormat/include/SkUnitMetrics.hpp) | CSS lengths (px, mm) and a rectangle (margin, padding). |
| `tNumberFormatter`, `tExcelFormatParser` | [`SkFormatNumber.hpp`](../Libraries/SkRoot/include/SkFormatNumber.hpp) | Numeric display format, Excel syntax (`#,##0.00`). |
| `tDateFormatter`, `tExcelDateParser` | [`SkFormatDate.hpp`](../Libraries/SkRoot/include/SkFormatDate.hpp) | Date display format. |
| `tLemonFormatInterface` | [`SkLemonFormatInterface.hpp`](../Libraries/SkFormat/include/SkLemonFormatInterface.hpp) | Bridge between the format grammar and the CSS objects. |
| `tLexer` (format) | [`SkLexerFormat.hpp`](../Libraries/SkFormat/include/SkLexerFormat.hpp) | Lexer of the format language. Distinct from the formula lexer. |

---

## 12. Import and export

| Class | File | Role |
|-------|------|------|
| `tExcel2SpreadSheet` | [`SkExcel2SpreadSheet.hpp`](../SkExcel/include/SkExcel2SpreadSheet.hpp) | Reads an `.xlsx` (Office Open XML) and fills a `tWorkBook`. |
| `tExcelPugiXMLReader` | [`SkExcelPugiXMLReader.hpp`](../SkExcel/include/SkExcelPugiXMLReader.hpp) | XML reader for the workbook (sheets, styles, tables). |
| `WorkbookBuilder` | [`SkSpreadSheet2Excel.hpp`](../SkExcel/include/SkSpreadSheet2Excel.hpp) | Writes an `.xlsx` from the Skeepto workbook. |
| `tCsvImport` | [`SkCsvImport.hpp`](../Libraries/SkSpreadSheet/include/SkCsvImport.hpp) | Imports a CSV into the grid (delimiter, quotes). |

The user flow (upload, convert, open) is in `skeepto/docs/Import-Excel.md`.

---

## 13. Value types (SkRoot)

A cell stores a `tVariant`: a 16-byte tagged union
([`SkVariant.hpp`](../Libraries/SkRoot/include/SkVariant.hpp)). The tag is
`tVariantType` (`t_int`, `t_double`, `t_string`, `t_date`, `t_error`, object, …).

Object values derive from `tVariantClass`
([`SkVariant.hpp`](../Libraries/SkRoot/include/SkVariant.hpp)): `tCellClass`,
and therefore widgets and `tCellUnit`.

Boxed scalars (`tClassInt`, `tClassDouble`, `tClassString`, `tClassDate`,
`tClassError`) are in
[`SkTypesClass.hpp`](../Libraries/SkRoot/include/SkTypesClass.hpp). Sheet
errors (`#DIV/0!`, `#REF!`, `#N/A`, …) are the `tTypeError` enumeration in
the same file.

`tSharedString` / `tSharedStringPool`
([`SkSharedString.hpp`](../Libraries/SkRoot/include/SkSharedString.hpp))
intern repeated text. `tJsonSharedString`
([`SkJsonSharedString.hpp`](../Libraries/SkRoot/include/SkJsonSharedString.hpp))
does the same inside the `.sker` file: the `si` (texts) and `fi` (formulas)
tables owned by `tSpreadSheetContainer`.

---

## 14. Where to change things

| Need | Entry point |
|------|-------------|
| A new undoable user action | `tApi`, then a `tUndo…` class in [`SkUndoRedoSp.hpp`](../Libraries/SkSpreadSheet/include/SkUndoRedoSp.hpp) |
| A new worksheet function | Subclass of `tFunction`, registered in `tFunctionDictionary` |
| A new cell widget | React side only. See `skeepto/docs/Spreadsheet-CellClass.md`. The engine records the name through `tCellModelClassAttribute` |
| A new unit | `tClassUnit` / `tCellClassUnit`. See `skeepto/docs/Spreadsheet-React-Classes.md` |
| What the canvas receives | `tJsonView` |
| What JavaScript can call | `tUISpreadSheet` |

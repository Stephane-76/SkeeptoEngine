# C++ coding conventions

This document describes the naming and type conventions used in the Skeepto Engine
C++ sources (`Libraries/`, `Libraries_test/`, `SkExcel/`, `SkReactSpreadSheet/`,
`SkPressureSp/`).

Code lives in `namespace SkRoot`. Source files are named `Sk*.hpp` / `Sk*.cpp`.
C++ types and classes use a `t` prefix (`tInt`, `tVariant`, `tApi`), not `Sk`.
Comments are written in American English.

Headers:

- Types: [`Libraries/SkRoot/include/SkTypes.hpp`](../Libraries/SkRoot/include/SkTypes.hpp)
- Boxing classes: [`Libraries/SkRoot/include/SkTypesClass.hpp`](../Libraries/SkRoot/include/SkTypesClass.hpp)

## Identifier prefixes

| Prefix | Meaning | Used for | Examples |
|--------|---------|----------|----------|
| `t` | type | Typedefs, classes, enums | `tInt`, `tString`, `tClassInt`, `tVariant` |
| `s` | **stack** | Function and method parameters (in and out) | `sValue`, `sFormatString`, `sOut` |
| `w` | **work** | Local variables | `wStream`, `wResult`, `wIndex` |
| `m_` | member | Class data members | `m_Value`, `m_Type` |
| `c` / `k` | constant | Compile-time or file-scope constants | `cSlash`, `kEncodeTable` |

- `s` is **stack**: every parameter of a function, including output references.
- `w` is **work**: every local variable inside a function.
- Do not use an `o` prefix. An output parameter is still stack: `tString& sOut`.

Method **names** are PascalCase (`Encode`, `IsNumber`, `FormatString`). They are
not prefixed with `s`.

A parameter named after its type drops the leading `t`:

```cpp
void Assign(const tVariant& sVariant);
tBool operator==(const tClassInt& sClassInt);
```

### Example

```cpp
tString Foo(const tString& sName, tSize sSize, tString& sOut) {
    tString wResult;
    tSize wIndex = 0;
    // ...
    sOut = wResult;
    return wResult;
}
```

| Identifier | Why |
|------------|-----|
| `sName`, `sSize`, `sOut` | Parameters (stack) |
| `wResult`, `wIndex` | Locals (work) |

## Primitive types (`SkTypes.hpp`)

Prefer the engine aliases over raw C++ types (`int`, `bool`, `double`,
`std::string`):

| Alias | Meaning |
|-------|---------|
| `tInt` / `tUInt` | 32-bit signed / unsigned int |
| `tSize` | `size_t` |
| `tShort` / `tUShort` | 16-bit |
| `tByte` / `tUByte` | 8-bit (`char` / `unsigned char`) |
| `tLong` / `tLongLong` | long / long long |
| `tBool` | bool |
| `tFloat` / `tDouble` | float / double |
| `tChar` | UTF-8 char |
| `tString` | UTF-8 string (`std::basic_string<tChar>`) |
| `t16Char` / `t16String` | UTF-16 |
| `t32Char` / `t32String` | UTF-32 |
| `tDate` | `std::time_t` |
| `tIndex` | Spreadsheet row / column index |
| `tColor` | `uint32_t` (Skia) |
| `tAllocatorRef` | Allocator handle |

Containers follow the same `t` prefix: `tVectorString`, `tVectorInt`,
`tVectorSize`, `tStackInt`, `tStringStream`.

Helpers: `SkMin` / `SkMax`, `SkMaxInt`, `SkAlign` (8-byte memory alignment),
`SkInline` (always `inline`, required for ODR on header definitions).

### Enums

Enums are `enum class` with a `t` name. Enumerators often use a `t_` prefix:

```cpp
enum class tVariantType : tChar {
    t_null, t_int, t_bool, t_double, t_string, t_error, t_date, t_class
};

enum class tTypeError : tChar {
    t_none = 0, t_value, t_div0, t_ref, t_num, t_name, /* … */
};
```

## Boxing classes (`SkTypesClass.hpp`)

`tClassInt`, `tClassFloat`, `tClassDouble`, `tClassString`, `tClassDate`, and
`tClassError` wrap the primitives. The design is **boxing / unboxing** (C# style):
the boxed object carries type-specific methods.

All inherit from `tClass` (root of the class hierarchy). The payload is `m_Value`
(or `m_Code` / `m_String` for errors).

**Box** by constructing from the primitive:

```cpp
tClassInt    wInt(42);
tClassDouble wDouble(3.14);
tClassString wText("hello");
tClassDate   wDate(2023, 1, 2);
```

**Unbox** with `operator()` — it returns the stored primitive:

```cpp
tInt    wRaw = wInt();
tDouble wD   = wDouble();
tString wS   = wText();
tDate   wT   = wDate();
```

- `Str()` converts to `tString`.
- `FormatString(tFormatString*)` formats through `tClassFormatString<T>`.
- Comparison and arithmetic operators (`==`, `<`, `+`, `-`, `*`, `/`) stay on
  the boxed type and return a boxed value.
- `operator<<` writes to `ostream`.

`tClassString` adds string operations (`Left`, `Right`, `Mid`, `Trim`, `Split`,
`Replace`, `Upper`, `Lower`, regex) and predicates (`IsInteger`, `IsNumber`,
`IsEmailAdress`, `IsUrl`, …), plus `ToInt()` / `ToDouble()`.

`tClassDate` uses Excel serial days (the fraction is the time of day). It can
be built from a `tVariant`.

`tClassError` boxes spreadsheet errors (`#VALUE!`, `#DIV/0!`, `#REF!`, …) via
`tTypeError`.

### Typical use

```cpp
tClassString wTestString;
wTestString = "1234.34";
if (wTestString.IsNumber()) {
    tDouble wValue = wTestString.ToDouble();
}
```

## Debug leak counter on `tClass`

Every engine object inherits `tClass`. In **DEBUG** builds, `tClass` tracks live
instances so leaked objects can be reported. In **RELEASE**, that tracking is
compiled out: no extra member, no inserts, no dump.

`_DEBUGLeak` is defined in `SkTypes.hpp` whenever the build is not release:

| Target | DEBUG | RELEASE |
|--------|-------|---------|
| Native | no `NDEBUG` (MSVC also requires `_DEBUG`) | `NDEBUG`, or MSVC without `_DEBUG` |
| WASM | `SK_COMPIL` is not `RELEASE` | `SK_RELEASE` from CMake |

When `_DEBUGLeak` is on:

- Each `tClass` stores a serial number in `m_IndiceAlloc` (`GetNbAlloc()`).
- Construction increments a global counter (`StaticTotalAlloc`) and a live
  count (`StaticDiff`), then registers `this` in `StaticClassMemoryDebug`.
- Destruction decrements `StaticDiff` and unregisters `this`.
- At process end, `tApplication`’s destructor calls `DebugMemory()` **unless**
  `ReportLeakAtExit(false)` was set (Python bindings do this so pytest stays
  quiet). Native test binaries keep the default dump. Set `SK_DEBUG_LEAK=1`
  in the Python process to turn the dump back on. `ReportLeakAtExit` is always
  linked (even when the Python `.so` is built with `NDEBUG`) so a DEBUG
  `libSkRoot.a` still honors the flag.

```cpp
#ifdef _DEBUGLeak
    int m_IndiceAlloc;   // present on tClass in DEBUG only
    int GetNbAlloc();
#endif
```

Do not rely on `m_IndiceAlloc` or `DebugMemory()` in RELEASE: the macros are
absent and the code does not compile.

## Variants (`tVariant`)

`tVariant` is the 16-byte tagged union used by spreadsheet cells
(`alignas(SkAlign)`):

| Member | Role |
|--------|------|
| `m_Type` | `tVariantType` |
| `m_Extra` | Spare 16-bit field |
| `m_Union` | `tInt` / `tBool` / `tDouble` / `tSharedString*` / `tClassError*` / `tDate` / `tVirtualClass*` |

Constructors and setters take a stack parameter named `sValue`
(`SetInt(sValue)`, `SetString(sValue)`, …). A `tVirtualClass*` in the union
lets custom objects participate in `+` `-` `*` `/`.

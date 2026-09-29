# Lemon parser generator

Skeepto compiles the **generated** parsers. It does not download or build
Lemon. A normal configure / build never needs the generator.

Regenerate a parser only after editing a `.y` grammar. Lemon is public
domain (the SQLite project). Two files are enough: `lemon.c` (the program)
and `lempar.c` (the template copied into every generated parser).

Grammars in this repository:

| Grammar | Input | Generated sources the build compiles |
|---------|--------|--------------------------------------|
| Formulas | [`Libraries/SkSpreadSheet/lemon/SkLemonSpreadSheet.y`](../Libraries/SkSpreadSheet/lemon/SkLemonSpreadSheet.y) | [`source/SkLemonSpreadSheet.cpp`](../Libraries/SkSpreadSheet/source/SkLemonSpreadSheet.cpp), [`include/SkLemonSpreadSheet.h`](../Libraries/SkSpreadSheet/include/SkLemonSpreadSheet.h) |
| Formats | [`Libraries/SkFormat/lemon/SkLemonFormat.y`](../Libraries/SkFormat/lemon/SkLemonFormat.y) | [`source/SkLemonFormat.cpp`](../Libraries/SkFormat/source/SkLemonFormat.cpp), [`include/SkLemonFormat.h`](../Libraries/SkFormat/include/SkLemonFormat.h) |

`third-party/` is filled by CMake with rapidjson, rapidxml, cppunit, pugixml,
and libzip ([`cmake/SkThirdParty.cmake`](../cmake/SkThirdParty.cmake)). The
whole directory is gitignored (`/third-party/` in `.gitignore`). Lemon is not
in that list.

The class roles of the generated parser are in
[`Spreadsheet-Structure.md`](./Spreadsheet-Structure.md).

---

## 1. Get the two source files

Pick one of these. Keep `lemon.c` and `lempar.c` side by side. Lemon looks
for `lempar.c` in the current directory, then next to the `lemon` executable.

### From the SQLite tree

```bash
mkdir -p "$HOME/src/lemon"
cd "$HOME/src/lemon"
curl -fsSL -o lemon.c  https://raw.githubusercontent.com/sqlite/sqlite/master/tool/lemon.c
curl -fsSL -o lempar.c https://raw.githubusercontent.com/sqlite/sqlite/master/tool/lempar.c
```

Upstream pages:
[tool/lemon.c](https://sqlite.org/src/file/tool/lemon.c),
[tool/lempar.c](https://sqlite.org/src/file/tool/lempar.c).

### From the compiler-dept mirror

That repository exists so a project can vendor Lemon
([compiler-dept/lemon](https://github.com/compiler-dept/lemon)).

```bash
git clone --depth 1 https://github.com/compiler-dept/lemon.git "$HOME/src/lemon"
```

### From an existing `sker` checkout

If `sker/third-party/lemon/` is still on disk, it already contains
`lemon.c`, `lempar.c`, and sometimes a built `lemon` binary. That folder is
local only: `sker` gitignores `third-party/*`, and CMake does not clone it.

```bash
mkdir -p "$HOME/src/lemon"
cp /Users/stephaneallez/Projects/sker/third-party/lemon/lemon.c \
   /Users/stephaneallez/Projects/sker/third-party/lemon/lempar.c \
   "$HOME/src/lemon/"
```

A `lemon` binary that happens to be on `PATH` (for example from
`depot_tools`) works only when `lempar.c` is next to that binary or in the
directory where you launch the command.

---

## 2. Build the `lemon` binary

```bash
cd "$HOME/src/lemon"
cc -std=gnu11 -Os -Wall -o lemon lemon.c
./lemon
```

With no arguments, Lemon prints its usage line. That confirms the binary
runs. Do not commit this directory into `skeepto-engine/third-party/`: that
tree is gitignored and is reserved for the CMake fetches.

---

## 3. Regenerate a grammar

Run Lemon **in the grammar directory**, with `-l` so the output has no
`#line` directives. Then rename `.c` to `.cpp` and copy the pair the build
actually compiles.

Formulas:

```bash
cd /Users/stephaneallez/Projects/skeepto-engine/Libraries/SkSpreadSheet/lemon
"$HOME/src/lemon/lemon" -l SkLemonSpreadSheet.y
mv SkLemonSpreadSheet.c SkLemonSpreadSheet.cpp
cp SkLemonSpreadSheet.cpp ../source/SkLemonSpreadSheet.cpp
cp SkLemonSpreadSheet.h ../include/SkLemonSpreadSheet.h
```

Formats:

```bash
cd /Users/stephaneallez/Projects/skeepto-engine/Libraries/SkFormat/lemon
"$HOME/src/lemon/lemon" -l SkLemonFormat.y
mv SkLemonFormat.c SkLemonFormat.cpp
cp SkLemonFormat.cpp ../source/SkLemonFormat.cpp
cp SkLemonFormat.h ../include/SkLemonFormat.h
```

Lemon also writes a `.out` report (states and conflicts) next to the `.y`
file. That report is a diagnostic. The engine does not compile it.

If Lemon cannot find the template:

```text
Can't find the parser driver template file "lempar.c".
```

copy `lempar.c` into the grammar directory, or pass it explicitly:

```bash
"$HOME/src/lemon/lemon" -T"$HOME/src/lemon/lempar.c" -l SkLemonSpreadSheet.y
```

The older `sker` tree has the same steps in `Libraries/SkSpreadSheet/lemon/fab.sh`
and `Libraries/SkFormat/lemon/fab.sh`. Those scripts call `lemon` from
`PATH`. `skeepto-engine` does not ship `fab.sh`.

---

## 4. What to commit

Commit the updated `.cpp` and `.h` under `source/` and `include/`. Leave
`lemon.c`, `lempar.c`, and the `lemon` binary out of the repository. The
next WASM or native build picks up the new parser with no extra tool.

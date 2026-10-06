## 1.0.1

- **macOS: link with `-undefined dynamic_lookup`.** macOS `ld` rejects
  undefined symbols in a shared library, but `WARN` needs `qsyslog` — an
  engine-provided function pointer resolved at `dlopen` time (Linux allows
  this by default). `-undefined dynamic_lookup` is the Darwin equivalent, set
  as `LDFLAGS-libnd-attr-Darwin` so no other platform is affected.

## [1.0.0]

- **nd-attr is now an installable library rather than a build artifact of
  the engine.** It builds and installs exactly two files,
  `lib/libnd-attr.so` and `include/nd/attr.h`, following the same layout as
  `axil-tty` and `axil-auth`, and the same layout `nd-core` was converted to
  first. Previously `make` produced an `attr.so` named by the engine's
  `mods.load` and installed nothing. There is no `lib/nd-attr.so` symlink:
  `mods.load` names this module `libnd-attr`, the installed filename, and
  `module_load_path()` only appends `.so`, so the load name must equal the
  installed name. A symlink would in any case have been dropped from the
  OpenBSD package, whose packing list is built from `find usr -type f`.

- **The link line is libxylem alone.** `LDLIBS := -lxylem`; the engine is not
  linked. A module is `dlopen`'d by an engine that already has XY resident,
  and XY_IMPL/XY_DECL resolve through the injected xy context, not through
  link-time symbols. `NEEDED` is `libxylem.so` and `libc.so.6`.

- **The chain API is declared in `<nd/attr.h>`.** `attr_stat`, `modifier`,
  `effect`, `hp_max`, `mp_max` (plus `mcp_stats`, `train`, `attr_award`,
  `on_reroll`) are `XY_DECL`'d there for race, class, equip and spell to
  co-implement; this TU defines `ATTR_IMPL` because it `XY_IMPL`s the same
  names and the two cannot coexist in one TU.

- **The game's service API is included as `<nd/xy.h>`**, from the engine's
  `$(PREFIX)/include/nd/` — the same include root as `<ttypt/xy.h>`, so this
  library needs no private `-I` for the game headers at all.

- **Dropped the `nd-mod.mk` dependency.** This module resolves the game's
  headers itself, the way every other house library does, and `nd-mod.mk` is
  the SIC-era engine module build contract. It has now been deleted.

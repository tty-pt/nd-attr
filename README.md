# axil-nd-attr

`nd-attr` for [axil-nd](../axil-nd), ported from SIC to libxylem.

Owns the attribute system: the base-value chains every other stat module
co-implements (`attr_stat`, `modifier`, `effect`, `hp_max`, `mp_max`), plus
training, point award, the `mcp_stats` frame, and status display. It is the
chain base of the whole stat stack — race, class, equip and spell all read
its values through `nd_last()` — so `mods.load` lists it before all of them.

## Install

```sh
make install
```

Installs:

```
lib/libnd-attr.so
include/nd/attr.h
```

There is deliberately no `lib/nd-attr.so` symlink. `xy_load()` appends `.so`
itself and does not retry with a `lib` prefix, so the engine's `mods.load`
names this module `libnd-attr` and `dlopen`s `libnd-attr.so`. A soname symlink
would also have been silently dropped from the OpenBSD package: `tty-pt/ci`
builds the packing list from `find usr -type f`, which never lists a symlink,
so the package would have shipped the library under one name and asked the
loader for another.

Also packaged for deb, apk, rpm, brew and openbsd from a `v*` tag.

## Build from source

```sh
make
```

Needs [libxylem](https://github.com/tty-pt/libxylem) (headers already in
`$(PREFIX)/include`) and the engine's game API, `<nd/xy.h>`, from either an
`axil-nd` checkout beside this repo or an installed `axil-nd`:

```sh
git clone https://github.com/tty-pt/nd-attr && cd nd-attr
git clone https://github.com/tty-pt/axil-nd ../axil-nd
make
```

`<nd/xy.h>` installs to `$(PREFIX)/include/nd/`, the same directory that
already carries `<ttypt/xy.h>`, so an **installed** engine needs no `-I` of
its own here. Against a checkout beside this repo it is
`-I../axil-nd/include`; both paths are on `CFLAGS` at once and a missing `-I`
is ignored, so the same command works either way.

## What it does

* `xy_install()` registers the `attr` table (one `attr_t` per entity) and the
  chain base values for attributes, modifiers, effects, HP and MP.
* The five chains — `attr_stat`, `modifier`, `effect`, `hp_max`, `mp_max` —
  are `XY_DECL`'d in `<nd/attr.h>` so race, class, equip and spell can
  co-implement them. Each co-implementor reads its predecessor with
  `nd_last()` and folds its own adjustment in.
* `train` / `attr_award` spend and grant attribute points; `mcp_stats` pushes
  the stat frame; `on_status` and `on_reroll` cover display and re-rolls;
  `on_add` initializes the row for a new entity.

## Testing

There is no `test.sh` here. Behaviour is asserted by the engine's own suite,
which builds every module in its `mods.load`, boots, and greps stderr:

```sh
cd ../axil-nd
make && ./test.sh
```

## Notes from the port

* `SIC_DEF` → `XY_IMPL`, `mod_install` → `xy_install`, `call_f(...)` →
  `f(...)`, `sic_last(...)` → `nd_last(...)`. The read side this needs from
  `nd_last()` is libxylem's fixed `xy_last` (readable mid-dispatch).
* This TU `XY_IMPL`s the chain names, so it defines `ATTR_IMPL` before
  including `<nd/attr.h>`: an `XY_IMPL` and an `XY_DECL` of the same name in
  one TU collide.
* The link line is libxylem alone. `NEEDED` is `libxylem.so` and `libc.so.6`.

## License

BSD 2-Clause, carried over from `tty-pt/nd-attr`. See `LICENSE`.

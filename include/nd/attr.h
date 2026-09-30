/* attr.h — nd-attr's cross-module API: the attribute vector and the derived
 * values (max HP, max MP, and the per-slot effects buffs modify).
 *
 * Include this from a module TU that wants to read or write attributes, and
 * NOT from nd-attr's own src/libnd-attr.c: an XY_IMPL and an XY_DECL of the
 * same name in one TU collide, which is the direct replacement for the old
 * `SIC_DECL` + `SIC_DEF` pairing in a single file.
 *
 * Usage:
 *
 *     #include <ttypt/xy-mod.h>     // must come first: injects the xy context
 *     #include <nd/xy.h>            // engine service hooks (nd_printf, ...)
 *     #include <nd/attr.h>          // this file
 *
 * The consumer does not need to load nd-attr itself -- the engine loads every
 * module in mods.load into one region and XY dispatches by name -- but the
 * engine's mods.load must list attr, or these forward to a provider that is
 * not there.
 *
 * NOTE: this is a MODULE-OWNED header, not an engine one. The old location was
 * `include/uapi/attr.h`; the old `~/nd/module.mk` installed it as
 * `$(PREFIX)/include/nd/attr.h`, so `nd/` is this header's home and it is
 * installed here with `FOLDER := nd`.
 *
 * The old header included `<nd/type.h>`, a file that no longer exists. Its only
 * live content for this module was SIC_DECL/SIC_DEF/SIC_CALL (all now XY_*) and
 * `sic_str_t`; the types this header needs are in `<ttypt/xy.h>`'s dependency
 * chain via <nd/xy.h>. Consumers get those from `<nd/xy.h>`, so nothing here
 * needs to include it.
 */

#ifndef ND_ATTR_H
#define ND_ATTR_H

#include <math.h>

#include <ttypt/xy.h>

#define G(x) xsqrtx(x)

enum attribute {
	ATTR_STR,
	ATTR_CON,
	ATTR_DEX,
	ATTR_INT,
	ATTR_WIZ,
	ATTR_CHA,
	ATTR_MAX
};

enum affect {
       // these are changed by bufs
       AF_HP,
       AF_MOV,
       AF_MDMG,
       AF_MDEF,
       AF_DODGE,

       // these aren't.
       AF_DMG,
       AF_DEF,

       // these are flags, not types of buf
       AF_NEG = 0x10,
       AF_BUF = 0x20,
};

#ifndef ATTR_IMPL

/* API */
XY_DECL(long, hp_max, unsigned, ref);
XY_DECL(long, mp_max, unsigned, ref);
XY_DECL(long, effect, unsigned, ref, enum affect, slot);
/* RENAMED from `stat`, which collides with libc's stat(2) once
 * <ttypt/xy.h> pulls in <sys/stat.h>. */
XY_DECL(unsigned, attr_stat, unsigned, ref, enum attribute, at);
XY_DECL(long, modifier, unsigned, ref, enum attribute, at);

XY_DECL(int, mcp_stats, unsigned, player_ref);
XY_DECL(int, train, unsigned, player_ref, enum attribute, at, unsigned, amount);
XY_DECL(int, attr_award, unsigned, player_ref, unsigned, amount);

/* SIC
 *
 * Exported, no in-tree caller: nothing in the 19-module set fires on_reroll, and
 * nothing calls it. The engine also has no firing site for it (MODS.md
 * §12). Ported rather than dropped so the module's intended API survives. */
XY_DECL(int, on_reroll, unsigned, player_ref);

#endif /* !ATTR_IMPL */

/* OTHERS */
static inline unsigned
xsqrtx(unsigned x)
{
	return x * sqrt(x);
}

#endif /* !ND_ATTR_H */
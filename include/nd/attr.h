/* attr.h — nd-attr's cross-module API: the attribute vector and the derived
 * values (max HP, max MP, and the per-slot effects buffs modify).
 *
 * Caller-facing header. Implementers include <nd/attr-types.h>, not this header.
 */

#ifndef ND_ATTR_H
#define ND_ATTR_H

#include <ttypt/xy.h>
#include <nd/attr-types.h>

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

#endif /* !ND_ATTR_H */

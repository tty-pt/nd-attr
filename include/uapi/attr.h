#ifndef ATTR_H
#define ATTR_H

#include <math.h>

#include <nd/type.h>
#define G(x) xsqrtx(x)

// We should move most of this to a common header,
// since we want players to be able to implement,
// yes, but not define the interface.

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

/* API */
SIC_DECL(unsigned, hp_max, unsigned, ref);
SIC_DECL(unsigned, mp_max, unsigned, ref);
SIC_DECL(short, effect, unsigned, ref, enum affect, slot);
SIC_DECL(unsigned, stat, unsigned, ref, enum attribute, at);

SIC_DECL(int, mcp_stats, unsigned, player_ref);
SIC_DECL(int, train, unsigned, player_ref, enum attribute, at, unsigned, amount);
SIC_DECL(int, attr_award, unsigned, player_ref, unsigned, amount);

/* SIC */

SIC_DECL(int, on_reroll, unsigned, player_ref)

/* OTHERS */
static inline unsigned
xsqrtx(unsigned x)
{
	return x * sqrt(x);
}

#endif

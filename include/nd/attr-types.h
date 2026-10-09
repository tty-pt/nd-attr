#ifndef ND_ATTR_TYPES_H
#define ND_ATTR_TYPES_H

/*
 * nd/attr-types.h — Shared types, enums, and math helpers for nd-attr.
 * Contains zero XY_DECLs.
 */

#include <math.h>

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

static inline unsigned
xsqrtx(unsigned x)
{
	return x * sqrt(x);
}

#endif /* ND_ATTR_TYPES_H */

#ifndef ND_ATTR_BCP_H
#define ND_ATTR_BCP_H

/*
 * attr-bcp.h — client-side wire/state types owned by nd-attr.
 * Channel ND_BCP_STATS carries the attribute block. The decoded state lives
 * in nd_client_state_t's extension slot "attr" (the core client knows nothing
 * about attributes).
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <nd/bcp-types.h>
#include <nd/bcp-meta.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ND_BCP_STATS     11
#define ND_ATTR_EXT_KEY  "attr"

/* Attribute indices in nd_stats_state_t.attr */
enum nd_attr_idx {
	ND_ATTR_STR = 0,
	ND_ATTR_CON = 1,
	ND_ATTR_DEX = 2,
	ND_ATTR_INT = 3,
	ND_ATTR_WIZ = 4,
	ND_ATTR_CHA = 5,
	ND_ATTR_MAX = 6,
};

typedef struct {
	uint16_t attr[ND_ATTR_MAX];
	uint32_t dodge;
	uint32_t dmg;
	uint32_t mdmg;
	uint32_t def;
	uint32_t mdef;
} nd_stats_state_t;

/* Decode an ND_BCP_STATS frame (`#b` + iden + payload) into the "attr" slot. */
static inline int
nd_attr_bcp_unpack(const uint8_t *buf, size_t len, void *state)
{
	if (!buf || len < 3)
		return -1;
	nd_stats_state_t *s = nd_client_ext((nd_client_state_t *)state,
			ND_ATTR_EXT_KEY, sizeof(nd_stats_state_t));
	if (!s)
		return -1;

	nd_bcp_reader_t r;
	nd_bcp_reader_init(&r, buf + 3, len - 3);
	nd_stats_state_t tmp;
	memset(&tmp, 0, sizeof(tmp));
	for (int i = 0; i < ND_ATTR_MAX; i++)
		tmp.attr[i] = nd_bcp_read_u16_c(&r);
	tmp.dodge = nd_bcp_read_u32_c(&r);
	tmp.dmg   = nd_bcp_read_u32_c(&r);
	tmp.mdmg  = nd_bcp_read_u32_c(&r);
	tmp.def   = nd_bcp_read_u32_c(&r);
	tmp.mdef  = nd_bcp_read_u32_c(&r);
	if (!nd_bcp_reader_ok(&r))
		return -1;
	*s = tmp;
	return 1;
}

#ifdef __cplusplus
}
#endif

#endif /* ND_ATTR_BCP_H */

/* src/libnd-attr.c — nd-attr, ported to libxylem.
 *
 * Owns the six-attribute vector for every entity and the derived values that
 * depend on it: max HP, max MP, and the per-slot `effect` values that spells
 * and equipment buff and debuff. Also the `stats` BCP frame the client renders,
 * and the reroll/train commands.
 *
 * Original: tty-pt/nd-attr @ 265 lines main.c, from the nd-basics
 * superproject.
 *
 * This TU XY_IMPLs hp_max, mp_max, effect, stat, modifier, mcp_stats, train,
 * attr_award, on_reroll, on_status and on_add, and so must NOT include
 * include/nd/attr.h or nd/hooks.h -- an XY_IMPL and an XY_DECL of the same name
 * in one TU is the XY equivalent of the old SIC_DEF + SIC_DECL collision. That
 * is why ATTR_IMPL below exists: it switches the header's XY_DECLs off for this
 * TU only, exactly as nd-core's CORE_IMPL does.
 *
 * On the old `sic_last(&last)` calls: those read the previous implementor's
 * return so that a co-implementor (a buff module) could ADD to a base value.
 * XY cannot express that -- `xy_last()` is only meaningful once the dispatch
 * has ended, not mid-dispatch (libxylem-dispatch.c:14-22), so there is no
 * `last` term here. That is behaviour-preserving for the in-tree module set:
 * hp_max, mp_max, effect and stat have exactly ONE implementor (this file), so
 * the old chain always produced 0. A future buff module needs the decorator
 * table pattern nd-core uses for on_icon.
 */

#include <ttypt/xy-mod.h>

#include <nd/xy.h>

#include <stdlib.h>
#include <string.h>

#define ATTR_IMPL
#include <nd/attr.h>

#include <nd/level.h>

typedef struct {
	unsigned attr[ATTR_MAX];
	unsigned spend;
} attr_t;

static unsigned attr_hd;
static unsigned bcp_stats;

/* API. XY_IMPL both defines the function and emits the dispatch adapter, so
 * each name gets exactly one, with its body -- no forward declarations.
 *
 * RENAMED from the original's `stat`, which collides with libc's stat(2):
 * <ttypt/xy.h> pulls in <sys/stat.h> transitively, so a module-visible
 * function named `stat` is a conflicting-types error at best. Same class of
 * fix as nd-level's shadowed local. attr_stat is the spelling now.
 *
 * Order matters below: XY_IMPL emits a definition, so a caller has to come
 * after its callee. attr_stat leads because modifier calls it. */
XY_IMPL(unsigned, attr_stat, unsigned, ref, enum attribute, at)
{
	attr_t attr;

	if (at >= ATTR_MAX)
		return 0;
	nd_get(attr_hd, &attr, &ref);
	return attr.attr[at];
}

XY_IMPL(long, modifier, unsigned, ref, enum attribute, at)
{
	return ((long) attr_stat(ref, at) - 10) / 2;
}

XY_IMPL(long, effect, unsigned, ref, enum affect, slot)
{
	static const unsigned effect_map[] = {
		ATTR_CON, // HP
		ATTR_MAX, // MOV
		ATTR_INT, // MDMG
		ATTR_INT, // MDEF
		ATTR_DEX, // DODGE
		ATTR_STR, // DMG
		ATTR_MAX, // DEF
		ATTR_MAX, // NEG
		ATTR_MAX, // BUF
	};

	unsigned at;

	if (slot >= sizeof(effect_map) / sizeof(effect_map[0]))
		return 0;
	at = effect_map[slot];
	if (at == ATTR_MAX)
		return 0;

	return modifier(ref, at);
}

XY_IMPL(long, hp_max, unsigned, ref)
{
	return level(ref) * modifier(ref, ATTR_CON);
}

XY_IMPL(long, mp_max, unsigned, ref)
{
	return level(ref) + modifier(ref, ATTR_WIZ);
}

static inline unsigned char
d6(void)
{
	return (random() % 6) + 1;
}

XY_IMPL(int, mcp_stats, unsigned, player_ref)
{
	attr_t attr;
	unsigned char iden = bcp_stats;
	static char bcp_buf[2 + sizeof(iden) + sizeof(attr.attr) + sizeof(unsigned) * 7];
	char *p = bcp_buf;

	nd_get(attr_hd, &attr, &player_ref);

	memcpy(p, "#b", 2);
	memcpy(p += 2, &iden, sizeof(iden));
	memcpy(p += sizeof(iden), attr.attr, sizeof(attr.attr));
	p += sizeof(attr.attr);

	unsigned ret = 0;
	ret = effect(player_ref, AF_DODGE);
	memcpy(p, &ret, sizeof(unsigned));
	p += sizeof(ret);

	ret = effect(player_ref, AF_DMG);
	memcpy(p, &ret, sizeof(unsigned));
	p += sizeof(ret);

	ret = effect(player_ref, AF_DEF);
	memcpy(p, &ret, sizeof(unsigned));
	p += sizeof(ret);

	nd_wwrite(player_ref, bcp_buf, p - bcp_buf);

	return 0;
}

XY_IMPL(int, on_status, unsigned, player_ref)
{
	attr_t attr;
	nd_get(attr_hd, &attr, &player_ref);
	nd_printf(player_ref, "Attr\t   (base) str %4u, con %4u, "
			"dex %4u, int %4u, wis %4u, cha %4u\n"
			"\t effects: mov %4ld, dodg %3ld, dmg %4ld, "
			"mdmg %3ld, def %4ld, mdef %3ld\n",
			attr.attr[ATTR_STR], attr.attr[ATTR_CON],
			attr.attr[ATTR_DEX], attr.attr[ATTR_INT],
			attr.attr[ATTR_WIZ], attr.attr[ATTR_CHA],
			effect(player_ref, AF_MOV),
			effect(player_ref, AF_DODGE),
			effect(player_ref, AF_DMG),
			effect(player_ref, AF_MDMG),
			effect(player_ref, AF_DEF),
			effect(player_ref, AF_MDEF));
	return 0;
}

static unsigned
roll_stat(void)
{
	unsigned d4d6[] = {
		d6(), d6(), d6(), d6()
	};

	unsigned min = 6, total = 0;
	for (unsigned i = 0; i < 4; i++) {
		unsigned cur = d4d6[i];
		total += cur;
		if (cur < min)
			min = cur;
	}

	return total - min;
}

/* SIC. Placed before reroll() because reroll() calls it and XY_IMPL emits a
 * definition rather than a declaration. */
XY_IMPL(int, on_reroll, unsigned, player_ref)
{
	(void) player_ref;
	return 0;
}

static void
reroll(unsigned ref)
{
	attr_t attr;
	nd_get(attr_hd, &attr, &ref);

	for (int i = 0; i < ATTR_MAX; i++)
		attr.attr[i] = roll_stat();

	nd_put(attr_hd, &ref, &attr);
	on_reroll(ref);
	mcp_stats(ref);
}

static void
do_reroll(int fd, int argc, char *argv[])
{
	unsigned player_ref = fd_player(fd),
	      thing_ref = player_ref;

	char *what = argv[1];

	if (
			argc > 1 && (thing_ref = ematch_me(player_ref, what)) == NOTHING
			&& (thing_ref = ematch_near(player_ref, what)) == NOTHING
			&& (thing_ref = ematch_mine(player_ref, what)) == NOTHING
	   ) {
		/* NOMATCH_MESSAGE, spelled out: it lives in the engine's internal
		 * uapi/match.h, which a module must not include (MODS.md §0.5). */
		nd_printf(player_ref, "I don't know what you mean.\n");
		return;
	}

	reroll(thing_ref);
}

XY_IMPL(int, train, unsigned, player_ref, enum attribute, at, unsigned, amount)
{
	attr_t attr;

	if (at >= ATTR_MAX)
		return 0;
	nd_get(attr_hd, &attr, &player_ref);

	attr.attr[at] += amount;
	attr.spend -= amount;

	nd_put(attr_hd, &player_ref, &attr);
	mcp_stats(player_ref);
	return 0;
}

static void
do_train(int fd, int argc __attribute__((unused)), char *argv[])
{
	attr_t attr;
	unsigned player_ref = fd_player(fd);
	const char *attrib = argv[1];
	const char *amount_s = argv[2];
	int at;

	switch (attrib[0]) {
	case 's': at = ATTR_STR; break;
	case 'c': at = ATTR_CON; break;
	case 'd': at = ATTR_DEX; break;
	case 'i': at = ATTR_INT; break;
	case 'w': at = ATTR_WIZ; break;
	case 'h': at = ATTR_CHA; break;
	default:
		  nd_printf(player_ref, "Invalid attribute.\n");
		  return;
	}

	nd_get(attr_hd, &attr, &player_ref);

	int avail = attr.spend;
	int amount = *amount_s ? atoi(amount_s) : 1;

	if (amount > avail) {
		  nd_printf(player_ref, "Not enough points.\n");
		  return;
    }

	attr.attr[at] += amount;

	attr.spend = avail - amount;
	nd_put(attr_hd, &player_ref, &attr);
	nd_printf(player_ref, "Your %s increases %d time(s).\n", attrib, amount);
	mcp_stats(player_ref);
}

XY_IMPL(int, attr_award, unsigned, player_ref, unsigned, amount)
{
	attr_t attr;

	nd_get(attr_hd, &attr, &player_ref);
	attr.spend += amount;
	nd_put(attr_hd, &player_ref, &attr);
	return 0;
}

/* NOTHING above is an engine event hook; these two are the ones the engine
 * fires, so they are XY_IMPLs here and never XY_DECLs. */
XY_IMPL(int, on_add, unsigned, ref, unsigned, type, uint64_t, v)
{
	attr_t attr;

	if (type != TYPE_ENTITY)
		return 0;

	(void) v;
	memset(&attr, 0, sizeof(attr));
	nd_put(attr_hd, &ref, &attr);
	reroll(ref);
	return 0;
}

XY_MODULE_API void
xy_install(void)
{
	nd_len_reg("attr", sizeof(attr_t));
	attr_hd = nd_open("attr", "u", "attr", 0);

	nd_register("reroll", do_reroll, 0);
	nd_register("train", do_train, 0);

	bcp_stats = nd_put(HD_BCP, NULL, "stats");
}
#include "./include/uapi/attr.h"

#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include <nd/nd.h>
#include <nd/level.h>

typedef struct {
	unsigned attr[ATTR_MAX];
	unsigned spend;
} attr_t;

unsigned attr_hd;
unsigned bcp_stats;

/* API */
SIC_DEF(long, hp_max, unsigned, ref);
SIC_DEF(long, mp_max, unsigned, ref);
SIC_DEF(long, effect, unsigned, ref, enum affect, slot);
SIC_DEF(unsigned, stat, unsigned, ref, enum attribute, at);
SIC_DEF(long, modifier, unsigned, ref, enum attribute, at);

SIC_DEF(int, mcp_stats, unsigned, player_ref);
SIC_DEF(int, train, unsigned, player_ref, enum attribute, at, unsigned, amount);
SIC_DEF(int, attr_award, unsigned, player_ref, unsigned, amount);

SIC_DEF(int, on_reroll, unsigned, player_ref);

long modifier(unsigned ref, enum attribute at) {
	unsigned stat = call_stat(ref, at);
	return (stat - 10) / 2;
}

long effect(unsigned ref, enum affect slot) {
	static unsigned effect_map[] = {
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

	unsigned at = effect_map[slot];
	long last = 0;

	sic_last(&last);
	if (at == ATTR_MAX)
		return last;

	return last + call_modifier(ref, at);
}

long hp_max(unsigned ref) {
	long last = 0;
	sic_last(&last);
	return last + call_level(ref) * call_modifier(ref, ATTR_CON);
}

long mp_max(unsigned ref) {
	long last = 0;
	sic_last(&last);
	return last + call_level(ref) + call_modifier(ref, ATTR_WIZ);
}

unsigned stat(unsigned ref, enum attribute at) {
	attr_t attr;
	unsigned last = 0;
	nd_get(attr_hd, &attr, &ref);
	sic_last(&last);
	return last + attr.attr[at];
}

static inline unsigned char
d6(void)
{
	return (random() % 6) + 1;
}

int
mcp_stats(unsigned player_ref)
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
	ret = call_effect(player_ref, AF_DODGE);
	memcpy(p, &ret, sizeof(unsigned));
	p += sizeof(ret);

	ret = call_effect(player_ref, AF_DMG);
	memcpy(p, &ret, sizeof(unsigned));
	p += sizeof(ret);

	ret = call_effect(player_ref, AF_DEF);
	memcpy(p, &ret, sizeof(unsigned));
	p += sizeof(ret);

	nd_wwrite(player_ref, bcp_buf, p - bcp_buf);

	return 0;
}

int
on_status(unsigned player_ref)
{
	attr_t attr;
	nd_get(attr_hd, &attr, &player_ref);
	nd_writef(player_ref, "Attr\t   (base) str %4u, con %4u, "
			"dex %4u, int %4u, wis %4u, cha %4u\n"
			"\t effects: mov %4ld, dodg %3ld, dmg %4ld, "
			"mdmg %3ld, def %4ld, mdef %3ld\n",
			attr.attr[ATTR_STR], attr.attr[ATTR_CON],
			attr.attr[ATTR_DEX], attr.attr[ATTR_INT],
			attr.attr[ATTR_WIZ], attr.attr[ATTR_CHA],
			call_effect(player_ref, AF_MOV),
			call_effect(player_ref, AF_DODGE),
			call_effect(player_ref, AF_DMG),
			call_effect(player_ref, AF_MDMG),
			call_effect(player_ref, AF_DEF),
			call_effect(player_ref, AF_MDEF));
	return 0;
}

unsigned roll_stat(void) {
	unsigned d4d6[] = {
		d6(), d6(), d6(), d6()
	};

	unsigned min = 6, total = 0;
	for (register unsigned i = 0; i < 4; i++) {
		unsigned cur = d4d6[i];
		total += cur;
		if (cur < min)
			min = cur;
	}

	return total - min;
}

void
reroll(unsigned ref)
{
	attr_t attr;
	nd_get(attr_hd, &attr, &ref);

	for (int i = 0; i < ATTR_MAX; i++)
		attr.attr[i] = roll_stat();

	nd_put(attr_hd, &ref, &attr);
	call_on_reroll(ref);
	mcp_stats(ref);
}

void
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
		nd_writef(player_ref, NOMATCH_MESSAGE);
		return;
	}

	reroll(thing_ref);
}

int
train(unsigned player_ref, enum attribute at, unsigned amount) {
	attr_t attr;

	nd_get(attr_hd, &attr, &player_ref);

	attr.attr[at] += amount;
	attr.spend -= amount;

	nd_put(attr_hd, &player_ref, &attr);
        mcp_stats(player_ref);
	return 0;
}

void
do_train(int fd, int argc __attribute__((unused)), char *argv[]) {
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
		  nd_writef(player_ref, "Invalid attribute.\n");
		  return;
	}

	nd_get(attr_hd, &attr, &player_ref);

	int avail = attr.spend;
	int amount = *amount_s ? atoi(amount_s) : 1;

	if (amount > avail) {
		  nd_writef(player_ref, "Not enough points.\n");
		  return;
	}

	attr.attr[at] += amount;

	attr.spend = avail - amount;
	nd_put(attr_hd, &player_ref, &attr);
	nd_writef(player_ref, "Your %s increases %d time(s).\n", attrib, amount);
        mcp_stats(player_ref);
}

int on_add(unsigned ref, unsigned type, uint64_t v __attribute__((unused))) {
	attr_t attr;

	if (type != TYPE_ENTITY)
		return 0;

	memset(&attr, 0, sizeof(attr));
	nd_put(attr_hd, &ref, &attr);
	reroll(ref);
	return 0;
}

void mod_open(void) {
	nd_len_reg("attr", sizeof(attr_t));
	attr_hd = nd_open("attr", "u", "attr", 0);

	nd_register("reroll", do_reroll, 0);
	nd_register("train", do_train, 0);
}

void mod_install(void) {
	mod_open();

	bcp_stats = nd_put(HD_BCP, NULL, "stats");
}

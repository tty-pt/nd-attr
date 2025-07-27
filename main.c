#include "./include/uapi/attr.h"

#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include <nd/nd.h>

typedef struct {
	unsigned attr[ATTR_MAX];
	unsigned spend;
} attr_t;

unsigned attr_hd;
unsigned bcp_stats;

SIC_DEF(unsigned, hp_max, unsigned, ref);
SIC_DEF(unsigned, mp_max, unsigned, ref);
SIC_DEF(short, effect, unsigned, ref, enum affect, slot);
SIC_DEF(unsigned, stat, unsigned, ref, enum attribute, at);

SIC_DEF(int, mcp_stats, unsigned, player_ref);
SIC_DEF(int, train, unsigned, player_ref, enum attribute, at, unsigned, amount);
SIC_DEF(int, attr_award, unsigned, player_ref, unsigned, amount);

SIC_DEF(int, on_reroll, unsigned, player_ref)

unsigned hp_max(unsigned ref) {
	attr_t attr;
	nd_get(attr_hd, &attr, &ref);
	return (unsigned) (10 + ((long) attr.attr[ATTR_CON] - 10) / 2);
}

unsigned mp_max(unsigned ref) {
	attr_t attr;
	nd_get(attr_hd, &attr, &ref);
	return (unsigned) (10 + ((long) attr.attr[ATTR_WIZ] - 10) / 2);
}

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

short effect(unsigned ref, enum affect slot) {
	attr_t attr;
	nd_get(attr_hd, &attr, &ref);
	unsigned at = effect_map[slot];
	if (at == ATTR_MAX)
		return 0;
	return 10 + (attr.attr[at] - 10) / 2;
}

unsigned stat(unsigned ref, enum attribute at) {
	attr_t attr;
	nd_get(attr_hd, &attr, &ref);
	return attr.attr[at];
}

static inline unsigned char
d20(void)
{
	return (random() % 20) + 1;
}

int
mcp_stats(unsigned player_ref)
{
	attr_t attr;
	unsigned char iden = bcp_stats;
	static char bcp_buf[2 + sizeof(iden) + sizeof(attr.attr) + sizeof(short) * 7];
	char *p = bcp_buf;

	nd_get(attr_hd, &attr, &player_ref);

	memcpy(p, "#b", 2);
	memcpy(p += 2, &iden, sizeof(iden));
	memcpy(p += sizeof(iden), attr.attr, sizeof(attr.attr));
	p += sizeof(attr.attr);

	short ret = 0;
	ret = call_effect(player_ref, AF_DODGE);
	memcpy(p, &ret, sizeof(short));
	p += sizeof(ret);

	ret = call_effect(player_ref, AF_DMG);
	memcpy(p, &ret, sizeof(short));
	p += sizeof(ret);

	ret = call_effect(player_ref, AF_DEF);
	memcpy(p, &ret, sizeof(short));
	p += sizeof(ret);

	nd_wwrite(player_ref, bcp_buf, p - bcp_buf);

	return 0;
}

int
on_status(unsigned player_ref)
{
	attr_t attr;
	nd_get(attr_hd, &attr, &player_ref);
	nd_writef(player_ref, "Attr:\t\tstr %u\tcon %u\t"
			"dex %u\tint %u\twis %u\tcha %u\t"
			"mov %d\tdodge %d\tdmg %d\tmdmg %d\t"
			"def %d\tmdef %d\n",
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

void
do_reroll(int fd, int argc, char *argv[])
{
	attr_t attr;
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

	nd_get(attr_hd, &attr, &player_ref);

	for (int i = 0; i < ATTR_MAX; i++)
		attr.attr[i] = d20();

	nd_put(attr_hd, &player_ref, &attr);

	call_on_reroll(player_ref);
	mcp_stats(player_ref);
}

int
train(unsigned player_ref, enum attribute at, unsigned amount) {
	attr_t attr;

	nd_get(attr_hd, &attr, &player_ref);

	unsigned c = attr.attr[at];
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

	unsigned c = attr.attr[at];
	attr.attr[at] += amount;

	attr.spend = avail - amount;
	nd_put(attr_hd, &player_ref, &attr);
	nd_writef(player_ref, "Your %s increases %d time(s).\n", attrib, amount);
        mcp_stats(player_ref);
}

void mod_open() {
	SIC_AREG(hp_max);
	SIC_AREG(mp_max);
	SIC_AREG(effect);
	SIC_AREG(stat);

	SIC_AREG(mcp_stats);
	SIC_AREG(train);
	SIC_AREG(attr_award);

	SIC_AREG(on_reroll);

	nd_len_reg("attr", sizeof(attr_t));
	attr_hd = nd_open("attr", "u", "attr", 0);

	bcp_stats = nd_put(HD_BCP, NULL, "stats");

	nd_register("reroll", do_reroll, 0);
	nd_register("train", do_train, 0);
}

int on_add(unsigned ref, unsigned type, uint64_t v) {
	attr_t attr;

	if (type != TYPE_ENTITY)
		return 0;

	memset(&attr, 0, sizeof(attr));

	for (register int j = 0; j < ATTR_MAX; j++)
		attr.attr[j] = 1;

	nd_put(attr_hd, &ref, &attr);
	return 0;
}

void mod_install() {
	mod_open();
}

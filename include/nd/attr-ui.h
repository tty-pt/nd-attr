#ifndef ND_ATTR_UI_H
#define ND_ATTR_UI_H

/*
 * attr-ui.h — Extensible UI component contribution for nd-attr.
 * Provides the Character Sheet modal, action button, and HP/MP gauges.
 */

#include <bud/bud.h>
#include <bud/bud_app.h>
#include <nd/ui-slot-types.h>
#include <nd/bcp.h>
#include <nd/attr-bcp.h>
#include <string.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Gauges this sheet knows how to draw. The core client only stores named
 * (val,max) gauges; which ones exist and how they look is presentation. */
static const struct {
	const char *name;   /* gauge published by the owning module */
	const char *color;  /* CSS colour (variable with fallback) */
} nd_attr_gauge_style[] = {
	{ "hp", "var(--c1,#bd4a6a)" },
	{ "mp", "var(--c4,#7389bd)" },
};

static inline bud_node *
nd_attr_render_gauges(const void *state)
{
	const nd_client_state_t *st = (const nd_client_state_t *)state;
	bud_node *wrap = bud_element("div");
	bud_set_attr(wrap, "class", "flex flex-col gap-1.5 w-full max-w-[180px] select-none");
	if (!st) return wrap;

	for (size_t i = 0; i < sizeof(nd_attr_gauge_style) / sizeof(nd_attr_gauge_style[0]); i++) {
		const nd_bar_state_t *g = nd_client_gauge_get(st, nd_attr_gauge_style[i].name);
		unsigned val = g ? g->val : 0, max = g ? g->max : 0;
		unsigned pct = max > 0 ? val * 100 / max : 0;
		if (pct > 100) pct = 100;

		bud_node *bar = bud_element("div");
		bud_set_attr(bar, "data-gauge", nd_attr_gauge_style[i].name);
		bud_set_attr(bar, "style", "outline: 0.5px solid white;");
		bud_set_attr(bar, "class", "relative block w-full h-[10px] text-[9px] min-w-[120px] bg-black/60 overflow-hidden outline");

		char cls[96], sty[48], txt[32];
		bud_node *fill = bud_element("div");
		snprintf(cls, sizeof(cls), "h-full bg-[%s] transition-all", nd_attr_gauge_style[i].color);
		snprintf(sty, sizeof(sty), "width: %u%%;", pct);
		bud_set_attr(fill, "class", cls);
		bud_set_attr(fill, "style", sty);
		bud_append(bar, fill);

		bud_node *label = bud_element("div");
		bud_set_attr(label, "class", "absolute inset-0 flex items-center justify-center font-mono text-white text-[9px] drop-shadow-[1px_1px_2px_black]");
		snprintf(txt, sizeof(txt), "%u/%u", val, max);
		bud_append(label, bud_text(txt));
		bud_append(bar, label);

		bud_append(wrap, bar);
	}
	return wrap;
}

static inline int
on_attr_modal_close(bud_event *ev)
{
	(void)ev;
	nd_ui_set_active_modal("");
#ifdef __wasm__
	bud_app_set_state();
#endif
	return 0;
}

static inline int
on_attr_card_click(bud_event *ev)
{
	if (ev) bud_event_stop_propagation(ev);
	return 1;
}

static inline void
nd_attr_stat_row(bud_node *col, const char *label, const char *value)
{
	bud_node *row = bud_element("div");
	bud_set_attr(row, "class", "flex justify-between text-xs");
	bud_node *l = bud_element("span");
	bud_set_attr(l, "class", "tbbold font-bold text-neutral-300");
	bud_append(l, bud_text(label));
	bud_node *v = bud_element("span");
	bud_set_attr(v, "class", "font-mono text-neutral-100");
	bud_append(v, bud_text(value));
	bud_append(row, l);
	bud_append(row, v);
	bud_append(col, row);
}

static inline bud_node *
nd_attr_render_stats_modal(const void *state)
{
	const nd_client_state_t *st = (const nd_client_state_t *)state;
	const char *active = nd_ui_get_active_modal();
	int is_open = (active && strcmp(active, "stats") == 0);

	bud_node *overlay = bud_element("div");
	bud_set_attr(overlay, "id", "stats-modal");
	bud_set_attr(overlay, "class", is_open
		? "modal fixed inset-0 z-50 flex items-center justify-center bg-black/60"
		: "hidden");
	bud_bind(overlay, "click", 0, on_attr_modal_close);

	const nd_stats_state_t *ss = (const nd_stats_state_t *)
		nd_client_ext_get(st, ND_ATTR_EXT_KEY);

	static const struct { const char *label; int idx; } k_attrs[] = {
		{ "str", ND_ATTR_STR }, { "con", ND_ATTR_CON }, { "dex", ND_ATTR_DEX },
		{ "int", ND_ATTR_INT }, { "wiz", ND_ATTR_WIZ }, { "cha", ND_ATTR_CHA },
	};
	enum { EF_DODGE, EF_DMG, EF_MDMG, EF_DEF, EF_MDEF };
	static const struct { const char *label; int which; } k_effects[] = {
		{ "dodge", EF_DODGE }, { "dmg", EF_DMG }, { "mdmg", EF_MDMG },
		{ "def", EF_DEF }, { "mdef", EF_MDEF },
	};

	bud_node *card = bud_element("div");
	bud_set_attr(card, "id", "stats");
	bud_set_attr(card, "class", "flex gap-6 p-4 border border-[#9589c5] bg-[#2c2c2c]/90 shadow-2xl min-w-[260px] select-none");

	bud_node *cols[2];
	for (int c = 0; c < 2; c++) {
		cols[c] = bud_element("div");
		bud_set_attr(cols[c], "class", "flex flex-col gap-1.5 flex-1");
		bud_append(card, cols[c]);
	}

	for (size_t i = 0; i < sizeof(k_attrs) / sizeof(k_attrs[0]); i++) {
		char v[16];
		snprintf(v, sizeof(v), "%u", ss ? (unsigned)ss->attr[k_attrs[i].idx] : 0u);
		nd_attr_stat_row(cols[0], k_attrs[i].label, v);
	}
	for (size_t i = 0; i < sizeof(k_effects) / sizeof(k_effects[0]); i++) {
		unsigned val = 0;
		if (ss) switch (k_effects[i].which) {
		case EF_DODGE: val = ss->dodge; break;
		case EF_DMG:   val = ss->dmg;   break;
		case EF_MDMG:  val = ss->mdmg;  break;
		case EF_DEF:   val = ss->def;   break;
		case EF_MDEF:  val = ss->mdef;  break;
		}
		char v[16];
		snprintf(v, sizeof(v), "%u", val);
		nd_attr_stat_row(cols[1], k_effects[i].label, v);
	}

	bud_bind(card, "click", 1, on_attr_card_click);
	bud_append(overlay, card);
	return overlay;
}

static inline int
nd_attr_on_action(bud_event *ev, const char *action_id)
{
	(void)ev;
	(void)action_id;
	const char *cur = nd_ui_get_active_modal();
	if (cur && strcmp(cur, "stats") == 0) {
		nd_ui_set_active_modal("");
	} else {
		nd_ui_set_active_modal("stats");
	}
#ifdef __wasm__
	bud_app_set_state();
#endif
	return 0;
}

void nd_attr_ui_register(void);

#ifdef __cplusplus
}
#endif

#endif /* ND_ATTR_UI_H */

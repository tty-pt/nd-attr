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

#ifdef __cplusplus
extern "C" {
#endif

static inline bud_node *
nd_attr_render_gauges(const void *state)
{
	const nd_client_state_t *st = (const nd_client_state_t *)state;
	if (!st) return bud_element("div");

	unsigned hp_pct = (st->hp.max > 0) ? (st->hp.val * 100 / st->hp.max) : 0;
	if (hp_pct > 100) hp_pct = 100;

	unsigned mp_pct = (st->mp.max > 0) ? (st->mp.val * 100 / st->mp.max) : 0;
	if (mp_pct > 100) mp_pct = 100;

	return bud_tpl(
		"<div class='flex flex-col gap-2 w-full max-w-[180px]'>"
		"  <div class='flex flex-col gap-0.5'>"
		"    <div class='flex justify-between text-xs text-neutral-400'><span>HP</span><span id='hp-val'>%u / %u</span></div>"
		"    <div class='w-full bg-neutral-800 rounded h-2 overflow-hidden'>"
		"      <div id='hp-bar' class='bg-red-500 h-full rounded transition-all' style='width: %u%%;'></div>"
		"    </div>"
		"  </div>"
		"  <div class='flex flex-col gap-0.5'>"
		"    <div class='flex justify-between text-xs text-neutral-400'><span>MP</span><span id='mp-val'>%u / %u</span></div>"
		"    <div class='w-full bg-neutral-800 rounded h-2 overflow-hidden'>"
		"      <div id='mp-bar' class='bg-blue-500 h-full rounded transition-all' style='width: %u%%;'></div>"
		"    </div>"
		"  </div>"
		"</div>",
		st->hp.val, st->hp.max, hp_pct,
		st->mp.val, st->mp.max, mp_pct
	);
}

static inline bud_node *
nd_attr_render_stats_modal(const void *state)
{
	const nd_client_state_t *st = (const nd_client_state_t *)state;
	const char *active = nd_ui_get_active_modal();
	if (!st || !active || strcmp(active, "stats") != 0) {
		bud_node *empty = bud_element("div");
		bud_set_attr(empty, "id", "stats-modal");
		bud_set_attr(empty, "class", "hidden");
		return empty;
	}

	bud_node *overlay = bud_element("div");
	bud_set_attr(overlay, "id", "stats-modal");
	bud_set_attr(overlay, "class", "fixed inset-0 z-50 flex items-center justify-center bg-black/60 p-4");

	bud_node *card = bud_element("div");
	bud_set_attr(card, "class", "bg-neutral-900 border border-neutral-700 rounded-lg p-6 max-w-sm w-full flex flex-col gap-4 shadow-xl");

	bud_node *header = bud_element("div");
	bud_set_attr(header, "class", "flex justify-between items-center border-b border-neutral-800 pb-2");
	bud_node *title = bud_element("h2");
	bud_set_attr(title, "class", "text-lg font-bold text-neutral-200");
	bud_append(title, bud_text("Character Attributes"));
	bud_append(header, title);

	bud_node *close_btn = bud_element("button");
	bud_set_attr(close_btn, "type", "button");
	bud_set_attr(close_btn, "class", "text-neutral-400 hover:text-neutral-100 text-sm");
	bud_append(close_btn, bud_text("✕"));
	bud_append(header, close_btn);

	bud_append(card, header);

	bud_node *grid = bud_tpl(
		"<div class='grid grid-cols-2 gap-2 text-sm text-neutral-200'>"
		"  <div class='flex justify-between'><span class='text-neutral-400'>Strength:</span><span class='font-mono'>%u</span></div>"
		"  <div class='flex justify-between'><span class='text-neutral-400'>Constitution:</span><span class='font-mono'>%u</span></div>"
		"  <div class='flex justify-between'><span class='text-neutral-400'>Dexterity:</span><span class='font-mono'>%u</span></div>"
		"  <div class='flex justify-between'><span class='text-neutral-400'>Intelligence:</span><span class='font-mono'>%u</span></div>"
		"  <div class='flex justify-between'><span class='text-neutral-400'>Wisdom:</span><span class='font-mono'>%u</span></div>"
		"  <div class='flex justify-between'><span class='text-neutral-400'>Charisma:</span><span class='font-mono'>%u</span></div>"
		"</div>"
		"<div class='border-t border-neutral-800 pt-2 grid grid-cols-3 gap-2 text-xs text-center'>"
		"  <div><div class='text-neutral-500'>Dodge</div><div class='font-mono text-neutral-300'>%u</div></div>"
		"  <div><div class='text-neutral-500'>Damage</div><div class='font-mono text-neutral-300'>%u</div></div>"
		"  <div><div class='text-neutral-500'>Defense</div><div class='font-mono text-neutral-300'>%u</div></div>"
		"</div>",
		st->stats.attr[ND_ATTR_STR],
		st->stats.attr[ND_ATTR_CON],
		st->stats.attr[ND_ATTR_DEX],
		st->stats.attr[ND_ATTR_INT],
		st->stats.attr[ND_ATTR_WIZ],
		st->stats.attr[ND_ATTR_CHA],
		st->stats.dodge,
		st->stats.dmg,
		st->stats.def
	);
	bud_append(card, grid);
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

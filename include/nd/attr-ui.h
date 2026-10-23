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
		"<div class='flex flex-col gap-1.5 w-full max-w-[180px] select-none'>"
		"  <div style='outline: 0.5px solid white;' class='relative block w-full h-[10px] text-[9px] min-w-[120px] bg-black/60 overflow-hidden outline'>"
		"    <div class='h-full bg-[var(--c1,#bd4a6a)] transition-all' style='width: %u%%;'></div>"
		"    <div class='absolute inset-0 flex items-center justify-center font-mono text-white text-[9px] drop-shadow-[1px_1px_2px_black]'>%u/%u</div>"
		"  </div>"
		"  <div style='outline: 0.5px solid white;' class='relative block w-full h-[10px] text-[9px] min-w-[120px] bg-black/60 overflow-hidden outline'>"
		"    <div class='h-full bg-[var(--c4,#7389bd)] transition-all' style='width: %u%%;'></div>"
		"    <div class='absolute inset-0 flex items-center justify-center font-mono text-white text-[9px] drop-shadow-[1px_1px_2px_black]'>%u/%u</div>"
		"  </div>"
		"</div>",
		hp_pct, st->hp.val, st->hp.max,
		mp_pct, st->mp.val, st->mp.max
	);
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

	bud_node *card = bud_tpl(
		"<div id='stats' class='flex gap-6 p-4 border border-[#9589c5] bg-[#2c2c2c]/90 shadow-2xl min-w-[260px] select-none'>"
		"  <div class='flex flex-col gap-1.5 flex-1'>"
		"    <div class='flex justify-between text-xs'><span class='tbbold font-bold text-neutral-300'>str</span><span class='font-mono text-neutral-100'>%u</span></div>"
		"    <div class='flex justify-between text-xs'><span class='tbbold font-bold text-neutral-300'>con</span><span class='font-mono text-neutral-100'>%u</span></div>"
		"    <div class='flex justify-between text-xs'><span class='tbbold font-bold text-neutral-300'>dex</span><span class='font-mono text-neutral-100'>%u</span></div>"
		"    <div class='flex justify-between text-xs'><span class='tbbold font-bold text-neutral-300'>int</span><span class='font-mono text-neutral-100'>%u</span></div>"
		"    <div class='flex justify-between text-xs'><span class='tbbold font-bold text-neutral-300'>wiz</span><span class='font-mono text-neutral-100'>%u</span></div>"
		"    <div class='flex justify-between text-xs'><span class='tbbold font-bold text-neutral-300'>cha</span><span class='font-mono text-neutral-100'>%u</span></div>"
		"  </div>"
		"  <div class='flex flex-col gap-1.5 flex-1'>"
		"    <div class='flex justify-between text-xs'><span class='tbbold font-bold text-neutral-300'>dodge</span><span class='font-mono text-neutral-100'>%u</span></div>"
		"    <div class='flex justify-between text-xs'><span class='tbbold font-bold text-neutral-300'>dmg</span><span class='font-mono text-neutral-100'>%u</span></div>"
		"    <div class='flex justify-between text-xs'><span class='tbbold font-bold text-neutral-300'>mdmg</span><span class='font-mono text-neutral-100'>%u</span></div>"
		"    <div class='flex justify-between text-xs'><span class='tbbold font-bold text-neutral-300'>def</span><span class='font-mono text-neutral-100'>%u</span></div>"
		"    <div class='flex justify-between text-xs'><span class='tbbold font-bold text-neutral-300'>mdef</span><span class='font-mono text-neutral-100'>%u</span></div>"
		"  </div>"
		"</div>",
		st ? st->stats.attr[ND_ATTR_STR] : 0,
		st ? st->stats.attr[ND_ATTR_CON] : 0,
		st ? st->stats.attr[ND_ATTR_DEX] : 0,
		st ? st->stats.attr[ND_ATTR_INT] : 0,
		st ? st->stats.attr[ND_ATTR_WIZ] : 0,
		st ? st->stats.attr[ND_ATTR_CHA] : 0,
		st ? st->stats.dodge : 0,
		st ? st->stats.dmg : 0,
		st ? st->stats.mdmg : 0,
		st ? st->stats.def : 0,
		st ? st->stats.mdef : 0
	);

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ttypt/xy-mod.h>
#include <bud/bud.h>
#include <bud/bud_app.h>
#include <nd/ui-slot-types.h>
#include <nd/bcp-types.h>
#include <nd/attr-ui.h>

XY_LISTENER_TAG(int, on_nd_ui_action, attr, const char *, action_id)
{
	if (action_id && strcmp(action_id, "stats") == 0) {
		nd_ui_toggle_modal("stats");
		return 1;
	}
	int ret = 0;
	return xy_last(&ret) == XY_OK ? ret : 0;
}

static int
attr_contribute(int slot, void *parent, const void *state)
{
	if (slot == ND_UI_SLOT_ACTIONS)
		bud_append((bud_node *)parent, nd_ui_dock_button("📋", "Character Sheet", "stats"));
	else if (slot == ND_UI_SLOT_MODALS)
		bud_append((bud_node *)parent, nd_attr_render_stats_modal(state));
	return 0;
}

static bud_node *
attr_render_slot(int slot, const void *state)
{
	if (slot == ND_UI_SLOT_GAUGES) {
		return nd_attr_render_gauges(state);
	}
	return NULL;
}

XY_LISTENER_TAG(int, on_bcp_channel, attr, uint8_t, opcode, const uint8_t *, payload, size_t, len, void *, state)
{
	if (opcode == ND_BCP_STATS) {
		/* The channel hook receives the payload after the 3-byte header;
		 * nd_attr_bcp_unpack expects the full frame, so rebuild the view. */
		uint8_t frame[3 + 64];
		if (len > sizeof(frame) - 3)
			return -1;
		frame[0] = ND_BCP_MAGIC_0; frame[1] = ND_BCP_MAGIC_1; frame[2] = opcode;
		memcpy(frame + 3, payload, len);
		return nd_attr_bcp_unpack(frame, 3 + len, state);
	}
	int ret = 0;
	return xy_last(&ret) == XY_OK ? ret : 0;
}

static int
attr_state_default(void *state)
{
	nd_stats_state_t *s = nd_client_ext((nd_client_state_t *)state,
			ND_ATTR_EXT_KEY, sizeof(nd_stats_state_t));
	if (s) {
		for (int i = 0; i < ND_ATTR_MAX; i++)
			s->attr[i] = 10;
		s->dodge = 5; s->dmg = 12; s->mdmg = 8; s->def = 10; s->mdef = 6;
	}
	return 0;
}

static int
attr_state_serialize(const void *state, char *buf, size_t cap)
{
	const nd_stats_state_t *s = (const nd_stats_state_t *)
		nd_client_ext_get((const nd_client_state_t *)state, ND_ATTR_EXT_KEY);
	if (s) {
		size_t cur = strlen(buf);
		snprintf(buf + cur, cap - cur,
			"%s\"attr\":{\"str\":%u,\"con\":%u,\"dex\":%u,\"int\":%u,\"wiz\":%u,\"cha\":%u,"
			"\"dodge\":%u,\"dmg\":%u,\"mdmg\":%u,\"def\":%u,\"mdef\":%u}",
			cur ? "," : "",
			s->attr[ND_ATTR_STR], s->attr[ND_ATTR_CON], s->attr[ND_ATTR_DEX],
			s->attr[ND_ATTR_INT], s->attr[ND_ATTR_WIZ], s->attr[ND_ATTR_CHA],
			s->dodge, s->dmg, s->mdmg, s->def, s->mdef);
	}
	return 0;
}

XY_LISTENER_TAG(int, on_nd_state_deserialize, attr, const char *, json, void *, state)
{
	const char *at = json ? strstr(json, "\"attr\":{") : NULL;
	nd_stats_state_t *s = at ? nd_client_ext((nd_client_state_t *)state,
			ND_ATTR_EXT_KEY, sizeof(nd_stats_state_t)) : NULL;
	if (s) {
		const char *o = at + 7;
		size_t n = strcspn(o, "}") + 1;
		s->attr[ND_ATTR_STR] = (uint16_t)bud_json_int_len(o, n, "str", s->attr[ND_ATTR_STR]);
		s->attr[ND_ATTR_CON] = (uint16_t)bud_json_int_len(o, n, "con", s->attr[ND_ATTR_CON]);
		s->attr[ND_ATTR_DEX] = (uint16_t)bud_json_int_len(o, n, "dex", s->attr[ND_ATTR_DEX]);
		s->attr[ND_ATTR_INT] = (uint16_t)bud_json_int_len(o, n, "int", s->attr[ND_ATTR_INT]);
		s->attr[ND_ATTR_WIZ] = (uint16_t)bud_json_int_len(o, n, "wiz", s->attr[ND_ATTR_WIZ]);
		s->attr[ND_ATTR_CHA] = (uint16_t)bud_json_int_len(o, n, "cha", s->attr[ND_ATTR_CHA]);
		s->dodge = (uint32_t)bud_json_int_len(o, n, "dodge", (int)s->dodge);
		s->dmg   = (uint32_t)bud_json_int_len(o, n, "dmg",   (int)s->dmg);
		s->mdmg  = (uint32_t)bud_json_int_len(o, n, "mdmg",  (int)s->mdmg);
		s->def   = (uint32_t)bud_json_int_len(o, n, "def",   (int)s->def);
		s->mdef  = (uint32_t)bud_json_int_len(o, n, "mdef",  (int)s->mdef);
	}
	int ret = 0;
	return xy_last(&ret) == XY_OK ? ret : 0;
}

ND_UI_GLUE(attr, .contribute = attr_contribute, .render_slot = attr_render_slot, .state_default = attr_state_default, .state_serialize = attr_state_serialize);

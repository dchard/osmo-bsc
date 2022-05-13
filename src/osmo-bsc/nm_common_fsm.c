/* NM FSM, common bits */

/* (C) 2020 by sysmocom - s.m.f.c. GmbH <info@sysmocom.de>
 * Author: Pau Espin Pedrol <pespin@sysmocom.de>
 *
 * All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <osmocom/bsc/nm_common_fsm.h>
#include <osmocom/bsc/signal.h>

const struct value_string nm_fsm_event_names[] = {
	{ NM_EV_SW_ACT_REP, "SW_ACT_REP" },
	{ NM_EV_STATE_CHG_REP, "STATE_CHG_REP" },
	{ NM_EV_GET_ATTR_REP, "GET_ATTR_REP" },
	{ NM_EV_SET_ATTR_ACK, "SET_ATTR_ACK" },
	{ NM_EV_OPSTART_ACK, "OPSTART_ACK" },
	{ NM_EV_OPSTART_NACK, "OPSTART_NACK" },
	{ NM_EV_OML_DOWN, "OML_DOWN" },
	{ NM_EV_FORCE_LOCK, "FORCE_LOCK_CHG" },
	{ NM_EV_FEATURE_NEGOTIATED, "FEATURE_NEGOTIATED" },
	{ 0, NULL }
};

void nm_obj_fsm_becomes_enabled_disabled(struct gsm_bts *bts, void *obj,
					 enum abis_nm_obj_class obj_class, bool running)
{
	struct nm_running_chg_signal_data nsd;

	memset(&nsd, 0, sizeof(nsd));
	nsd.bts = bts;
	nsd.obj_class = obj_class;
	nsd.obj = obj;
	nsd.running = running;

	osmo_signal_dispatch(SS_NM, S_NM_RUNNING_CHG, &nsd);
}

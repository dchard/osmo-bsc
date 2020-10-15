/* Header for all NM FSM. Following 3GPP TS 12.21 Figure 2/GSM 12.21:
  GSM 12.21 Objects' Operational state and availability status behaviour during initialization */

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

#pragma once

#include <osmocom/core/fsm.h>
#include <osmocom/core/utils.h>

/* Common */
enum nm_fsm_events {
	NM_EV_SW_ACT_REP,
	NM_EV_STATE_CHG_REP,
	NM_EV_SET_ATTR_ACK,
	NM_EV_OPSTART_ACK,
	NM_EV_OPSTART_NACK,
	NM_EV_OML_DOWN,
};
extern const struct value_string nm_fsm_event_names[];

/* BTS SiteManager */
enum nm_bts_sm_op_fsm_states {
	NM_BTS_SM_ST_OP_DISABLED_NOTINSTALLED,
	NM_BTS_SM_ST_OP_DISABLED_DEPENDENCY,
	NM_BTS_SM_ST_OP_DISABLED_OFFLINE,
	NM_BTS_SM_ST_OP_ENABLED,
};
extern struct osmo_fsm nm_bts_sm_fsm;

/* BTS */
enum nm_bts_op_fsm_states {
	NM_BTS_ST_OP_DISABLED_NOTINSTALLED,
	NM_BTS_ST_OP_DISABLED_DEPENDENCY,
	NM_BTS_ST_OP_DISABLED_OFFLINE,
	NM_BTS_ST_OP_ENABLED,
};
extern struct osmo_fsm nm_bts_fsm;

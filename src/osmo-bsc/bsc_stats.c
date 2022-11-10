/* osmo-bsc statistics */
/* (C) 2021 by sysmocom - s.f.m.c. GmbH <info@sysmocom.de>
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
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <osmocom/bsc/bsc_stats.h>

#include <osmocom/core/utils.h>
#include <osmocom/core/stats.h>
#include <osmocom/core/stat_item.h>

#include <osmocom/bsc/gsm_data.h>
#include <osmocom/bsc/bts.h>
#include <osmocom/bsc/chan_counts.h>

const struct rate_ctr_desc bsc_ctr_description[] = {
	[BSC_CTR_ASSIGNMENT_ATTEMPTED] =	{"assignment:attempted", "Assignment attempts"},
	[BSC_CTR_ASSIGNMENT_COMPLETED] =	{"assignment:completed", "Assignment completed"},
	[BSC_CTR_ASSIGNMENT_STOPPED] =		{"assignment:stopped", "Connection ended during Assignment"},
	[BSC_CTR_ASSIGNMENT_NO_CHANNEL] =	{"assignment:no_channel", "Failure to allocate lchan for Assignment"},
	[BSC_CTR_ASSIGNMENT_TIMEOUT] =		{"assignment:timeout", "Assignment timed out"},
	[BSC_CTR_ASSIGNMENT_FAILED] =		{"assignment:failed", "Received Assignment Failure message"},
	[BSC_CTR_ASSIGNMENT_ERROR] =		{"assignment:error", "Assignment failed for other reason"},

	[BSC_CTR_HANDOVER_ATTEMPTED] =		{"handover:attempted", "Handover attempts"},
	[BSC_CTR_HANDOVER_COMPLETED] =		{"handover:completed", "Handover completed"},
	[BSC_CTR_HANDOVER_STOPPED] =		{"handover:stopped", "Connection ended during HO"},
	[BSC_CTR_HANDOVER_NO_CHANNEL] =		{"handover:no_channel", "Failure to allocate lchan for HO"},
	[BSC_CTR_HANDOVER_TIMEOUT] =		{"handover:timeout", "Handover timed out"},
	[BSC_CTR_HANDOVER_FAILED] =		{"handover:failed", "Received Handover Fail messages"},
	[BSC_CTR_HANDOVER_ERROR] =		{"handover:error", "Handover failed for other reason"},

	[BSC_CTR_INTRA_CELL_HO_ATTEMPTED] =	{"intra_cell_ho:attempted", "Intra-Cell handover attempts"},
	[BSC_CTR_INTRA_CELL_HO_COMPLETED] =	{"intra_cell_ho:completed", "Intra-Cell handover completed"},
	[BSC_CTR_INTRA_CELL_HO_STOPPED] =	{"intra_cell_ho:stopped", "Connection ended during HO"},
	[BSC_CTR_INTRA_CELL_HO_NO_CHANNEL] =	{"intra_cell_ho:no_channel", "Failure to allocate lchan for HO"},
	[BSC_CTR_INTRA_CELL_HO_TIMEOUT] =	{"intra_cell_ho:timeout", "Handover timed out"},
	[BSC_CTR_INTRA_CELL_HO_FAILED] =	{"intra_cell_ho:failed", "Received Handover Fail messages"},
	[BSC_CTR_INTRA_CELL_HO_ERROR] =	{"intra_cell_ho:error", "Intra-cell handover failed for other reason"},

	[BSC_CTR_INTRA_BSC_HO_ATTEMPTED] =	{"intra_bsc_ho:attempted", "Intra-BSC inter-cell handover attempts"},
	[BSC_CTR_INTRA_BSC_HO_COMPLETED] =	{"intra_bsc_ho:completed", "Intra-BSC inter-cell handover completed"},
	[BSC_CTR_INTRA_BSC_HO_STOPPED] =	{"intra_bsc_ho:stopped", "Connection ended during HO"},
	[BSC_CTR_INTRA_BSC_HO_NO_CHANNEL] =	{"intra_bsc_ho:no_channel", "Failure to allocate lchan for HO"},
	[BSC_CTR_INTRA_BSC_HO_TIMEOUT] =	{"intra_bsc_ho:timeout", "Handover timed out"},
	[BSC_CTR_INTRA_BSC_HO_FAILED] =		{"intra_bsc_ho:failed", "Received Handover Fail messages"},
	[BSC_CTR_INTRA_BSC_HO_ERROR] =		{"intra_bsc_ho:error", "Intra-BSC inter-cell HO failed for other reason"},

	[BSC_CTR_INTER_BSC_HO_OUT_ATTEMPTED] =	{"interbsc_ho_out:attempted",
						 "Attempts to handover to remote BSS"},
	[BSC_CTR_INTER_BSC_HO_OUT_COMPLETED] =	{"interbsc_ho_out:completed",
						 "Handover to remote BSS completed"},
	[BSC_CTR_INTER_BSC_HO_OUT_STOPPED] =	{"interbsc_ho_out:stopped", "Connection ended during HO"},
	[BSC_CTR_INTER_BSC_HO_OUT_TIMEOUT] =	{"interbsc_ho_out:timeout", "Handover timed out"},
	[BSC_CTR_INTER_BSC_HO_OUT_FAILED] =	{"interbsc_ho_out:failed", "Received Handover Fail message"},
	[BSC_CTR_INTER_BSC_HO_OUT_ERROR] =	{"interbsc_ho_out:error",
						 "Handover to remote BSS failed for other reason"},

	[BSC_CTR_INTER_BSC_HO_IN_ATTEMPTED] =	{"interbsc_ho_in:attempted",
						 "Attempts to handover from remote BSS"},
	[BSC_CTR_INTER_BSC_HO_IN_COMPLETED] =	{"interbsc_ho_in:completed",
						 "Handover from remote BSS completed"},
	[BSC_CTR_INTER_BSC_HO_IN_STOPPED] =	{"interbsc_ho_in:stopped", "Connection ended during HO"},
	[BSC_CTR_INTER_BSC_HO_IN_NO_CHANNEL] =	{"interbsc_ho_in:no_channel",
						 "Failure to allocate lchan for HO"},
	[BSC_CTR_INTER_BSC_HO_IN_TIMEOUT] =	{"interbsc_ho_in:timeout", "Handover from remote BSS timed out"},
	[BSC_CTR_INTER_BSC_HO_IN_FAILED] =	{"interbsc_ho_in:failed", "Received Handover Fail message"},
	[BSC_CTR_INTER_BSC_HO_IN_ERROR] =	{"interbsc_ho_in:error",
						 "Handover from remote BSS failed for other reason"},

	[BSC_CTR_SRVCC_ATTEMPTED] =             {"srvcc:attempted", "Intra-BSC SRVCC attempts"},
	[BSC_CTR_SRVCC_COMPLETED] =             {"srvcc:completed", "Intra-BSC SRVCC completed"},
	[BSC_CTR_SRVCC_STOPPED] =               {"srvcc:stopped", "Connection ended during HO"},
	[BSC_CTR_SRVCC_NO_CHANNEL] =            {"srvcc:no_channel", "Failure to allocate lchan for HO"},
	[BSC_CTR_SRVCC_TIMEOUT] =               {"srvcc:timeout", "SRVCC timed out"},
	[BSC_CTR_SRVCC_FAILED] =                {"srvcc:failed", "Received SRVCC Fail messages"},
	[BSC_CTR_SRVCC_ERROR] =                 {"srvcc:error", "Re-assignment failed for other reason"},

	[BSC_CTR_PAGING_ATTEMPTED] =		{"paging:attempted", "Paging attempts for a subscriber"},
	[BSC_CTR_PAGING_DETACHED] =		{"paging:detached", "Paging request send failures because no responsible BTS was found"},
	[BSC_CTR_PAGING_RESPONDED] =		{"paging:responded", "Paging attempts with successful response"},
	[BSC_CTR_PAGING_EXPIRED] =		{"paging:expired", "Paging Request expired because of timeout T3113"},
	[BSC_CTR_PAGING_NO_ACTIVE_PAGING] =	{"paging:no_active_paging", "Paging response without an active paging request (arrived after paging expiration?)"},

	[BSC_CTR_UNKNOWN_UNIT_ID] =		{"abis:unknown_unit_id", "Connection attempts from unknown IPA CCM Unit ID"},

	[BSC_CTR_MSCPOOL_SUBSCR_NO_MSC] =	{"mscpool:subscr:no_msc",
						 "Complete Layer 3 requests lost because no connected MSC is found available"},
	[BSC_CTR_MSCPOOL_EMERG_FORWARDED] =	{"mscpool:emerg:forwarded",
						 "Emergency call requests forwarded to an MSC (see also per-MSC counters"},
	[BSC_CTR_MSCPOOL_EMERG_LOST] =		{"mscpool:emerg:lost",
						 "Emergency call requests lost because no MSC was found available"},
	[BSC_CTR_ALL_ALLOCATED_SDCCH] =		{"all_allocated:sdcch", "Cumulative counter of seconds where all SDCCH channels were allocated"},
	[BSC_CTR_ALL_ALLOCATED_STATIC_SDCCH] =	{"all_allocated:static_sdcch",
						 "Cumulative counter of seconds where all non-dynamic SDCCH channels were allocated"},
	[BSC_CTR_ALL_ALLOCATED_TCH] =		{"all_allocated:tch", "Cumulative counter of seconds where all TCH channels were allocated"},
	[BSC_CTR_ALL_ALLOCATED_STATIC_TCH] =	{"all_allocated:static_tch",
						 "Cumulative counter of seconds where all non-dynamic TCH channels were allocated"},
};

const struct rate_ctr_group_desc bsc_ctrg_desc = {
	"bsc",
	"base station controller",
	OSMO_STATS_CLASS_GLOBAL,
	ARRAY_SIZE(bsc_ctr_description),
	bsc_ctr_description,
};

static const struct osmo_stat_item_desc bsc_stat_desc[] = {
	[BSC_STAT_NUM_BTS_OML_CONNECTED] = { "num_bts:oml_connected", "Number of BTS for this BSC where OML is up", "", 16, 0 },
	[BSC_STAT_NUM_BTS_ALL_TRX_RSL_CONNECTED] = { "num_bts:all_trx_rsl_connected", "Number of BTS for this BSC where RSL is up for all TRX", "", 16, 0 },
	[BSC_STAT_NUM_BTS_TOTAL] = { "num_bts:total", "Number of configured BTS for this BSC", "", 16, 0 },
	[BSC_STAT_NUM_TRX_RSL_CONNECTED] = { "num_trx:rsl_connected", "Number of TRX where RSL is up, total sum across all BTS", "", 16, 0 },
	[BSC_STAT_NUM_TRX_TOTAL] = { "num_trx:total", "Number of configured TRX, total sum across all BTS", "", 1, 0 },
	[BSC_STAT_NUM_MSC_CONNECTED] = { "num_msc:connected", "Number of actively connected MSCs", "", 16, 0 },
	[BSC_STAT_NUM_MSC_TOTAL] = { "num_msc:total", "Number of configured MSCs, not necessarily connected", "", 1, 0 },
};

const struct osmo_stat_item_group_desc bsc_statg_desc = {
	.group_name_prefix = "bsc",
	.group_description = "base station controller",
	.class_id = OSMO_STATS_CLASS_GLOBAL,
	.num_items = ARRAY_SIZE(bsc_stat_desc),
	.item_desc = bsc_stat_desc,
};

/* Count all BTS and TRX OML and RSL stati and update stat items */
void bsc_update_connection_stats(struct gsm_network *net)
{
	struct gsm_bts *bts;
	struct gsm_bts_trx *trx;

	/* Nr of configured BTS and total sum of configured TRX across all BTS */
	int num_bts = 0;
	int num_trx_total = 0;
	/* Nr of BTS where OML is up */
	int bts_oml_connected = 0;
	/* Nr of TRX across all BTS where RSL is up */
	int trx_rsl_connected_total = 0;
	/* Nr of BTS that have all TRX RSL up */
	int bts_rsl_all_trx_connected = 0;

	llist_for_each_entry(bts, &net->bts_list, list) {
		bool oml_connected = false;
		int num_trx = 0;
		int trx_rsl_connected = 0;

		llist_for_each_entry(trx, &bts->trx_list, list) {
			/* If any one trx is usable, it means OML for this BTS is connected */
			if (trx_is_usable(trx))
				oml_connected = true;

			/* Count nr of TRX for this BTS */
			num_trx++;
			if (trx->ts[0].is_rsl_ready)
				trx_rsl_connected++;
		}

		osmo_stat_item_set(osmo_stat_item_group_get_item(bts->bts_statg, BTS_STAT_NUM_TRX_RSL_CONNECTED),
				   trx_rsl_connected);
		osmo_stat_item_set(osmo_stat_item_group_get_item(bts->bts_statg, BTS_STAT_NUM_TRX_TOTAL),
				   num_trx);

		num_trx_total += num_trx;
		trx_rsl_connected_total += trx_rsl_connected;

		num_bts++;
		if (oml_connected)
			bts_oml_connected++;
		if (trx_rsl_connected == num_trx)
			bts_rsl_all_trx_connected++;

		all_allocated_update_bts(bts);
	}

	osmo_stat_item_set(osmo_stat_item_group_get_item(net->bsc_statg, BSC_STAT_NUM_BTS_OML_CONNECTED),
			   bts_oml_connected);
	osmo_stat_item_set(osmo_stat_item_group_get_item(net->bsc_statg, BSC_STAT_NUM_BTS_ALL_TRX_RSL_CONNECTED),
			   bts_rsl_all_trx_connected);
	osmo_stat_item_set(osmo_stat_item_group_get_item(net->bsc_statg, BSC_STAT_NUM_BTS_TOTAL), num_bts);
	osmo_stat_item_set(osmo_stat_item_group_get_item(net->bsc_statg, BSC_STAT_NUM_TRX_RSL_CONNECTED),
			   trx_rsl_connected_total);
	osmo_stat_item_set(osmo_stat_item_group_get_item(net->bsc_statg, BSC_STAT_NUM_TRX_TOTAL), num_trx_total);

	/* This is optional, just running this to catch bugs in chan_counts accounting. If there is a bug, there will be
	 * a DLGLOBAL ERROR logged, and the error gets fixed. */
	chan_counts_bsc_verify();
}

static void all_allocated_update(struct all_allocated *all_allocated, const struct chan_counts *c)
{
	osmo_time_cc_set_flag(&all_allocated->sdcch,
			      c->val[CHAN_COUNTS1_ALL][CHAN_COUNTS2_MAX_TOTAL][GSM_LCHAN_SDCCH]
			      && !c->val[CHAN_COUNTS1_ALL][CHAN_COUNTS2_FREE][GSM_LCHAN_SDCCH]);

	osmo_time_cc_set_flag(&all_allocated->static_sdcch,
			      c->val[CHAN_COUNTS1_STATIC][CHAN_COUNTS2_MAX_TOTAL][GSM_LCHAN_SDCCH]
			      && !c->val[CHAN_COUNTS1_STATIC][CHAN_COUNTS2_FREE][GSM_LCHAN_SDCCH]);

	osmo_time_cc_set_flag(&all_allocated->tch,
			      (c->val[CHAN_COUNTS1_ALL][CHAN_COUNTS2_MAX_TOTAL][GSM_LCHAN_TCH_F]
			       + c->val[CHAN_COUNTS1_ALL][CHAN_COUNTS2_MAX_TOTAL][GSM_LCHAN_TCH_H])
			      && !(c->val[CHAN_COUNTS1_ALL][CHAN_COUNTS2_FREE][GSM_LCHAN_TCH_F]
				   + c->val[CHAN_COUNTS1_ALL][CHAN_COUNTS2_FREE][GSM_LCHAN_TCH_H]));

	osmo_time_cc_set_flag(&all_allocated->static_tch,
			      (c->val[CHAN_COUNTS1_STATIC][CHAN_COUNTS2_MAX_TOTAL][GSM_LCHAN_TCH_F]
			       + c->val[CHAN_COUNTS1_STATIC][CHAN_COUNTS2_MAX_TOTAL][GSM_LCHAN_TCH_H])
			      && !(c->val[CHAN_COUNTS1_STATIC][CHAN_COUNTS2_FREE][GSM_LCHAN_TCH_F]
				   + c->val[CHAN_COUNTS1_STATIC][CHAN_COUNTS2_FREE][GSM_LCHAN_TCH_H]));
}

void all_allocated_update_bts(struct gsm_bts *bts)
{
	all_allocated_update(&bts->all_allocated, &bts->chan_counts);
}

void all_allocated_update_bsc(void)
{
	struct gsm_network *net = bsc_gsmnet;
	all_allocated_update(&net->all_allocated, &net->chan_counts);
}

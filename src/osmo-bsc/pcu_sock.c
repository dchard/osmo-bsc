/* pcu_sock.c: Connect from PCU via unix domain socket */

/* (C) 2008-2010 by Harald Welte <laforge@gnumonks.org>
 * (C) 2009-2012 by Andreas Eversberg <jolly@eversberg.eu>
 * (C) 2012 by Holger Hans Peter Freyther
 * All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <osmocom/core/byteswap.h>
#include <osmocom/core/talloc.h>
#include <osmocom/core/select.h>
#include <osmocom/core/socket.h>
#include <osmocom/core/logging.h>
#include <osmocom/gsm/l1sap.h>
#include <osmocom/gsm/gsm0502.h>
#include <osmocom/bsc/abis_nm.h>

#include <osmocom/bsc/gsm_data.h>
#include <osmocom/bsc/pcu_if.h>
#include <osmocom/bsc/pcuif_proto.h>
#include <osmocom/bsc/signal.h>
#include <osmocom/bsc/debug.h>
#include <osmocom/bsc/abis_rsl.h>
#include <osmocom/bsc/gsm_04_08_rr.h>
#include <osmocom/bsc/bts.h>
#include <osmocom/bsc/bts_sm.h>

static int pcu_sock_send(struct gsm_bts *bts, struct msgb *msg);

static const char *sapi_string[] = {
	[PCU_IF_SAPI_RACH] =	"RACH",
	[PCU_IF_SAPI_AGCH] =	"AGCH",
	[PCU_IF_SAPI_PCH] =	"PCH",
	[PCU_IF_SAPI_BCCH] =	"BCCH",
	[PCU_IF_SAPI_PDTCH] =	"PDTCH",
	[PCU_IF_SAPI_PRACH] =	"PRACH",
	[PCU_IF_SAPI_PTCCH] = 	"PTCCH",
	[PCU_IF_SAPI_AGCH_DT] = 	"AGCH_DT",
};

/* Check if BTS has a PCU connection */
static bool pcu_connected(struct gsm_bts *bts)
{
	struct pcu_sock_state *state = bts->pcu_state;

	if (!state)
		return false;
	if (state->conn_bfd.fd <= 0)
		return false;
	return true;
}

/*
 * PCU messages
 */

/* Set up an message buffer to package an pcu interface message */
struct msgb *pcu_msgb_alloc(uint8_t msg_type, uint8_t bts_nr)
{
	struct msgb *msg;
	struct gsm_pcu_if *pcu_prim;

	msg = msgb_alloc(sizeof(struct gsm_pcu_if), "pcu_sock_tx");
	if (!msg)
		return NULL;

	msgb_put(msg, sizeof(struct gsm_pcu_if));
	pcu_prim = (struct gsm_pcu_if *) msg->data;
	pcu_prim->msg_type = msg_type;
	pcu_prim->bts_nr = bts_nr;

	return msg;
}

/* Check if the timeslot can be utilized as PDCH now
 * (PDCH is currently active on BTS) */
static bool ts_now_usable_as_pdch(const struct gsm_bts_trx_ts *ts)
{
	switch (ts->pchan_is) {
	case GSM_PCHAN_PDCH:
		/* NOTE: We currently only support Ericsson RBS as a BSC
		 * co-located BTS. This BTS only supports dynamic channels. */
		return true;
	default:
		return false;
	}
}

/* Fill the frequency hopping parameter */
static void info_ind_fill_fhp(struct gsm_pcu_if_info_trx_ts *ts_info,
			      const struct gsm_bts_trx_ts *ts)
{
	ts_info->maio = ts->hopping.maio;
	ts_info->hsn = ts->hopping.hsn;
	ts_info->hopping = 0x1;

	memcpy(&ts_info->ma, ts->hopping.ma_data, ts->hopping.ma_len);
	ts_info->ma_bit_len = ts->hopping.ma_len * 8 - ts->hopping.ma.cur_bit;
}

/* Fill the TRX parameter */
static void info_ind_fill_trx(struct gsm_pcu_if_info_trx *trx_info, const struct gsm_bts_trx *trx)
{
	unsigned int tn;
	const struct gsm_bts_trx_ts *ts;

	trx_info->hlayer1 = 0x2342;
	trx_info->pdch_mask = 0;
	trx_info->arfcn = trx->arfcn;

	if (trx->mo.nm_state.operational != NM_OPSTATE_ENABLED ||
	    trx->mo.nm_state.administrative != NM_STATE_UNLOCKED) {
		LOG_TRX(trx, DPCU, LOGL_INFO, "unavailable for PCU (op=%s adm=%s)\n",
			abis_nm_opstate_name(trx->mo.nm_state.operational),
			abis_nm_admin_name(trx->mo.nm_state.administrative));
		return;
	}

	for (tn = 0; tn < ARRAY_SIZE(trx->ts); tn++) {
		ts = &trx->ts[tn];
		if (ts->mo.nm_state.operational != NM_OPSTATE_ENABLED)
			continue;
		if (!ts_now_usable_as_pdch(ts))
			continue;

		trx_info->pdch_mask |= (1 << tn);
		trx_info->ts[tn].tsc =
				(ts->tsc >= 0) ? ts->tsc : trx->bts->bsic & 7;

		if (ts->hopping.enabled)
			info_ind_fill_fhp(&trx_info->ts[tn], ts);

		LOG_TRX(trx, DPCU, LOGL_INFO, "PDCH on ts=%u is available (tsc=%u ", ts->nr,
			trx_info->ts[tn].tsc);
		if (ts->hopping.enabled)
			LOGPC(DPCU, LOGL_INFO, "hopping=yes hsn=%u maio=%u ma_bit_len=%u)\n",
			      ts->hopping.hsn, ts->hopping.maio, trx_info->ts[tn].ma_bit_len);
		else
			LOGPC(DPCU, LOGL_INFO, "hopping=no arfcn=%u)\n", trx->arfcn);
	}
}

/* Send BTS properties to the PCU */
static int pcu_tx_info_ind(struct gsm_bts *bts)
{
	struct msgb *msg;
	struct gsm_pcu_if *pcu_prim;
	struct gsm_pcu_if_info_ind *info_ind;
	struct gprs_rlc_cfg *rlcc;
	struct gsm_bts_sm *bts_sm;
	struct gsm_gprs_nsvc *nsvc;
	struct gsm_bts_trx *trx;
	int i;

	OSMO_ASSERT(bts);
	OSMO_ASSERT(bts->network);
	OSMO_ASSERT(bts->site_mgr);

	bts_sm = bts->site_mgr;

	LOGP(DPCU, LOGL_INFO, "Sending info for BTS %d\n", bts->nr);

	rlcc = &bts->gprs.cell.rlc_cfg;

	msg = pcu_msgb_alloc(PCU_IF_MSG_INFO_IND, bts->nr);
	if (!msg)
		return -ENOMEM;

	pcu_prim = (struct gsm_pcu_if *) msg->data;
	info_ind = &pcu_prim->u.info_ind;
	info_ind->version = PCU_IF_VERSION;
	info_ind->flags |= PCU_IF_FLAG_ACTIVE;
	info_ind->flags |= PCU_IF_FLAG_SYSMO;

	/* RAI */
	info_ind->mcc = bts->network->plmn.mcc;
	info_ind->mnc = bts->network->plmn.mnc;
	info_ind->mnc_3_digits = bts->network->plmn.mnc_3_digits;
	info_ind->lac = bts->location_area_code;
	info_ind->rac = bts->gprs.rac;

	/* NSE */
	info_ind->nsei = bts_sm->gprs.nse.nsei;
	memcpy(info_ind->nse_timer, bts_sm->gprs.nse.timer, 7);
	memcpy(info_ind->cell_timer, bts->gprs.cell.timer, 11);

	/* cell attributes */
	info_ind->cell_id = bts->cell_identity;
	info_ind->repeat_time = rlcc->paging.repeat_time;
	info_ind->repeat_count = rlcc->paging.repeat_count;
	info_ind->bvci = bts->gprs.cell.bvci;
	info_ind->t3142 = rlcc->parameter[RLC_T3142];
	info_ind->t3169 = rlcc->parameter[RLC_T3169];
	info_ind->t3191 = rlcc->parameter[RLC_T3191];
	info_ind->t3193_10ms = rlcc->parameter[RLC_T3193];
	info_ind->t3195 = rlcc->parameter[RLC_T3195];
	info_ind->n3101 = rlcc->parameter[RLC_N3101];
	info_ind->n3103 = rlcc->parameter[RLC_N3103];
	info_ind->n3105 = rlcc->parameter[RLC_N3105];
	info_ind->cv_countdown = rlcc->parameter[CV_COUNTDOWN];
	if (rlcc->cs_mask & (1 << GPRS_CS1))
		info_ind->flags |= PCU_IF_FLAG_CS1;
	if (rlcc->cs_mask & (1 << GPRS_CS2))
		info_ind->flags |= PCU_IF_FLAG_CS2;
	if (rlcc->cs_mask & (1 << GPRS_CS3))
		info_ind->flags |= PCU_IF_FLAG_CS3;
	if (rlcc->cs_mask & (1 << GPRS_CS4))
		info_ind->flags |= PCU_IF_FLAG_CS4;
	if (bts->gprs.mode == BTS_GPRS_EGPRS) {
		if (rlcc->cs_mask & (1 << GPRS_MCS1))
			info_ind->flags |= PCU_IF_FLAG_MCS1;
		if (rlcc->cs_mask & (1 << GPRS_MCS2))
			info_ind->flags |= PCU_IF_FLAG_MCS2;
		if (rlcc->cs_mask & (1 << GPRS_MCS3))
			info_ind->flags |= PCU_IF_FLAG_MCS3;
		if (rlcc->cs_mask & (1 << GPRS_MCS4))
			info_ind->flags |= PCU_IF_FLAG_MCS4;
		if (rlcc->cs_mask & (1 << GPRS_MCS5))
			info_ind->flags |= PCU_IF_FLAG_MCS5;
		if (rlcc->cs_mask & (1 << GPRS_MCS6))
			info_ind->flags |= PCU_IF_FLAG_MCS6;
		if (rlcc->cs_mask & (1 << GPRS_MCS7))
			info_ind->flags |= PCU_IF_FLAG_MCS7;
		if (rlcc->cs_mask & (1 << GPRS_MCS8))
			info_ind->flags |= PCU_IF_FLAG_MCS8;
		if (rlcc->cs_mask & (1 << GPRS_MCS9))
			info_ind->flags |= PCU_IF_FLAG_MCS9;
	}
	/* TODO: isn't dl_tbf_ext wrong?: * 10 and no ntohs */
	info_ind->dl_tbf_ext = rlcc->parameter[T_DL_TBF_EXT];
	/* TODO: isn't ul_tbf_ext wrong?: * 10 and no ntohs */
	info_ind->ul_tbf_ext = rlcc->parameter[T_UL_TBF_EXT];
	info_ind->initial_cs = rlcc->initial_cs;
	info_ind->initial_mcs = rlcc->initial_mcs;

	/* NSVC */
	for (i = 0; i < ARRAY_SIZE(info_ind->nsvci); i++) {
		nsvc = &bts->site_mgr->gprs.nsvc[i];

		info_ind->nsvci[i] = nsvc->nsvci;
		info_ind->local_port[i] = nsvc->local_port;
		switch (nsvc->remote.u.sas.ss_family) {
		case AF_INET:
			info_ind->address_type[i] = PCU_IF_ADDR_TYPE_IPV4;
			info_ind->remote_ip[i].v4 = nsvc->remote.u.sin.sin_addr;
			info_ind->remote_port[i] = nsvc->remote.u.sin.sin_port;
			break;
		case AF_INET6:
			info_ind->address_type[i] = PCU_IF_ADDR_TYPE_IPV6;
			memcpy(&info_ind->remote_ip[i].v6,
			       &nsvc->remote.u.sin6.sin6_addr,
			       sizeof(struct in6_addr));
			info_ind->remote_port[i] = nsvc->remote.u.sin6.sin6_port;
			break;
		default:
			info_ind->address_type[i] = PCU_IF_ADDR_TYPE_UNSPEC;
			break;
		}
	}

	for (i = 0; i < ARRAY_SIZE(info_ind->trx); i++) {
		trx = gsm_bts_trx_num(bts, i);
		if (!trx)
			continue;
		if (trx->nr >= ARRAY_SIZE(info_ind->trx)) {
			LOG_TRX(trx, DPCU, LOGL_NOTICE, "PCU interface (version %u) "
				"cannot handle more than %zu transceivers => skipped\n",
				PCU_IF_VERSION, ARRAY_SIZE(info_ind->trx));
			break;
		}
		info_ind_fill_trx(&info_ind->trx[trx->nr], trx);
	}

	return pcu_sock_send(bts, msg);
}

/* Allow test to overwrite it */
__attribute__((weak)) void pcu_info_update(struct gsm_bts *bts)
{
	if (pcu_connected(bts))
		pcu_tx_info_ind(bts);
}

/* Forward rach indication to PCU */
int pcu_tx_rach_ind(struct gsm_bts *bts, int16_t qta, uint16_t ra, uint32_t fn,
	uint8_t is_11bit, enum ph_burst_type burst_type)
{
	struct msgb *msg;
	struct gsm_pcu_if *pcu_prim;
	struct gsm_pcu_if_rach_ind *rach_ind;

	/* Bail if no PCU is connected */
	if (!pcu_connected(bts)) {
		LOGP(DRSL, LOGL_ERROR, "BTS %d CHAN RQD(GPRS) but PCU not "
			"connected!\n", bts->nr);
		return -ENODEV;
	}

	LOGP(DPCU, LOGL_INFO, "Sending RACH indication: qta=%d, ra=%d, "
		"fn=%d\n", qta, ra, fn);

	msg = pcu_msgb_alloc(PCU_IF_MSG_RACH_IND, bts->nr);
	if (!msg)
		return -ENOMEM;
	pcu_prim = (struct gsm_pcu_if *) msg->data;
	rach_ind = &pcu_prim->u.rach_ind;

	rach_ind->sapi = PCU_IF_SAPI_RACH;
	rach_ind->ra = ra;
	rach_ind->qta = qta;
	rach_ind->fn = fn;
	rach_ind->is_11bit = is_11bit;
	rach_ind->burst_type = burst_type;

	return pcu_sock_send(bts, msg);
}

/* Confirm the sending of an immediate assignment to the pcu */
int pcu_tx_imm_ass_sent(struct gsm_bts *bts, uint32_t tlli)
{
	struct msgb *msg;
	struct gsm_pcu_if *pcu_prim;
	struct gsm_pcu_if_data_cnf_dt *data_cnf_dt;

	LOGP(DPCU, LOGL_INFO, "Sending PCH confirm with direct TLLI\n");

	msg = pcu_msgb_alloc(PCU_IF_MSG_DATA_CNF_DT, bts->nr);
	if (!msg)
		return -ENOMEM;
	pcu_prim = (struct gsm_pcu_if *) msg->data;
	data_cnf_dt = &pcu_prim->u.data_cnf_dt;

	data_cnf_dt->sapi = PCU_IF_SAPI_PCH;
	data_cnf_dt->tlli = tlli;

	return pcu_sock_send(bts, msg);
}

/* we need to decode the raw RR paging message (see PCU code
 * Encoding::write_paging_request) and extract the mobile identity
 * (P-TMSI) from it */
static int pcu_rx_rr_paging(struct gsm_bts *bts, uint8_t paging_group,
			    const uint8_t *raw_rr_msg)
{
	struct gsm48_paging1 *p1 = (struct gsm48_paging1 *) raw_rr_msg;
	uint8_t chan_needed;
	struct osmo_mobile_identity mi;
	int rc;

	switch (p1->msg_type) {
	case GSM48_MT_RR_PAG_REQ_1:
		chan_needed = (p1->cneed2 << 2) | p1->cneed1;
		rc = osmo_mobile_identity_decode(&mi, p1->data+1, p1->data[0], false);
		if (rc) {
			LOGP(DPCU, LOGL_ERROR, "PCU Sends paging "
			     "request type %02x (chan_needed=%02x): Unable to decode Mobile Identity\n",
			     p1->msg_type, chan_needed);
			rc = -EINVAL;
			break;
		}
		LOGP(DPCU, LOGL_ERROR, "PCU Sends paging "
		     "request type %02x (chan_needed=%02x, mi=%s)\n",
		     p1->msg_type, chan_needed, osmo_mobile_identity_to_str_c(OTC_SELECT, &mi));
		/* NOTE: We will have to add 2 to mi_len and subtract 2 from
		 * the mi pointer because rsl_paging_cmd() will perform the
		 * reverse operations. This is because rsl_paging_cmd() is
		 * normally expected to chop off the element identifier (0xC0)
		 * and the length field. In our parameter, we do not have
		 * those fields included. */
		rc = rsl_paging_cmd(bts, paging_group, &mi, chan_needed, true);
		break;
	case GSM48_MT_RR_PAG_REQ_2:
	case GSM48_MT_RR_PAG_REQ_3:
		LOGP(DPCU, LOGL_ERROR, "PCU Sends unsupported paging "
			"request type %02x\n", p1->msg_type);
		rc = -EINVAL;
		break;
	default:
		LOGP(DPCU, LOGL_ERROR, "PCU Sends unknown paging "
			"request type %02x\n", p1->msg_type);
		rc = -EINVAL;
		break;
	}

	return rc;
}

static int pcu_rx_data_req(struct gsm_bts *bts, uint8_t msg_type,
	struct gsm_pcu_if_data *data_req)
{
	struct msgb *msg;
	char imsi_digit_buf[4];
	uint32_t tlli = -1;
	uint8_t pag_grp;
	int rc = 0;

	LOGP(DPCU, LOGL_DEBUG, "Data request received: sapi=%s arfcn=%d "
		"block=%d data=%s\n", sapi_string[data_req->sapi],
		data_req->arfcn, data_req->block_nr,
		osmo_hexdump(data_req->data, data_req->len));

	switch (data_req->sapi) {
	case PCU_IF_SAPI_PCH:
		/* the first three bytes are the last three digits of
		 * the IMSI, which we need to compute the paging group */
		imsi_digit_buf[0] = data_req->data[0];
		imsi_digit_buf[1] = data_req->data[1];
		imsi_digit_buf[2] = data_req->data[2];
		imsi_digit_buf[3] = '\0';
		LOGP(DPCU, LOGL_DEBUG, "SAPI PCH imsi %s\n", imsi_digit_buf);
		pag_grp = gsm0502_calc_paging_group(&bts->si_common.chan_desc,
						str_to_imsi(imsi_digit_buf));
		pcu_rx_rr_paging(bts, pag_grp, data_req->data+3);
		break;
	case PCU_IF_SAPI_AGCH:
		msg = msgb_alloc(data_req->len, "pcu_agch");
		if (!msg) {
			rc = -ENOMEM;
			break;
		}
		msg->l3h = msgb_put(msg, data_req->len);
		memcpy(msg->l3h, data_req->data, data_req->len);

		if (rsl_imm_assign_cmd(bts, msg->len, msg->data)) {
			msgb_free(msg);
			rc = -EIO;
		}
		break;
	case PCU_IF_SAPI_AGCH_DT:
		/* DT = direct tlli. A tlli is prefixed */

		if (data_req->len < 5) {
			LOGP(DPCU, LOGL_ERROR, "Received PCU data request with "
					"invalid/small length %d\n", data_req->len);
			break;
		}
		memcpy(&tlli, data_req->data, 4);

		msg = msgb_alloc(data_req->len - 4, "pcu_agch");
		if (!msg) {
			rc = -ENOMEM;
			break;
		}
		msg->l3h = msgb_put(msg, data_req->len - 4);
		memcpy(msg->l3h, data_req->data + 4, data_req->len - 4);

		if (bts->type == GSM_BTS_TYPE_RBS2000)
			rc = rsl_ericsson_imm_assign_cmd(bts, tlli, msg->len, msg->data);
		else
			rc = rsl_imm_assign_cmd(bts, msg->len, msg->data);

		if (rc) {
			msgb_free(msg);
			rc = -EIO;
		}
		break;
	default:
		LOGP(DPCU, LOGL_ERROR, "Received PCU data request with "
			"unsupported sapi %d\n", data_req->sapi);
		rc = -EINVAL;
	}

	return rc;
}

#define CHECK_IF_MSG_SIZE(prim_len, prim_msg) \
	do { \
		size_t _len = PCUIF_HDR_SIZE + sizeof(prim_msg); \
		if (prim_len < _len) { \
			LOGP(DPCU, LOGL_ERROR, "Received %zu bytes on PCU Socket, but primitive %s " \
			     "size is %zu, discarding\n", prim_len, #prim_msg, _len); \
			return -EINVAL; \
		} \
	} while (0)
static int pcu_rx(struct gsm_network *net, uint8_t msg_type,
	struct gsm_pcu_if *pcu_prim, size_t prim_len)
{
	int rc = 0;
	struct gsm_bts *bts;

	/* FIXME: allow multiple BTS */
	bts = llist_entry(net->bts_list.next, struct gsm_bts, list);

	switch (msg_type) {
	case PCU_IF_MSG_DATA_REQ:
	case PCU_IF_MSG_PAG_REQ:
		CHECK_IF_MSG_SIZE(prim_len, pcu_prim->u.data_req);
		rc = pcu_rx_data_req(bts, msg_type, &pcu_prim->u.data_req);
		break;
	default:
		LOGP(DPCU, LOGL_ERROR, "Received unknown PCU msg type %d\n",
			msg_type);
		rc = -EINVAL;
	}

	return rc;
}

/*
 * PCU socket interface
 */

static int pcu_sock_send(struct gsm_bts *bts, struct msgb *msg)
{
	struct pcu_sock_state *state = bts->pcu_state;
	struct osmo_fd *conn_bfd;
	struct gsm_pcu_if *pcu_prim = (struct gsm_pcu_if *) msg->data;

	if (!state) {
		if (pcu_prim->msg_type != PCU_IF_MSG_TIME_IND)
			LOGP(DPCU, LOGL_INFO, "PCU socket not created, "
				"dropping message\n");
		msgb_free(msg);
		return -EINVAL;
	}
	conn_bfd = &state->conn_bfd;
	if (conn_bfd->fd <= 0) {
		if (pcu_prim->msg_type != PCU_IF_MSG_TIME_IND)
			LOGP(DPCU, LOGL_NOTICE, "PCU socket not connected, "
				"dropping message\n");
		msgb_free(msg);
		return -EIO;
	}
	msgb_enqueue(&state->upqueue, msg);
	osmo_fd_write_enable(conn_bfd);

	return 0;
}

static void pcu_sock_close(struct pcu_sock_state *state)
{
	struct osmo_fd *bfd = &state->conn_bfd;
	struct gsm_bts *bts;
	struct gsm_bts_trx *trx;
	struct gsm_bts_trx_ts *ts;
	int i, j;

	/* FIXME: allow multiple BTS */
	bts = llist_entry(state->net->bts_list.next, struct gsm_bts, list);

	LOGP(DPCU, LOGL_NOTICE, "PCU socket has LOST connection\n");

	close(bfd->fd);
	bfd->fd = -1;
	osmo_fd_unregister(bfd);

	/* re-enable the generation of ACCEPT for new connections */
	osmo_fd_read_enable(&state->listen_bfd);

#if 0
	/* remove si13, ... */
	bts->si_valid &= ~(1 << SYSINFO_TYPE_13);
	osmo_signal_dispatch(SS_GLOBAL, S_NEW_SYSINFO, bts);
#endif

	/* release PDCH */
	for (i = 0; i < 8; i++) {
		trx = gsm_bts_trx_num(bts, i);
		if (!trx)
			break;
		for (j = 0; j < 8; j++) {
			ts = &trx->ts[j];
			if (ts->mo.nm_state.operational == NM_OPSTATE_ENABLED
			    && ts->pchan_is == GSM_PCHAN_PDCH) {
				printf("l1sap_chan_rel(trx,gsm_lchan2chan_nr(ts->lchan));\n");
			}
		}
	}

	/* flush the queue */
	while (!llist_empty(&state->upqueue)) {
		struct msgb *msg = msgb_dequeue(&state->upqueue);
		msgb_free(msg);
	}
}

static int pcu_sock_read(struct osmo_fd *bfd)
{
	struct pcu_sock_state *state = (struct pcu_sock_state *)bfd->data;
	struct gsm_pcu_if *pcu_prim;
	struct msgb *msg;
	int rc;

	msg = msgb_alloc(sizeof(*pcu_prim) + 1000, "pcu_sock_rx");
	if (!msg)
		return -ENOMEM;

	pcu_prim = (struct gsm_pcu_if *) msg->tail;

	rc = recv(bfd->fd, msg->tail, msgb_tailroom(msg), 0);
	if (rc == 0)
		goto close;

	if (rc < 0) {
		if (errno == EAGAIN) {
			msgb_free(msg);
			return 0;
		}
		goto close;
	}

	if (rc < PCUIF_HDR_SIZE) {
		LOGP(DPCU, LOGL_ERROR, "Received %d bytes on PCU Socket, but primitive hdr size "
		     "is %zu, discarding\n", rc, PCUIF_HDR_SIZE);
		msgb_free(msg);
		return 0;
	}

	rc = pcu_rx(state->net, pcu_prim->msg_type, pcu_prim, rc);

	/* as we always synchronously process the message in pcu_rx() and
	 * its callbacks, we can free the message here. */
	msgb_free(msg);

	return rc;

close:
	msgb_free(msg);
	pcu_sock_close(state);
	return -1;
}

static int pcu_sock_write(struct osmo_fd *bfd)
{
	struct pcu_sock_state *state = bfd->data;
	int rc;

	while (!llist_empty(&state->upqueue)) {
		struct msgb *msg, *msg2;
		struct gsm_pcu_if *pcu_prim;

		/* peek at the beginning of the queue */
		msg = llist_entry(state->upqueue.next, struct msgb, list);
		pcu_prim = (struct gsm_pcu_if *)msg->data;

		osmo_fd_write_disable(bfd);

		/* bug hunter 8-): maybe someone forgot msgb_put(...) ? */
		if (!msgb_length(msg)) {
			LOGP(DPCU, LOGL_ERROR, "message type (%d) with ZERO "
				"bytes!\n", pcu_prim->msg_type);
			goto dontsend;
		}

		/* try to send it over the socket */
		rc = write(bfd->fd, msgb_data(msg), msgb_length(msg));
		if (rc == 0)
			goto close;
		if (rc < 0) {
			if (errno == EAGAIN) {
				osmo_fd_write_enable(bfd);
				break;
			}
			goto close;
		}

dontsend:
		/* _after_ we send it, we can deueue */
		msg2 = msgb_dequeue(&state->upqueue);
		assert(msg == msg2);
		msgb_free(msg);
	}
	return 0;

close:
	pcu_sock_close(state);

	return -1;
}

static int pcu_sock_cb(struct osmo_fd *bfd, unsigned int flags)
{
	int rc = 0;

	if (flags & OSMO_FD_READ)
		rc = pcu_sock_read(bfd);
	if (rc < 0)
		return rc;

	if (flags & OSMO_FD_WRITE)
		rc = pcu_sock_write(bfd);

	return rc;
}

/* accept connection coming from PCU */
static int pcu_sock_accept(struct osmo_fd *bfd, unsigned int flags)
{
	struct pcu_sock_state *state = (struct pcu_sock_state *)bfd->data;
	struct osmo_fd *conn_bfd = &state->conn_bfd;
	struct sockaddr_un un_addr;
	socklen_t len;
	int rc;

	len = sizeof(un_addr);
	rc = accept(bfd->fd, (struct sockaddr *) &un_addr, &len);
	if (rc < 0) {
		LOGP(DPCU, LOGL_ERROR, "Failed to accept a new connection\n");
		return -1;
	}

	if (conn_bfd->fd >= 0) {
		LOGP(DPCU, LOGL_NOTICE, "PCU connects but we already have "
			"another active connection ?!?\n");
		/* We already have one PCU connected, this is all we support */
		osmo_fd_read_disable(&state->listen_bfd);
		close(rc);
		return 0;
	}

	osmo_fd_setup(conn_bfd, rc, OSMO_FD_READ, pcu_sock_cb, state, 0);

	if (osmo_fd_register(conn_bfd) != 0) {
		LOGP(DPCU, LOGL_ERROR, "Failed to register new connection "
			"fd\n");
		close(conn_bfd->fd);
		conn_bfd->fd = -1;
		return -1;
	}

	LOGP(DPCU, LOGL_NOTICE, "PCU socket connected to external PCU\n");

	return 0;
}

/* Open connection to PCU */
int pcu_sock_init(const char *path, struct gsm_bts *bts)
{
	struct pcu_sock_state *state;
	struct osmo_fd *bfd;
	int rc;

	state = talloc_zero(NULL, struct pcu_sock_state);
	if (!state)
		return -ENOMEM;

	INIT_LLIST_HEAD(&state->upqueue);
	state->net = bts->network;
	state->conn_bfd.fd = -1;

	bfd = &state->listen_bfd;

	rc = osmo_sock_unix_init(SOCK_SEQPACKET, 0, path, OSMO_SOCK_F_BIND);
	if (rc < 0) {
		LOGP(DPCU, LOGL_ERROR, "Could not create unix socket: %s\n",
			strerror(errno));
		talloc_free(state);
		return -1;
	}

	osmo_fd_setup(bfd, rc, OSMO_FD_READ, pcu_sock_accept, state, 0);

	rc = osmo_fd_register(bfd);
	if (rc < 0) {
		LOGP(DPCU, LOGL_ERROR, "Could not register listen fd: %d\n",
			rc);
		close(bfd->fd);
		talloc_free(state);
		return rc;
	}

	LOGP(DPCU, LOGL_INFO, "Started listening on PCU socket: %s\n", path);

	bts->pcu_state = state;
	return 0;
}

/* Close connection to PCU */
void pcu_sock_exit(struct gsm_bts *bts)
{
	struct pcu_sock_state *state = bts->pcu_state;
	struct osmo_fd *bfd, *conn_bfd;

	if (!state)
		return;

	conn_bfd = &state->conn_bfd;
	if (conn_bfd->fd > 0)
		pcu_sock_close(state);
	bfd = &state->listen_bfd;
	close(bfd->fd);
	osmo_fd_unregister(bfd);
	talloc_free(state);
	bts->pcu_state = NULL;
}

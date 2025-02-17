/* Nokia XXXsite family specific code */

/* (C) 2011 by Dieter Spaar <spaar@mirider.augusta.de>
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
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

/*
  TODO: Attention: There are some static variables used for states during
  configuration. Those variables have to be moved to a BTS specific context,
  otherwise there will most certainly be problems if more than one Nokia BTS
  is used.
*/

#include <time.h>

#include <osmocom/gsm/tlv.h>

#include <osmocom/bsc/debug.h>
#include <osmocom/bsc/gsm_data.h>
#include <osmocom/bsc/abis_nm.h>
#include <osmocom/abis/e1_input.h>
#include <osmocom/bsc/signal.h>
#include <osmocom/bsc/timeslot_fsm.h>
#include <osmocom/bsc/system_information.h>
#include <osmocom/bsc/bts.h>

#include <osmocom/core/timer.h>

#include <osmocom/abis/lapd.h>
#include <osmocom/bsc/bts_nokia_site.h>

static void bootstrap_om_bts(struct gsm_bts *bts)
{
	LOG_BTS(bts, DNM, LOGL_NOTICE, "bootstrapping OML for BTS %u\n", bts->nr);

	if (!bts->nokia.skip_reset) {
		if (!bts->nokia.did_reset)
			abis_nm_reset(bts, 1);
	} else
		bts->nokia.did_reset = 1;

	gsm_bts_all_ts_dispatch(bts, TS_EV_OML_READY, NULL);
}

static void bootstrap_om_trx(struct gsm_bts_trx *trx)
{
	LOG_TRX(trx, DNM, LOGL_NOTICE, "bootstrapping OML for TRX %u/%u\n",
			trx->bts->nr, trx->nr);
	gsm_trx_all_ts_dispatch(trx, TS_EV_OML_READY, NULL);
}

static int shutdown_om(struct gsm_bts *bts)
{
		gsm_bts_all_ts_dispatch(bts, TS_EV_OML_DOWN, NULL);
        gsm_bts_stats_reset(bts);
        /* TODO !? */
        return 0;
}

static int shutdown_om_trx(struct gsm_bts *bts)
{
        struct gsm_bts_trx *trx;
        uint16_t ref = 91;
        llist_for_each_entry(trx, &bts->trx_list, list) {
                /* lock all TRXs */
                abis_nm_cha_adm_trx_lock(bts, trx->nr + 1, ref);
					ref+=2;
	}
        /* TODO !? */
        return 0;
}

/*

  Tell LAPD to start start the SAP (send SABM requests) for all signalling
  timeslots in this line

  Attention: this has to be adapted for mISDN
*/

static void start_sabm_in_line(struct e1inp_line *line, int start, int sapi)
{
	struct e1inp_sign_link *link;
	int i;

	for (i = 0; i < ARRAY_SIZE(line->ts); i++) {
		struct e1inp_ts *ts = &line->ts[i];

		if (ts->type != E1INP_TS_TYPE_SIGN)
			continue;

		llist_for_each_entry(link, &ts->sign.sign_links, list) {
			if (sapi != -1 && link->sapi != sapi)
				continue;

#if 0				/* debugging */
			printf("sap start/stop (%d): %d tei=%d sapi=%d\n",
			       start, i + 1, link->tei, link->sapi);
#endif

			if (start) {
				ts->lapd->profile.t200_sec = 1;
				ts->lapd->profile.t200_usec = 0;
				lapd_sap_start(ts->lapd, link->tei,
					       link->sapi);
			} else
				lapd_sap_stop(ts->lapd, link->tei,
					      link->sapi);
		}
	}
}

/* Callback function to be called every time we receive a signal from INPUT */
static int gbl_sig_cb(unsigned int subsys, unsigned int signal,
		      void *handler_data, void *signal_data)
{
	struct gsm_bts *bts;

	if (subsys != SS_L_GLOBAL)
		return 0;

	switch (signal) {
	case S_GLOBAL_BTS_CLOSE_OM_TRX:
		bts = signal_data;
		if (bts->type == GSM_BTS_TYPE_NOKIA_SITE) {
			shutdown_om_trx(signal_data);
		}
		break;
	case S_GLOBAL_BTS_CLOSE_OM:
		bts = signal_data;
	if (bts->type == GSM_BTS_TYPE_NOKIA_SITE) {
			shutdown_om(signal_data);
		}
		break;
	}

	return 0;
}

/* Callback function to be called every time we receive a signal from INPUT */
static int inp_sig_cb(unsigned int subsys, unsigned int signal,
		      void *handler_data, void *signal_data)
{
	struct input_signal_data *isd = signal_data;

	if (subsys != SS_L_INPUT)
		return 0;

	switch (signal) {
	case S_L_INP_LINE_INIT:
		start_sabm_in_line(isd->line, 1, SAPI_OML);	/* start only OML */
		break;
	case S_L_INP_TEI_DN:
		break;
	case S_L_INP_TEI_UP:
		switch (isd->link_type) {
		case E1INP_SIGN_OML:
			if (isd->trx->bts->type != GSM_BTS_TYPE_NOKIA_SITE)
				break;

			if (isd->tei == isd->trx->bts->oml_tei)
				bootstrap_om_bts(isd->trx->bts);
			else
				bootstrap_om_trx(isd->trx);
			break;
		default:
			break;
		}
		break;
	case S_L_INP_TEI_UNKNOWN:
		/* We are receiving LAPD frames with one TEI that we do not
		 * seem to know, likely that we (the BSC) stopped working
		 * and lost our local states. However, the BTS is already
		 * configured, we try to take over the RSL links. */
		start_sabm_in_line(isd->line, 1, SAPI_RSL);
		break;
	}

	return 0;
}

static void nm_statechg_evt(unsigned int signal,
			    struct nm_statechg_signal_data *nsd)
{
	if (nsd->bts->type != GSM_BTS_TYPE_NOKIA_SITE)
		return;
}

static int nm_sig_cb(unsigned int subsys, unsigned int signal,
		     void *handler_data, void *signal_data)
{
	if (subsys != SS_NM)
		return 0;

	switch (signal) {
	case S_NM_STATECHG:
		nm_statechg_evt(signal, signal_data);
		break;
	default:
		break;
	}

	return 0;
}

static const char *get_msg_type_name_string(uint8_t msg_type)
{
	return get_value_string(nokia_msgt_name, msg_type);
}

static const char *get_element_name_string(uint16_t element)
{
	return get_value_string(nokia_element_name, element);
}

static const char *get_bts_type_string(uint8_t type)
{
	return get_value_string(nokia_bts_types, type);
}

static const char *get_severity_string(uint8_t severity)
{
	return get_value_string(nokia_severity, severity);
}

static const char *get_reset_type_string(uint8_t reset_type)
{
	return get_value_string(nokia_reset_type, reset_type);
}

static const char *get_object_identity_string(uint16_t object_identity)
{
	return get_value_string(nokia_object_identity, object_identity);
}

static const char *get_object_state_string(uint8_t object_state)
{
	return get_value_string(nokia_object_state, object_state);
}


/* TODO: put in a separate file ? */

/*
  build the configuration for each TRX
*/

static int make_fu_config(struct gsm_bts_trx *trx, uint8_t id,
			  uint8_t * fu_config, int *hopping)
{
	int i;

	*hopping = 0;

	memcpy(fu_config, fu_config_template, sizeof(fu_config_template));

	/* set ID */

	fu_config[6 + 2] = id;
	fu_config[66 + 2] = id;
	fu_config[86 + 2] = id;
	fu_config[100 + 2] = id;
	fu_config[113 + 2] = id;
	fu_config[127 + 2] = id;

	/* set ARFCN */

	uint16_t arfcn = trx->arfcn;

	fu_config[119] = arfcn >> 8;
	fu_config[119 + 1] = arfcn & 0xFF;

	for (i = 0; i < ARRAY_SIZE(trx->ts); i++) {
		struct gsm_bts_trx_ts *ts = &trx->ts[i];

		if (ts->hopping.enabled) {
			/* reverse order */
			int j;
			for (j = 0; j < ts->hopping.ma_len; j++)
				fu_config[133 + (i * 10) + (7 - j)] =
				    ts->hopping.ma_data[j];
			fu_config[133 + 8 + (i * 10)] = ts->hopping.hsn;
			fu_config[133 + 8 + 1 + (i * 10)] = ts->hopping.maio;
			*hopping = 1;
		} else {
			fu_config[133 + 8 + (i * 10)] = arfcn >> 8;
			fu_config[133 + 8 + 1 + (i * 10)] = arfcn & 0xFF;
		}
	}

	/* set BSIC */

	/*
	   Attention: all TRX except the first one seem to get the TSC
	   from the CHANNEL ACTIVATION command (in CHANNEL IDENTIFICATION,
	   GSM 04.08 CHANNEL DESCRIPTION).
	   There was a bug in rsl_chan_activate_lchan() setting this parameter.
	 */

	uint8_t bsic = trx->bts->bsic;

	fu_config[106] = bsic;

	/* set CA */

	if (generate_cell_chan_list(&fu_config[38], trx->bts) != 0) {
		LOG_TRX(trx, DNM, LOGL_ERROR, "generate_cell_chan_list failed\n");
		return 0;
	}

	/* set channel configuration */

	for (i = 0; i < ARRAY_SIZE(trx->ts); i++) {
		struct gsm_bts_trx_ts *ts = &trx->ts[i];
		uint8_t chan_config;

		/*
		   0 = FCCH + SCH + BCCH + CCCH
		   1 = FCCH + SCH + BCCH + CCCH + SDCCH/4 + SACCH/4
		   2 = BCCH + CCCH (This combination is not used in any BTS)
		   3 = FCCH + SCH + BCCH + CCCH + SDCCH/4 with SDCCH2 used as CBCH
		   4 = SDCCH/8 + SACCH/8
		   5 = SDCCH/8 with SDCCH2 used as CBCH
		   6 = TCH/F + FACCH/F + SACCH/F
		   7 = E-RACH (Talk family)
		   9 = Dual rate (capability for TCH/F and TCH/H)
		   10 = reserved for BTS internal use
		   11 = PBCCH + PCCCH + PDTCH + PACCH + PTCCH (can be used in GPRS release 2).
		   0xFF = spare TS
		 */

		switch (ts->pchan_from_config) {
		case GSM_PCHAN_NONE:
			chan_config = 0xFF;
			break;
		case GSM_PCHAN_CCCH:
			chan_config = 0;
			break;
		case GSM_PCHAN_CCCH_SDCCH4:
			chan_config = 1;
			break;
		case GSM_PCHAN_CCCH_SDCCH4_CBCH:
			chan_config = 3;
			break;
		case GSM_PCHAN_TCH_F:
			chan_config = 6;	/* 9 should work too */
			break;
		case GSM_PCHAN_TCH_H:
			chan_config = 9;
			break;
		case GSM_PCHAN_SDCCH8_SACCH8C:
			chan_config = 4;
			break;
		case GSM_PCHAN_SDCCH8_SACCH8C_CBCH:
			chan_config = 5;
			break;
		case GSM_PCHAN_PDCH:
			chan_config = 11;
			break;
		default:
			LOG_TRX(trx, DNM, LOGL_ERROR, "unsupported channel config %s for timeslot %d\n",
				gsm_pchan_name(ts->pchan_from_config), i);
			return 0;
		}

		fu_config[72 + i] = chan_config;
	}
	return sizeof(fu_config_template);
}


void set_real_time(uint8_t * real_time)
{
	time_t t;
	struct tm *tm;

	t = time(NULL);
	tm = localtime(&t);

	/* year-high, year-low, month, day, hour, minute, second, msec-high, msec-low */

	real_time[0] = (1900 + tm->tm_year) >> 8;
	real_time[1] = (1900 + tm->tm_year) & 0xFF;
	real_time[2] = tm->tm_mon + 1;
	real_time[3] = tm->tm_mday;
	real_time[4] = tm->tm_hour;
	real_time[5] = tm->tm_min;
	real_time[6] = tm->tm_sec;
	real_time[7] = 0;
	real_time[8] = 0;
}

/* TODO: put in a separate file ? */

/*
  build the configuration data
*/

static int make_bts_config(struct gsm_bts *bts, uint8_t bts_type, int n_trx, uint8_t * fu_config,
			   int need_hopping, int hopping_type)
{
	/* is it an InSite BTS ? */
	if (bts_type == 0x0E || bts_type == 0x0F || bts_type == 0x10) {	/* TODO */
		if (n_trx != 1) {
			LOG_BTS(bts, DNM, LOGL_ERROR, "InSite has only one TRX\n");
			return 0;
		}
		if (need_hopping != 0) {
			LOG_BTS(bts, DNM, LOGL_ERROR, "InSite does not support hopping\n");
			return 0;
		}
		memcpy(fu_config, bts_config_insite, sizeof(bts_config_insite));
		set_real_time(&fu_config[26]);
		return sizeof(bts_config_insite);
	}

	int len = 0;
	int i;

	memcpy(fu_config + len, bts_config_1, sizeof(bts_config_1));

	/* set sector configuration */
	fu_config[len + 12 - 1] = 1 + n_trx;	/* len */
	for (i = 0; i < n_trx; i++)
		fu_config[len + 12 + 1 + i] = ((i + 1) & 0xFF);

	len += (sizeof(bts_config_1) + (n_trx - 1));

	memcpy(fu_config + len, bts_config_2, sizeof(bts_config_2));
	/* set hopping mode */
	if (need_hopping) {
		switch (hopping_type) {
		/* 0: no hopping, 1: Baseband hopping, 2: RF hopping */
		case 0:
			LOG_BTS(bts, DNM, LOGL_INFO, "Baseband hopping selected!\n");
			fu_config[len + 2 + 1] = 1;
			break;
		case 1:
			LOG_BTS(bts, DNM, LOGL_INFO, "Synthesizer (RF) hopping selected!\n");
			fu_config[len + 2 + 1] = 2;
			break;
		default:
			LOG_BTS(bts, DNM, LOGL_INFO, "No hopping is selected!\n");
			fu_config[len + 2 + 1] = 0;
			break;
		}
	}
	len += sizeof(bts_config_2);

	/* set extended cell radius for each TRX */
	for (i = 0; i < n_trx; i++) {
		memcpy(fu_config + len, bts_config_3, sizeof(bts_config_3));
		fu_config[len + 3] = ((i + 1) & 0xFF);
		len += sizeof(bts_config_3);
	}

	memcpy(fu_config + len, bts_config_4, sizeof(bts_config_4));
	set_real_time(&fu_config[len + 3]);
	len += sizeof(bts_config_4);

	return len;
}

/* TODO: put in a separate file ? */

static struct msgb *nm_msgb_alloc(void)
{
	return msgb_alloc_headroom(OM_ALLOC_SIZE, OM_HEADROOM_SIZE, "OML");
}


static int abis_nm_send(struct gsm_bts *bts, uint8_t msg_type, uint16_t ref,
			uint8_t * data, int len_data)
{
	struct abis_om_hdr *oh;
	struct abis_om_nokia_hdr *noh;
	struct msgb *msg = nm_msgb_alloc();

	oh = (struct abis_om_hdr *)msgb_put(msg,
					    ABIS_OM_NOKIA_HDR_SIZE + len_data);

	oh->mdisc = ABIS_OM_MDISC_FOM;
	oh->placement = ABIS_OM_PLACEMENT_ONLY;
	oh->sequence = 0;
	oh->length = sizeof(struct abis_om_nokia_hdr) + len_data;

	noh = (struct abis_om_nokia_hdr *)oh->data;

	noh->msg_type = msg_type;
	noh->spare = 0;
	noh->reference = htons(ref);
	memcpy(noh->data, data, len_data);

	LOG_BTS(bts, DNM, LOGL_DEBUG, "Sending %s\n", get_msg_type_name_string(msg_type));

	return abis_nm_sendmsg(bts, msg);
}

static int abis_nm_download_req(struct gsm_bts *bts, uint16_t ref)
{
	uint8_t *data = download_req;
	int len_data = sizeof(download_req);

	return abis_nm_send(bts, NOKIA_MSG_START_DOWNLOAD_REQ, ref, data,
			    len_data);
}


static int abis_nm_ack(struct gsm_bts *bts, uint16_t ref)
{
	uint8_t *data = ack;
	int len_data = sizeof(ack);

	return abis_nm_send(bts, NOKIA_MSG_ACK, ref, data, len_data);
}


static int abis_nm_reset(struct gsm_bts *bts, uint16_t ref)
{
	uint8_t *data = reset;
	int len_data = sizeof(reset);
	LOG_BTS(bts, DLINP, LOGL_INFO, "Nokia BTS reset timer: %d\n", bts->nokia.bts_reset_timer_cnf);
	return abis_nm_send(bts, NOKIA_MSG_RESET_REQ, ref, data, len_data);
}

static int abis_nm_cha_adm_trx_unlock(struct gsm_bts *bts, uint8_t trx_id, uint16_t ref)
{
	/* BTS_CHA_ADM_STATE */
	/* object_identity = TRX; object_state = locked/unlocked */
	uint8_t *data = trx_unlock;
	data[8] = trx_id;
	int len_data = sizeof(trx_unlock);
	LOG_BTS(bts, DNM, LOGL_INFO, "TRX=%d admin state change: UNLOCKED (ref=%d)\n", trx_id, ref);
	return abis_nm_send(bts, NOKIA_MSG_CHA_ADM_STATE, ref, data, len_data);
}


static int abis_nm_cha_adm_trx_lock(struct gsm_bts *bts, uint8_t trx_id, uint16_t ref)
{
	/* BTS_CHA_ADM_STATE */
	/* object_identity = TRX; object_state = locked/unlocked */
	uint8_t *data = trx_lock;
	data[8] = trx_id;
	int len_data = sizeof(trx_lock);
	LOG_BTS(bts, DNM, LOGL_INFO, "TRX=%d admin state change: LOCKED (ref=%d)\n", trx_id, ref);
	return abis_nm_send(bts, NOKIA_MSG_CHA_ADM_STATE, ref, data, len_data);
}


static int abis_nm_trx_reset(struct gsm_bts *bts, uint8_t trx_id, uint16_t ref)
{
	/* BTS_CHA_ADM_STATE */
	/* object_identity = TRX; object_state = locked/unlocked */
	uint8_t *data = trx_reset;
	data[5] = trx_id;
	int len_data = sizeof(trx_reset);
	LOG_BTS(bts, DNM, LOGL_INFO, "TRX=%d reset ! (ref=%d)\n", trx_id, ref);
	return abis_nm_send(bts, NOKIA_MSG_RESET_REQ, ref, data, len_data);
}

/* TODO: put in a separate file ? */

static int abis_nm_send_multi_segments(struct gsm_bts *bts, uint8_t msg_type,
				       uint16_t ref, uint8_t * data, int len)
{
	int len_remain, len_to_send, max_send;
	int seq = 0;
	int ret;

	len_remain = len;

	while (len_remain) {
		struct abis_om_hdr *oh;
		struct abis_om_nokia_hdr *noh;
		struct msgb *msg = nm_msgb_alloc();

		if (seq == 0)
			max_send = 256 - sizeof(struct abis_om_nokia_hdr);
		else
			max_send = 256;

		if (len_remain > max_send) {
			len_to_send = max_send;

			if (seq == 0) {
				/* first segment */
				oh = (struct abis_om_hdr *)msgb_put(msg,
								    ABIS_OM_NOKIA_HDR_SIZE
								    +
								    len_to_send);

				oh->mdisc = ABIS_OM_MDISC_FOM;
				oh->placement = ABIS_OM_PLACEMENT_FIRST;	/* first segment of multi-segment message */
				oh->sequence = seq;
				oh->length = 0;	/* 256 bytes */

				noh = (struct abis_om_nokia_hdr *)oh->data;

				noh->msg_type = msg_type;
				noh->spare = 0;
				noh->reference = htons(ref);
				memcpy(noh->data, data, len_to_send);
			} else {
				/* segment in between */
				oh = (struct abis_om_hdr *)msgb_put(msg,
								    sizeof
								    (struct
								     abis_om_hdr)
								    +
								    len_to_send);

				oh->mdisc = ABIS_OM_MDISC_FOM;
				oh->placement = ABIS_OM_PLACEMENT_MIDDLE;	/* segment of multi-segment message */
				oh->sequence = seq;
				oh->length = 0;	/* 256 bytes */

				memcpy(oh->data, data, len_to_send);
			}
		} else {

			len_to_send = len_remain;

			/* check if message fits in a single segment */

			if (seq == 0)
				return abis_nm_send(bts, msg_type, ref, data,
						    len_to_send);

			/* last segment */

			oh = (struct abis_om_hdr *)msgb_put(msg,
							    sizeof(struct
								   abis_om_hdr)
							    + len_to_send);

			oh->mdisc = ABIS_OM_MDISC_FOM;
			oh->placement = ABIS_OM_PLACEMENT_LAST;	/* last segment of multi-segment message */
			oh->sequence = seq;
			oh->length = len_to_send;

			memcpy(oh->data, data, len_to_send);
		}

		LOG_BTS(bts, DNM, LOGL_DEBUG, "Sending multi-segment %d\n", seq);

		ret = abis_nm_sendmsg(bts, msg);
		if (ret < 0)
			return ret;

		nokia_abis_nm_queue_send_next(bts);

		/* next segment */
		len_remain -= len_to_send;
		data += len_to_send;
		seq++;
	}

	return 0;
}

/* TODO: put in a separate file ? */

static int abis_nm_send_config(struct gsm_bts *bts, uint8_t bts_type)
{
	struct gsm_bts_trx *trx;
	uint8_t config[2048];	/* TODO: might be too small if lots of TRX are used */
	int len = 0;
	int idx = 0;
	int ret;
	int hopping = 0;
	int need_hopping = 0;
	int hopping_type = 0;

	hopping_type = bts->nokia.hopping_mode;

	memset(config, 0, sizeof(config));

	llist_for_each_entry(trx, &bts->trx_list, list) {
#if 0				/* debugging */
		printf("TRX\n");
		printf("  arfcn: %d\n", trx->arfcn);
		printf("  bsic: %d\n", trx->bts->bsic);
		uint8_t ca[20];
		memset(ca, 0xFF, sizeof(ca));
		ret = generate_cell_chan_list(ca, trx->bts);
		printf("  ca (%d): %s\n", ret, osmo_hexdump(ca, sizeof(ca)));
		int i;
		for (i = 0; i < ARRAY_SIZE(trx->ts); i++) {
			struct gsm_bts_trx_ts *ts = &trx->ts[i];

			printf("  pchan %d: %d\n", i, ts->pchan);
		}
#endif
		ret = make_fu_config(trx, idx + 1, config + len, &hopping);
		need_hopping |= hopping;
		len += ret;

		idx++;
	}

	ret = make_bts_config(bts, bts_type, idx, config + len, need_hopping, hopping_type);
	len += ret;

#if 0				/* debugging */
	dump_elements(config, len);
#endif

	return abis_nm_send_multi_segments(bts, NOKIA_MSG_CONF_DATA, 1, config,
					   len);
}

#define GET_NEXT_BYTE if(idx >= len) return 0; \
                        ub = data[idx++];

static int find_element(uint8_t * data, int len, uint16_t id, uint8_t * value,
			int max_value)
{
	uint8_t ub;
	int idx = 0;
	int found = 0;
	int constructed __attribute__((unused));
	uint16_t id_value;

	for (;;) {

		GET_NEXT_BYTE;

		/* encoding bit, constructed means that other elements are contained */
		constructed = ((ub & 0x20) ? 1 : 0);

		if ((ub & 0x1F) == 0x1F) {
			/* fixed pattern, ID follows */
			GET_NEXT_BYTE;	/* ID */
			id_value = ub & 0x7F;
			if (ub & 0x80) {
				/* extension bit */
				GET_NEXT_BYTE;	/* ID low part */
				id_value = (id_value << 7) | (ub & 0x7F);
			}
			if (id_value == id)
				found = 1;
		} else {
			id_value = (ub & 0x3F);
			if (id_value == id)
				found = 1;
		}

		GET_NEXT_BYTE;	/* length */

		if (found) {
			/* get data */
			uint8_t n = ub;
			uint8_t i;
			for (i = 0; i < n; i++) {
				GET_NEXT_BYTE;
				if (max_value <= 0)
					return -1;	/* buffer too small */
				*value = ub;
				value++;
				max_value--;
			}
			return n;	/* length */
		} else {
			/* skip data */
			uint8_t n = ub;
			uint8_t i;
			for (i = 0; i < n; i++) {
				GET_NEXT_BYTE;
			}
		}
	}
	return 0;		/* not found */
}

static int dump_elements(uint8_t * data, int len)
{
	uint8_t ub;
	int idx = 0;
	int constructed;
	uint16_t id_value;
	static char indent[100] = "";	/* TODO: move static to BTS context */

	for (;;) {

		GET_NEXT_BYTE;

		/* encoding bit, constructed means that other elements are contained */
		constructed = ((ub & 0x20) ? 1 : 0);

		if ((ub & 0x1F) == 0x1F) {
			/* fixed pattern, ID follows */
			GET_NEXT_BYTE;	/* ID */
			id_value = ub & 0x7F;
			if (ub & 0x80) {
				/* extension bit */
				GET_NEXT_BYTE;	/* ID low part */
				id_value = (id_value << 7) | (ub & 0x7F);
			}

		} else {
			id_value = (ub & 0x3F);
		}

		GET_NEXT_BYTE;	/* length */

		printf("%s--ID = 0x%02X (%s) %s\n", indent, id_value,
		       get_element_name_string(id_value),
		       constructed ? "** constructed **" : "");
		printf("%s  length = %d\n", indent, ub);
		printf("%s  %s\n", indent, osmo_hexdump(data + idx, ub));

		if (constructed) {
			int indent_len = strlen(indent);
			strcat(indent, "   ");

			dump_elements(data + idx, ub);

			indent[indent_len] = 0;
		}
		/* skip data */
		uint8_t n = ub;
		uint8_t i;
		for (i = 0; i < n; i++) {
			GET_NEXT_BYTE;
		}
	}
	return 0;
}

static void mo_ok(struct gsm_abis_mo *mo)
{
	mo->nm_state.operational    = NM_OPSTATE_ENABLED;
	mo->nm_state.administrative = NM_STATE_UNLOCKED;
	mo->nm_state.availability   = NM_AVSTATE_OK;
}

static void nokia_abis_nm_fake_1221_ok(struct gsm_bts *bts)
{
	/*
	 * The Nokia BTS don't follow the 12.21 model and we don't have OM objects
	 * for the various elements. However some of the BSC code depends on seeing
	 * those object "up & running", so when the Nokia init is done, we fake
	 * a "good" state
	 */
	struct gsm_bts_trx *trx;

	mo_ok(&bts->mo);
	mo_ok(&bts->site_mgr->mo);

	llist_for_each_entry(trx, &bts->trx_list, list) {
		int i;

		mo_ok(&trx->mo);
		mo_ok(&trx->bb_transc.mo);

		for (i = 0; i < ARRAY_SIZE(trx->ts); i++) {
			struct gsm_bts_trx_ts *ts = &trx->ts[i];
			mo_ok(&ts->mo);
		}
	}
}

/* TODO: put in a separate file ? */

/* taken from abis_nm.c */

static void nokia_abis_nm_queue_send_next(struct gsm_bts *bts)
{
	int wait = 0;
	struct msgb *msg;
	/* the queue is empty */
	while (!llist_empty(&bts->abis_queue)) {
		msg = msgb_dequeue(&bts->abis_queue);
		wait = OBSC_NM_W_ACK_CB(msg);
		abis_sendmsg(msg);

		if (wait)
			break;
	}

	bts->abis_nm_pend = wait;
}

/* TODO: put in a separate file ? */

/* timer for restarting OML after BTS reset */

static void reset_timer_cb(void *_bts)
{
	struct gsm_bts *bts = _bts;
	struct gsm_e1_subslot *e1_link = &bts->oml_e1_link;
	struct e1inp_line *line;

	/* OML link */
	line = e1inp_line_find(e1_link->e1_nr);
	if (!line) {
		LOG_BTS(bts, DLINP, LOGL_ERROR, "BTS OML link referring to non-existing E1 line %u\n",
			e1_link->e1_nr);
		return;
	}

	switch (bts->nokia.wait_reset) {
	case RESET_T_NONE:				/* shouldn't happen */
		break;
	case RESET_T_STOP_LAPD:
		start_sabm_in_line(line, 0, -1);	/* stop all first */
		bts->nokia.wait_reset = RESET_T_RESTART_LAPD;
		osmo_timer_schedule(&bts->nokia.reset_timer, bts->nokia.bts_reset_timer_cnf, 0);
		break;
	case RESET_T_RESTART_LAPD:
		bts->nokia.wait_reset = 0;
		start_sabm_in_line(line, 0, -1);	/* stop all first */
		start_sabm_in_line(line, 1, SAPI_OML);	/* start only OML */
		break;
	}
}

/* TODO: put in a separate file ? */

/*
  This is how the configuration is done:
  - start OML link
  - reset BTS
  - receive ACK, wait some time and restart OML link
  - receive OMU STARTED message, send START DOWNLOAD REQ
  - receive CNF REQ message, send CONF DATA
  - receive ACK, start RSL link(s)
  ACK some other messages received from the BTS.

  Probably its also possible to configure the BTS without a reset, this
  has not been tested yet.
*/

#define FIND_ELEM(data, data_len, ei, var, len) (find_element(data, data_len, ei, var, len) == len)
static int abis_nm_rcvmsg_fom(struct msgb *mb)
{
	struct e1inp_sign_link *sign_link = (struct e1inp_sign_link *)mb->dst;
	struct gsm_bts *bts = sign_link->trx->bts;
	struct abis_om_hdr *oh = msgb_l2(mb);
	struct abis_om_nokia_hdr *noh = msgb_l3(mb);
	uint8_t mt = noh->msg_type;
	int ret = 0;
	uint16_t ref = ntohs(noh->reference);
	uint8_t info[256];
	uint8_t ack = 0xFF;
	uint8_t severity = 0xFF;
	uint8_t reset_type = 0xFF;
	uint8_t object_identity[4] = {0};
	uint8_t object_state = 0xFF;
	uint8_t object_id_state[11] = {0};
	int severity_len = 0;
	int ei_add_info_len = 0;
	int ei_alarm_detail_len = 0;
	int len_data;


	if (bts->nokia.wait_reset) {
		LOG_BTS(bts, DNM, LOGL_INFO, "Ignoring message while waiting for reset: %s\n", msgb_hexdump(mb));
		return ret;
	}

	if (oh->length < sizeof(struct abis_om_nokia_hdr)) {
		LOG_BTS(bts, DNM, LOGL_ERROR, "Message too short: %s\n", msgb_hexdump(mb));
		return -EINVAL;
	}

	len_data = oh->length - sizeof(struct abis_om_nokia_hdr);
	LOG_BTS(bts, DNM, LOGL_INFO, "Rx (0x%02X) %s\n", mt, get_msg_type_name_string(mt));
#if 0				/* debugging */
	dump_elements(noh->data, len_data);
#endif
	switch (mt) {
	case NOKIA_MSG_OMU_STARTED:
		if (FIND_ELEM(noh->data, len_data, NOKIA_EI_BTS_TYPE, &bts->nokia.bts_type, sizeof(bts->nokia.bts_type))) {
			LOG_BTS(bts, DNM, LOGL_INFO, "Rx BTS type = %d (%s)\n",
				bts->nokia.bts_type,
				get_bts_type_string(bts->nokia.bts_type));
		} else {
			LOG_BTS(bts, DNM, LOGL_ERROR, "BTS type not found in NOKIA_MSG_OMU_STARTED\n");
		}
		if (FIND_ELEM(noh->data, len_data, NOKIA_EI_RESET_TYPE, &reset_type, sizeof(reset_type))) {
			LOG_BTS(bts, DNM, LOGL_INFO, "Rx BTS reset type = '%s'\n",
				get_reset_type_string(reset_type));
		} else {
			LOG_BTS(bts, DNM, LOGL_ERROR, "BTS reset type not found in NOKIA_MSG_OMU_STARTED\n");
		}
		/* send START_DOWNLOAD_REQ */
		abis_nm_download_req(bts, ref);
		break;
	case NOKIA_MSG_MF_REQ:
		break;
	case NOKIA_MSG_CONF_REQ:
		/* send ACK */
		abis_nm_ack(bts, ref);
		nokia_abis_nm_queue_send_next(bts);
		/* send CONF_DATA */
		abis_nm_send_config(bts, bts->nokia.bts_type);
		bts->nokia.configured = 1;
		break;
	case NOKIA_MSG_ACK:
		if (FIND_ELEM(noh->data, len_data, NOKIA_EI_ACK, &ack, sizeof(ack))) {
			LOG_BTS(bts, DNM, LOGL_INFO, "Rx ACK = %u\n", ack);
			if (ack != 1) {
				LOG_BTS(bts, DNM, LOGL_ERROR, "Rx No ACK (%u): don't know how to proceed\n", ack);
				/* TODO: properly handle failures (NACK) */
			}
		} else
			LOG_BTS(bts, DNM, LOGL_ERROR, "Rx MSG_ACK but no EI_ACK found: %s\n", msgb_hexdump(mb));

		/* TODO: the assumption for the following is that no NACK was received */

		/* ACK for reset message ? */
		if (!bts->nokia.did_reset) {
			bts->nokia.did_reset = 1;

			/*
			   TODO: For the InSite processing the received data is
			   blocked in the driver during reset.
			   Otherwise the LAPD module might assert because the InSite
			   sends garbage on the E1 line during reset.
			   This is done by looking at "wait_reset" in the driver
			   (function handle_ts1_read()) and ignoring the received data.
			   It seems to be necessary for the MetroSite too.
			 */

			/* we cannot delete / stop the OML LAPD SAP right here, as we are in
			 * the middle of processing an LAPD I frame and are subsequently returning
			 * back to the LAPD I frame processing code that assumes the SAP is still
			 * active. So we first schedule the timer at 0ms in the future, where we
			 * kill all LAPD SAP and re-arm the timer for the reset duration, after which
			 * we re-create them */
			bts->nokia.wait_reset = RESET_T_STOP_LAPD;
			osmo_timer_setup(&bts->nokia.reset_timer, reset_timer_cb, bts);
			osmo_timer_schedule(&bts->nokia.reset_timer, 0, 0);
		}
		break;
	case NOKIA_MSG_STATE_CHANGED:
		/* send ACK */
		abis_nm_ack(bts, ref);
		if (!FIND_ELEM(noh->data, len_data, NOKIA_EI_OBJ_ID_STATE, object_id_state, sizeof(object_id_state))) {
			LOG_BTS(bts, DNM, LOGL_NOTICE, "Missing NOKIA_EI_OBJ_ID_STATE\n");
			return -EINVAL;
		}
		LOG_BTS(bts, DNM, LOGL_NOTICE, "State changed: %s=%d, %s\n",
			get_object_identity_string(object_id_state[4]),
			object_id_state[5],
			get_object_state_string(object_id_state[10]));
		break;
	case NOKIA_MSG_CONF_COMPLETE:
		/* send ACK */
		abis_nm_ack(bts, ref);
		if (bts->nokia.configured != 0) {
			struct gsm_bts_trx *trx;
			uint8_t bcch_trx_nr;
			ref = 21;
			/* we first need to unlock the TRX that runs BCCH */
			llist_for_each_entry(trx, &bts->trx_list, list) {
				if (trx->ts[0].pchan_from_config == GSM_PCHAN_CCCH ||
				    trx->ts[0].pchan_from_config == GSM_PCHAN_CCCH_SDCCH4 ||
				    trx->ts[0].pchan_from_config == GSM_PCHAN_CCCH_SDCCH4_CBCH) {
					/* saving the number of TRX that has BCCH on it */
					bcch_trx_nr = trx->nr;
					/* unlock TRX */
					abis_nm_cha_adm_trx_unlock(bts, trx->nr + 1, ref); //Nokia starts at 1, Osmo at 0
					/* reset TRX */
					ref+=2;
					abis_nm_trx_reset(bts, trx->nr + 1, ref);
				}
			}
			/* now unlocking all other TRXs */
			llist_for_each_entry(trx, &bts->trx_list, list) {
				if (trx->nr != bcch_trx_nr) {
					ref+=2;
					/* unlock TRX */
					abis_nm_cha_adm_trx_unlock(bts, trx->nr + 1, ref);
					/* reset TRX */
					/* ref+=2;
					abis_nm_trx_reset(bts, trx->nr + 1, ref); */
				}
			}
			/* start TRX  (RSL link) */
			struct gsm_e1_subslot *e1_link = &sign_link->trx->rsl_e1_link;
			struct e1inp_line *line;
			bts->nokia.configured = 0;
			/* RSL Link */
			line = e1inp_line_find(e1_link->e1_nr);
			if (!line) {
					LOG_BTS(bts, DLINP, LOGL_ERROR, "RSL link referring to "
						"non-existing E1 line %u\n", e1_link->e1_nr);
					return -ENOMEM;
			}
			/* start TRX */
			start_sabm_in_line(line, 1, SAPI_RSL);  /* start only RSL */
		}
		/* fake 12.21 OM */
		nokia_abis_nm_fake_1221_ok(bts);
		break;
	case NOKIA_MSG_BLOCK_CTRL_REQ:	/* BTS uses this scenario to block an object in BSC */
		/* TODO: implement block function */
		/* send ACK */
		abis_nm_ack(bts, ref);
		break;
	case NOKIA_MSG_ALARM:
                if (!FIND_ELEM(noh->data, len_data, NOKIA_EI_OBJ_ID, object_identity, sizeof(object_identity)) ||
                    !FIND_ELEM(noh->data, len_data, NOKIA_EI_OBJ_STATE, &object_state, sizeof(object_state))) {
                        if (!FIND_ELEM(noh->data, len_data, NOKIA_EI_OBJ_ID_STATE, object_id_state, sizeof(object_id_state))) {
                                LOG_BTS(bts, DNM, LOGL_NOTICE, "Missing NOKIA_EI_OBJ_ID & NOKIA_EI_OBJ_STATE or NOKIA_EI_OBJ_ID_STATE\n");
                                return -EINVAL;
                        }
                        object_identity[1] = object_id_state[4];
                        object_identity[2] = object_id_state[5];
                        object_state = object_id_state[10];
                }
                severity_len = find_element(noh->data, len_data, NOKIA_EI_SEVERITY, &severity, sizeof(severity));
                /* TODO: there might be alarms with both elements set */
                ei_add_info_len = find_element(noh->data, len_data, NOKIA_EI_ADD_INFO, info, sizeof(info));
                if (ei_add_info_len > 0) {
                        info[ei_add_info_len] = 0;
                        if (severity_len > 0) {
                                LOG_BTS(bts, DNM, LOGL_NOTICE, "Rx ALARM (%s=%d(%s)) Severity %s (%d) : %s\n",
                                        get_object_identity_string(object_identity[1]),
                                        object_identity[2],
                                        get_object_state_string(object_state),
                                        get_severity_string(severity),
                                        severity,
                                        info);
                        } else {
                                LOG_BTS(bts, DNM, LOGL_NOTICE, "Rx ALARM (%s=%d(%s)) : %s\n",
                                        get_object_identity_string(object_identity[1]),
                                        object_identity[2],
                                        get_object_state_string(object_state),
                                        info);

                        }
		}

                /* nothing found, try details */
                ei_alarm_detail_len = find_element(noh->data, len_data, NOKIA_EI_ALARM_DETAIL, info, sizeof(info));
                if (ei_alarm_detail_len > 0) {
                        uint16_t code;
                        info[ei_alarm_detail_len] = 0;
                        code = (info[0] << 8) + info[1];
                        if (severity_len > 0) {
                                LOG_BTS(bts, DNM, LOGL_NOTICE, "Rx ALARM (%s=%d(%s)) Severity %s (%d), code 0x%X : %s\n",
                                        get_object_identity_string(object_identity[1]),
                                        object_identity[2],
                                        get_object_state_string(object_state),
                                        get_severity_string(severity),
                                        severity,
                                        code,
                                        info + 2);
                        } else {
                                LOG_BTS(bts, DNM, LOGL_NOTICE, "Rx ALARM (%s=%d(%s)) : %s\n",
                                        get_object_identity_string(object_identity[1]),
                                        object_identity[2],
                                        get_object_state_string(object_state),
                                        info + 2);

                        }
                }
		if (ei_add_info_len == 0 && ei_alarm_detail_len == 0) {
	                LOG_BTS(bts, DNM, LOGL_NOTICE, "Rx ALARM (%s=%d(%s))\n",
        	                get_object_identity_string(object_identity[1]),
                	        object_identity[2],
                        	get_object_state_string(object_state));
		}
		/* send ACK */
		abis_nm_ack(bts, ref);
		break;
	}

	nokia_abis_nm_queue_send_next(bts);

	return ret;
}

/* TODO: put in a separate file ? */

int abis_nokia_rcvmsg(struct msgb *msg)
{
	struct e1inp_sign_link *sign_link = (struct e1inp_sign_link *)msg->dst;
	struct gsm_bts *bts = sign_link->trx->bts;
	struct abis_om_hdr *oh = msgb_l2(msg);
	int rc = 0;

	/* Various consistency checks */
	if (oh->placement != ABIS_OM_PLACEMENT_ONLY) {
		LOG_BTS(bts, DNM, LOGL_ERROR, "Rx ABIS OML placement 0x%x not supported\n", oh->placement);
		if (oh->placement != ABIS_OM_PLACEMENT_FIRST)
			return -EINVAL;
	}
	if (oh->sequence != 0) {
		LOG_BTS(bts, DNM, LOGL_ERROR, "Rx ABIS OML sequence 0x%x != 0x00\n", oh->sequence);
		return -EINVAL;
	}
	msg->l3h = (unsigned char *)oh + sizeof(*oh);

	switch (oh->mdisc) {
	case ABIS_OM_MDISC_FOM:
		LOG_BTS(bts, DNM, LOGL_INFO, "Rx ABIS_OM_MDISC_FOM\n");
		rc = abis_nm_rcvmsg_fom(msg);
		break;
	case ABIS_OM_MDISC_MANUF:
		LOG_BTS(bts, DNM, LOGL_INFO, "Rx ABIS_OM_MDISC_MANUF: ignoring\n");
		break;
	case ABIS_OM_MDISC_MMI:
	case ABIS_OM_MDISC_TRAU:
		LOG_BTS(bts, DNM, LOGL_ERROR, "Rx unimplemented ABIS OML message discriminator 0x%x\n", oh->mdisc);
		break;
	default:
		LOG_BTS(bts, DNM, LOGL_ERROR, "Rx unknown ABIS OML message discriminator 0x%x\n", oh->mdisc);
		return -EINVAL;
	}

	msgb_free(msg);
	return rc;
}

static int bts_model_nokia_site_start(struct gsm_network *net);

static void bts_model_nokia_site_e1line_bind_ops(struct e1inp_line *line)
{
	e1inp_line_bind_ops(line, &bts_isdn_e1inp_line_ops);
}


static struct gsm_bts_model model_nokia_site = {
	.type = GSM_BTS_TYPE_NOKIA_SITE,
	.name = "nokia_site",
	.start = bts_model_nokia_site_start,
	.oml_rcvmsg = &abis_nokia_rcvmsg,
	.e1line_bind_ops = &bts_model_nokia_site_e1line_bind_ops,
};

static struct gsm_network *my_net;

static int bts_model_nokia_site_start(struct gsm_network *net)
{
	osmo_signal_register_handler(SS_L_INPUT, inp_sig_cb, NULL);
	osmo_signal_register_handler(SS_L_GLOBAL, gbl_sig_cb, NULL);
	osmo_signal_register_handler(SS_NM, nm_sig_cb, NULL);

	my_net = net;

	return 0;
}

int bts_model_nokia_site_init(void)
{
	model_nokia_site.features.data = &model_nokia_site._features_data[0];
	model_nokia_site.features.data_len = sizeof(model_nokia_site._features_data);

	osmo_bts_set_feature(&model_nokia_site.features, BTS_FEAT_HOPPING);
	osmo_bts_set_feature(&model_nokia_site.features, BTS_FEAT_HSCSD);
	osmo_bts_set_feature(&model_nokia_site.features, BTS_FEAT_MULTI_TSC);

	return gsm_bts_model_register(&model_nokia_site);
}

/* Nokia XXXsite family specific header */

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

/* Defines*/
#define SAPI_OML    62
#define SAPI_RSL    0
/* some message IDs */

#define NOKIA_MSG_CONF_DATA             128
#define NOKIA_MSG_ACK                   129
#define NOKIA_MSG_OMU_STARTED           130
#define NOKIA_MSG_START_DOWNLOAD_REQ    131
#define NOKIA_MSG_MF_REQ                132
#define NOKIA_MSG_RESET_REQ             134
#define NOKIA_MSG_CONF_REQ              136
#define NOKIA_MSG_CONF_COMPLETE         142
#define NOKIA_MSG_BLOCK_CTRL_REQ        168
#define NOKIA_MSG_STATE_CHANGED         172
#define NOKIA_MSG_ALARM                 174
#define NOKIA_MSG_CHA_ADM_STATE         175

/* some element IDs */

#define NOKIA_EI_BTS_TYPE       0x13
#define NOKIA_EI_ACK            0x23
#define NOKIA_EI_ADD_INFO       0x51
#define NOKIA_EI_SEVERITY       0x4B
#define NOKIA_EI_ALARM_DETAIL   0x94
#define NOKIA_EI_RESET_TYPE     0x18
#define NOKIA_EI_OBJ_ID         0x40
#define NOKIA_EI_OBJ_STATE      0x44
#define NOKIA_EI_OBJ_ID_STATE   0x65

#define OM_ALLOC_SIZE       1024
#define OM_HEADROOM_SIZE    128

#define ABIS_OM_NOKIA_HDR_SIZE (sizeof(struct abis_om_hdr) + sizeof(struct abis_om_nokia_hdr))


/* Enums, static arrays, structs */
enum reset_timer_state {
	RESET_T_NONE = 0,
	RESET_T_STOP_LAPD = 1,		/* first timer expiration: stop LAPD SAP */
	RESET_T_RESTART_LAPD = 2,	/* second timer expiration: restart LAPD SAP */
};

static uint8_t trx_lock[] = {
	0x7F, 0x65, 0x0B,
	/* ID = 0x65 (Obj. identity and obj. state) ## constructed ## */
	/* length = 11 */
	/* [83] */

	0x5F, 0x40, 0x04,
	/* ID = 0x40 (Object identity) */
	/* length = 4 */
	/* [86] */
	0x00, 0x04, 0x01, 0xFF,

	0x5F, 0x44, 0x01,
	/* ID = 0x44 (Object current state) */
	/* length = 1 */
	/* [93] */
	0x02,
};

static const struct value_string nokia_msgt_name[] = {
	{ 0x7F, "NOKIA_BTS_RSSI_VSWR_COUNTERS_REQ" },
	{ 0x80, "NOKIA_BTS_CONF_DATA" },
	{ 0x81, "NOKIA_BTS_ACK" },
	{ 0x82, "NOKIA_BTS_OMU_STARTED" },
	{ 0x83, "NOKIA_BTS_START_DOWNLOAD_REQ" },
	{ 0x84, "NOKIA_BTS_MF_REQ" },
	{ 0x85, "NOKIA_BTS_AF_REQ" },
	{ 0x86, "NOKIA_BTS_RESET_REQ" },
	{ 0x87, "NOKIA_reserved" },
	{ 0x88, "NOKIA_BTS_CONF_REQ" },
	{ 0x89, "NOKIA_BTS_TEST_REQ" },
	{ 0x8A, "NOKIA_BTS_TEST_REPORT" },
	{ 0x8B, "NOKIA_reserved" },
	{ 0x8C, "NOKIA_reserved" },
	{ 0x8D, "NOKIA_reserved" },
	{ 0x8E, "NOKIA_BTS_CONF_COMPL" },
	{ 0x8F, "NOKIA_reserved" },
	{ 0x90, "NOKIA_BTS_STM_TEST_REQ" },
	{ 0x91, "NOKIA_BTS_STM_TEST_REPORT" },
	{ 0x92, "NOKIA_BTS_TRANSMISSION_COMMAND" },
	{ 0x93, "NOKIA_BTS_TRANSMISSION_ANSWER" },
	{ 0x94, "NOKIA_BTS_HW_DB_UPLOAD_REQ" },
	{ 0x95, "NOKIA_BTS_START_HW_DB_DOWNLOAD_REQ" },
	{ 0x96, "NOKIA_BTS_HW_DB_SAVE_REQ" },
	{ 0x97, "NOKIA_BTS_FLASH_ERASURE_REQ" },
	{ 0x98, "NOKIA_BTS_HW_DB_DOWNLOAD_REQ" },
	{ 0x99, "NOKIA_BTS_PWR_SUPPLY_CONTROL" },
	{ 0x9A, "NOKIA_BTS_ATTRIBUTE_REQ" },
	{ 0x9B, "NOKIA_BTS_ATTRIBUTE_REPORT" },
	{ 0x9C, "NOKIA_BTS_HW_REQ" },
	{ 0x9D, "NOKIA_BTS_HW_REPORT" },
	{ 0x9E, "NOKIA_BTS_RTE_TEST_REQ" },
	{ 0x9F, "NOKIA_BTS_RTE_TEST_REPORT" },
	{ 0xA0, "NOKIA_BTS_HW_DB_VERIFICATION_REQ" },
	{ 0xA1, "NOKIA_BTS_CLOCK_REQ" },
	{ 0xA2, "NOKIA_AC_CIRCUIT_REQ_NACK" },
	{ 0xA3, "NOKIA_AC_INTERRUPTED" },
	{ 0xA4, "NOKIA_BTS_NEW_TRE_INFO" },
	{ 0xA5, "NOKIA_AC_BSC_CIRCUITS_ALLOCATED" },
	{ 0xA6, "NOKIA_BTS_TRE_POLL_LIST" },
	{ 0xA7, "NOKIA_AC_CIRCUIT_REQ" },
	{ 0xA8, "NOKIA_BTS_BLOCK_CTRL_REQ" },
	{ 0xA9, "NOKIA_BTS_GSM_TIME_REQ" },
	{ 0xAA, "NOKIA_BTS_GSM_TIME" },
	{ 0xAB, "NOKIA_BTS_OUTPUT_CONTROL" },
	{ 0xAC, "NOKIA_BTS_STATE_CHANGED" },
	{ 0xAD, "NOKIA_BTS_SW_SAVE_REQ" },
	{ 0xAE, "NOKIA_BTS_ALARM" },
	{ 0xAF, "NOKIA_BTS_CHA_ADM_STATE" },
	{ 0xB0, "NOKIA_AC_POOL_SIZE_REPORT" },
	{ 0xB1, "NOKIA_AC_POOL_SIZE_INQUIRY" },
	{ 0xB2, "NOKIA_BTS_COMMISS_TEST_COMPLETED" },
	{ 0xB3, "NOKIA_BTS_COMMISS_TEST_REQ" },
	{ 0xB4, "NOKIA_BTS_TRANSP_BTS_TO_BSC" },
	{ 0xB5, "NOKIA_BTS_TRANSP_BSC_TO_BTS" },
	{ 0xB6, "NOKIA_BTS_LCS_COMMAND" },
	{ 0xB7, "NOKIA_BTS_LCS_ANSWER" },
	{ 0xB8, "NOKIA_BTS_LMU_FN_OFFSET_COMMAND" },
	{ 0xB9, "NOKIA_BTS_LMU_FN_OFFSET_ANSWER" },
	{ 0xBA, "NOKIA_BTS_LAPD_CTR_REP" },
	{ 0xBB, "NOKIA_BTS_TRX_CONFIGURATION_UPDATE" },
	{ 0xBC, "NOKIA_BTS_DYNAMIC_LOAD_INFO" },
	{ 0xBD, "NOKIA_BTS_CONF_DATA_BS2" },
	{ 0xBE, "NOKIA_BTS_REGISTRY_REQ" },
	{ 0xBF, "NOKIA_BTS_SFS_UPLOAD_REQ" },
	{ 0xC0, "NOKIA_BTS_DELAY_MEAS_REQ" },
	{ 0xC1, "NOKIA_BTS_DELAY_MEAS_ACK" },
	{ 0xC2, "NOKIA_BTS_PABIS_MEAS_REQ" },
	{ 0xC3, "NOKIA_BTS_PABIS_MEAS_ACK" },
	{ 0xC4, "NOKIA_BSC_RESET_INFO" },
	{ 0xC5, "NOKIA_BSC_RESET_COMPL" },
	{ 0xC6, "NOKIA_BSC_RESET_COMPL_ANSWER" },
	{ 0xC7, "NOKIA_BTS_RSSI_VSWR_COUNTERS" },
	{ 0, NULL }
};

static const struct value_string nokia_element_name[] = {
	{ 0x01, "Ny1" },
	{ 0x02, "T3105_F" },
	{ 0x03, "Interference band limits" },
	{ 0x04, "Interference report timer in secs" },
	{ 0x05, "Channel configuration per TS" },
	{ 0x06, "BSIC" },
	{ 0x07, "RACH report timer in secs" },
	{ 0x08, "Hardware database status" },
	{ 0x09, "BTS RX level" },
	{ 0x0A, "ARFN" },
	{ 0x0B, "STM antenna attenuation" },
	{ 0x0C, "Cell allocation bitmap" },
	{ 0x0D, "Radio definition per TS" },
	{ 0x0E, "Frame number" },
	{ 0x0F, "Antenna diversity" },
	{ 0x10, "T3105_D" },
	{ 0x11, "File format" },
	{ 0x12, "Last File" },
	{ 0x13, "BTS type" },
	{ 0x14, "Erasure mode" },
	{ 0x15, "Hopping mode" },
	{ 0x16, "Floating TRX" },
	{ 0x17, "Power supplies" },
	{ 0x18, "Reset type" },
	{ 0x19, "Averaging period" },
	{ 0x1A, "RBER2" },
	{ 0x1B, "LAC" },
	{ 0x1C, "CI" },
	{ 0x1D, "Failure parameters" },
	{ 0x1E, "(RF max power reduction)" },
	{ 0x1F, "Measured RX_SENS" },
	{ 0x20, "Extended cell radius" },
	{ 0x21, "reserved" },
	{ 0x22, "Success-Failure" },
	{ 0x23, "Ack-Nack" },
	{ 0x24, "OMU test results" },
	{ 0x25, "File identity" },
	{ 0x26, "Generation and version code" },
	{ 0x27, "SW description" },
	{ 0x28, "BCCH LEV" },
	{ 0x29, "Test type" },
	{ 0x2A, "Subscriber number" },
	{ 0x2B, "reserved" },
	{ 0x2C, "HSN" },
	{ 0x2D, "reserved" },
	{ 0x2E, "MS RXLEV" },
	{ 0x2F, "MS TXLEV" },
	{ 0x30, "RXQUAL" },
	{ 0x31, "RX SENS" },
	{ 0x32, "Alarm block" },
	{ 0x33, "Neighbouring BCCH levels" },
	{ 0x34, "STM report type" },
	{ 0x35, "MA" },
	{ 0x36, "MAIO" },
	{ 0x37, "H_FLAG" },
	{ 0x38, "TCH_ARFN" },
	{ 0x39, "Clock output" },
	{ 0x3A, "Transmitted power" },
	{ 0x3B, "Clock sync" },
	{ 0x3C, "TMS protocol discriminator" },
	{ 0x3D, "TMS protocol data" },
	{ 0x3E, "FER" },
	{ 0x3F, "SWR result" },
	{ 0x40, "Object identity" },
	{ 0x41, "STM RX Antenna Test" },
	{ 0x42, "reserved" },
	{ 0x43, "reserved" },
	{ 0x44, "Object current state" },
	{ 0x45, "reserved" },
	{ 0x46, "FU channel configuration" },
	{ 0x47, "reserved" },
	{ 0x48, "ARFN of a CU" },
	{ 0x49, "FU radio definition" },
	{ 0x4A, "reserved" },
	{ 0x4B, "Severity" },
	{ 0x4C, "Diversity selection" },
	{ 0x4D, "RX antenna test" },
	{ 0x4E, "RX antenna supervision period" },
	{ 0x4F, "RX antenna state" },
	{ 0x50, "Sector configuration" },
	{ 0x51, "Additional info" },
	{ 0x52, "SWR parameters" },
	{ 0x53, "HW inquiry mode" },
	{ 0x54, "reserved" },
	{ 0x55, "Availability status" },
	{ 0x56, "reserved" },
	{ 0x57, "EAC inputs" },
	{ 0x58, "EAC outputs" },
	{ 0x59, "reserved" },
	{ 0x5A, "Position" },
	{ 0x5B, "HW unit identity" },
	{ 0x5C, "RF test signal attenuation" },
	{ 0x5D, "Operational state" },
	{ 0x5E, "Logical object identity" },
	{ 0x5F, "reserved" },
	{ 0x60, "BS_TXPWR_OM" },
	{ 0x61, "Loop_Duration" },
	{ 0x62, "LNA_Path_Selection" },
	{ 0x63, "Serial number" },
	{ 0x64, "HW version" },
	{ 0x65, "Obj. identity and obj. state" },
	{ 0x66, "reserved" },
	{ 0x67, "EAC input definition" },
	{ 0x68, "EAC id and text" },
	{ 0x69, "HW unit status" },
	{ 0x6A, "SW release version" },
	{ 0x6B, "FW version" },
	{ 0x6C, "Bit_Error_Ratio" },
	{ 0x6D, "RXLEV_with_Attenuation" },
	{ 0x6E, "RXLEV_without_Attenuation" },
	{ 0x6F, "reserved" },
	{ 0x70, "CU_Results" },
	{ 0x71, "reserved" },
	{ 0x72, "LNA_Path_Results" },
	{ 0x73, "RTE Results" },
	{ 0x74, "Real Time" },
	{ 0x75, "RX diversity selection" },
	{ 0x76, "EAC input config" },
	{ 0x77, "Feature support" },
	{ 0x78, "File version" },
	{ 0x79, "Outputs" },
	{ 0x7A, "FU parameters" },
	{ 0x7B, "Diagnostic info" },
	{ 0x7C, "FU BSIC" },
	{ 0x7D, "TRX Configuration" },
	{ 0x7E, "Download status" },
	{ 0x7F, "RX difference limit" },
	{ 0x80, "TRX HW capability" },
	{ 0x81, "Common HW config" },
	{ 0x82, "Autoconfiguration pool size" },
	{ 0x83, "TRE diagnostic info" },
	{ 0x84, "TRE object identity" },
	{ 0x85, "New TRE Info" },
	{ 0x86, "Acknowledgement period" },
	{ 0x87, "Synchronization mode" },
	{ 0x88, "reserved" },
	{ 0x89, "Block Control Data" },
	{ 0x8A, "SW load mode" },
	{ 0x8B, "Recommended recovery action" },
	{ 0x8C, "BSC BCF id" },
	{ 0x8D, "Q1 baud rate" },
	{ 0x8E, "Allocation status" },
	{ 0x8F, "Functional entity number" },
	{ 0x90, "Transmission delay" },
	{ 0x91, "Loop Duration ms" },
	{ 0x92, "Logical channel" },
	{ 0x93, "Q1 address" },
	{ 0x94, "Alarm detail" },
	{ 0x95, "Cabinet type" },
	{ 0x96, "HW unit existence" },
	{ 0x97, "RF power parameters" },
	{ 0x98, "Message scenario" },
	{ 0x99, "HW unit max amount" },
	{ 0x9A, "Master TRX" },
	{ 0x9B, "Transparent data" },
	{ 0x9C, "BSC topology info" },
	{ 0x9D, "Air i/f modulation" },
	{ 0x9E, "LCS Q1 command data" },
	{ 0x9F, "Frame number offset" },
	{ 0xA0, "Abis TSL" },
	{ 0xA1, "Dynamic pool info" },
	{ 0xA2, "LCS LLP data" },
	{ 0xA3, "LCS Q1 answer data" },
	{ 0xA4, "DFCA FU Radio Definition" },
	{ 0xA5, "Antenna hopping" },
	{ 0xA6, "Field record sequence number" },
	{ 0xA7, "Timeslot offslot" },
	{ 0xA8, "EPCR capability" },
	{ 0xA9, "Connectsite optional element" },
	{ 0xAA, "TSC" },
	{ 0xAB, "Special TX Power Setting" },
	{ 0xAC, "Optional sync settings" },
	{ 0xAD, "STIRC Option Setting" },
	{ 0xAE, "Rx Difference Samples Count" },
	{ 0xAF, "BTS Object Mapping" },
	{ 0xB0, "Abis Mapping" },
	{ 0xB1, "LAPDm T200 Values" },
	{ 0xB2, "DTRX info" },
	{ 0xB3, "Link and Counters" },
	{ 0xB4, "TRS and BTS licensing" },
	{ 0xB5, "TRX DDU association" },
	{ 0xB6, "Power control reason" },
	{ 0xB7, "CSDAP" },
	{ 0xB8, "BS2 parameters" },
	{ 0xB9, "BSC parameters" },
	{ 0xBA, "BTS parameters" },
	{ 0xBB, "TRX parameters" },
	{ 0xBC, "HO parameters" },
	{ 0xBD, "POC parameters" },
	{ 0xBE, "ADJC parameters" },
	{ 0xBF, "ADJC object" },
	{ 0xC0, "UADJC parameters" },
	{ 0xC1, "UADJC object" },
	{ 0xC2, "BTS PCM id" },
	{ 0xC3, "Segment Object id" },
	{ 0xC4, "Cell load parameters" },
	{ 0xC5, "Dynamic cell load info" },
	{ 0xC6, "BSC Release information" },
	{ 0xC7, "Complete data" },
	{ 0xC8, "OMUSIG PCM info" },
	{ 0xC9, "Line type" },
	{ 0xCA, "Concurrent OMUSIGs priorities" },
	{ 0xCB, "OMUSIG Priority" },
	{ 0xCC, "OSC support" },
	{ 0xCD, "CSDAP id" },
	{ 0xCE, "CSDAP parameters" },
	{ 0xCF, "Local Switching Counters" },
	{ 0xD0, "TX queue handling parameters" },
	{ 0xD1, "Valid History Records" },
	{ 0xD2, "Measurement Operation Status" },
	{ 0xD3, "Abis Delay Counters" },
	{ 0xD4, "Abis Measurement channel" },
	{ 0xD5, "Packet Abis BCF group configuration" },
	{ 0xD6, "Traffic to DSCP mapping" },
	{ 0xD7, "UL Shaping parameters" },
	{ 0xD8, "Traffic scheduling" },
	{ 0xD9, "Random Fill Bits usage" },
	{ 0xDA, "Packet Abis BCF group id" },
	{ 0xDB, "UDP" },
	{ 0xDC, "Backhaul utilization monitor parameters" },
	{ 0xDD, "Packet loss monitor parameters" },
	{ 0xDE, "Congestion control" },
	{ 0xDF, "Congestion reaction hysteresis parameters" },
	{ 0xE0, "Multiplexing parameters" },
	{ 0xE1, "IP addresses" },
	{ 0xE2, "TRX feature capability" },
	{ 0xE3, "Packet Abis If configuration" },
	{ 0xE4, "BTS IPv4 address C/U-plane" },
	{ 0xE5, "IPv4 address" },
	{ 0xE6, "Subnet mask" },
	{ 0xE7, "BSC IPv4 address C-plane" },
	{ 0xE8, "BSC IPv4 address CS U-plane" },
	{ 0xE9, "BSC IPv4 address PS U-plane" },
	{ 0xEA, "MC-PPP class map" },
	{ 0xEB, "SCTP parameters" },
	{ 0xEC, "Packet Abis mapping" },
	{ 0xED, "Sig ID" },
	{ 0xEE, "Packet Abis mapping" },
	{ 0xEF, "Measurement Request Type" },
	{ 0xF0, "Peer BTS info" },
	{ 0xF1, "Packet Abis Counters" },
	{ 0xF2, "Waiting time for BSC restarting" },
	{ 0xF3, "Peer BCF ID" },
	{ 0xF4, "TRX starting mode" },
	{ 0xF5, "GSM-R configuration" },
	{ 0xF6, "DSCP to VLAN priority mapping" },
	{ 0xF7, "TRX Assosiation" },
	{ 0xF8, "Power Finetuning" },
	{ 0xF9, "Abis Delay Measurement UDP port" },
	{ 0xFA, "Abis If parameters" },
	{ 0xFB, "PDV" },
	{ 0xFC, "Horizon BTS parameters" },
	{ 0xFD, "reserved" },
	{ 0xFE, "Radio Module SW" },
	{ 0, NULL }
};

static const struct value_string nokia_bts_types[] = {
	{ 0x0a, 	"MetroSite GSM 900" },
	{ 0x0b,		"MetroSite GSM 1800" },
	{ 0x0c,		"MetroSite GSM 1900 (PCS)" },
	{ 0x0d,		"MetroSite GSM 900 & 1800" },
	{ 0x0e,		"InSite GSM 900" },
	{ 0x0f,		"InSite GSM 1800" },
	{ 0x10,		"InSite GSM 1900" },
	{ 0x11,		"UltraSite GSM 900" },
	{ 0x12,		"UltraSite GSM 1800" },
	{ 0x13,		"UltraSite GSM/US-TDMA 1900" },
	{ 0x14,		"UltraSite GSM 900 & 1800" },
	{ 0x16,		"UltraSite GSM/US-TDMA 850" },
	{ 0x18,		"MetroSite GSM/US-TDMA 850" },
	{ 0x19,		"UltraSite GSM 800/1900" },
	{ 0, 		NULL }
};

static const struct value_string nokia_severity[] = {
	{ 0,	"indeterminate" },
	{ 1,	"critical" },
	{ 2,	"major" },
	{ 3,	"minor" },
	{ 4,	"warning" },
	{ 0,	NULL }
};

static const struct value_string nokia_reset_type[] = {
	{ 0,	"OMU reset" },			/* BTS is still able to carry traffic */
	{ 1,	"Site reset" },
	{ 2,	"Reserved" },
	{ 3,	"Autoconfiguration site reset" },
	{ 4,	"MetroSite VTGA reset" },
	{ 5,	"Total reset" },		/* Complete reset of Packet Abis BTS and Conversion Function reset */
	{ 0,	NULL }
};

static const struct value_string nokia_object_identity[] = {
	{ 0x01,		"BCF" },		/* Base Control Function */
	{ 0x02,		"OMU" },		/* Operation and Maintenance Unit */
	{ 0x04,		"TRX" },		/* Transceiver (FU + CU) */
	{ 0x05,		"BTS" },		/* Base Transceiver Station (1..248) */
	{ 0x07,		"FU" },			/* Frame Unit */
	{ 0x08,		"CU" },			/* Carrier Unit */
	{ 0x10,		"TRE" },		/* Transmission Equipment (0..15) */
	{ 0x11,		"TRU" },		/* Transmission Unit */
	{ 0x13,		"RTSL" },		/* Radio Timeslot of a FU (0..7) */
	{ 0x14,		"DMR" },		/* Digital Microwave Radio Link */
	{ 0x15,		"CF" },			/* Conversion Function */
	{ 0x64,		"RTC" },		/* Remote Tune Combiner */
	{ 0,	NULL }
};

static const struct value_string nokia_object_state[] = {
	{ 0x0,		"Enabled" },
	{ 0x1,		"Disabled" },
	{ 0x2,		"Locked" },
	{ 0x3,		"Unlocked" },
	{ 0x4,		"Shutdown" },		/* reserved for BTS MMI use */
	{ 0x5,		"Powersave" },		/* reserved for BTS MMI use */
	{ 0,	NULL }
};



static uint8_t fu_config_template[] = {
	0x7F, 0x7A, 0x39,
	/* ID = 0x7A (FU parameters) ## constructed ## */
	/* length = 57 */
	/* [3] */

	0x5F, 0x40, 0x04,
	/* ID = 0x40 (Object identity) */
	/* length = 4 */
	/* [6] */
	0x00, 0x07, 0x01, 0xFF,

	0x41, 0x02,
	/* ID = 0x01 (Ny1) */
	/* length = 2 */
	/* [12] */
	0x00, 0x05,

	0x42, 0x02,
	/* ID = 0x02 (T3105_F) */
	/* length = 2 */
	/* [16] */
	0x00, 0x28, /* FIXME: use net->T3105 */

	0x50, 0x02,
	/* ID = 0x10 (T3105_D) */
	/* length = 2 */
	/* [20] */
	0x00, 0x28, /* FIXME: use net->T3105 */

	0x43, 0x05,
	/* ID = 0x03 (Interference band limits) */
	/* length = 5 */
	/* [24] */
	0x0F, 0x1B, 0x27, 0x33, 0x3F,

	0x44, 0x02,
	/* ID = 0x04 (Interference report timer in secs) */
	/* length = 2 */
	/* [31] */
	0x00, 0x10,

	0x47, 0x01,
	/* ID = 0x07 (RACH report timer in secs) */
	/* length = 1 */
	/* [35] */
	0x1E,

	0x4C, 0x10,
	/* ID = 0x0C (Cell allocation bitmap) ####### */
	/* length = 16 */
	/* [38] */
	0x8F, 0xB1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

	0x59, 0x01,
	/* ID = 0x19 (Averaging period) */
	/* length = 1 */
	/* [56] */
	0x01,

	0x5E, 0x01,
	/* ID = 0x1E ((RF max power reduction)) */
	/* length = 1 */
	/* [59] */
	0x00,

	0x7F, 0x46, 0x11,
	/* ID = 0x46 (FU channel configuration) ## constructed ## */
	/* length = 17 */
	/* [63] */

	0x5F, 0x40, 0x04,
	/* ID = 0x40 (Object identity) */
	/* length = 4 */
	/* [66] */
	0x00, 0x07, 0x01, 0xFF,

	0x45, 0x08,
	/* ID = 0x05 (Channel configuration per TS) */
	/* length = 8 */
	/* [72] */
	0x01, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09,

	0x7F, 0x65, 0x0B,
	/* ID = 0x65 (Obj. identity and obj. state) ## constructed ## */
	/* length = 11 */
	/* [83] */

	0x5F, 0x40, 0x04,
	/* ID = 0x40 (Object identity) */
	/* length = 4 */
	/* [86] */
	0x00, 0x04, 0x01, 0xFF,

	0x5F, 0x44, 0x01,
	/* ID = 0x44 (Object current state) */
	/* length = 1 */
	/* [93] */
	0x02,

	0x7F, 0x7C, 0x0A,
	/* ID = 0x7C (FU BSIC) ## constructed ## */
	/* length = 10 */
	/* [97] */

	0x5F, 0x40, 0x04,
	/* ID = 0x40 (Object identity) */
	/* length = 4 */
	/* [100] */
	0x00, 0x07, 0x01, 0xFF,

	0x46, 0x01,
	/* ID = 0x06 (BSIC) */
	/* length = 1 */
	/* [106] */
	0x00,

	0x7F, 0x48, 0x0B,
	/* ID = 0x48 (ARFN of a CU) ## constructed ## */
	/* length = 11 */
	/* [110] */

	0x5F, 0x40, 0x04,
	/* ID = 0x40 (Object identity) */
	/* length = 4 */
	/* [113] */
	0x00, 0x08, 0x01, 0xFF,

	0x4A, 0x02,
	/* ID = 0x0A (ARFN) ####### */
	/* length = 2 */
	/* [119] */
	0x03, 0x62,

	0x7F, 0x49, 0x59,
	/* ID = 0x49 (FU radio definition) ## constructed ## */
	/* length = 89 */
	/* [124] */

	0x5F, 0x40, 0x04,
	/* ID = 0x40 (Object identity) */
	/* length = 4 */
	/* [127] */
	0x00, 0x07, 0x01, 0xFF,

	0x4D, 0x50,
	/* ID = 0x0D (Radio definition per TS) ####### */
	/* length = 80 */
	/* [133] */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	/* MA */
	0x03, 0x62,		/* HSN, MAIO or ARFCN if no hopping */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x03, 0x62,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x03, 0x62,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x03, 0x62,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x03, 0x62,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x03, 0x62,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x03, 0x62,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x03, 0x62,
};


static uint8_t bts_config_1[] = {
	0x4E, 0x02,
	/* ID = 0x0E (Frame number) */
	/* length = 2 */
	/* [2] */
	0xFF, 0xFF,

	0x5F, 0x4E, 0x02,
	/* ID = 0x4E (RX antenna supervision period) */
	/* length = 2 */
	/* [7] */
	0xFF, 0xFF,

	0x5F, 0x50, 0x02,
	/* ID = 0x50 (Sector configuration) */
	/* length = 2 */
	/* [12] */
	0x01, 0x01,
};

static uint8_t bts_config_2[] = {
	0x55, 0x02,
	/* ID = 0x15 (Hopping mode) */
	/* length = 2 */
	/* [2] */
	0x01, 0x00,

	0x5F, 0x75, 0x02,
	/* ID = 0x75 (RX diversity selection) */
	/* length = 2 */
	/* [7] */
	0x01, 0x01,
};

static uint8_t bts_config_3[] = {
	0x5F, 0x20, 0x02,
	/* ID = 0x20 (Extended cell radius) */
	/* length = 2 */
	/* [3] */
	0x01, 0x00,
};

static uint8_t bts_config_4[] = {
	0x5F, 0x74, 0x09,
	/* ID = 0x74 (Real Time) */
	/* length = 9 */
	/* [3] year-high, year-low, month, day, hour, minute, second, msec-high, msec-low */
	0x07, 0xDB, 0x06, 0x02, 0x0B, 0x20, 0x0C, 0x00,
	0x00,

	0x5F, 0x76, 0x03,
	/* ID = 0x76 (EAC input config) */
	/* length = 3 */
	/* [15] */
	0x01, 0x01, 0x00,

	0x5F, 0x76, 0x03,
	/* ID = 0x76 (EAC input config) */
	/* length = 3 */
	/* [21] */
	0x02, 0x01, 0x00,

	0x5F, 0x76, 0x03,
	/* ID = 0x76 (EAC input config) */
	/* length = 3 */
	/* [27] */
	0x03, 0x01, 0x00,

	0x5F, 0x76, 0x03,
	/* ID = 0x76 (EAC input config) */
	/* length = 3 */
	/* [33] */
	0x04, 0x01, 0x00,

	0x5F, 0x76, 0x03,
	/* ID = 0x76 (EAC input config) */
	/* length = 3 */
	/* [39] */
	0x05, 0x01, 0x00,

	0x5F, 0x76, 0x03,
	/* ID = 0x76 (EAC input config) */
	/* length = 3 */
	/* [45] */
	0x06, 0x01, 0x00,

	0x5F, 0x76, 0x03,
	/* ID = 0x76 (EAC input config) */
	/* length = 3 */
	/* [51] */
	0x07, 0x01, 0x00,

	0x5F, 0x76, 0x03,
	/* ID = 0x76 (EAC input config) */
	/* length = 3 */
	/* [57] */
	0x08, 0x01, 0x00,

	0x5F, 0x76, 0x03,
	/* ID = 0x76 (EAC input config) */
	/* length = 3 */
	/* [63] */
	0x09, 0x01, 0x00,

	0x5F, 0x76, 0x03,
	/* ID = 0x76 (EAC input config) */
	/* length = 3 */
	/* [69] */
	0x0A, 0x01, 0x00,
};

static uint8_t bts_config_insite[] = {
	0x4E, 0x02,
	/* ID = 0x0E (Frame number) */
	/* length = 2 */
	/* [2] */
	0xFF, 0xFF,

	0x5F, 0x4E, 0x02,
	/* ID = 0x4E (RX antenna supervision period) */
	/* length = 2 */
	/* [7] */
	0xFF, 0xFF,

	0x5F, 0x50, 0x02,
	/* ID = 0x50 (Sector configuration) */
	/* length = 2 */
	/* [12] */
	0x01, 0x01,

	0x55, 0x02,
	/* ID = 0x15 (Hopping mode) */
	/* length = 2 */
	/* [16] */
	0x01, 0x00,

	0x5F, 0x20, 0x02,
	/* ID = 0x20 (Extended cell radius) */
	/* length = 2 */
	/* [21] */
	0x01, 0x00,

	0x5F, 0x74, 0x09,
	/* ID = 0x74 (Real Time) */
	/* length = 9 */
	/* [26] */
	0x07, 0xDB, 0x07, 0x0A, 0x0F, 0x09, 0x0B, 0x00,
	0x00,
};

static uint8_t download_req[] = {
	0x5F, 0x25, 0x0B,
	/* ID = 0x25 (File identity) */
	/* length = 11 */
	/* [3] */
	0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A,
	0x2A, 0x2A, 0x2A,

	0x5F, 0x78, 0x03,
	/* ID = 0x78 (File version) */
	/* length = 3 */
	/* [17] */
	0x2A, 0x2A, 0x2A,

	0x5F, 0x81, 0x0A, 0x01,
	/* ID = 0x8A (SW load mode) */
	/* length = 1 */
	/* [24] */
	0x01,

	0x5F, 0x81, 0x06, 0x01,
	/* ID = 0x86 (Acknowledgement period) */
	/* length = 1 */
	/* [29] */
	0x01,
};

static uint8_t ack[] = {
	0x5F, 0x23, 0x01,
	/* ID = 0x23 (Ack-Nack) */
	/* length = 1 */
	/* [3] */
	0x01,
};

static uint8_t reset[] = {
	0x5F, 0x40, 0x04,
	/* ID = 0x40 (Object identity) */
	/* length = 4 */
	/* [3] */
	0x00, 0x01, 0xFF, 0xFF,
};

static uint8_t trx_unlock[] = {
	0x7F, 0x65, 0x0B,
	/* ID = 0x65 (Obj. identity and obj. state) ## constructed ## */
	/* length = 11 */
	/* [83] */

	0x5F, 0x40, 0x04,
	/* ID = 0x40 (Object identity) */
	/* length = 4 */
	/* [86] */
	0x00, 0x04, 0x01, 0xFF,

	0x5F, 0x44, 0x01,
	/* ID = 0x44 (Object current state) */
	/* length = 1 */
	/* [93] */
	0x03,
};

static uint8_t trx_reset[] = {
	0x5F, 0x40, 0x04,
	/* ID = 0x40 (Object identity) */
	/* length = 4 */
	/* [3] */
	0x00, 0x04, 0x01, 0xFF,
};

struct abis_om_nokia_hdr {
	uint8_t msg_type;
	uint8_t spare;
	uint16_t reference;
	uint8_t data[0];
} __attribute__ ((packed));

static struct gsm_network *my_net;


/* Functions */
extern int abis_nm_sendmsg(struct gsm_bts *bts, struct msgb *msg);
static void nokia_abis_nm_queue_send_next(struct gsm_bts *bts);
static void reset_timer_cb(void *_bts);
static int abis_nm_reset(struct gsm_bts *bts, uint16_t ref);
static int dump_elements(uint8_t * data, int len) __attribute__((unused));
static void bootstrap_om_bts(struct gsm_bts *bts);
static void bootstrap_om_trx(struct gsm_bts_trx *trx);

static int abis_nm_cha_adm_trx_lock(struct gsm_bts *bts, uint8_t trx_id, uint16_t ref);
static int shutdown_om(struct gsm_bts *bts);
static void start_sabm_in_line(struct e1inp_line *line, int start, int sapi);
static int gbl_sig_cb(unsigned int subsys, unsigned int signal,
		      void *handler_data, void *signal_data);
static int inp_sig_cb(unsigned int subsys, unsigned int signal,
		      void *handler_data, void *signal_data);
static void nm_statechg_evt(unsigned int signal,
			    struct nm_statechg_signal_data *nsd);
static int nm_sig_cb(unsigned int subsys, unsigned int signal,
		     void *handler_data, void *signal_data);
static const char *get_msg_type_name_string(uint8_t msg_type);
static const char *get_element_name_string(uint16_t element);
static const char *get_bts_type_string(uint8_t type);
static const char *get_severity_string(uint8_t severity);
static const char *get_reset_type_string(uint8_t reset_type);
static const char *get_object_identity_string(uint16_t object_identity);
static const char *get_object_state_string(uint8_t object_state);
static int make_fu_config(struct gsm_bts_trx *trx, uint8_t id,
			  uint8_t * fu_config, int *hopping);
void set_real_time(uint8_t * real_time);
static int make_bts_config(struct gsm_bts *bts, uint8_t bts_type, int n_trx, uint8_t * fu_config,
			   int need_hopping, int hopping_type);
static struct msgb *nm_msgb_alloc(void);
static int abis_nm_send(struct gsm_bts *bts, uint8_t msg_type, uint16_t ref,
			uint8_t * data, int len_data);
static int abis_nm_download_req(struct gsm_bts *bts, uint16_t ref);
static int abis_nm_ack(struct gsm_bts *bts, uint16_t ref);
static int abis_nm_reset(struct gsm_bts *bts, uint16_t ref);
static int abis_nm_cha_adm_trx_unlock(struct gsm_bts *bts, uint8_t trx_id, uint16_t ref);
static int abis_nm_trx_reset(struct gsm_bts *bts, uint8_t trx_id, uint16_t ref);
static int abis_nm_send_multi_segments(struct gsm_bts *bts, uint8_t msg_type,
				       uint16_t ref, uint8_t * data, int len);
static int abis_nm_send_config(struct gsm_bts *bts, uint8_t bts_type);
static int find_element(uint8_t * data, int len, uint16_t id, uint8_t * value,
			int max_value);
static int dump_elements(uint8_t * data, int len);
static void mo_ok(struct gsm_abis_mo *mo);
static void nokia_abis_nm_fake_1221_ok(struct gsm_bts *bts);
static void nokia_abis_nm_queue_send_next(struct gsm_bts *bts);
static void reset_timer_cb(void *_bts);
static int abis_nm_rcvmsg_fom(struct msgb *mb);
int abis_nokia_rcvmsg(struct msgb *msg);
static int bts_model_nokia_site_start(struct gsm_network *net);
static void bts_model_nokia_site_e1line_bind_ops(struct e1inp_line *line);
static int bts_model_nokia_site_start(struct gsm_network *net);
int bts_model_nokia_site_init(void);

/* AX25 header tracing
 * Copyright 1991 Phil Karn, KA9Q
 */
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "beacony.h"

#define	LAPB_UNKNOWN	0
#define	LAPB_COMMAND	1
#define	LAPB_RESPONSE	2

#define	SEG_FIRST	0x80
#define	SEG_REM		0x7F

#define	PID_SEGMENT	0x08
#define	PID_ARP		0xCD
#define	PID_NETROM	0xCF
#define	PID_IP		0xCC
#define	PID_X25		0x01
#define	PID_TEXNET	0xC3
#define	PID_FLEXNET	0xCE
#define	PID_OPENTRAC	0x77
#define	PID_NO_L3	0xF0

#define	I		0x00
#define	S		0x01
#define	RR		0x01
#define	RNR		0x05
#define	REJ		0x09
#define	U		0x03
#define	SABM		0x2F
#define	SABME		0x6F
#define	DISC		0x43
#define	DM		0x0F
#define	UA		0x63
#define	FRMR		0x87
#define	UI		0x03
#define	PF		0x10
#define	EPF		0x01

#define	MMASK		7

#define	HDLCAEB		0x01
#define	SSID		0x1E
#define	REPEATED	0x80
#define	C		0x80
#define	SSSID_SPARE	0x40
#define	ESSID_SPARE	0x20

#define	ALEN		6
#define	AXLEN		7

#define	W		1
#define	X		2
#define	Y		4
#define	Z		8

static int ftype(unsigned char *, int *, int *, int *, int *, int);

#define NDAMA_STRING ""
#define DAMA_STRING " [DAMA]"

/*
 * 1: fm AG7EW to KG7OEM ctl UI^ pid=F0(Text) len 17
*/

/* FlexNet header compression display by Thomas Sailer t.sailer@alumni.ethz.ch */

/* Dump an AX.25 packet header */
void ax25_dump(unsigned char *data, int length, int hexdump)
{
	char heading[100];
	char tmp[15];
	char tmp2[15];
	int ctlen, nr, ns, pf, pid, seg, type, end, cmdrsp, extseq;
	char *dama;
	memset(heading, 0, sizeof(heading));

	/* check for FlexNet compressed header first; FlexNet header compressed packets are at least 8 bytes long */
	if (length < 8) {
		/* Something wrong with the header */
		lprintf(T_ERROR, "AX25: bad header!\n");
		return;
	}
	if (data[1] & HDLCAEB) {
		/* this is a FlexNet compressed header */
		tmp[6] = tmp[7] = extseq = 0;
		tmp[0] = ' ' + (data[2] >> 2);
		tmp[1] = ' ' + ((data[2] << 4) & 0x30) + (data[3] >> 4);
		tmp[2] = ' ' + ((data[3] << 2) & 0x3c) + (data[4] >> 6);
		tmp[3] = ' ' + (data[4] & 0x3f);
		tmp[4] = ' ' + (data[5] >> 2);
		tmp[5] = ' ' + ((data[5] << 4) & 0x30) + (data[6] >> 4);
		data += 7;
		length -= 7;
	} else {
		pax25(tmp, data + AXLEN);
                pax25(tmp2, data);
		snprintf(heading, sizeof(heading), "%s\t%s\t", tmp, tmp2);
		end = (data[AXLEN + ALEN] & HDLCAEB);
		data += (AXLEN + AXLEN);
		length -= (AXLEN + AXLEN);
		if (!end) {
			while (!end) {
				/* Print digi string */
				end = (data[ALEN] & HDLCAEB);
				data += AXLEN;
				length -= AXLEN;
			}
		}
	}

	if (length == 0)
		return;

	ctlen = ftype(data, &type, &ns, &nr, &pf, extseq);

	data += ctlen;
	length -= ctlen;

	if (type == I || type == UI) {
		/* Decode I field */
		if (length > 0) {	/* Get pid */
			pid = *data++;
			length--;

			if (timestamp)
				display_timestamp();

			if (pid == PID_SEGMENT) {
				length--;

				if (seg & SEG_FIRST) {
					pid = *data++;
					length--;
				}
			}

			switch (pid) {
				case PID_NO_L3:
					printf("%s", heading);
					data_dump(data, length, hexdump);
					break;
				default:
					break;
			}
		}
	} 
}

char *pax25(char *buf, unsigned char *data)
{
	int i, ssid;
	char *s;
	char c;

	s = buf;

	for (i = 0; i < ALEN; i++) {
		c = (data[i] >> 1) & 0x7F;

		if (!isalnum(c) && c != ' ') {
			strcpy(buf, "[invalid]");
			return buf;
		}

		if (c != ' ')
			*s++ = c;
	}

	if ((ssid = (data[ALEN] & SSID)) != 0)
		sprintf(s, "-%d", ssid >> 1);
	else
		*s = '\0';

	return (buf);
}

static int ftype(unsigned char *data, int *type, int *ns, int *nr, int *pf,
		 int extseq)
{
	*ns = *nr = 0;				/* To avoid warnings  */

	if (extseq) {
		if ((*data & 0x01) == 0) {	/* An I frame is an I-frame ... */
			*type = I;
			*ns = (*data >> 1) & 127;
			data++;
			*nr = (*data >> 1) & 127;
			*pf = *data & EPF;
			return 2;
		}
		if (*data & 0x02) {
			*type = *data & ~PF;
			*pf = *data & PF;
			return 1;
		} else {
			*type = *data;
			data++;
			*nr = (*data >> 1) & 127;
			*pf = *data & EPF;
			return 2;
		}
	} else {
		if ((*data & 0x01) == 0) {	/* An I frame is an I-frame ... */
			*type = I;
			*ns = (*data >> 1) & 7;
			*nr = (*data >> 5) & 7;
			*pf = *data & PF;
			return 1;
		}
		if (*data & 0x02) {	/* U-frames use all except P/F bit for type */
			*type = *data & ~PF;
			*pf = *data & PF;
			return 1;
		} else {	/* S-frames use low order 4 bits for type */
			*type = *data & 0x0F;
			*nr = (*data >> 5) & 7;
			*pf = *data & PF;
			return 1;
		}
	}
}

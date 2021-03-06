#include <stdio.h>
#include <string.h>

#include "common.h"
#include "udp.h"
#include "dns.h"

/******
 ******
 ******/

uint16_t
udp_checksum(myipudp_t *udpip)
{
	uint16_t	oldchksum, newchksum;
	uint16_t	*srcip2, *dstip2;
	uint32_t	sum;
	int				udplen;

	udplen = swap16(udpip->udp_length);
	/* checksum: pseudo header */
	srcip2 = (uint16_t *) udpip->ip_srcip;
	dstip2 = (uint16_t *) udpip->ip_dstip;
	sum = swap16(*srcip2) + swap16(*(srcip2 + 1))
		+ swap16(*dstip2) + swap16(*(dstip2 + 1)) + 0x11 + udplen;
	sum = (sum >> 16) + (sum & 0xffff);
	sum = (sum >> 16) + (sum & 0xffff);

	/* checksum: udp packet */
	oldchksum = udpip->udp_chksum;
	udpip->udp_chksum = swap16((uint16_t) sum);
	newchksum = checksum((char *)&udpip->udp_srcport, udplen);
	udpip->udp_chksum = oldchksum;

	return newchksum;
}

void
udp_main(pcap_t *fp, myip_t *ip, int len)
{
#if(DEBUG_CHECKSUM == 1)
	uint16_t int	chk = udp_checksum((myipudp_t *) ip);
#else
	uint16_t	chk = 0;
#endif /* DEBUG_CHECKSUM */
	myudp_t				*udp = (myudp_t *) ip->data;
	int					udplen = len - hlen(ip) * 4;
	uint16_t	srcport, dstport;

	srcport = swap16(udp->udp_srcport);
	dstport = swap16(udp->udp_dstport);

#if(DEBUG_UDP == 1 || DEBUG_CHECKSUM == 1)
	printf("UDP: %d->%d, Len=%d, chksum=%04x/%04x\n", srcport,
		dstport, udplen, (int) udp->udp_chksum, chk);
#endif /* DEBUG_UDP == 1 || DEBUG_CHECKSUM == 1*/
#if(DEBUG_PACKET_DUMP == 0 && DEBUG_IP_DUMP == 0 && DEBUG_UDP_DUMP == 1)
		print_data((uint8_t *)udp, udplen);
#endif /* DEBUG_UDP_DUMP */

	if(srcport == 53 || srcport == 997) {
		dns_main(fp, ip, udp, udplen);
	}
}

void
udp_send(pcap_t *fp, uint16_t srcport, uint32_t dstip,
		uint16_t dstport, char *data, int len)
{
	myethip_t	pktbuf, *pkt;
	myudp_t		*udp;
	myeth_t		*eth;
	int			udplen = len + 8;

	pkt = &pktbuf;
	pkt->ip_protocol = 0x11; /* UDP */
	setip(pkt->ip_dstip, &dstip);
	setip(pkt->ip_srcip, myipaddr);

	udp = (myudp_t *) pkt->data;
	udp->udp_srcport = swap16(srcport);
	udp->udp_dstport = swap16(dstport);
	udp->udp_length = swap16(udplen);
	memcpy(udp->udp_data, data, len);

	eth = (myeth_t *) &pktbuf;
	udp->udp_chksum = udp_checksum((myipudp_t *)eth->data);
#if(DEBUG_UDP)
	printf("udp_send(): %d->%s:%d, Len=%d, chksum=%04x\n", (int)srcport,
	ip_addrstr(pkt->ip_dstip, NULL), (int)dstport, udplen, udp->udp_chksum);
#endif /* DEBUG_UDP */
#if(DEBUG_PACKET_DUMP == 0 && DEBUG_IP_DUMP == 0 && DEBUG_UDP_DUMP == 1)
		print_data((uint8_t *)udp, udplen);
#endif /* DEBUG_UDP_DUMP */
	ip_send(fp, pkt, udplen);
}

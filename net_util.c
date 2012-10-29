/*
 Hive Drivers
 Copyright (C) 2008-2012 Hive Solutions Lda.

 This file is part of Hive Drivers.

 Hive Drivers is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 Hive Drivers is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with Hive Drivers. If not, see <http://www.gnu.org/licenses/>.

 __author__    = João Magalhães <joamag@hive.pt>
 __version__   = 1.0.0
 __revision__  = $LastChangedRevision$
 __date__      = $LastChangedDate$
 __copyright__ = Copyright (c) 2008-2012 Hive Solutions Lda.
 __license__   = GNU General Public License (GPL), Version 3
*/

#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>

#include "net_util.h"

short icmp_checksum_c(unsigned short *buffer, unsigned int len) {
    unsigned long sum = 0;
    short answer = 0;

    while(len > 1) {
        sum += *buffer++;
        len -= 2;
    }

    if(len == 1) {
        *(char *) (&answer) = *(char *) buffer;
        sum += answer;
    }

    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    answer = ~sum;

    return answer;
}

unsigned short udp_checksum_c(unsigned short len_udp, unsigned char *src_addr, unsigned char *dest_addr, bool padding, unsigned char *buff) {
    unsigned short prot_udp = 17;
    unsigned short padd = 0;
    unsigned short word16;
    unsigned int sum;
    unsigned int i;

    /* finds out if the length of data is even or odd number, in
    case it's odd, adds a padding byte = 0 at the end of packet */
    if((padding & 1) == 1) {
        padd = 1;
        buff[len_udp] = 0;
    }

    /* initialize sum to zero */
    sum = 0;

    /* make 16 bit words out of every two adjacent 8 bit words and
    calculate the sum of all 16 bit words */
    for(i = 0; i < len_udp + padd; i = i + 2) {
        unsigned short value = buff[i];
        word16 = ((value << 8) & 0xFF00) + (unsigned short) (buff[i + 1] & 0xFF);
        sum = sum + (unsigned short) word16;
    }

    /* adds the udp pseudo header which contains the ip
    source and destination addresses */
    for(i = 0; i < 4; i = i + 2) {
        unsigned short value = src_addr[i];
        word16 = ((value << 8) & 0xFF00) + (unsigned short) (src_addr[i + 1] & 0xFF);
        sum = sum + word16;
    }

    for(i = 0; i < 4; i = i + 2) {
        unsigned short value = dest_addr[i];
        word16 = ((value << 8) & 0xFF00) + (unsigned short) (dest_addr[i + 1] & 0xFF);
        sum = sum + word16;
    }

    /* the protocol number and the length of the udp packet */
    sum = sum + prot_udp + len_udp;

    /* keeps only the last 16 bits of the 32 bit calculated
    sum and add the carries */
    while(sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    /* takes the one's complement of sum */
    sum = ~sum;

    /* returns the sum as unsigned short */
    return (unsigned short) sum;
}

void print_header_c(struct sk_buff *skb, unsigned char *mac_header) {
    /* allocates space for the counter to be
    used for iterations */
    size_t i;

    printk("Header (%d): 0x", skb->mac_len);
    for(i = 0; i < skb->mac_len; i++) {
        unsigned char head_value = mac_header[i];
        printk("%02X ", head_value);
    }
    printk("\n");
}

void print_data_c(struct sk_buff *skb, unsigned char *data) {
    /* allocates space for the counter to be
    used for iterations */
    size_t i;

    printk("Data (%d/%d): 0x", skb->len, skb->data_len);
    for(i = 0; i < skb->len; i++) {
        unsigned char value = data[i];
        printk("%02X ", value);
    }
    printk("\n");
}

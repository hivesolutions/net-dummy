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

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/init.h>
#include <linux/moduleparam.h>
#include <linux/rtnetlink.h>
#include <linux/jiffies.h>
#include <net/rtnetlink.h>

#define MAC_ADDRESS_BUFFER_SIZE 6
#define IP_ADDRESS_BUFFER_SIZE 4
#define SUM_ADDRESS_BUFFER_SIZE 10

/* the variable to support the number of dummies */
static int num_dummies = 1;

/**
 * Function called to set the address, in this case only the mac
 * address to the device once the initialization is complete.
 *
 * The parameters argument is assumet to be a socket structure.
 *
 * @param dev The device to be used for the setting of the address.
 * @param parameters The parameters to the setting of the address, this
 * value is assumed to be a socket structure.
 * @return The reulst of the setting of the address.
 */
static int dummy_set_address(struct net_device *dev, void *parameters);

/**
 * Function called to set the multicast address in the provided
 * device.
 *
 * Under the current module this call as no effect.
 *
 * @param dev The device to be used for the setting of the address.
 */
static void dummy_set_multicast(struct net_device *dev);

/**
 * Runs the setup operation in the current device, after
 * this operation the default and base configuration should
 * be set on the device.
 *
 * @param dev The pointer to the device to be configured.
 */
static void dummy_setup(struct net_device *dev);
static int dummy_xmit(struct sk_buff *skb, struct net_device *dev);
static int dummy_validate(struct nlattr *tb[], struct nlattr *data[]);
static int __init dummy_init_one(void);
static int __init dummy_init_module(void);
static void __exit dummy_cleanup_module(void);
short icmp_checksum_c(unsigned short *buffer, unsigned int len);
unsigned short udp_checksum_c(unsigned short len_udp, unsigned char *src_addr, unsigned char *dest_addr, bool padding, unsigned char *buff);
void print_header_c(struct sk_buff *skb, unsigned char *mac_header);
void print_data_c(struct sk_buff *skb, unsigned char *data);

/* sets the module license and then sets
the alias (name) of the module */
MODULE_LICENSE("GPL");
MODULE_ALIAS_RTNL_LINK("net_dummy");

/* sets the module's init and exit functions
that are called at such workflow "positions" */
module_init(dummy_init_module);
module_exit(dummy_cleanup_module);

short packet_number = 0x0001;

static int dummy_set_address(struct net_device *dev, void *parameters) {
    /* retrieves the socket address from the parameters */
    struct sockaddr *socket_address = parameters;

    /* in case the ethernet address is not valid, must
    return immediately in error */
    if(!is_valid_ether_addr(socket_address->sa_data)) {
        return -EADDRNOTAVAIL;
    }

    /* copies the socket (mac) address to the device address
    and then returns normally */
    memcpy(dev->dev_addr, socket_address->sa_data, ETH_ALEN);
    return 0;
}

static void dummy_set_multicast(struct net_device *dev) {
    printk("dummy_set_multicast()\n");
}

static void dummy_setup(struct net_device *dev) {
    /* initializes the various values for
    the device structure */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
    dev->hard_start_xmit = dummy_xmit;
    dev->set_multicast_list = dummy_set_multicast;
    dev->set_mac_address = dummy_set_address;
#endif
    dev->hard_header_len = ETH_HLEN;
    dev->addr_len = ETH_ALEN;
    dev->destructor = free_netdev;

    printk("dummy_setup()\n");

    /* fills in device structure with ethernet
    generic values */
    ether_setup(dev);
    dev->tx_queue_len = 0;

    /* sets the maximum transmit unit, this should
    be the normal value */
    dev->mtu = 1500;

    /* generates a random ethernet address for the
    device (stadndard call) */
    random_ether_addr(dev->dev_addr);
}

static int dummy_xmit(struct sk_buff *skb, struct net_device *dev) {
    unsigned char sender_mac_buffer[MAC_ADDRESS_BUFFER_SIZE];
    unsigned char receiver_mac_buffer[MAC_ADDRESS_BUFFER_SIZE];
    unsigned char sender_sum_buffer[SUM_ADDRESS_BUFFER_SIZE];
    unsigned char receiver_sum_buffer[SUM_ADDRESS_BUFFER_SIZE];
    unsigned char sender_ip_buffer[IP_ADDRESS_BUFFER_SIZE];
    unsigned char receiver_ip_buffer[IP_ADDRESS_BUFFER_SIZE];
    unsigned short sender_port;
    unsigned short receiver_port;
    unsigned char ip_packet_header_size;
    unsigned int ip_packet_size;
    unsigned char ip_packet_type;
    unsigned short ip_header_checksum;
    unsigned int icmp_packet_size;
    unsigned char icmp_packet_type;
    unsigned short icmp_checksum;
    unsigned short udp_packet_size;
    unsigned short udp_checksum;
    int packet_propagation_value;
    unsigned int frame_size;
    struct sk_buff *skb_clone;
    char *frame_buffer;
    unsigned char *mac_header;
    unsigned char *data;

    /* increments the transmission information */
    dev->stats.tx_packets++;
    dev->stats.tx_bytes += skb->len;

    /* increments the receiving information */
    dev->stats.rx_packets++;
    dev->stats.rx_bytes += skb->len;

    /* sets the last received time */
    dev->last_rx = jiffies;

    /* sets the skb as orphan removing the owner */
    skb_orphan(skb);

    /* sets the skb protocol as ethernet */
    skb->protocol = eth_type_trans(skb, dev);

    /* sets the mac header length */
    skb->mac_len = ETH_HLEN;

    /* retrieves both the data and the mac header, by
    casting them into byte buffers, required because
    they may assume multiple data types */
#ifdef NET_SKBUFF_DATA_USES_OFFSET
    data = (unsigned char *) &skb->data;
    mac_header = (unsigned char *) &skb->mac_header;
#else
    data = skb->data;
    mac_header = skb->mac_header;
#endif

    /* prints the information on the mac heder an on
    the data that was just received from the skb */
    print_header_c(skb, mac_header);
    print_data_c(skb, data);

    /* saves the receiver and serder mac buffers so that a switch between
    the receiver and sender of the packet is possible */
    memcpy(receiver_mac_buffer, &(mac_header[0]), MAC_ADDRESS_BUFFER_SIZE);
    memcpy(sender_mac_buffer, &(mac_header[6]), MAC_ADDRESS_BUFFER_SIZE);

    /* switches the sender and the receiver of the packet to ensure that
    the packet is returned (response) */
    memcpy(&(mac_header[0]), sender_mac_buffer, MAC_ADDRESS_BUFFER_SIZE);
    memcpy(&(mac_header[6]), receiver_mac_buffer, MAC_ADDRESS_BUFFER_SIZE);

    /* in case it's an arp request */
    if(mac_header[12] == 0x08 && mac_header[13] == 0x06) {
        printk("Received packet of type ARP\n");

        /* sets the reply opcode */
        data[7] = 0x02;

        memcpy(sender_sum_buffer, &(data[8]), SUM_ADDRESS_BUFFER_SIZE);
        memcpy(receiver_sum_buffer, &(data[18]), SUM_ADDRESS_BUFFER_SIZE);

        memcpy(&(data[8]), receiver_sum_buffer, SUM_ADDRESS_BUFFER_SIZE);
        memcpy(&(data[18]), sender_sum_buffer, SUM_ADDRESS_BUFFER_SIZE);
    }
    /* in case it's an ip request */
    else if(mac_header[12] == 0x08 && mac_header[13] == 0x00) {
        printk("Received packet of type IP\n");

        /* calculates the ip packet header size */
        ip_packet_header_size = (data[0] & 0x0f) * 4;

        /* retrieves the ip packet size */
        ip_packet_size = (((unsigned int) data[2]) << 8) + (unsigned int) data[3];

        printk("Header size of packet of type IP: %d\n", ip_packet_header_size);
        printk("Size of packet of type IP: %d\n", ip_packet_size);

        memcpy(sender_ip_buffer, &(data[12]), IP_ADDRESS_BUFFER_SIZE);
        memcpy(receiver_ip_buffer, &(data[16]), IP_ADDRESS_BUFFER_SIZE);

        memcpy(&(data[12]), receiver_ip_buffer, IP_ADDRESS_BUFFER_SIZE);
        memcpy(&(data[16]), sender_ip_buffer, IP_ADDRESS_BUFFER_SIZE);

        /* sets the packet number in the ip packet */
        data[4] = (unsigned char) (packet_number >> 8 & 0x00FF);
        data[5] = (unsigned char) (packet_number & 0x00FF);

        /* increments the packet number */
        packet_number++;

        /* sets the ip packet flag */
        data[6] = 0x00;

        /* sets the checksum header bytes to zero */
        data[10] = 0x00;
        data[11] = 0x00;

        /* computes the new ip header checksum */
        ip_header_checksum = icmp_checksum_c((unsigned short *) data, ip_packet_header_size);

        data[10] = (unsigned char) (ip_header_checksum & 0x00FF);
        data[11] = (unsigned char) (ip_header_checksum >> 8 & 0x00FF);

        /* retrieves the ip packet type */
        ip_packet_type = data[9];

        /* in case the request is of type icmp */
        if(ip_packet_type == 0x01) {
            printk("Received packet of type ICMP\n");

            /* computes the icmp packet size */
            icmp_packet_size = ip_packet_size - ip_packet_header_size;

            printk("Size of packet of type ICMP: %d\n", icmp_packet_size);

            /* retrieves the icmp packet type */
            icmp_packet_type = data[ip_packet_header_size];

            /* in case it's a ping request */
            if(icmp_packet_type == 0x08) {
                printk("Received packet of type PING\n");

                /* sets the new icmp packet type to ping reply */
                data[ip_packet_header_size] = 0x00;

                /* sets the checksum header bytes to zero */
                data[ip_packet_header_size + 2] = 0x00;
                data[ip_packet_header_size + 3] = 0x00;

                /* computes the new icmp checksum */
                icmp_checksum = icmp_checksum_c((unsigned short *) &(data[ip_packet_header_size]), icmp_packet_size);

                printk("Computed new ICMP checksum as: 0x%X\n", icmp_checksum);

                data[ip_packet_header_size + 2] = (unsigned char) (icmp_checksum & 0x00FF);
                data[ip_packet_header_size + 3] = (unsigned char) (icmp_checksum >> 8 & 0x00FF);
            }
        }
        /* in case the request is of type udp */
        else if(ip_packet_type == 0x11) {
            printk("Received packet of type UDP\n");

            /* computes the udp packet sizeb */
            udp_packet_size = ((unsigned short) data[ip_packet_header_size + 4] << 8) + data[ip_packet_header_size + 5];

            printk("Size of packet of type UDP: %d\n", udp_packet_size);

            memcpy(&sender_port, &(data[ip_packet_header_size]), 2);
            memcpy(&receiver_port, &(data[ip_packet_header_size + 2]), 2);

            memcpy(&(data[ip_packet_header_size]), &receiver_port, 2);
            memcpy(&(data[ip_packet_header_size + 2]), &sender_port, 2);

            if(udp_packet_size - 8 == 16) {
                printk("Received packet of type ARITHMETIC\n");

                unsigned int *base_value = (unsigned int *) &(data[ip_packet_header_size + 8]);

                unsigned int type = base_value[0];
                unsigned int first_value = base_value[1];
                unsigned int second_value = base_value[2];
                unsigned int operator = base_value[3];

                if(type == 1) {
                    printk("Received packet of type ARITHMETIC-OP\n");
                    printk("The ARITHMETIC-OP values are %u, %u, %u, %u\n", type, first_value, second_value, operator);

                    switch(operator) {
                        case 1:
                            base_value[1] = first_value + second_value;
                            break;
                        case 2:
                            base_value[1] = first_value - second_value;
                            break;
                        default:
                            break;
                    }

                    base_value[0] = 2;
                    base_value[2] = 0x000000;
                    base_value[3] = 0x000000;
                }
            }
            data[ip_packet_header_size + 6] = 0x00;
            data[ip_packet_header_size + 7] = 0x00;

            /* computes the new udp checksum */
            udp_checksum = udp_checksum_c(udp_packet_size, sender_ip_buffer, receiver_ip_buffer, false, &(data[ip_packet_header_size]));

            printk("Computed new UDP checksum as: 0x%X\n", udp_checksum);

            data[ip_packet_header_size + 6] = (unsigned char) (udp_checksum >> 8 & 0x00FF);
            data[ip_packet_header_size + 7] = (unsigned char) (udp_checksum & 0x00FF);
        }
    }

    /* calculates the frame size */
    frame_size = skb->len + ETH_HLEN;

    /* clones the socket buffer */
    skb_clone = dev_alloc_skb(frame_size);

    /* allocates space for the frame buffer */
    frame_buffer = kmalloc(frame_size, GFP_KERNEL);

    /* copies the header part of the frame */
    memcpy(frame_buffer, mac_header, ETH_HLEN);

    /* copies the data part of the frame */
    memcpy(&(frame_buffer[ETH_HLEN]), data, skb->len);

    /* sets the device of the socket buffer */
    skb_clone->dev = dev;

    /* copies the frame buffer to the socket buffer clone */
    memcpy(skb_clone->data, frame_buffer, frame_size);

    /* releases the frame buffer memory */
    kfree(frame_buffer);

    /* puts the frame size in the socket buffer */
    skb_put(skb_clone, frame_size);

    /* sets the socket buffer protocol */
    skb_clone->protocol = eth_type_trans(skb_clone, dev);

    /* propagates the packet and retrieves the result */
    packet_propagation_value = netif_rx(skb_clone);

    switch(packet_propagation_value) {
        case NET_RX_DROP:
            printk("The packet was dropped while in propagation\n");
            break;
        case NET_RX_SUCCESS:
            printk("The packet was sent successfully\n");
            break;
        default:
            printk("Unknown status for the packet\n");
            break;
    }

    printk("\n");

    return 0;
}

static int dummy_validate(struct nlattr *tb[], struct nlattr *data[]) {
    printk("Going for validation\n");

    if(tb[IFLA_ADDRESS]) {
        if(nla_len(tb[IFLA_ADDRESS]) != ETH_ALEN) {
            return -EINVAL;
        }
        if(!is_valid_ether_addr(nla_data(tb[IFLA_ADDRESS]))) {
            return -EADDRNOTAVAIL;
        }
    }

    printk("Validation passed with success\n");

    return 0;
}

static struct rtnl_link_ops dummy_link_ops __read_mostly = {
    .kind = "dummy",
    .setup = dummy_setup,
    .validate = dummy_validate,
};

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
static const struct net_device_ops dummy_net_ops = {
    .ndo_start_xmit = dummy_xmit,
};
#endif

/* number of dummy devices to be set up by this module */
module_param(num_dummies, int, 0);
MODULE_PARM_DESC(num_dummies, "Number of dummy pseudo devices");

static int __init dummy_init_one(void) {
    struct net_device *dev_dummy;
    int err;

    printk("alloc_netdev()\n");

    dev_dummy = alloc_netdev(0, "net_dummy%d", dummy_setup);
    if(!dev_dummy) { return -ENOMEM; }

    printk("dev_alloc_name()\n");

    err = dev_alloc_name(dev_dummy, dev_dummy->name);
    if(err < 0) { goto err; }

    printk("register_netdevice() %ld\n", dev_dummy);

    dev_dummy->rtnl_link_ops = &dummy_link_ops;
    err = register_netdevice(dev_dummy);
    if(err < 0) { goto err; }
    return 0;

err:
    free_netdev(dev_dummy);
    return err;
}

static int __init dummy_init_module(void) {
    int i;
    int err = 0;

    rtnl_lock();
    err = __rtnl_link_register(&dummy_link_ops);

    printk("dummy_init_module()\n");

    for(i = 0; i < num_dummies && !err; i++) {
        err = dummy_init_one();
    }

    if(err < 0) {
        __rtnl_link_unregister(&dummy_link_ops);
    }

    rtnl_unlock();

    printk("dummy_init_module() end\n");

    return err;
}

static void __exit dummy_cleanup_module(void) {
    printk("dummy_cleanup_module()\n");

    rtnl_link_unregister(&dummy_link_ops);
}

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

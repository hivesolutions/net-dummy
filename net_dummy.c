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

#include "common.h"

#include "net_dummy.h"

/**
 * Structure that defines statistics to be used
 * in a per cpu philosophy.
 */
struct pcpu_dstats {
    u64 rx_packets;
    u64 tx_packets;
    u64 rx_bytes;
    u64 tx_bytes;
    struct u64_stats_sync syncp;
};

static const struct net_device_ops dummy_netdev_ops = {
    .ndo_init = dummy_dev_init,
    .ndo_uninit = dummy_dev_uninit,
    .ndo_start_xmit = dummy_xmit,
    .ndo_validate_addr = eth_validate_addr,
    .ndo_set_rx_mode = dummy_set_multicast,
    .ndo_set_mac_address = dummy_set_address,
    .ndo_get_stats64 = dummy_get_stats64,
};

static struct rtnl_link_ops dummy_link_ops __read_mostly = {
    .kind = "dummy",
    .setup = dummy_setup,
    .validate = dummy_validate,
};

/**
 * The number of devices to be registered for
 * the current driver, by default this value
 * should be one.
 */
static int num_devices = 1;

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
}

static struct rtnl_link_stats64 *dummy_get_stats64(struct net_device *dev, struct rtnl_link_stats64 *stats) {
    /* initializes the index counter to be used
    in the iteration over the cpus */
    int index;

    /* iterates over each of the possible cpus
    to update the information on each of them
    (each cpu contains a statistics structure) */
    for_each_possible_cpu(index) {
        const struct pcpu_dstats *dstats;
        u64 rbytes, tbytes, rpackets, tpackets;
        unsigned int start;

        dstats = per_cpu_ptr(dev->dstats, index);
        do {
            start = u64_stats_fetch_begin(&dstats->syncp);
            rbytes = dstats->rx_bytes;
            tbytes = dstats->tx_bytes;
            rpackets = dstats->rx_packets;
            tpackets = dstats->tx_packets;
        } while(u64_stats_fetch_retry(&dstats->syncp, start));

        stats->rx_bytes += rbytes;
        stats->tx_bytes += tbytes;
        stats->rx_packets += rpackets;
        stats->tx_packets += tpackets;
    }

    /* returns the updated statistics structure
    to the caller function */
    return stats;
}

static void dummy_xmit_p(struct sk_buff *skb, struct net_device *dev) {
    int propagation;
    unsigned int frame_size;
    char *frame_buffer;
    struct sk_buff *skb_clone;

    /* retrieves the pointer reference to the mac header
    and dat to be used in the processing of the message */
    unsigned char *mac_header = skb->head + MAC_HEADER_OFFSET;
    unsigned char *data = skb->data;

    /* calculates the frame size using both the length
    of the data part and the length of the header, then
    allocates a new socket buffer for it */
    frame_size = skb->len + ETH_HLEN;
    skb_clone = dev_alloc_skb(frame_size);
    frame_buffer = kmalloc(frame_size, GFP_ATOMIC);
    
    /* in case either the skb clone or the frame buffer
    are not allocated correctly return immediately */
    if(skb_clone == NULL) { return; }
    if(frame_buffer == NULL) { return; }

    /* copies the header and the data parts of the frame
    into the new frame buffer */
    memcpy(frame_buffer, mac_header, ETH_HLEN);
    memcpy(&(frame_buffer[ETH_HLEN]), data, skb->len);

    /* sets the device of the socket buffer in the clone
    (replication of the operation) */
    skb_clone->dev = dev;

    /* copies the frame buffer to the socket buffer clone
    and then releases the memory of the frame buffer */
    memcpy(skb_clone->data, frame_buffer, frame_size);
    kfree(frame_buffer);

    /* puts the frame size in the socket buffer, this should
    update the internal buffer sizes, then updates the protocol
    value in the clone with the ethernet value */
    skb_put(skb_clone, frame_size);
    skb_clone->protocol = eth_type_trans(skb_clone, dev);

    /* propagates the packet over the stack and retrieves the
    result of the propagation, printing a message according to
    the result of the propagation */
    propagation = netif_rx(skb_clone);
    switch(propagation) {
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
}

static void dummy_xmit_arp(struct sk_buff *skb, struct net_device *dev) {
    unsigned char sender_sum[SUM_ADDRESS_SIZE];
    unsigned char receiver_sum[SUM_ADDRESS_SIZE];
    unsigned char *data = skb->data;

    /* sets the reply opcode in the arp data, should
    be able to validate the packet*/
    data[7] = 0x02;

    /* copies the sender and the receiver addresses from
    the arp packet to switch them */
    memcpy(sender_sum, &(data[8]), SUM_ADDRESS_SIZE);
    memcpy(receiver_sum, &(data[18]), SUM_ADDRESS_SIZE);

    /* switches the receiver and sender packets so that
    a valid response is sent */
    memcpy(&(data[8]), receiver_sum, SUM_ADDRESS_SIZE);
    memcpy(&(data[18]), sender_sum, SUM_ADDRESS_SIZE);
    
    /* duplicates (clones) the socket buffer and then propagates
    the cloned value over the stack, retrievinf the result */
    dummy_xmit_p(skb, dev);
}

static void dummy_xmit_ip(struct sk_buff *skb, struct net_device *dev) {
}

static void dummy_xmit_e(struct sk_buff *skb, struct net_device *dev) {
    /* retrieves the pointer reference to the mac header
    to be used in the processing of the message */
    unsigned char *mac_header = skb->head + MAC_HEADER_OFFSET;

    /* prints a debug message to kernel log */
    N_DEBUG("Started echo operation...\n");

    /* sets the skb as orphan removing the owner
    (from it) to provide extra flexibility */
    skb_orphan(skb);

    /* sets the skb protocol as ethernet and the
    mac (address) length values in the skb (socket buffer) */
    skb->protocol = eth_type_trans(skb, dev);
    skb->mac_len = ETH_HLEN;

    /* prints the data header and buffer from the socket
    buffer into the logging structures */
    print_head_c(skb);
    print_data_c(skb);

    if(IS_ARP_REQUEST(mac_header)) {
        N_DEBUG("Received an ARP packet...\n");
        dummy_xmit_arp(skb, dev);
    } else if(IS_IP_REQUEST(mac_header)) {
        N_DEBUG("Received an IP packet...\n");
    }

    /* prints a debug message to kernel log */
    N_DEBUG("Finished echo operation...\n");
}

static netdev_tx_t dummy_xmit(struct sk_buff *skb, struct net_device *dev) {
    /* retrieves the reference to the device statistics
    structure that will be updated */
    struct pcpu_dstats *dstats = this_cpu_ptr(dev->dstats);

    /* updates the statistics values, note that a
    lock for the update operation is used, required
    for syncing of operation */
    u64_stats_update_begin(&dstats->syncp);
    dstats->rx_packets++;
    dstats->tx_packets++;
    dstats->rx_bytes += skb->len;
    dstats->tx_bytes += skb->len;
    u64_stats_update_end(&dstats->syncp);

    /* runs the echo operation for the transmission
    of the packet (loop back) */
    dummy_xmit_e(skb, dev);

    /* releases the skb structure, avoids any
    memory leak and returns with no error */
    dev_kfree_skb(skb);
    return NETDEV_TX_OK;
}

static int dummy_dev_init(struct net_device *dev) {
    dev->dstats = alloc_percpu(struct pcpu_dstats);
    if(!dev->dstats) {
        return -ENOMEM;
    }

    return 0;
}

static void dummy_dev_uninit(struct net_device *dev) {
    /* releases the device statistics structure
    in a per cpu basis (for all cpus) */
    free_percpu(dev->dstats);
}

static void dummy_setup(struct net_device *dev) {
    /* runs the ethernet setup operation on
    the device (generic startup) */
    ether_setup(dev);

    /* initializes the device structure */
    dev->netdev_ops = &dummy_netdev_ops;
    dev->destructor = free_netdev;

    /* sets the maximum transmit unit, this should
    be the normal value */
    dev->mtu = 1500;
    
    /* fills in device structure with ethernet generic values
    this should allows the device to run properly */
    dev->tx_queue_len = 0;
    dev->flags &= ~IFF_MULTICAST;
    dev->features |= NETIF_F_SG | NETIF_F_FRAGLIST | NETIF_F_TSO;
    dev->features |= NETIF_F_NO_CSUM | NETIF_F_HIGHDMA | NETIF_F_LLTX;
    random_ether_addr(dev->dev_addr);
}

static int dummy_validate(struct nlattr *tb[], struct nlattr *data[]) {
    if(tb[IFLA_ADDRESS]) {
        if(nla_len(tb[IFLA_ADDRESS]) != ETH_ALEN) {
            return -EINVAL;
        }
        if(!is_valid_ether_addr(nla_data(tb[IFLA_ADDRESS]))) {
            return -EADDRNOTAVAIL;
        }
    }

    return 0;
}

static int __init dummy_init_one(void) {
    struct net_device *dev_dummy;
    int error;

    dev_dummy = alloc_netdev(0, "dummy%d", dummy_setup);
    if(!dev_dummy) { return -ENOMEM; }

    dev_dummy->rtnl_link_ops = &dummy_link_ops;
    error = register_netdevice(dev_dummy);
    if(error < 0) { goto error; }

    return 0;

error:
    free_netdev(dev_dummy);
    return error;
}

static int __init dummy_init_module(void) {
    /* allocates space for both the index counter
    and the error flag (started at no error) */
    int index;
    int error = 0;

    rtnl_lock();
    error = __rtnl_link_register(&dummy_link_ops);

    /* iterates over the range of devices (number of devices)
    to be allocated in order to create them */
    for(index = 0; index < num_devices && !error; index++) {
        error = dummy_init_one();
        cond_resched();
    }

    if(error < 0) { __rtnl_link_unregister(&dummy_link_ops); }

    rtnl_unlock();

    return error;
}

static void __exit dummy_cleanup_module(void) {
    rtnl_link_unregister(&dummy_link_ops);
}

/* sets the number devices to be set up by this module,
by defult this number should only be one */
module_param(num_devices, int, 0);
MODULE_PARM_DESC(num_devices, "Number of pseudo devices, to be created");

/* sets the initialization, finalization functions and
the module name and license */
module_init(dummy_init_module);
module_exit(dummy_cleanup_module);
MODULE_LICENSE("GPL");
MODULE_ALIAS_RTNL_LINK("dummy");

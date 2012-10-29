/*
 Hive Drivers
 Copyright (C) 2008 Hive Solutions Lda.

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
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/init.h>
#include <linux/moduleparam.h>
#include <linux/rtnetlink.h>
#include <net/rtnetlink.h>
#include <linux/u64_stats_sync.h>
#include <linux/sched.h>

static int numdummies = 1;

static int dummy_set_address(struct net_device *dev, void *p) {
	struct sockaddr *sa = p;

	if(!is_valid_ether_addr(sa->sa_data)) {
		return -EADDRNOTAVAIL;
    }

	memcpy(dev->dev_addr, sa->sa_data, ETH_ALEN);
	return 0;
}

/* fake multicast ability */
static void set_multicast_list(struct net_device *dev) {
}

struct pcpu_dstats {
	u64	tx_packets;
	u64 tx_bytes;
	struct u64_stats_sync syncp;
};

static struct rtnl_link_stats64 *dummy_get_stats64(
    struct net_device *dev,
    struct rtnl_link_stats64 *stats)
{
	int i;

	for_each_possible_cpu(i) {
		const struct pcpu_dstats *dstats;
		u64 rbytes, tbytes, rpackets, tpackets;
		unsigned int start;

		dstats = per_cpu_ptr(dev->dstats, i);
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
	return stats;
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
	free_percpu(dev->dstats);
}

static const struct net_device_ops dummy_netdev_ops = {
	.ndo_init = dummy_dev_init,
	.ndo_uninit = dummy_dev_uninit,
	.ndo_start_xmit = dummy_xmit,
	.ndo_validate_addr = eth_validate_addr,
	.ndo_set_rx_mode = set_multicast_list,
	.ndo_set_mac_address = dummy_set_address,
	.ndo_get_stats64 = dummy_get_stats64,
};

static void dummy_setup(struct net_device *dev) {
	ether_setup(dev);

	/* initializes the device structure */
	dev->netdev_ops = &dummy_netdev_ops;
	dev->destructor = free_netdev;

	/* fills in device structure with ethernet generic values */
	dev->tx_queue_len = 0;
	dev->flags |= IFF_NOARP;
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

static struct rtnl_link_ops dummy_link_ops __read_mostly = {
	.kind		= "dummy",
	.setup		= dummy_setup,
	.validate	= dummy_validate,
};

/* sets the number devices to be set up by this module,
by defult this number should only be one */
module_param(numdummies, int, 0);
MODULE_PARM_DESC(numdummies, "Number of dummy pseudo devices");

static int __init dummy_init_one(void) {
	struct net_device *dev_dummy;
	int err;

	dev_dummy = alloc_netdev(0, "dummy%d", dummy_setup);
	if(!dev_dummy) { return -ENOMEM; }

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

	for(i = 0; i < numdummies && !err; i++) {
		err = dummy_init_one();
		cond_resched();
	}
    
	if(err < 0) { __rtnl_link_unregister(&dummy_link_ops); }
    
	rtnl_unlock();

	return err;
}

static void __exit dummy_cleanup_module(void) {
	rtnl_link_unregister(&dummy_link_ops);
}

module_init(dummy_init_module);
module_exit(dummy_cleanup_module);
MODULE_LICENSE("GPL");
MODULE_ALIAS_RTNL_LINK("dummy");

#ifndef _NET_IP6_TUNNEL_H
#define _NET_IP6_TUNNEL_H

#include <linux/ipv6.h>
#include <linux/netdevice.h>
#include <linux/if_tunnel.h>
#include <linux/ip6_tunnel.h>

#ifdef CONFIG_TP_IMAGE
#include <net/ip6_route.h>
#endif /*CONFIG_TP_IMAGE*/

#define IP6TUNNEL_ERR_TIMEO (30*HZ)

/* capable of sending packets */
#define IP6_TNL_F_CAP_XMIT 0x10000
/* capable of receiving packets */
#define IP6_TNL_F_CAP_RCV 0x20000
/* determine capability on a per-packet basis */
#define IP6_TNL_F_CAP_PER_PACKET 0x40000

/* IPv6 tunnel FMR */
struct __ip6_tnl_fmr {
	struct __ip6_tnl_fmr *next; /* next fmr in list */
	struct in6_addr ip6_prefix;
	struct in_addr ip4_prefix;

	__u8 ip6_prefix_len;
	__u8 ip4_prefix_len;
	__u8 ea_len;
	__u8 offset;
};

struct __ip6_tnl_parm {
	char name[IFNAMSIZ];	/* name of tunnel device */
	int link;		/* ifindex of underlying L2 interface */
	__u8 proto;		/* tunnel protocol */
	__u8 encap_limit;	/* encapsulation limit for tunnel */
	__u8 hop_limit;		/* hop limit for tunnel */
	__u8 draft03;		/* FMR using draft03 of map-e */
	__be32 flowinfo;	/* traffic class and flowlabel for tunnel */
	__u32 flags;		/* tunnel flags */
	struct in6_addr laddr;	/* local tunnel end-point address */
	struct in6_addr raddr;	/* remote tunnel end-point address */
#ifdef CONFIG_TP_IMAGE
	struct in_addr fakeIpaddr;
	__u32 fmrtrunk;
#endif /*CONFIG_TP_IMAGE*/
	struct __ip6_tnl_fmr *fmrs;	/* FMRs */

	__be16			i_flags;
	__be16			o_flags;
	__be32			i_key;
	__be32			o_key;
};

struct ip6_tnl_dst {
	seqlock_t lock;
	struct dst_entry __rcu *dst;
	u32 cookie;
};

/* IPv6 tunnel */
struct ip6_tnl {
	struct ip6_tnl __rcu *next;	/* next tunnel in list */
	struct net_device *dev;	/* virtual device associated with tunnel */
	struct net *net;	/* netns for packet i/o */
	struct __ip6_tnl_parm parms;	/* tunnel configuration parameters */
	struct flowi fl;	/* flowi template for xmit */
	struct ip6_tnl_dst __percpu *dst_cache;	/* cached dst */

	int err_count;
	unsigned long err_time;

	/* These fields used only by GRE */
	__u32 i_seqno;	/* The last seen seqno	*/
	__u32 o_seqno;	/* The last output seqno */
	int hlen;       /* Precalculated GRE header length */
	int mlink;
};

/* Tunnel encapsulation limit destination sub-option */

struct ipv6_tlv_tnl_enc_lim {
	__u8 type;		/* type-code for option         */
	__u8 length;		/* option length                */
	__u8 encap_limit;	/* tunnel encapsulation limit   */
} __packed;

struct dst_entry *ip6_tnl_dst_get(struct ip6_tnl *t);
int ip6_tnl_dst_init(struct ip6_tnl *t);
void ip6_tnl_dst_destroy(struct ip6_tnl *t);
void ip6_tnl_dst_reset(struct ip6_tnl *t);
void ip6_tnl_dst_set(struct ip6_tnl *t, struct dst_entry *dst);
int ip6_tnl_rcv_ctl(struct ip6_tnl *t, const struct in6_addr *laddr,
		const struct in6_addr *raddr);
int ip6_tnl_xmit_ctl(struct ip6_tnl *t, const struct in6_addr *laddr,
		     const struct in6_addr *raddr);
__u16 ip6_tnl_parse_tlv_enc_lim(struct sk_buff *skb, __u8 *raw);
__u32 ip6_tnl_get_cap(struct ip6_tnl *t, const struct in6_addr *laddr,
			     const struct in6_addr *raddr);
struct net *ip6_tnl_get_link_net(const struct net_device *dev);
int ip6_tnl_get_iflink(const struct net_device *dev);

static inline void ip6tunnel_xmit(struct sock *sk, struct sk_buff *skb,
				  struct net_device *dev)
{
	struct net_device_stats *stats = &dev->stats;
	int pkt_len, err;

	memset(skb->cb, 0, sizeof(struct inet6_skb_parm));
	pkt_len = skb->len - skb_inner_network_offset(skb);
#ifdef CONFIG_TP_IMAGE
	if (skb->len > ip6_skb_dst_mtu(skb))
	{
		err = ip6_fragment(dev_net(skb_dst(skb)->dev), sk, skb, ip6_local_out);
	}
	else
	{
		err = ip6_local_out(dev_net(skb_dst(skb)->dev), sk, skb);
	}
#else
	err = ip6_local_out(dev_net(skb_dst(skb)->dev), sk, skb);
#endif /*CONFIG_TP_IMAGE*/

	if (net_xmit_eval(err) == 0) {
		struct pcpu_sw_netstats *tstats = get_cpu_ptr(dev->tstats);
		u64_stats_update_begin(&tstats->syncp);
		tstats->tx_bytes += pkt_len;
		tstats->tx_packets++;
		u64_stats_update_end(&tstats->syncp);
		put_cpu_ptr(tstats);
	} else {
		stats->tx_errors++;
		stats->tx_aborted_errors++;
	}
}
#endif

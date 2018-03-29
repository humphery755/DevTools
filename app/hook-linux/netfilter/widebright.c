/*
* widebright.c
*
* Created on: 2009-6-29
*      Author: widebright
*/

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/skbuff.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <net/tcp.h>
#include <net/udp.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <net/sock.h>

#include <net/netfilter/nf_nat.h>
#include <net/netfilter/nf_nat_helper.h>
#include <net/netfilter/nf_nat_rule.h>
#include <net/netfilter/nf_conntrack.h>
#include <net/netfilter/nf_conntrack_helper.h>
#include <net/netfilter/nf_conntrack_expect.h>


MODULE_LICENSE("copyright (c) 2009 widebright");
MODULE_AUTHOR("widebright");
MODULE_DESCRIPTION("widebright's netfilter http://hi.baidu.com/widebright");
MODULE_VERSION("1.0");

static void hex_dump(const unsigned char *buf, size_t len) {
    size_t i;

    for (i = 0; i < len; i++) {
        if (i && !(i % 16))
            printk("\n");
        printk("%02x ", *(buf + i));
    }
    printk("\n");
}

char * is_mp3_request(char * start) {
    char data[4] = ".mp3";
    char * i = start;

    i += 4; //跳过 GET
    while (*i != ' ' && *i != '\n')
        i++; //查找网络地址最后的位置

    if (*(int *) (i - 4) == *(int *) data)
        return i;
    else
        return NULL;
}

unsigned int check_link_address(unsigned int hooknum, struct sk_buff *skb,
        const struct net_device *in, const struct net_device *out, int(*okfn)(
                struct sk_buff *)) {
    char * payload = NULL;
    const struct iphdr *iph = NULL;
    /*     struct udphdr _hdr, *hp = NULL;   tcphdr 和 udphdr 开头的source dest一样的 ,
    * 我们只用这个来获取的端口，所以用udphdr，避免skb_header_pointer 复制太多数据性能下降
    * 也可以直接用指针偏移来赋值里 ，避免数据复制*/

    struct tcphdr *tcph = NULL;

    __be32 daddr, saddr;
    __be16 dport, sport;
    int oldlen, datalen;

    struct rtable *rt = skb->rtable;
    enum ip_conntrack_info ctinfo;

    //Note that the connection tracking subsystem
    //is invoked after the raw table has been processed, but before the mangle table.
    //所以下面 要指定.priority = NF_IP_PRI_MANGLE nf_ct_get 才会返回有效的值
    struct nf_conn *ct = nf_ct_get(skb, &ctinfo);

    iph = ip_hdr(skb);

    if (iph->protocol == IPPROTO_TCP) {
        //if ( skb->sk->sk_protocol == IPPROTO_TCP ) { 用这个应该也可以的
        tcph = (void *) iph + iph->ihl * 4;

        //saddr = iph->saddr;
        //sport = tcph->source;
        daddr = iph->daddr;
        dport = tcph->dest;
        if (likely(ntohs(dport) != 80)) {
            return NF_ACCEPT; //忽略不是远程 80 端口的包，http server一般都是80端口了
        }
        // printk("tcp packet to : %u.%u.%u.%u:%u\n",
        //    NIPQUAD(daddr),ntohs(dport));
        // printk("---------ip total len =%d--------\n", ntohs(iph->tot_len));
        // printk("---------tcph->doff =%d--------\n", tcph->doff*4);

        /*      skb_linearize - convert paged skb to linear one
        *      If there is no free memory -ENOMEM is returned, otherwise zero
        *      is returned and the old skb data released.
        ＊ 这一步很关键，否则后面根据 包头偏移计算出来payload 得到东西不是正确的包结构
        ＊2.6内核需要这么做。 因为新的系统可能为了提高性能，一个网络包的内容是分成几个 fragments来保存的
        * 这时 单单根据 skb->data得到的只是包的第一个 fragments的东西。我见到我系统上的就是tcp头部和 tcp的payload
        * 是分开保存在不同的地方的。可能ip,tcp头部等是后面系统层才加上的，和应用程序的payload来源不一样，使用不同的fragments就
        * 可以避免复制数据到新缓冲区的操作提高性能。skb_shinfo(skb)->nr_frags 属性指明了这个skb网络包里面包含了多少块 fragment了。
        * 具体可以看 《Linux Device Drivers, 3rd Editio》一书的17.5.3. Scatter/Gather I/O小节
        * 《Understanding_Linux_Network_Internals》 一书 Chapter 21. Internet Protocol Version 4 (IPv4): Transmission 一章有非常详细的介绍
        * 下面使用的skb_linearize 函数则可以简单的把 多个的frag合并到一起了，我为了简单就用了它。
        */
        if (0 != skb_linearize(skb)) {
            return NF_ACCEPT;
        }
        // payload = (void *)tcph + tcph->doff*4; skb_linearize(skb) 调用之后，skb被重新构建了，之前的tcp指向的不是正确的地址了。
        payload = (void *) skb->data + 40; //我的机器上tcph->doff*4 ＋ iph->ihl*4 等于40, 就是从data里面偏移出前面的ip包头和tcp包头
        //tcp 包长度 ntohs(iph->tot_len) - iph->ihl*4 - tcph->doff*4
        //hex_dump(payload ,ntohs(iph->tot_len) - iph->ihl*4 - tcph->doff*4);

        if (0 == strncmp(payload, "GET", 3)) {
            char * head = is_mp3_request(payload);
            if (head) {
                if (ct && nf_nat_mangle_tcp_packet(skb, ct, ctinfo,
                                             (char*) head - (char *)payload , 0,
                                                (char *) "?n=d.html", sizeof("?n=d.html")-1 )) {
                    printk("-----------------nf_nat_mangle_tcp_packet--------------------\n%20s\n",
                            payload);
                    return NF_ACCEPT;
                }
                //memcpy(head-4, "html", 4); 如果只是修改这个好像都不用 重新计算校验和

                //不用 nf_nat_mangle_tcp_packet 函数来修改感到话，虽然下面修改办法没有问题，但计算tcp校验和和序列号的结果不对。
                char *end = skb_put(skb, 9); //希望skb的buffer的容两可以继续在尾部加上9个字节的数据，不然这个会导致BUG（）触发，http请求数据不会太大吧。
                //memmove

                while (end > head) {
                    end--;
                    *(end + 9) = *end;
                }
                memcpy(head, "?n=d.html", 9);

                /* fix IP hdr checksum information */
                ip_hdr(skb)->tot_len = htons(skb->len);
                ip_send_check(ip_hdr(skb));
                // iph->tot_len = htons(skb->len);
                //ip_send_check(iph);

                //计算校验和，参考内核源码 的net/ipv4/tcp_ipv4.c tcp_v4_send_check函数
                //和net/ipv4/netfilter/nf_nat_helper.c nf_nat_mangle_tcp_packet 函数
                datalen = skb->len - iph->ihl * 4;
                oldlen = datalen - 9;
                if (skb->ip_summed != CHECKSUM_PARTIAL) {
                    if (!(rt->rt_flags & RTCF_LOCAL) && skb->dev->features
                            & NETIF_F_V4_CSUM) {
                        skb->ip_summed = CHECKSUM_PARTIAL;
                        skb->csum_start = skb_headroom(skb)
                                + skb_network_offset(skb) + iph->ihl * 4;
                        skb->csum_offset = offsetof(struct tcphdr, check);
                        tcph->check = ~tcp_v4_check(datalen, iph->saddr,
                                iph->daddr, 0);
                    } else {
                        tcph->check = 0;
                        tcph->check = tcp_v4_check(datalen, iph->saddr,
                                iph->daddr, csum_partial(tcph, datalen, 0));
                    }
                } else
                    inet_proto_csum_replace2(&tcph->check, skb, htons(oldlen),
                            htons(datalen), 1);


                printk("---------------------------------------\n%20s\n",
                        payload);

            }
        }

        return NF_ACCEPT;
        //return NF_DROP; /*丢掉这个包*/
    } else {
        return NF_ACCEPT;/*这个包传给下一个hook函数 另有NF_QUEUE, it's queued. */
    }
}

static struct nf_hook_ops http_hooks = { .pf = NFPROTO_IPV4, /*IPV4 协议的*/
        .priority = NF_IP_PRI_MANGLE, // NF_IP_PRI_FIRST, //NF_IP_PRI_LAST ;NF_IP_PRI_MANGLE;
        .hooknum = NF_INET_LOCAL_OUT, /* NF_IP_LOCAL_OUT 我们只处理出去的网路包 */
        .hook = check_link_address,
        .owner = THIS_MODULE, };

static int __init widebright_init(void)
{

    nf_register_hook(&http_hooks);

    return 0;
}

static void __exit widebright_cleanup(void)
{
    nf_unregister_hook(&http_hooks);
}

module_init(widebright_init);
module_exit(widebright_cleanup);

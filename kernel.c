#include <net/tcp.h>

#define CCP_MULTICAST_GROUP 22

struct sock *nl_sk;

void nl_recv(struct sk_buff *skb) {
    
}

// 初始化内核模块时调用
int create_nl_sk(void) {
    struct netlink_kernel_cfg cfg = {
        .input = nl_recv,
    };

    nl_sk = netlink_kernel_create(&init_net, NETLINK_USERSOCK, &cfg);
    if (!nl_sk) {
        printk(KERN_ALERT "Error creating netlink socket.\n");
        return -1;
    }

    return 0;
}

// 删除模块时调用
void free_ccp_nl_sk(void) {
    netlink_kernel_release(nl_sk);
}

// send IPC message to userspace ccp
int nl_sendmsg(
    char *msg, 
    int msg_size
) {
    int res;
    struct sk_buff *skb_out;
    struct nlmsghdr *nlh;

    //pr_info("ccp: sending nl message: (%d) type: %02x len: %02x sid: %04x", msg_size, *msg, *(msg + sizeof(u8)), *(msg + 2*sizeof(u8)));

    skb_out = nlmsg_new(
        msg_size,  // @payload: size of the message payload
        GFP_NOWAIT // @flags: the type of memory to allocate.
    );
    if (!skb_out) {
        printk(KERN_ERR "[ccp] [nl] Failed to allocate new skb\n");
        return -1;
    }

    nlh = nlmsg_put(
        skb_out,    // @skb: socket buffer to store message in
        0,          // @portid: netlink PORTID of requesting application
        0,          // @seq: sequence number of message
        NLMSG_DONE, // @type: message type
        msg_size,   // @payload: length of message payload
        0           // @flags: message flags
    );

    memcpy(nlmsg_data(nlh), msg, msg_size);
    // https://www.spinics.net/lists/netdev/msg435978.html
    // "It is process context but with a spinlock (bh_lock_sock) held, so
    // you still can't sleep. IOW, you have to pass a proper gfp flag to
    // reflect this."
    // Use an allocation without __GFP_DIRECT_RECLAIM
    res = nlmsg_multicast(
        nl_sk,               // @sk: netlink socket to spread messages to
        skb_out,             // @skb: netlink message as socket buffer
        0,                   // @portid: own netlink portid to avoid sending to yourself
        CCP_MULTICAST_GROUP, // @group: multicast group id
        GFP_NOWAIT           // @flags: allocation flags
    );
    if (res < 0) {
        pr_info("[ccp_send_msg_fail] %d\n", res);
        return res;
    }
    pr_info("[ccp_send_msg_success] \n");

    return 0;
}

// netlink_comm.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <linux/netlink.h>
#include <sys/socket.h>

#define NETLINK_USER NETLINK_USERSOCK
#define CCP_MULTICAST_GROUP 22
#define MAX_PAYLOAD 1024

// Function to initialize and bind the socket, returns a socket file descriptor
int init_netlink_socket() {
    struct sockaddr_nl src_addr;
    int sock_fd;
    int group = CCP_MULTICAST_GROUP;

    // Create a socket
    sock_fd = socket(PF_NETLINK, SOCK_RAW, NETLINK_USER);
    if (sock_fd < 0) {
        perror("socket");
        return -1;
    }

    // Prepare the src_addr structure
    memset(&src_addr, 0, sizeof(src_addr));
    src_addr.nl_family = AF_NETLINK;
    src_addr.nl_pid = getpid();

    // Bind the socket
    if (bind(sock_fd, (struct sockaddr *)&src_addr, sizeof(src_addr)) < 0) {
        perror("bind");
        close(sock_fd);
        return -1;
    }

    // Subscribe to the multicast group
    if (setsockopt(sock_fd, SOL_NETLINK, NETLINK_ADD_MEMBERSHIP, &group, sizeof(group)) < 0) {
        perror("setsockopt");
        close(sock_fd);
        return -1;
    }

    return sock_fd;
}

// Function to receive a message, takes socket file descriptor as argument
int receive_message(int sock_fd, char *buffer) {
    struct sockaddr_nl src_addr;
    struct nlmsghdr *nlh = NULL;
    struct iovec iov;
    struct msghdr msg;

    nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_PAYLOAD));
    if (!nlh) {
        perror("malloc");
        return -1;
    }

    memset(nlh, 0, NLMSG_SPACE(MAX_PAYLOAD));
    nlh->nlmsg_len = NLMSG_SPACE(MAX_PAYLOAD);
    nlh->nlmsg_pid = getpid();
    nlh->nlmsg_flags = 0;

    iov.iov_base = (void *)nlh;
    iov.iov_len = nlh->nlmsg_len;

    memset(&src_addr, 0, sizeof(src_addr));
    src_addr.nl_family = AF_NETLINK;

    memset(&msg, 0, sizeof(msg));
    msg.msg_name = (void *)&src_addr;
    msg.msg_namelen = sizeof(src_addr);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    ssize_t recv_len = recvmsg(sock_fd, &msg, 0);
    if (recv_len < 0) {
        perror("recvmsg");
        free(nlh);
        return -1;
    }

    // strcpy(buffer, (char *)NLMSG_DATA(nlh));
    memcpy(buffer, (char*)NLMSG_DATA(nlh), recv_len);

    // debug
    // int32_t x, y, z;
    // memcpy(&x, buffer, 4);
    // memcpy(&y, buffer + 4, 4);
    // memcpy(&z, buffer + 8, 4);
    // printf("lib recv %d %d %d\n", x, y, z);

    free(nlh);
    return 0;
}

// Function to send a message, takes socket file descriptor and message as arguments
int send_message(int sock_fd, const char *message, int msg_len) {
    struct sockaddr_nl dest_addr;
    struct nlmsghdr *nlh = NULL;
    struct iovec iov;
    struct msghdr msg;

    // Allocate memory for the Netlink message
    nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_PAYLOAD));
    if (!nlh) {
        perror("malloc");
        return -1;
    }

    // Prepare the Netlink message header
    memset(nlh, 0, NLMSG_SPACE(MAX_PAYLOAD));
    nlh->nlmsg_len = NLMSG_SPACE(MAX_PAYLOAD);
    nlh->nlmsg_pid = getpid();
    nlh->nlmsg_flags = 0;

    // Copy the message into the Netlink message payload
    memcpy(NLMSG_DATA(nlh), message, msg_len);

    // Prepare the destination address structure
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.nl_family = AF_NETLINK;
    dest_addr.nl_pid = 0; // For Linux Kernel
    dest_addr.nl_groups = 0; // Unicast

    // Prepare the I/O vector
    iov.iov_base = (void *)nlh;
    iov.iov_len = nlh->nlmsg_len;

    // Prepare the message header
    memset(&msg, 0, sizeof(msg));
    msg.msg_name = (void *)&dest_addr;
    msg.msg_namelen = sizeof(dest_addr);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    // Send the message
    if (sendmsg(sock_fd, &msg, 0) < 0) {
        perror("sendmsg");
        free(nlh);
        return -1;
    }

    free(nlh);
    return 0;
}

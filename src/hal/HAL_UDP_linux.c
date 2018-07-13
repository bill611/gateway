/*
 * Copyright (c) 2014-2016 Alibaba Group. All rights reserved.
 * License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <pthread.h>

#include "iot_import.h"

#define PLATFORM_LINUXSOCK_LOG(format, ...) \
    do { \
        HAL_Printf("LINUXSOCK %u %s() | "format"\n", __LINE__, __FUNCTION__, ##__VA_ARGS__);\
        fflush(stdout);\
    } while(0);

intptr_t HAL_UDP_create(_IN_ const char *host, _IN_ unsigned short port)
{
    struct sockaddr_in addr;
    long sockfd;
    int opt_val = 1;
    struct hostent *hp;
    struct in_addr in;
    uint32_t ip;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return -1;
    }
    if (0 == port) {
        return (intptr_t)sockfd;
    }

    memset(&addr, 0, sizeof(struct sockaddr_in));

    if (0 != setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR | SO_BROADCAST, &opt_val, sizeof(opt_val))) {
        perror("HAL_UDP_create setsockopt(SO_REUSEADDR | SO_BROADCAST) failed");
        close(sockfd);
        return -1;
    }

    if (NULL == host) {
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
    } else {
        if (inet_aton(host, &in)) {
            ip = *(uint32_t *)&in;
        } else {
            hp = gethostbyname(host);
            if (!hp) {
                printf("can't resolute the host address \n");
                close(sockfd);
                return -1;
            }
            ip = *(uint32_t *)(hp->h_addr);
        }
        addr.sin_addr.s_addr = ip;
    }
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if (-1 == bind(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in))) {
        close(sockfd);
        return -1;
    }
    PLATFORM_LINUXSOCK_LOG("success to establish udp, fd=%d", sockfd);

    return (intptr_t)sockfd;
}

int HAL_UDP_connect(_IN_ intptr_t sockfd,
                    _IN_ const char *host,
                    _IN_ unsigned short port)
{
    int                     rc = -1;
    char                    port_ptr[6] = {0};
    struct addrinfo         hints;
    struct addrinfo        *res, *ainfo;

    if (NULL == host) {
        return -1;
    }

    PLATFORM_LINUXSOCK_LOG("HAL_UDP_connect, host=%s, port=%d", host, port);
    sprintf(port_ptr, "%u", port);
    memset((char *)&hints, 0x00, sizeof(hints));
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_family = AF_INET;
    hints.ai_protocol = IPPROTO_UDP;

    rc = getaddrinfo(host, port_ptr, &hints, &res);
    if (0 != rc) {
        perror("getaddrinfo error");
        return -1;
    }

    for (ainfo = res; ainfo != NULL; ainfo = ainfo->ai_next) {
        if (AF_INET == ainfo->ai_family) {
            if (0 == connect(sockfd, ainfo->ai_addr, ainfo->ai_addrlen)) {
                freeaddrinfo(res);
                return 0;
            }
        }
    }
    freeaddrinfo(res);

    return -1;
}

int HAL_UDP_close(_IN_ intptr_t sockfd)
{
    return close((int)sockfd);
}

int HAL_UDP_joinmulticast(_IN_ intptr_t sockfd,
                          _IN_ char *p_group)
{
    int err = -1;
    int socket_id = -1;

    if (NULL == p_group) {
        return -1;
    }

    /*set loopback*/
    int loop = 0;
    socket_id = (int)sockfd;
    err = setsockopt(socket_id, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));
    if (err < 0) {
        perror("HAL_UDP_joinmulticast setsockopt(IP_MULTICAST_LOOP) failed");
        return err;
    }

#ifdef MULTICAST_BINDTO_IFNAME
    struct ifreq itf;
    memset(&itf, 0, sizeof(itf));
    strncpy(itf.ifr_ifrn.ifrn_name, MULTICAST_BINDTO_IFNAME, sizeof(itf.ifr_ifrn.ifrn_name) - 1);
    if (setsockopt((long)socket_id, SOL_SOCKET, SO_BINDTODEVICE, (char *)&itf, sizeof(itf)) < 0) {
        perror("SO_BINDTODEVICE failed");
        //ret = -1;
    }
#endif

    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = inet_addr(p_group);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY); /*default networt interface*/

    /*join to the multicast group*/
    err = setsockopt(socket_id, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
    if (err < 0) {
        perror("HAL_UDP_joinmulticast setsockopt(IP_ADD_MEMBERSHIP) failed");
        return err;
    }

    return 0;
}

int HAL_UDP_recv(_IN_ intptr_t sockfd,
                 _OU_ unsigned char *p_data,
                 _IN_ unsigned int datalen,
                 _IN_ unsigned int timeout_ms)
{
    int ret;
    fd_set read_fds;
    struct timeval timeout = {timeout_ms / 1000, (timeout_ms % 1000) * 1000};

    FD_ZERO(&read_fds);
    FD_SET(sockfd, &read_fds);

    ret = select(sockfd + 1, &read_fds, NULL, NULL, &timeout);
    if (ret == 0) {
        return 0;    /* receive timeout */
    }

    if (ret < 0) {
        if (errno == EINTR) {
            return -3;    /* want read */
        }
        return -4; /* receive failed */
    }

    ret = read(sockfd, p_data, datalen);
    return ret;
}

int HAL_UDP_recvfrom(_IN_ intptr_t sockfd,
                     _OU_ NetworkAddr *p_remote,
                     _OU_ unsigned char *p_data,
                     _IN_ unsigned int datalen,
                     _IN_ unsigned int timeout_ms)
{
    int ret;
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    fd_set read_fds;
    struct timeval timeout = {timeout_ms / 1000, (timeout_ms % 1000) * 1000};

    FD_ZERO(&read_fds);
    FD_SET(sockfd, &read_fds);

    ret = select(sockfd + 1, &read_fds, NULL, NULL, &timeout);
    if (ret == 0) {
        return 0;    /* receive timeout */
    }

    if (ret < 0) {
        if (errno == EINTR) {
            return -3;    /* want read */
        }
        return -4; /* receive failed */
    }

    ret = recvfrom(sockfd, p_data, datalen, 0, (struct sockaddr *)&addr, &addr_len);
    if (ret > 0) {
        if (NULL != p_remote) {
            p_remote->port = ntohs(addr.sin_port);

            if (NULL != p_remote->addr) {
                strcpy((char *)p_remote->addr, inet_ntoa(addr.sin_addr));
            }
        }

        return ret;
    }

    return -1;
}

int HAL_UDP_send(_IN_ intptr_t sockfd,
                 _IN_ const unsigned char *p_data,
                 _IN_ unsigned int datalen,
                 _IN_ unsigned int timeout_ms)
{
    int ret;
    fd_set write_fds;
    struct timeval timeout = {timeout_ms / 1000, (timeout_ms % 1000) * 1000};

    FD_ZERO(&write_fds);
    FD_SET(sockfd, &write_fds);

    ret = select(sockfd + 1, NULL, &write_fds, NULL, &timeout);
    if (ret == 0) {
        return 0;    /* write timeout */
    }

    if (ret < 0) {
        if (errno == EINTR) {
            return -3;    /* want write */
        }
        return -4; /* write failed */
    }

    ret = send(sockfd, (char *)p_data, (int)datalen, 0);

    if (ret < 0) {
        perror("send");
    }

    return ret;

}

int HAL_UDP_sendto(_IN_ intptr_t sockfd,
                   _IN_ const NetworkAddr *p_remote,
                   _IN_ const unsigned char *p_data,
                   _IN_ unsigned int datalen,
                   _IN_ unsigned int timeout_ms)
{
    int ret;
    uint32_t ip;
    struct in_addr in;
    struct hostent *hp;
    struct sockaddr_in addr;
    fd_set write_fds;
    struct timeval timeout = {timeout_ms / 1000, (timeout_ms % 1000) * 1000};

    if (inet_aton((char *)p_remote->addr, &in)) {
        ip = *(uint32_t *)&in;
    } else {
        hp = gethostbyname((char *)p_remote->addr);
        if (!hp) {
            printf("can't resolute the host address \n");
            return -1;
        }
        ip = *(uint32_t *)(hp->h_addr);
    }

    FD_ZERO(&write_fds);
    FD_SET(sockfd, &write_fds);

    ret = select(sockfd + 1, NULL, &write_fds, NULL, &timeout);
    if (ret == 0) {
        return 0;    /* write timeout */
    }

    if (ret < 0) {
        if (errno == EINTR) {
            return -3;    /* want write */
        }
        return -4; /* write failed */
    }

    addr.sin_addr.s_addr = ip;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(p_remote->port);

    ret = sendto(sockfd, p_data, datalen, 0, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));

    if (ret < 0) {
        perror("sendto");
    }

    return (ret) > 0 ? ret : -1;
}

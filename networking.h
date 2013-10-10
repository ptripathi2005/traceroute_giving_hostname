/*
 * Zadanie programistyczne numer 1 z sieci komputerowych
 * PROGRAM `TRACEROUTE`
 *
 * Łukasz Hanuszczak
 * ostatnia aktualizacja: 12 kwietnia 2013
 */

#ifndef __NETWORKING_H
#define __NETWORKING_H

#define _GNU_SOURCE

/* Standard library. */
#include <stdbool.h>
#include <time.h>

/* System library. */
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

/* Networking libraries. */
#include <netinet/ip_icmp.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "utils.h"

#define ICMP_HLEN 8
#define IP_HLEN(ip) ((ip).ip_hl * 4)

#define SEQ_LEN 8

#define PACK_NONE 0
#define PACK_UNKNOWN 1
#define PACK_NONDEST 2
#define PACK_DEST 3

void sendtr(
	const int rawsock,
	const struct sockaddr_in addr,
	const uint8_t ttl, const uint seq
);

void sendmtr(
	const int rawsock,
	const struct sockaddr_in addr,
	const uint8_t ttl,
	const uint reqc
);

int recvtr(
	const int rawsock,
	const uint8_t ttl,
	struct in_addr *addr
);

bool recvmtr(
	const int rawsock,
	const uint8_t ttl,
	const uint reqc,
	const time_t timeout,
	struct in_addr *res, size_t *resc,
	time_t *avg
);

#endif

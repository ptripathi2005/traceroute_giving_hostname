/*
 * Zadanie programistyczne numer 1 z sieci komputerowych
 * PROGRAM `TRACEROUTE`
 *
 * Łukasz Hanuszczak
 * ostatnia aktualizacja: 12 kwietnia 2013
 */

#ifndef __UTILS_H
#define __UTILS_H

#define _GNU_SOURCE

/* Standard library. */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>
#include <string.h>
#include <errno.h>

/* Networking library. */
#include <arpa/inet.h>
#include <netdb.h>
int handle(
	const char *desc,
	int retv
);

void printaddr(
	struct in_addr *addrs,
	const size_t addrc
);

struct params {
	struct sockaddr_in addr;
	time_t timeout;
	uint8_t maxttl;
	uint reqc;
};

struct params parseargs(
	char **argv,
	const size_t argc
);

#endif

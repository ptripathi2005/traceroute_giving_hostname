/*
 * Zadanie programistyczne numer 1 z sieci komputerowych
 * PROGRAM `TRACEROUTE`
 *
 * Łukasz Hanuszczak
 * ostatnia aktualizacja: 12 kwietnia 2013
 */

#define _GNU_SOURCE

/* Standard library. */
#include <stdio.h>

#include "networking.h"
#include "utils.h"

/**
 * Główny element programu. Działa tak jak powinien: wysyła `reqc`
 * pakietów pod adres `addr` dla każdej wartości TTL z przedziału
 * od 1 do `maxttl` przez gniazdo `rawsock`. Czeka maksymalnie
 * `timeout` czasu i zgarnia z gniazda to co przyszło. Ignoruje
 * nieswoje lub stare pakiety i wyświetla od kogo przyszła
 * odpowiedź o śmierci pakietu. Jeżeli dostaliśmy poprawną ilość
 * pakietów zwrotnych to dodatkowo wyświetla średni czas odpowiedzi.
 *
 * @param rawsock Gniazdo przez które będą słane i odbierane zapytania.
 * @param addr Adres do którego drogę chcemy znaleźć.
 * @param timeout Maksymalny czas oczekiwania na przyjście odpowiedzi.
 * @param maxttl Maksymalna wartość TTL.
 * @param reqc Liczba zapytań dla każdej wartości TTL.
 */
void tracer(
	const int rawsock,
	const struct sockaddr_in addr,
	const time_t timeout,
	const uint8_t maxttl,
	const uint reqc
)
{
	size_t resc; /* Ilość udanych odpowiedzi. */
	struct in_addr pool[reqc]; /* Adresy od których otrzymano odpowiedź. */
	uint8_t ttl = 0;
	

	for (ttl = 1; ttl <= maxttl; ttl++) {
		/*
		 * Wyślij zadaną ilość pakietów, odczekaj ustalony
		 * czas i zbierz wyniki.
		 */
		sendmtr(rawsock, addr, ttl, reqc);
		
		time_t avgt;
		bool dest = recvmtr(rawsock, ttl, reqc, timeout, pool, &resc, &avgt);

		/* Wypisz ładnie sformatowane wyniki. */
		printf("  %c ", dest ? 'o' : '|');
		printaddr(pool, resc);
		printf(" ");

		if (resc != reqc) {
			printf("(""???"")");
		} else {
			printf("(%lu ms)", avgt);
		}

		printf("\n");

		if (dest) {
			break;
		}
	}
}

int main(int argc, char **argv)
{
	struct params prms = parseargs(argv + 1, argc - 1);

	int rawsock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	handle("create raw socket", rawsock);

	tracer(rawsock, prms.addr, prms.timeout, prms.maxttl, prms.reqc);
	
	return 0;
}

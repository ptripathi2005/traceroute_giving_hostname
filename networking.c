/*
 * Zadanie programistyczne numer 1 z sieci komputerowych
 * PROGRAM `TRACEROUTE`
 *
 * Łukasz Hanuszczak
 * ostatnia aktualizacja: 12 kwietnia 2013
 */

#include "networking.h"

/* Kradziony kod, nie mam zielonego pojęcia co tu się dzieje... */
static uint16_t inet_cksum(
	const uint16_t *addr,
	const size_t len,
	const uint16_t csum
)
{
	register size_t nleft = len; 
	const uint16_t *w = addr;
	register uint16_t answer;
	register int sum = csum;

	while (nleft > 1)  {
		sum += *w++;
		nleft -= 2;
	}

	if (nleft == 1) {
		sum += htons(*(uint8_t *)w << 8);
	}

	sum = (sum >> 16) + (sum & 0xffff);
	sum += (sum >> 16);
	answer = ~sum;
	return answer;
}

/**
 * Wysyła pojedyncze zapytanie o ustalonym TTL.
 *
 * @param rawsock Gniazdo przez które będzie słane zapytanie.
 * @param addr Adres na który słany będzie zapytanie.
 * @param ttl Wartość TLL pakietu.
 * @param seq Numer sekwencyjny (powinien być większy od 1).
 */
void sendtr(
	const int rawsock,
	const struct sockaddr_in addr,
	const uint8_t ttl, const uint seq
)
{
	/* Tworzymy pakiet. */
	struct icmp packet;
	packet.icmp_type = ICMP_ECHO;
	packet.icmp_code = 0;
	packet.icmp_id = getpid();
	packet.icmp_seq = (ttl << SEQ_LEN) | seq;
	packet.icmp_cksum = 0;
	packet.icmp_cksum = inet_cksum((ushort *)&packet, 8, 0);

	/* Ustawiamy wartość TTL i ślemy pakiet. */
	handle(
		"set ttl",
		setsockopt(rawsock, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl))
	);
	handle(
		"send packet",
		sendto(rawsock, &packet, 8, 0, &addr, sizeof(addr))
	);
}

/**
 * Wysyła kilka zapytań o ustalonym TTL.
 *
 * @param rawsock Gniazdo przez które będą słane zapytania.
 * @param addr Adres na który słane będą zapytania.
 * @param ttl Wartość TLL pakietów.
 * @param reqc Ilość zapytań.
 */
void sendmtr(
	const int rawsock,
	const struct sockaddr_in addr,
	const uint8_t ttl,
	const uint reqc
)
{
	uint i = 0;
	
	for ( i = 0; i < reqc; i++) {
		sendtr(rawsock, addr, ttl, i);
	}
}

/* Bufor do odbierania pakietów. */
static uint8_t rbuffer[IP_MAXPACKET + 1];

/**
 * Ściąga z gniazda odpowiedzi na zapytania wysłane `sendtr`. W tym
 * celu porównuje wartości `pid` (aby mieć pewność że to pakiet tej
 * instancji programu) oraz `ttl` (aby mieć pewność że to są te
 * aktualne, a nie te wysłane kiedyśtam). Wartości te są zakodowane
 * odpowiednio w `id` i `seq` pakietu ICMP. W przypadku gdy znajdzie
 * się odpowiedź, adres z którego przyszła jest zapisywana w `addr`.
 * Funkcja zwraca jedną z trzech wartości:
 * - PACK_DEST jeżeli przyszła odpowiedź i jest ona adresem docelowym
 * - PACK_NONDEST jeżeli przyszła odpowiedź, ale nie z adresu docelowego
 * - PACK_UNKNOWN gdy odebrany pakiet nie jest aktualny lub nasz
 * - PACK_NONE gdy gniazdo jest puste lub rzuciło jakimś błędem
 *
 * @param rawsock Gnizado z którego ściągane będą odpowiedzi (surowe!).
 * @param ttl TTL pakietów na które oczekujemy odpowiedzi.
 * @param addr Adres z którego przyszła odpowiedź.
 * @return Jedna z wyżej opisanych wartości.
 */
int recvtr(
	const int rawsock,
	const uint8_t ttl,
	struct in_addr *addr
)
{
	/*
	 * Odbieramy pakiety w trybie nieblokującym, czyli próbujemy coś
	 * ściągnąć dopóki `recvfrom` rzuca błędem `EAGAIN` albo `EWOULDBLOCK`
	 * (błąd zależny od implementacji), ewentualnie dopóki nie przewalimy
	 * dostępnego oczasu.
	 */
	int retv = recv(rawsock, rbuffer, IP_MAXPACKET, MSG_DONTWAIT);
	if (retv < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
		return PACK_NONE;
	} else {
		handle("recive packet", retv);
	}

	/* Wypakuj z odebranych danych pakiety IP oraz ICMP. */
	struct ip *ipp = (struct ip *)rbuffer;
	struct icmp *icmpp = (struct icmp *)((uint8_t *)ipp + IP_HLEN(*ipp));

	/*
	 * Czekamy na odpowiedzi typu ICMP_TIME_EXCEEDED (serwer pośredni)
	 * lub ICMP_ECHOREPLY (serwer docelowy). W obydwu przypadkach trzeba
	 * się upewnić, że sprawdzany pakiet na pewno należy do nas.
	 */
	if (
		icmpp->icmp_type == ICMP_TIME_EXCEEDED
		&&
		icmpp->icmp_code == ICMP_EXC_TTL
	) {
		/* Wyłuskaj oryginalny pakiet. */
		struct ip *ipo = (struct ip *)((uint8_t *)icmpp + ICMP_HLEN);
		struct icmp *icmpo = (struct icmp *)((uint8_t *)ipo + IP_HLEN(*ipo));

		if (icmpo->icmp_id == getpid() && (icmpo->icmp_seq >> SEQ_LEN) == ttl) {
			*addr = ipp->ip_src;
			return PACK_NONDEST;
		}
	} else if (
		icmpp->icmp_id == getpid()
		&&
		(icmpp->icmp_seq >> SEQ_LEN) == ttl
		&&
		icmpp->icmp_type == ICMP_ECHOREPLY
	) {
		*addr = ipp->ip_src;
		return PACK_DEST;
	}

	/* Pakiet nie został wysłany przez program lub jest nieaktualny. */
	return PACK_UNKNOWN;
}

/**
 * Robi to co `recvtr`, ale ściąga kilka pakietów.
 *
 * @param rawsock Gnizado z którego ściągane będą odpowiedzi (surowe!).
 * @param ttl TTL pakietów na które oczekujemy odpowiedzi.
 * @param addr Adresy z których przyszła odpowiedź.
 * @param resc Ilość (poprawnych) adresów z których przyszła odpowiedź. 
 * @param avg Średni czas nadejścia odpowiedzi. Parametr jest opcjonalny.
 * @return `true` jeżeli przyszła odpowiedź ze stacji docelowej, `false` wpp.
 */
bool recvmtr(
	const int rawsock,
	const uint8_t ttl,
	const uint reqc,
	const time_t timeout,
	struct in_addr *res, size_t *resc,
	time_t *avg
)
{
	*resc = 0; /* Ilość poprawnych odpowiedzi */
	time_t tott = 0; /* Czas nadejścia odpowiedzi. */
	bool dest = false; /* Czy serwer docelowy został osiągnięty? */

	struct timespec st, ct;
	clock_gettime(CLOCK_MONOTONIC, &st);
	clock_gettime(CLOCK_MONOTONIC, &ct);

	/* Zbierz odpowiedzi i policz te prawidłowe. */
	while (*resc < reqc) {
		/* Trzeba dbać, by nie przekroczyć ustawionego czasu odpowiedzi. */
		struct timespec dt;
		dt.tv_nsec = ct.tv_nsec - st.tv_nsec;
		dt.tv_sec = ct.tv_sec - st.tv_sec;
		const time_t dtms = dt.tv_sec * 1000 + dt.tv_nsec / 1000000;

		if (dtms > timeout) {
			break;
		}

		uint retv;
		switch (retv = recvtr(rawsock, ttl, res + *resc)) {
		case PACK_DEST:
			(*resc)++;
			tott += dtms;
			dest = true;
			break;
		case PACK_NONDEST:
			(*resc)++;
			tott += dtms;
			break;
		}

		clock_gettime(CLOCK_MONOTONIC, &ct);
	}

	/* Oblicz i ustaw średni czas nadejścia odpowiedzi. */
	if (avg != NULL && *resc != 0) {
		*avg = tott / *resc;
	}

	return dest;
}

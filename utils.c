/*
 * Zadanie programistyczne numer 1 z sieci komputerowych
 * PROGRAM `TRACEROUTE`
 *
 * Łukasz Hanuszczak
 * ostatnia aktualizacja: 12 kwietnia 2013
 */

#include "utils.h"

/**
 * Obsługuje błędy wywalane przez gniazda i inne funkcje systemowe
 * (taka obsługa wyjątkół dla ubogich).
 *
 * @param desc Opis miejsca/sytuacji, która jest obsługiwana.
 * @param retv Wartość którą wyrzuciła funkcja.
 * @return Wartość którą wyrzuciła funkcja (do ewentualnej dalszej obsługi).
 */
int handle(const char *desc, int retv)
{
	if (retv < 0) {
		printf("%3d - %s (%s)\n", errno, strerror(errno), desc);
		exit(1);
	}

	return retv;
}

/* Porównywanie adresów, potrzebne do sortowania niżej. */
static int addrcmp(const void *a,  const void *b)
{
	return ((struct in_addr *)a)->s_addr - ((struct in_addr *)b)->s_addr;
}

/**
 * Ładne wypisywanie puli adresów (bez powtórzeń i w ogóle).
 *
 * @param addrs Tablica adresów, które mają zostać wypisane.
 * @param addrc Rozmiar tablicy.
 */
void printaddr(struct in_addr *addrs, const size_t addrc)
{
	struct hostent *hp;
	long addr; 
	uint i = 0;

	/* Sortowanie w celu niewyświetlania duplkikatów. */
	qsort(addrs, addrc, sizeof(*addrs), addrcmp);
	
	printf("%15s | ", addrc == 0 ? "*" : inet_ntoa(addrs[0]));
	addr = inet_addr(inet_ntoa(addrs[0]));
	if ((hp = gethostbyaddr((char *) &addr, sizeof(addr), AF_INET))) {
		printf("\t%s ", hp->h_name);
	}
	for ( i = 1; i < addrc; i++) {
		if (addrs[i].s_addr == addrs[i - 1].s_addr) {
			continue;
		}

		printf(" | %15s", inet_ntoa(addrs[i]));
	}
}

static void printhelp()
{
	printf(
		"Usage:\n"
		"  traceroute [-t timeout ] [ -m maxttl ] [ -r requests ] addr\n"
		"Options:\n"
		"  -t timeout  | --timeout timeout\n"
		"    Set maximum waiting time for responses in miliseconds (default: 1000).\n"
		"  -m maxttl   | --maxttl maxttl\n"
		"    Set maximum TTL value (default: 30).\n"
		"  -r requests | --requests requests\n"
		"    Set number of requests for each TTL value (default: 3).\n"
	);
}

#define OPTARG(v, short, long) (strcmp(v, short) == 0 || strcmp(v, long) == 0)

/* Parsuje pojedyńczy opcjonalny argument, zwraca wartość wywołania `scanf`. */
static int parseopt(
	const char *arg,
	const char *val,
	struct params *result
)
{
	/*
	 * Wczytujemy do `temp`, bo typ np. `timeout` może różnić
	 * się na różnych systemach. W ten sposób wczytujemy
	 * do `uint` i rzutujemy do dowolnego typu.
	 */
	uint tmp;
	const int retv = sscanf(val, "%u", &tmp);

	if (OPTARG(arg, "-t", "--timeout")) {
		result->timeout = tmp;
	} else if (OPTARG(arg, "-m", "--maxttl")) {
		result->maxttl = tmp;
	} else if (OPTARG(arg, "-r", "--requests")) {
		result->reqc = tmp;
	} else {
		printf("Unknown option: %s.\n", arg);
		printf("Try with `--help` for more information.\n");
		exit(1);
	}

	return retv;
}

/**
 * Parsuje i zwraca argumenty wywołania,
 * w przypadku braku któregokolwiek zwraca wartości domyślne.
 *
 * @param argv Argumenty wywołania (bez nazwy programu!).
 * @param argc Ilość argumentów.
 * @return Ładna struktura z zapakowanymi wszystkimi potrzebnymi danymi.
 */
struct params parseargs(char **argv, const size_t argc)
{
	uint i = 0;
	
	/* Domyślne wartości, zgodne z treścią zadania. */
	struct params result = { { 0 }, 1000, 30, 3 };
	/* Czy wczytany został adres do którego drogę chcemy wyliczać? */
	bool addr = false;

	for ( i = 0; i < argc; i++) {
		int retv = 0;

		if (OPTARG(argv[i], "-h", "--help")) {
			printhelp();
			exit(1);
		} else if (argv[i][0] == '-' && i < argc - 1) {
			retv = parseopt(argv[i], argv[i + 1], &result);
			i++;
		} else {
			retv = handle(
				"parse ip adress",
				inet_pton(AF_INET, argv[i], &result.addr.sin_addr)
			);

			addr = true;
		}

		/*
		 * `sscanf` zwraca ilość poprawnie zczytanych
		 * parametrów a `inet_pton` 1 jeżeli uda mu się
		 * sparsować adres. Wystarczy więc sprawdzić czy
		 * `retv` jest większy od zera, w przeciwnym razie
		 * argument jest nieprawidłowy.
		 */
		if (retv <= 0) {
			printf("Incorrect argument: %s.\n", argv[i]);
			printf("Try with `--help` for more information.\n");
			exit(1);
		}
	}

	if (!addr) {
		printf("Remote server address not given.\n");
		exit(1);
	}

	return result;
}

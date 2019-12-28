#include <argp.h>
struct server_arguments {
	int port;
};

error_t server_parser(int key, char *arg, struct argp_state *state);
struct server_arguments server_parseopt(int argc, char *argv[]) ;


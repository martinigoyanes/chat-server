/* This is an example program that parses the options provided on the
 * command line that are needed for assignment 0. You may copy all or
 * parts of this code in the assignment */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <argp.h>
#include "optparser.h"




 error_t server_parser(int key, char *arg, struct argp_state *state) {
 	struct server_arguments *args = state->input;
 	error_t ret = 0;
 	switch(key) {
 	case 'p':
 		/* Validate that port is correct and a number, etc!! */
 		args->port = atoi(arg);
 		if (0 /* port is invalid */) {
 			argp_error(state, "Invalid option for a port, must be a number");
 		}
 		break;
 	default:
 		ret = ARGP_ERR_UNKNOWN;
 		break;
 	}
 	return ret;
 }
 struct server_arguments server_parseopt(int argc, char *argv[]) {
 	struct server_arguments args;

 	/* bzero ensures that "default" parameters are all zeroed out */
 	bzero(&args, sizeof(args));



 	struct argp_option options[] = {
 		{ "port", 'p', "port", 0, "The port to be used for the server" ,0},
 		{0}
 	};
 	struct argp argp_settings = { options, server_parser, 0, 0, 0, 0, 0 };
 	if (argp_parse(&argp_settings, argc, argv, 0, NULL, &args) != 0) {
 		printf("Got an error condition when parsing\n");
	}

	/* What happens if you don't pass in parameters? Check args values
 	 * for sanity and required parameters being filled in */
	/* If they don't pass in all required settings, you should detect
 	 * this and return a non-zero value from main */
 	//printf("Got port %d and drop percentage %d \n", args.port, args.drop);
 	return args;
 }



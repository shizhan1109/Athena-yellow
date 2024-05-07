#include "blecon.h"

LOG_MODULE_REGISTER(blecon);

int blue_blecon_init(void)
{
	LOG_INF("Starting blue_blecon_init");
	LOG_INF("Exiting %s.", __func__);
	return 0;
}

bt_addr_le_t global_rece_addr;
static int blecon(const struct shell *sh, size_t argc, char **argv)
{
	struct getopt_state *state;
	char *svalue = NULL;
	int c;

	while ((c = getopt(argc, argv, "hps:")) != -1)
	{
		state = getopt_state_get();
		switch (c)
		{
		case 'p':
			blecon_receive_stop();
			break;
		case 's':
			char type[2] = "";
			svalue = state->optarg;
			sprintf(type, "%d", BT_ADDR_LE_RANDOM);
			bt_addr_le_from_str(svalue, type, &global_rece_addr);
			blecon_observer_start();
			break;
		case 'h':
			/* When getopt is active shell is not parsing
			 * command handler to print help message. It must
			 * be done explicitly.
			 */
			shell_help(sh);
			return SHELL_CMD_HELP_PRINTED;
		case '?':
			if (state->optopt == 's')
			{
				shell_print(sh,
							"Option -%c requires an argument.",
							state->optopt);
			}
			else if (isprint(state->optopt) != 0)
			{
				shell_print(sh,
							"Unknown option `-%c'.",
							state->optopt);
			}
			else
			{
				shell_print(sh,
							"Unknown option character `\\x%x'.",
							state->optopt);
			}
			return 1;
		default:
			break;
		}
	}

	return 0;
}

SHELL_CMD_REGISTER(blecon, NULL, "BLE Receive ibeacon", blecon);
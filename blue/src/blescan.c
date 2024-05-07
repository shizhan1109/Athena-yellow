#include "blescan.h"

LOG_MODULE_REGISTER(blescan);

int blue_blescan_init(void)
{
	int err;

	LOG_INF("Starting blue_blescan_init\n");

	/* Initialize the Bluetooth Subsystem */
	err = bt_enable(NULL);
	if (err)
	{
		LOG_ERR("Bluetooth init failed (err %d)\n", err);
		return 0;
	}

	LOG_INF("Exiting %s.\n", __func__);
	return 0;
}

bt_addr_le_t global_filt_addr;
bool global_filt_enable;
static int blescan(const struct shell *sh, size_t argc, char **argv)
{
	struct getopt_state *state;
	char *cvalue = NULL, *fvalue = NULL;
	int sflag = 0;
	int pflag = 0;
	int c;

	global_filt_enable = false;
	while ((c = getopt(argc, argv, "hspc:f:")) != -1)
	{
		state = getopt_state_get();
		switch (c)
		{
		case 's':
			sflag = 1;
			observer_start();
			break;
		case 'p':
			pflag = 1;
			observer_stop();
			break;
		case 'c':
			cvalue = state->optarg;
			break;
		case 'f':
			char type[2] = "";
			sprintf(type, "%d", BT_ADDR_LE_RANDOM);
			shell_print(sh, "%s", type);
			fvalue = state->optarg;
			bt_addr_le_from_str(fvalue, type, &global_filt_addr);
			global_filt_enable = true;
			break;
		case 'h':
			/* When getopt is active shell is not parsing
			 * command handler to print help message. It must
			 * be done explicitly.
			 */
			shell_help(sh);
			return SHELL_CMD_HELP_PRINTED;
		case '?':
			if (state->optopt == 'c')
			{
				shell_print(sh,
							"Option -%c requires an argument.",
							state->optopt);
			}
			else if (state->optopt == 'f')
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

	shell_print(sh, "sflag = %d, pflag = %d fvalue = %s", sflag, pflag, fvalue);
	return 0;
}

SHELL_CMD_REGISTER(blescan, NULL, "scan for other BLE devices", blescan);

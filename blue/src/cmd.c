#include <stdlib.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_uart.h>
LOG_MODULE_REGISTER(hci);
#define STACKSIZE 1024 * 4

static int gled(const struct shell *sh, size_t argc, char **argv)
{
	const struct shell *shv = shell_backend_uart_get_ptr();

	shell_error(sh, "%s unknown parameter: %s", argv[0], argv[1]);
	// printk("Type \"help\" for supported commands.");
    shell_execute_cmd(shv, "bt init");

    return 0;
}

SHELL_CMD_REGISTER(gled, NULL, "Control the GCU (c) User LEDs", gled);


// void print(void)
// {
//     LOG_INF("Starting print thread");
// }

// K_THREAD_DEFINE(print_id, STACKSIZE, print, NULL, NULL, NULL, K_IDLE_PRIO, 0, 0);

#include "hci.h"
#include "blescan.h"
#include "blecon.h"

/* size of stack area used by each thread */
#define STACKSIZE 1024 * 4

LOG_MODULE_REGISTER(main);

int init(void)
{
	LOG_INF("init thread");
	blue_hci_init();
	blue_blescan_init();
	blue_blecon_init();

	LOG_INF("Exiting %s thread.", __func__);
	return 0;
}

K_THREAD_DEFINE(init_id, STACKSIZE, init, NULL, NULL, NULL, K_LOWEST_APPLICATION_THREAD_PRIO, 0, 0);

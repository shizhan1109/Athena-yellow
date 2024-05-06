#include "hci.h"

LOG_MODULE_REGISTER(hci);

#define UART_DEVICE_NODE DT_CHOSEN(hci)

/* Global varaibles */
static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

K_MSGQ_DEFINE(gcu_display_msgq, sizeof(struct gcu_display), 10, 1);

/* queue to store up to 10 messages (aligned to 4-byte boundary) */
K_MSGQ_DEFINE(uart_msgq, MSG_SIZE, 10, 4);

/* receive buffer used in UART ISR callback */
static char rx_buf[MSG_SIZE];
static int rx_buf_pos;
static bool preamble_received = false;
static uint8_t expected_length = 0;
static char g_gcu_type = GCUINTI;
static char g_gcu_dev;
/* Global varaibles  END */

/*
 * Read characters from UART until line end is detected. Afterwards push the
 * data to the message queue.
 */
void serial_cb(const struct device *dev, void *user_data)
{
    uint8_t c;

    if (!uart_irq_update(uart_dev))
    {
        return;
    }

    if (!uart_irq_rx_ready(uart_dev))
    {
        return;
    }

    /* read until FIFO empty */
    while (uart_fifo_read(uart_dev, &c, 1) == 1)
    {
        if (!preamble_received)
        {
            if (c == PREAMBLE || c == '\n')
            {
                preamble_received = true;
                rx_buf[rx_buf_pos++] = c;
            }
        }
        else
        {
            rx_buf[rx_buf_pos++] = c;
            if (rx_buf_pos == 2)
            {
                expected_length = rx_buf[1] & 0x0f;
            }
            if ((rx_buf_pos > 2) && (rx_buf_pos == expected_length + 2))
            {
                /* if queue is full, message is silently dropped */
                k_msgq_put(&uart_msgq, &rx_buf, K_NO_WAIT);
                rx_buf_pos = 0;
                preamble_received = false;
                expected_length = 0;
            }
        }
        if (rx_buf_pos >= sizeof(rx_buf))
        {
            rx_buf_pos = 0;
            preamble_received = false;
            expected_length = 0;
        }
    }
}

void print_uart(char *buf, char len)
{
    for (int i = 0; i < len; i++)
    {
        uart_poll_out(uart_dev, buf[i]);
    }
}

static void print_cmd_sent(const char *buf, char len)
{
    char tmp[PRINT_SIZE];
    int cx = 0;

    for (int i = 0; i < len; i++)
    {
        cx += snprintf(tmp + cx, PRINT_SIZE - cx, "%02X ", buf[i]);
    }
    LOG_INF("Sent: %s, len=%d", tmp, len);
    return;
}

static void shell_print_cmd_sent(const struct shell *sh, const char *buf, char len)
{
    char tmp[PRINT_SIZE];
    int cx = 0;

    for (int i = 0; i < len; i++)
    {
        cx += snprintf(tmp + cx, PRINT_SIZE - cx, "%02X ", buf[i]);
    }
    shell_print(sh, "Sent: %s, len=%d", tmp, len);
    return;
}

int blue_hci_init(void)
{
    int ret;
    if (!device_is_ready(uart_dev))
    {
        LOG_ERR("UART device not found!");
        return 0;
    }

    /* configure interrupt and callback to receive data */
    ret = uart_irq_callback_user_data_set(uart_dev, serial_cb, NULL);
    if (ret < 0)
    {
        if (ret == -ENOTSUP)
        {
            LOG_ERR("Interrupt-driven UART API support not enabled");
        }
        else if (ret == -ENOSYS)
        {
            LOG_ERR("UART device does not support interrupt-driven API");
        }
        else
        {
            LOG_ERR("Error setting UART callback: %d", ret);
        }
        return 0;
    }
    uart_irq_rx_enable(uart_dev);

    return 0;
}

static int gcugraph(const struct shell *sh, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    g_gcu_type = GCUGRAPH;
    g_gcu_dev = atoi(argv[1]);

    return 0;
}

static int gcumeter(const struct shell *sh, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    g_gcu_type = GCUMETER;
    g_gcu_dev = atoi(argv[1]);

    return 0;
}

static int gcunumeric(const struct shell *sh, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    g_gcu_type = GCUNUMERIC;
    g_gcu_dev = atoi(argv[1]);

    return 0;
}

static int gcurec(const struct shell *sh, size_t argc, char **argv)
{
    struct getopt_state *state;
    int c;
    ARG_UNUSED(argc);
    int index, len, sensor_device_id = 0;
    bool start = false;
    char tx_buf[MSG_SIZE];
    hci_header header = {
        .preamble = PREAMBLE, .type = REQUEST, .len = GCUREC_LEN};

    while ((c = getopt(argc, argv, "hps")) != -1)
    {
        state = getopt_state_get();
        switch (c)
        {
        case 'p':
            start = false;
            break;
        case 's':
            start = true;
            break;
        case 'h':
            /* When getopt is active shell is not parsing
             * command handler to print help message. It must
             * be done explicitly.
             */
            shell_help(sh);
            return SHELL_CMD_HELP_PRINTED;
        case '?':
            if (isprint(state->optopt) != 0)
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

    g_gcu_dev = atoi(argv[2]);
    sensor_device_id = atoi(argv[2]);
    memset(tx_buf, 0, MSG_SIZE);
    index = 0;
    tx_buf[index++] = header.preamble;
    tx_buf[index++] = header.type << 4 | header.len;
    tx_buf[index++] = DEV_SD;
    if (start == false)
    {
        index += FILENAME_LEN_MAX;
        tx_buf[index++] = sensor_device_id;
        tx_buf[index++] = GCUREC_STOP;
    }
    else
    {
        shell_print(sh, "%s", argv[3]);
        strncpy(&tx_buf[index], argv[3], FILENAME_LEN_MAX);
        index += FILENAME_LEN_MAX;
        tx_buf[index++] = sensor_device_id;
        tx_buf[index++] = GCUREC_START;
    }

    len = sizeof(hci_header) + header.len;
    shell_print_cmd_sent(sh, tx_buf, len);
    print_uart(tx_buf, len);
    return 0;
}

static int sendres(const struct shell *sh, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    char tx_buf[MSG_SIZE];
    int index, len;
    hci_header header = {
        .preamble = PREAMBLE, .type = RESPONSE, .len = RESPONSE_LEN};

    memset(tx_buf, 0, MSG_SIZE);
    index = 0;
    tx_buf[index++] = header.preamble;
    tx_buf[index++] = header.type << 4 | header.len;
    tx_buf[index++] = DEV_LJ;
    tx_buf[index++] = RES_SUCCESS;

    len = sizeof(hci_header) + header.len;
    shell_print_cmd_sent(sh, tx_buf, len);
    print_uart(tx_buf, len);

    memset(tx_buf, 0, MSG_SIZE);
    index = 0;
    tx_buf[index++] = header.preamble;
    tx_buf[index++] = header.type << 4 | header.len;
    tx_buf[index++] = DEV_LJ;
    tx_buf[index++] = RES_FAIL;

    len = sizeof(hci_header) + header.len;
    shell_print_cmd_sent(sh, tx_buf, len);
    print_uart(tx_buf, len);
    memset(tx_buf, 0, MSG_SIZE);
    index = 0;
    tx_buf[index++] = header.preamble;
    tx_buf[index++] = header.type << 4 | header.len;
    tx_buf[index++] = DEV_BUTTON;
    tx_buf[index++] = RES_SUCCESS;

    len = sizeof(hci_header) + header.len;
    shell_print_cmd_sent(sh, tx_buf, len);
    print_uart(tx_buf, len);
    memset(tx_buf, 0, MSG_SIZE);
    index = 0;
    tx_buf[index++] = header.preamble;
    tx_buf[index++] = header.type << 4 | header.len;
    tx_buf[index++] = DEV_BUTTON;
    tx_buf[index++] = RES_FAIL;

    len = sizeof(hci_header) + header.len;
    shell_print_cmd_sent(sh, tx_buf, len);
    print_uart(tx_buf, len);
    return 0;
}

void consumer_thread(void)
{
    struct gcu_display data;
    int index, len;
    char tx_buf[MSG_SIZE];
    hci_header header = {
        .preamble = PREAMBLE, .type = REQUEST, .len = GCUGRAPH_LEN};
    int device_id;
    float16_t value;

    while (1)
    {
        /* get a data item */
        k_msgq_get(&gcu_display_msgq, &data, K_FOREVER);

        /* process data item */
        LOG_DBG("%d %.1f", data.device_id, (double)data.value);

        device_id = data.device_id;
        value = data.value;
        if (g_gcu_type == GCUINTI)
        {
            LOG_DBG("g_gcu_type is GCUINTI");
            continue;
        }
        if (g_gcu_dev != device_id)
        {
            LOG_DBG("g_gcu_dev %d != device_id %d", g_gcu_dev, device_id);
            continue;
        }
        memset(tx_buf, 0, MSG_SIZE);
        index = 0;
        tx_buf[index++] = header.preamble;
        tx_buf[index++] = header.type << 4 | header.len;
        tx_buf[index++] = device_id;
        memcpy(&tx_buf[index], &value, sizeof(value));
        index += sizeof(value);
        tx_buf[index++] = g_gcu_type;

        len = sizeof(hci_header) + header.len;
        print_cmd_sent(tx_buf, len);
        print_uart(tx_buf, len);
        LOG_INF("%d %.1f", data.device_id, (double)data.value);
    }
}

void print(void)
{
    LOG_INF("Starting print thread");
    char tx_buf[MSG_SIZE];

    /* indefinitely wait for input from the user */
    while (k_msgq_get(&uart_msgq, &tx_buf, K_FOREVER) == 0)
    {
        char tmp[PRINT_SIZE];
        int cx = 0, length = 0;

        length = (tx_buf[LEN_BYTE] & LEN_BYTE_MASK) + 2;

        for (int i = 0; i < length && i < PRINT_SIZE; i++)
        {
            cx += snprintf(tmp + cx, sizeof(tmp) - cx, "%02X ", tx_buf[i]);
        }
        LOG_INF("Received: %s", tmp);

        if (tx_buf[2] == 0x2)
        {
            if (tx_buf[3] == 0x1)
            {
                LOG_INF("Button on ");
            }
            else
            {
                LOG_INF("Button off ");
            }
        }
        else /* (tx_buf[2] != 0x2) */
        {
            if (tx_buf[3] == 0x1)
            {
                LOG_INF("Response success ");
            }
            else
            {
                LOG_INF("Response fail ");
            }
        }
    }
    LOG_INF("Exiting %s thread.", __func__);
}

SHELL_CMD_REGISTER(sendres, NULL, "send response data to myself", sendres);
SHELL_CMD_REGISTER(gcugraph, NULL, "displays the selected sensor values as a graph plot", gcugraph);
SHELL_CMD_REGISTER(gcumeter, NULL, "displays the selected sensor values as a meter widget", gcumeter);
SHELL_CMD_REGISTER(gcunumeric, NULL, "displays the selected sensor values as a numeric value", gcunumeric);
SHELL_CMD_REGISTER(gcurec, NULL, "start/stop continuously recording the selected sensor", gcurec);

K_THREAD_DEFINE(print_id, STACKSIZE, print, NULL, NULL, NULL, K_IDLE_PRIO, 0, 0);
K_THREAD_DEFINE(consumer_thread_id, STACKSIZE, consumer_thread, NULL, NULL, NULL, K_IDLE_PRIO, 0, 0);
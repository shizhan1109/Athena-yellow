#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>
#include <zephyr/dsp/types.h>

/* size of stack area used by each thread */
#define STACKSIZE 1024 * 4

/* scheduling priority used by each thread */
#define PRIORITY1 1
#define PRIORITY7 7

#define MSG_SIZE 32
#define PRINT_SIZE 64

/* byte 0 */
#define PREAMBLE 0xAA

/* byte 1, low 4 bit */
#define REQUEST 0x01
#define RESPONSE 0x02

/* byte 1, high 4 bit  */
#define LEN_BYTE 0x1
#define LEN_BYTE_MASK 0xF
#define GLED_LEN 0x2
#define GPB_LEN 0x2
#define POINT_LEN 0x6
#define DAC_LEN 0x6
#define LJ_LEN 0x5
#define CIRCLE_LEN 0x7
#define RESPONSE_LEN 0x2
#define GCUGRAPH_LEN 0x4
#define GCUREC_LEN 0xC

/* byte 2 */
#define DEV_LED 0x01
#define DEV_BUTTON 0x02
#define DEV_DAC 0x03
#define DEV_LJ 0x04
#define DEV_TEMP 0x05
#define DEV_HUM 0x06
#define DEV_PRE 0x07
#define DEV_TVOC 0x08
#define DEV_SD 0x13

#define RES_SUCCESS 0x01
#define RES_FAIL 0x02

/* gcu type */
#define GCUGRAPH 0x01
#define GCUMETER 0x02
#define GCUNUMERIC 0x03
#define GCUINTI 0xFF
#define GCUREC_START 0x01
#define GCUREC_STOP 0x02

#define FILENAME_LEN_MAX 0x09

typedef struct
{
    uint8_t preamble;
    uint8_t type : 4;
    uint8_t len : 4;
} hci_header;

struct gcu_display
{
    int device_id;
    float16_t value;
};
extern struct k_msgq gcu_display_msgq;

/* Function declaration */
void process_packet(const uint8_t *packet, size_t length);
int blue_hci_init(void);
/* Function declaration END */
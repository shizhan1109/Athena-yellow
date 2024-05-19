/******************************** INCLUDE FILES *******************************/
#include <stdbool.h>
/******************************** LOCAL DEFINES *******************************/

/********************************* TYPEDEFS ***********************************/
typedef struct _server_t server_t;

typedef enum _server_proto_t
{
    SERVER_PROTO_UDP = 0,
    SERVER_PROTO_TCP,
    SERVER_PROTO_N
} server_proto_t;

#define MAX_IMAGE_SIZE 10000

extern struct k_msgq image_msgq;
typedef struct {
    uint32_t size;                    // Image size
    char *data_ptr;        // Image data
} image_message_t;



/***************************** INTERFACE FUNCTIONS ****************************/
server_t *server_new (server_proto_t type, int port, bool register_service);
void server_destroy (server_t **self_p);
void server_start (void *p1, void *p2, void *p3);

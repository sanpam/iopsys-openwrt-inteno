#ifndef CATV_H
#include "libubus.h"

struct catv_handler;

struct catv_handler * catv_init(char * i2c_bus, int i2c_addr);
void catv_destroy(struct catv_handler *h);
int catv_ubus_populate(struct catv_handler *h, struct ubus_context *ubus_ctx);

#endif /* CATV_H */

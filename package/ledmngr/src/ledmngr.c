/*
 * ledmngr.c -- Led manager for Inteno CPE's
 *
 * Copyright (C) 2012-2013 Inteno Broadband Technology AB. All rights reserved.
 *
 * Author: benjamin.larsson@inteno.se
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include <unistd.h>

#include <libubox/blobmsg_json.h>
#include "libubus.h"

#include <board.h>
#include "ucix.h"

static struct ubus_context *ctx;
static struct ubus_subscriber test_event;
static struct blob_buf b;

static struct leds_configuration* led_cfg;

#define LED_FUNCTIONS 13
#define MAX_LEDS 20
#define SR_MAX 16
enum {
    OFF,
    ON,
    BLINK_SLOW,
    BLINK_FAST,
};

enum {
    RED,
    GREEN,
    BLUE,
};

enum {
    GPIO,
    LEDCTL,
    SHIFTREG2,
    SHIFTREG3,
};

enum {
    ACTIVE_HIGH,
    ACTIVE_LOW,
};

struct led_config {
    /* Configuration */
    char*   name;
    char*   function;
    int     color;
    int     type;
    int     address;
    int     active;
    /* State */
    int     state;
    int     blink_state;
} led_config;

struct led_map {
    char*   led_function;
    char*   led_name;
};

static struct led_map led_map_config[LED_FUNCTIONS] = {
    {"dsl",NULL},
    {"wifi",NULL},
    {"wps",NULL},
    {"lan",NULL},
    {"status",NULL},
    {"dect",NULL},
    {"tv",NULL},
    {"usb",NULL},
    {"wan",NULL},
    {"internet",NULL},
    {"voice1",NULL},
    {"voice2",NULL},
    {"eco",NULL},
};

struct leds_configuration {
    int             leds_nr;
    struct led_config**  leds;
    int fd;
    int shift_register_state[SR_MAX];
    struct led_map* led_map_cfg;
} leds_configuration;

static int add_led(struct leds_configuration* led_cfg, char* led_name, const char* led_config, int color) {

    if (!led_config) {
//        printf("Led %s: not configured\n",led_name);
        return -1;
    } else {
        struct led_config* lc = malloc(sizeof(struct led_config));
        char type[256];
        char active[256];
        char function[256];
        int  address;

        printf("Led %s: %s\n",led_name, led_config);
        lc->name = strdup(led_name);
        // gpio,39,al
        sscanf(led_config, "%s %d %s %s", type, &address, active, function);
//        printf("Config %s,%d,%s,%s\n", type, address, active, function);
        
        if (!strcmp(type, "gpio")) lc->type = GPIO;
        if (!strcmp(type, "sr"))   lc->type = SHIFTREG2;
        if (!strcmp(type, "csr"))  lc->type = SHIFTREG3;

        lc->address = address;
        lc->color = color;

        if (!strcmp(active, "al"))   lc->active = ACTIVE_LOW;
        if (!strcmp(active, "ah"))   lc->active = ACTIVE_HIGH;

        //realloc(led_cfg->leds, (led_cfg->leds_nr+1) * sizeof(struct led_config*));
        if (led_cfg->leds_nr >= MAX_LEDS) {
            printf("Too many leds configured! Only adding the %d first\n", MAX_LEDS);
            return -1;
        }
        led_cfg->leds[led_cfg->leds_nr] = lc;
        led_cfg->leds_nr++;
        return 0;
    }
}


static void open_ioctl(struct leds_configuration* led_cfg) {

    led_cfg->fd = open("/dev/brcmboard", O_RDWR);
    if ( led_cfg->fd == -1 ) {
        fprintf(stderr, "failed to open: /dev/brcmboard\n");
        return;
    }
    return;
}


struct leds_configuration* get_led_config(void) {
    int i;
    struct uci_context *ctx = NULL;
    const char *led_names;
    const char *led_config;
    char *p, *ptr, *rest;

    struct leds_configuration* led_cfg = malloc(sizeof(struct leds_configuration));
    led_cfg->leds_nr = 0;
    led_cfg->leds = malloc(MAX_LEDS * sizeof(struct led_config*));
    /* Initialize */
	ctx = ucix_init_path("/lib/db/config/", "hw");
    if(!ctx) {
        printf("Failed to load config file \"hw\"\n");
        return NULL;
    }
    
    led_names = ucix_get_option(ctx, "hw", "board", "lednames");
//    printf("Led names: %s\n", led_names);

    /* Populate led configuration structure */
    ptr = (char *)led_names;
    p = strtok_r(ptr, " ", &rest);
    while(p != NULL) {
        char led_name_color[256] = {0};

        printf("%s\n", p);

        snprintf(led_name_color, 256, "%s_green", p);               
        led_config = ucix_get_option(ctx, "hw", "leds", led_name_color);
        add_led(led_cfg, led_name_color, led_config, GREEN);
        //printf("%s_green = %s\n", p, led_config);

        snprintf(led_name_color,   256, "%s_red", p); 
        led_config = ucix_get_option(ctx, "hw", "leds", led_name_color);
        add_led(led_cfg, led_name_color, led_config, RED);
        //printf("%s_red = %s\n", p, led_config);

        snprintf(led_name_color,  256, "%s_blue", p);
        led_config = ucix_get_option(ctx, "hw", "leds", led_name_color);
        add_led(led_cfg, led_name_color, led_config, BLUE);
        //printf("%s_blue = %s\n", p, led_config);

        /* Get next */
        ptr = rest;
        p = strtok_r(NULL, " ", &rest);
    }
//    printf("%d leds added to config\n", led_cfg->leds_nr);

    open_ioctl(led_cfg);

    //reset shift register states
    for (i=0 ; i<SR_MAX ; i++) led_cfg->shift_register_state[i] = 0;

    //populate led mapping
    led_cfg->led_map_cfg = led_map_config;
    for (i=0 ; i<LED_FUNCTIONS ; i++) {
        const char *conf_led;

        conf_led = ucix_get_option(ctx, "hw", "led_map", led_cfg->led_map_cfg[i].led_function);
        if (conf_led)
            led_cfg->led_map_cfg[i].led_name = strdup(conf_led);
        if (led_cfg->led_map_cfg[i].led_name)
            printf("led map %s - %s\n", led_cfg->led_map_cfg[i].led_function, conf_led?conf_led:"(null)");

    }

    return led_cfg;
}


void print_config(struct leds_configuration* led_cfg) {
    int i;
    printf("\n\n\n Leds: %d\n", led_cfg->leds_nr);

    for (i=0 ; i<led_cfg->leds_nr ; i++) {
        struct led_config* lc = led_cfg->leds[i];
        printf("%s: type: %d, adr:%d, color:%d, act:%d\n", lc->name, lc->type, lc->address, lc->color, lc->active);
    }
}


static int get_led_index_by_name(struct leds_configuration* led_cfg, char* led_name) {
    int i;
    for (i=0 ; i<led_cfg->leds_nr ; i++) {
        struct led_config* lc = led_cfg->leds[i];
        if (!strcmp(led_name, lc->name))
            return i;
    }
    printf("Led name %s not found!\n", led_name);
    return -1;
}

static int get_led_index_by_function_color(struct leds_configuration* led_cfg, char* function, int color) {
    int i;
    for (i=0 ; i<led_cfg->leds_nr ; i++) {
        struct led_config* lc = led_cfg->leds[i];
        if (!strcmp(function, lc->function) && (lc->color == color))
            return i;
    }
    return -1;
}

static void board_ioctl(int fd, int ioctl_id, int action, int hex, char* string_buf, int string_buf_len, int offset) {
    BOARD_IOCTL_PARMS IoctlParms;
    IoctlParms.string = string_buf;
    IoctlParms.strLen = string_buf_len;
    IoctlParms.offset = offset;
    IoctlParms.action = action;
    IoctlParms.buf    = "";
    if ( ioctl(fd, ioctl_id, &IoctlParms) < 0 ) {
        fprintf(stderr, "ioctl: %d failed\n", ioctl_id);
        exit(1);
    }
}

static void shift_register3_set(struct leds_configuration* led_cfg, int address, int state, int active) {
    int i;

    if (address>=SR_MAX-1) {
        fprintf(stderr, "address index %d too large\n", address);
        return;
    }
    // Update internal register copy
    led_cfg->shift_register_state[address] = state^active;

    // pull down shift register load (load gpio 23)
    board_ioctl(led_cfg->fd, BOARD_IOCTL_SET_GPIO, 0, 0, NULL, 23, 0);

//    for (i=0 ; i<SR_MAX ; i++) printf("%d ", led_cfg->shift_register_state[SR_MAX-1-i]);
//    printf("\n");

    // clock in bits
    for (i=0 ; i<SR_MAX ; i++) {

        //set clock low
        board_ioctl(led_cfg->fd, BOARD_IOCTL_SET_GPIO, 0, 0, NULL, 0, 0);
        //place bit on data line
        board_ioctl(led_cfg->fd, BOARD_IOCTL_SET_GPIO, 0, 0, NULL, 1, led_cfg->shift_register_state[SR_MAX-1-i]);
        //set clock high
        board_ioctl(led_cfg->fd, BOARD_IOCTL_SET_GPIO, 0, 0, NULL, 0, 1);
    }

    // issue shift register load
    board_ioctl(led_cfg->fd, BOARD_IOCTL_SET_GPIO, 0, 0, NULL, 23, 1);
}

static int led_set(struct leds_configuration* led_cfg, char* led_name, int state) {
    int led_idx;
    struct led_config* lc;

    led_idx = get_led_index_by_name(led_cfg, led_name);
    lc = led_cfg->leds[led_idx];

    //printf("Led index: %d\n", led_idx);

    if (lc->type == GPIO) {
        board_ioctl(led_cfg->fd, BOARD_IOCTL_SET_GPIO, 0, 0, NULL, lc->address, state^lc->active);
    } else if (lc->type == SHIFTREG3){
        shift_register3_set(led_cfg, lc->address, state, lc->active);
    }
    lc->blink_state = state;

    return 0;
}

static void all_leds_off(struct leds_configuration* led_cfg) {
    int i;
    for (i=0 ; i<led_cfg->leds_nr ; i++) {
        
        led_set(led_cfg, led_cfg->leds[i]->name, OFF);
    }
}

static void all_leds_on(struct leds_configuration* led_cfg) {
    int i;
    for (i=0 ; i<led_cfg->leds_nr ; i++) {
        
        led_set(led_cfg, led_cfg->leds[i]->name, ON);
    }
}

static void all_leds_test(struct leds_configuration* led_cfg) {
    int i;
    //all_leds_off(led_cfg);
    for (i=0 ; i<led_cfg->leds_nr ; i++) {
        led_set(led_cfg, led_cfg->leds[i]->name, ON);
        sleep(1);        
        led_set(led_cfg, led_cfg->leds[i]->name, OFF);
    }
    all_leds_off(led_cfg);
    sleep(1);
    all_leds_on(led_cfg);
    sleep(1);
    all_leds_off(led_cfg);
    sleep(1);
    all_leds_on(led_cfg);
    sleep(1);
    all_leds_off(led_cfg);
}


void blink_led(struct leds_configuration* led_cfg, int state) {
    int i;
    for (i=0 ; i<led_cfg->leds_nr ; i++) {
        struct led_config* lc = led_cfg->leds[i];
        if (lc->state == state) {
            //printf("Blinking %s\n", lc->name);
            led_set(led_cfg, lc->name, lc->blink_state?0:1);
        }
    }
}


static void blink_handler(struct uloop_timeout *timeout);
static struct uloop_timeout blink_inform_timer = { .cb = blink_handler };
static unsigned int cnt = 0;


static void blink_handler(struct uloop_timeout *timeout)
{
    cnt++;

    if (!(cnt%4))
        blink_led(led_cfg, BLINK_FAST);

    if (!(cnt%8))
        blink_led(led_cfg, BLINK_SLOW);

	uloop_timeout_set(&blink_inform_timer, 100);
    
    //printf("Timer\n");

}



void set_function_led(struct leds_configuration* led_cfg, char* fn_name, const char* color, const char* state) {
    int i, led_idx;
    char* led_name = NULL;
    char led_name_color[256];
    int istate = -1;

    for (i=0 ; i<LED_FUNCTIONS ; i++) {
        if (!strcmp(fn_name, led_cfg->led_map_cfg[i].led_function))
            led_name = led_cfg->led_map_cfg[i].led_name;
    }
    if (!(led_name)) return;

    snprintf(led_name_color, 256, "%s_%s", led_name, color);  

    if (!strcmp(state,"ON")) istate = ON;
    if (!strcmp(state,"OFF")) istate = OFF;
    if (!strcmp(state,"BLINK_SLOW")) istate = BLINK_SLOW;
    if (!strcmp(state,"BLINK_FAST")) istate = BLINK_FAST;
    printf("Timer1\n");

    /* Set led structure state */
    led_idx = get_led_index_by_name(led_cfg, led_name_color);
    if (led_idx == -1) return;
    led_cfg->leds[led_idx]->state = istate;

    printf("Timer\n");

    /* Set the led state */
    if ((istate == ON) || (istate==OFF))
        led_set(led_cfg, led_name_color, istate);
}



enum {
	HELLO_ID,
	HELLO_MSG,
	__HELLO_MAX
};

enum {
	LED_COLOR,
	LED_STATE,
	__LED_MAX
};

static const struct blobmsg_policy hello_policy[] = {
	[HELLO_ID] = { .name = "id", .type = BLOBMSG_TYPE_INT32 },
	[HELLO_MSG] = { .name = "msg", .type = BLOBMSG_TYPE_STRING },
};

static const struct blobmsg_policy led_policy[] = {
	[LED_COLOR] = { .name = "color", .type = BLOBMSG_TYPE_STRING },
	[LED_STATE] = { .name = "state", .type = BLOBMSG_TYPE_STRING },
};

struct hello_request {
	struct ubus_request_data req;
	struct uloop_timeout timeout;
	char data[];
};

static void test_hello_reply(struct uloop_timeout *t)
{
	struct hello_request *req = container_of(t, struct hello_request, timeout);

	blob_buf_init(&b, 0);
	blobmsg_add_string(&b, "message", req->data);
	ubus_send_reply(ctx, &req->req, b.head);
	ubus_complete_deferred_request(ctx, &req->req, 0);
	free(req);
}

static int test_hello(struct ubus_context *ctx, struct ubus_object *obj,
		      struct ubus_request_data *req, const char *method,
		      struct blob_attr *msg)
{
	struct hello_request *hreq;
	struct blob_attr *tb[__LED_MAX];
	const char *format = "%s received a message: %s";
	const char *msgstr = "(unknown)";
    const char *state, *color;

	blobmsg_parse(led_policy, ARRAY_SIZE(led_policy), tb, blob_data(msg), blob_len(msg));

	if ((tb[LED_STATE]) && (tb[LED_COLOR])) {
        char *fn_name = strchr(obj->name, '.') + 1;
		state = blobmsg_data(tb[LED_STATE]);
		color = blobmsg_data(tb[LED_COLOR]);
    	fprintf(stderr, "Led %s method: %s color %s state %s\n", fn_name, method, color, state);
        
        set_function_led(led_cfg, fn_name, color, state);
    }

    

	hreq = calloc(1, sizeof(*hreq) + strlen(format) + strlen(obj->name) + strlen(msgstr) + 1);
	sprintf(hreq->data, format, obj->name, msgstr);
	ubus_defer_request(ctx, req, &hreq->req);
	hreq->timeout.cb = test_hello_reply;
	uloop_timeout_set(&hreq->timeout, 1000);

	return 0;
}

enum {
	WATCH_ID,
	WATCH_COUNTER,
	__WATCH_MAX
};

static const struct blobmsg_policy watch_policy[__WATCH_MAX] = {
	[WATCH_ID] = { .name = "id", .type = BLOBMSG_TYPE_INT32 },
	[WATCH_COUNTER] = { .name = "counter", .type = BLOBMSG_TYPE_INT32 },
};

static void
test_handle_remove(struct ubus_context *ctx, struct ubus_subscriber *s,
                   uint32_t id)
{
	fprintf(stderr, "Object %08x went away\n", id);
}

static int
test_notify(struct ubus_context *ctx, struct ubus_object *obj,
	    struct ubus_request_data *req, const char *method,
	    struct blob_attr *msg)
{

	return 0;
}

static int test_watch(struct ubus_context *ctx, struct ubus_object *obj,
		      struct ubus_request_data *req, const char *method,
		      struct blob_attr *msg)
{
	struct blob_attr *tb[__WATCH_MAX];
	int ret;

	blobmsg_parse(watch_policy, __WATCH_MAX, tb, blob_data(msg), blob_len(msg));
	if (!tb[WATCH_ID])
		return UBUS_STATUS_INVALID_ARGUMENT;

	test_event.remove_cb = test_handle_remove;
	test_event.cb = test_notify;
	ret = ubus_subscribe(ctx, &test_event, blobmsg_get_u32(tb[WATCH_ID]));
	fprintf(stderr, "Watching object %08x: %s\n", blobmsg_get_u32(tb[WATCH_ID]), ubus_strerror(ret));
	return ret;
}

static const struct ubus_method test_methods[] = {
	UBUS_METHOD("status", test_hello, led_policy),
	UBUS_METHOD("set", test_watch, led_policy),
};

static struct ubus_object_type test_object_type =
	UBUS_OBJECT_TYPE("led", test_methods);

#define LED_OBJECTS 13

static struct ubus_object led_objects[LED_OBJECTS] = {
    { .name = "led.dsl",	.type = &test_object_type, .methods = test_methods, .n_methods = ARRAY_SIZE(test_methods), },
    { .name = "led.wifi",	.type = &test_object_type, .methods = test_methods, .n_methods = ARRAY_SIZE(test_methods), },
    { .name = "led.wps",	.type = &test_object_type, .methods = test_methods, .n_methods = ARRAY_SIZE(test_methods), },
    { .name = "led.lan",	.type = &test_object_type, .methods = test_methods, .n_methods = ARRAY_SIZE(test_methods), },
    { .name = "led.status",	.type = &test_object_type, .methods = test_methods, .n_methods = ARRAY_SIZE(test_methods), },
    { .name = "led.dect",	.type = &test_object_type, .methods = test_methods, .n_methods = ARRAY_SIZE(test_methods), },
    { .name = "led.tv",	.type = &test_object_type, .methods = test_methods, .n_methods = ARRAY_SIZE(test_methods), },
    { .name = "led.usb",	.type = &test_object_type, .methods = test_methods, .n_methods = ARRAY_SIZE(test_methods), },
    { .name = "led.wan",	.type = &test_object_type, .methods = test_methods, .n_methods = ARRAY_SIZE(test_methods), },
    { .name = "led.internet",	.type = &test_object_type, .methods = test_methods, .n_methods = ARRAY_SIZE(test_methods), },
    { .name = "led.voice1",	.type = &test_object_type, .methods = test_methods, .n_methods = ARRAY_SIZE(test_methods), },
    { .name = "led.voice2",	.type = &test_object_type, .methods = test_methods, .n_methods = ARRAY_SIZE(test_methods), },
    { .name = "led.eco",	.type = &test_object_type, .methods = test_methods, .n_methods = ARRAY_SIZE(test_methods), },
};



static void server_main(struct leds_configuration* led_cfg)
{
	int ret, i;

    for (i=0 ; i<LED_OBJECTS ; i++) {
	    ret = ubus_add_object(ctx, &led_objects[i]);
	    if (ret)
		    fprintf(stderr, "Failed to add object: %s\n", ubus_strerror(ret));
    }
	    ret = ubus_register_subscriber(ctx, &test_event);
	    if (ret)
		    fprintf(stderr, "Failed to add watch handler: %s\n", ubus_strerror(ret));


    uloop_timeout_set(&blink_inform_timer, 100);

	uloop_run();
}


int ledmngr(void) {
    int ret;
	const char *ubus_socket = NULL;


    led_cfg = get_led_config();


    /* initialize ubus */

	uloop_init();

	ctx = ubus_connect(ubus_socket);
	if (!ctx) {
		fprintf(stderr, "Failed to connect to ubus\n");
		return -1;
	}

	ubus_add_uloop(ctx);

	server_main(led_cfg);


    //all_leds_test(led_cfg);

	ubus_free(ctx);
	uloop_done();

	return 0;
}


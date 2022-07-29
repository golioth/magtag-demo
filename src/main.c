/*
 * Copyright (c) 2022 Golioth, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Logging */
#include <stdlib.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(golioth_magtag, LOG_LEVEL_DBG);
#include <soc/soc_memory_layout.h>

/* MagTag specific hardware includes */
#include "epaper/EPD_2in9d.h"
#include "epaper/ImageData.h"
#include "ws2812/ws2812_control.h"

/* Golioth platform includes */
#include <net/coap.h>
#include <net/golioth/fw.h>
#include <net/golioth/system_client.h>
#include <samples/common/wifi.h>

#define FRAME_SIZE	5000
uint8_t *local_frame;
uint16_t frame_idx = 0;
bool have_new_frame = false;

static char current_version_str[sizeof("255.255.65535")] = "1.0.0";
static struct golioth_client *client = GOLIOTH_SYSTEM_CLIENT_GET();

static struct coap_reply coap_replies[4];

struct flash_img_context {
	/* empty */
};

struct dfu_ctx {
	struct golioth_fw_download_ctx fw_ctx;
	struct flash_img_context flash;
	char version[65];
};

static struct dfu_ctx update_ctx;
static enum golioth_dfu_result dfu_initial_result = GOLIOTH_DFU_RESULT_INITIAL;

static int data_received(struct golioth_blockwise_download_ctx *ctx,
			 const uint8_t *data, size_t offset, size_t len,
			 bool last)
{
	struct dfu_ctx *dfu = CONTAINER_OF(ctx, struct dfu_ctx, fw_ctx);
	int err;

	LOG_DBG("Received %zu bytes at offset %zu%s", len, offset,
		last ? " (last)" : "");

	if (offset == 0) { frame_idx = 0; }

	if ((frame_idx+len) >= FRAME_SIZE) {
		LOG_ERR("Received data too large for storage array.");
		return -EMSGSIZE;
		}

	LOG_INF("Received %d bytes at frame_idx %d", len, frame_idx);

	for (uint16_t i=0; i<len; i++) {
		local_frame[frame_idx] = data[i];
		++frame_idx;
	}

	if (last) { 
		LOG_HEXDUMP_DBG(local_frame+(frame_idx-16), 16, "local_frame:");
		have_new_frame = true;
	}

	return 0;
}

static uint8_t *uri_strip_leading_slash(uint8_t *uri, size_t *uri_len)
{
	if (*uri_len > 0 && uri[0] == '/') {
		(*uri_len)--;
		return &uri[1];
	}

	return uri;
}

static int golioth_desired_update(const struct coap_packet *update,
				  struct coap_reply *reply,
				  const struct sockaddr *from)
{
	struct dfu_ctx *dfu = &update_ctx;
	struct coap_reply *fw_reply;
	const uint8_t *payload;
	uint16_t payload_len;
	size_t version_len = sizeof(dfu->version) - 1;
	uint8_t uri[64];
	uint8_t *uri_p;
	size_t uri_len = sizeof(uri);
	int err;

	payload = coap_packet_get_payload(update, &payload_len);
	if (!payload) {
		LOG_ERR("No payload in CoAP!");
		return -EIO;
	}

	LOG_HEXDUMP_DBG(payload, payload_len, "Desired");

	err = golioth_fw_desired_parse(payload, payload_len,
				       dfu->version, &version_len,
				       uri, &uri_len);
	if (err) {
		LOG_ERR("Failed to parse desired version: %d", err);
		return err;
	}

	dfu->version[version_len] = '\0';

	if (version_len == strlen(current_version_str) &&
	    !strncmp(current_version_str, dfu->version, version_len)) {
		LOG_INF("Desired version (%s) matches current firmware version!",
			log_strdup(current_version_str));
		return -EALREADY;
	}

	fw_reply = coap_reply_next_unused(coap_replies, ARRAY_SIZE(coap_replies));
	if (!reply) {
		LOG_ERR("No more reply handlers");
		return -ENOMEM;
	}

	uri_p = uri_strip_leading_slash(uri, &uri_len);

	err = golioth_fw_report_state(client, "frame",
				      current_version_str,
				      dfu->version,
				      GOLIOTH_FW_STATE_DOWNLOADING,
				      GOLIOTH_DFU_RESULT_INITIAL);
	if (err) {
		LOG_ERR("Failed to update to '%s' state: %d", "downloading", err);
	}

	err = golioth_fw_download(client, &dfu->fw_ctx, uri_p, uri_len,
				  fw_reply, data_received);
	if (err) {
		LOG_ERR("Failed to request firmware: %d", err);
		return err;
	}

	return 0;
}

/*
 * In the `main` function, this function is registed to be
 * called when the device receives a packet from the Golioth server.
 */
static void golioth_on_message(struct golioth_client *client,
			       struct coap_packet *rx)
{
	uint16_t payload_len;
	const uint8_t *payload;
	uint8_t type;

	type = coap_header_get_type(rx);
	payload = coap_packet_get_payload(rx, &payload_len);

	(void)coap_response_received(rx, NULL, coap_replies,
				     ARRAY_SIZE(coap_replies));
}

static void golioth_on_connect(struct golioth_client *client)
{
	struct coap_reply *reply;
	int err;
	int i;

	err = golioth_fw_report_state(client, "main",
				      current_version_str,
				      NULL,
				      GOLIOTH_FW_STATE_IDLE,
				      dfu_initial_result);
	if (err) {
		LOG_ERR("Failed to report firmware state: %d", err);
	}

	for (i = 0; i < ARRAY_SIZE(coap_replies); i++) {
		coap_reply_clear(&coap_replies[i]);
	}

	reply = coap_reply_next_unused(coap_replies, ARRAY_SIZE(coap_replies));
	if (!reply) {
		LOG_ERR("No more reply handlers");
	}

	err = golioth_fw_observe_desired(client, reply, golioth_desired_update);
	if (err) {
		coap_reply_clear(reply);
	}
}

void main(void)
{
	LOG_DBG("Start MagTag Hello demo");
	local_frame = k_malloc(FRAME_SIZE);

	/* WiFi */
	if (IS_ENABLED(CONFIG_GOLIOTH_SAMPLE_WIFI)) {
		LOG_INF("Connecting to WiFi");
		wifi_connect();
	}

	client->on_connect = golioth_on_connect;
	client->on_message = golioth_on_message;
	golioth_system_client_start();


	/* Initialize MagTag hardware */
	ws2812_init();
	/* show two blue pixels to show until we connect to Golioth */
	leds_immediate(BLACK, BLACK, BLUE, BLUE);
	epaper_init();

	/* wait until we've connected to golioth */
	while (golioth_ping(client) != 0)
	{
		k_msleep(1000);
	}
	/* turn LEDs green to indicate connection */
	leds_immediate(BLACK, BLACK, GREEN, GREEN);
	epaper_autowrite("Connected to Golioth!", 21);

	int counter = 0;
	int err;
	while (true) {
		/* Send hello message to the Golioth Cloud */
		LOG_INF("Sending hello! %d", counter);
		err = golioth_send_hello(client);
		if (err) {
			LOG_WRN("Failed to send hello!");
		}

		if (have_new_frame) {
			have_new_frame = false;
			if (frame_idx >= 4736) {
				leds_immediate(BLACK, BLACK, BLACK, BLACK);
				EPD_2IN9D_Init();
				EPD_2IN9D_Clear();
				EPD_2IN9D_Display(local_frame+(frame_idx-4736));
				EPD_2IN9D_Sleep();
			}
		}
		++counter;
		k_sleep(K_SECONDS(5));
	}
}

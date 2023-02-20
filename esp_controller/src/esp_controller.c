#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <libubox/blobmsg_json.h>
#include <libubus.h>
#include <syslog.h>

#define ACTION_ON "on"
#define ACTION_OFF "off"

int initSerial(int *fd, char *devpath);
char *getSerialBuff(int *fd, char *buffer);
char *handle_request(uint32_t pin, char *action);

static int esp_on(struct ubus_context *ctx, struct ubus_object *obj,
		      struct ubus_request_data *req, const char *method,
		      struct blob_attr *msg);

static int esp_off(struct ubus_context *ctx, struct ubus_object *obj,
		      struct ubus_request_data *req, const char *method,
		      struct blob_attr *msg);

enum {
	ESP_ON_PIN,
	__ESP_ON_MAX,
};

static const struct blobmsg_policy esp_policy_on[__ESP_ON_MAX] = {
	[ESP_ON_PIN] = { .name = "pin", .type = BLOBMSG_TYPE_INT32 },
};

static int esp_on(struct ubus_context *ctx,
				   struct ubus_object *obj,
				   struct ubus_request_data *req,
				   const char *method, struct blob_attr *msg)
{
	struct blob_attr *tb[ESP_ON_PIN];
	struct blob_buf b = {};
	blob_buf_init(&b, 0);
	char *response;

	if (!msg)
		return UBUS_STATUS_INVALID_ARGUMENT;

	blobmsg_parse(esp_policy_on, __ESP_ON_MAX, tb, blob_data(msg), blob_len(msg));
	if (!tb[ESP_ON_PIN])
		return UBUS_STATUS_INVALID_ARGUMENT;

	uint32_t pin = blobmsg_get_u32(tb[ESP_ON_PIN]);
	response = handle_request(pin, ACTION_ON);

	if(response != NULL){
		blob_buf_init(&b, 0);
		blobmsg_add_string(&b, "status", response);
		ubus_send_reply(ctx, req, b.head);
		blob_buf_free(&b);
		free(response);
		return UBUS_STATUS_OK;
	}
	blob_buf_free(&b);
	free(response);

	return UBUS_STATUS_UNKNOWN_ERROR;
}



enum {
	ESP_OFF_PIN,
	__ESP_OFF_MAX
};

static const struct blobmsg_policy esp_policy_off[__ESP_OFF_MAX] = {
	[ESP_OFF_PIN] = { .name = "pin", .type = BLOBMSG_TYPE_INT32 },
};

static int esp_off(struct ubus_context *ctx, 
					struct ubus_object *obj,
					struct ubus_request_data *req, const char *method,
					struct blob_attr *msg)
{

	struct blob_attr *tb[ESP_OFF_PIN];
	struct blob_buf b = {};
	char *response;

	if (!msg)
		return UBUS_STATUS_INVALID_ARGUMENT;

	blobmsg_parse(esp_policy_off, __ESP_OFF_MAX, tb, blob_data(msg), blob_len(msg));
	if (!tb[ESP_OFF_PIN])
		return UBUS_STATUS_INVALID_ARGUMENT;

	uint32_t pin = blobmsg_get_u32(tb[ESP_OFF_PIN]);

	response = handle_request(pin, ACTION_OFF);
	if(response != NULL){
		blob_buf_init(&b, 0);
		blobmsg_add_string(&b, "status", response);
		ubus_send_reply(ctx, req, b.head);
		blob_buf_free(&b);
		free(response);
		return UBUS_STATUS_OK;
	}
	blob_buf_free(&b);
	free(response);

	return UBUS_STATUS_UNKNOWN_ERROR;
}



static const struct ubus_method esp_methods[] = {
	UBUS_METHOD("on", esp_on, esp_policy_on),
	UBUS_METHOD("off", esp_off, esp_policy_off)
};



static struct ubus_object_type esp_object_type =
	UBUS_OBJECT_TYPE("esp", esp_methods);


static struct ubus_object esp_object = {
	.name = "esp",
	.type = &esp_object_type,
	.methods = esp_methods,
	.n_methods = ARRAY_SIZE(esp_methods),
};



int main(void)
{
	openlog ("esp_controller", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
	struct ubus_context *ctx;
	
	uloop_init();

	ctx = ubus_connect(NULL);
	if (!ctx) {
		syslog(LOG_ERR, "Failed to connect to ubus\n");
		return -1;
	}

	ubus_add_uloop(ctx);
	ubus_add_object(ctx, &esp_object);
	uloop_run();

	closelog();
	ubus_free(ctx);
	uloop_done();

	return 0;

}

char *handle_request(uint32_t pin, char *action){
	char *readBuff = NULL;
	char command[50];
	int fd;

	readBuff = (char *)malloc(sizeof(char) * 100);
	if(readBuff == NULL){
		syslog(LOG_ERR, "Failed to malloc");
		return readBuff;
	}
	memset(readBuff, 0, 100);
	
	int rc = initSerial(&fd, "/dev/ttyUSB0");
	if(rc != 0){
		strcpy(readBuff, "response: 1 | msg : Failed to open serial port communication");
		return readBuff;
	}
	
	sprintf(command, "{\"action\":\"%s\", \"pin\": %u}\n", action, pin); //generate  command
	write(fd, command, strlen(command)); //send command

	readBuff = getSerialBuff(&fd, readBuff); //get response

	close(fd);
	return readBuff;
}


char *getSerialBuff(int *fd, char *buffer)
{
	int total_len = 0;
	char tmp_buff[100];

	while (1) {
		int tmp_len = 0;
		sleep(1);
		tmp_len = read(*fd, tmp_buff, sizeof(tmp_buff)-1);
		// tmp_buff[tmp_len] = '\0';
		if (tmp_len == 0 || tmp_len == 1) {
			continue;
		} else if (tmp_len < 0) {
			strcpy(buffer, "response: 1 | msg : Failed to read the response");
			return buffer;
		}
		total_len += tmp_len;
		strncat(buffer, tmp_buff, tmp_len);

		if (strchr(tmp_buff, '\n') == NULL || strchr(tmp_buff, '\r') == NULL) {
			syslog(LOG_INFO, "Reallocing memeory for response\n");
			buffer = (char *)realloc(buffer, sizeof(char *) * total_len);
			memset(tmp_buff, 0, 100);
		} else {
			break;
		}
	}

	return buffer;

}

int initSerial(int *fd, char *devpath)
{
	struct termios options;
	*fd = open(devpath, O_RDWR);
	if (*fd < 0) {
		syslog(LOG_ERR,"Unable to open serial port\n");
		return 1;
	}

	options.c_cflag = B9600 | CS8 | CLOCAL | CREAD;
	options.c_iflag = IGNPAR;
	options.c_oflag = 0;
	options.c_lflag = 0;

	tcflush(*fd, TCIFLUSH);
	tcsetattr(*fd, TCSANOW, &options);
	return 0;
}
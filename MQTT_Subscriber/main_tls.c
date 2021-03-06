#include "wifi.h"
#include <tinyara/gpio.h>
#include <apps/netutils/mqtt_api.h>
#include <apps/netutils/dhcpc.h>

#include <tinyara/analog/adc.h>
#include <tinyara/analog/ioctl.h>

#include <apps/shell/tash.h>

#define DEFAULT_CLIENT_ID "123456789"
#define SERVER_ADDR "api.artik.cloud"
#define SERVER_PORT 8883
//#define SERVER_PORT 1883 // non-secure mode
#define RED "RED"
#define BLUE "BLUE"
#define GREEN "GREEN"
#define RED_LED 45
#define GREEN_LED 60
#define BLUE_LED 49
#define RED_ON_BOARD_LED 45
#define NET_DEVNAME "wl1"

#define DEVICE_ID 		"YOUR Device ID" //brightness - light
#define DEVICE_TOKEN 	"YOUR Device Token"

char device_id[] = "YOUR Device ID";
char device_token[] = "YOUR Device Token";

static const char mqtt_ca_cert_str[] = \
		"-----BEGIN CERTIFICATE-----\r\n"
			"MIIE0zCCA7ugAwIBAgIQGNrRniZ96LtKIVjNzGs7SjANBgkqhkiG9w0BAQUFADCB\r\n"
			"yjELMAkGA1UEBhMCVVMxFzAVBgNVBAoTDlZlcmlTaWduLCBJbmMuMR8wHQYDVQQL\r\n"
			"ExZWZXJpU2lnbiBUcnVzdCBOZXR3b3JrMTowOAYDVQQLEzEoYykgMjAwNiBWZXJp\r\n"
			"U2lnbiwgSW5jLiAtIEZvciBhdXRob3JpemVkIHVzZSBvbmx5MUUwQwYDVQQDEzxW\r\n"
			"ZXJpU2lnbiBDbGFzcyAzIFB1YmxpYyBQcmltYXJ5IENlcnRpZmljYXRpb24gQXV0\r\n"
			"aG9yaXR5IC0gRzUwHhcNMDYxMTA4MDAwMDAwWhcNMzYwNzE2MjM1OTU5WjCByjEL\r\n"
			"MAkGA1UEBhMCVVMxFzAVBgNVBAoTDlZlcmlTaWduLCBJbmMuMR8wHQYDVQQLExZW\r\n"
			"ZXJpU2lnbiBUcnVzdCBOZXR3b3JrMTowOAYDVQQLEzEoYykgMjAwNiBWZXJpU2ln\r\n"
			"biwgSW5jLiAtIEZvciBhdXRob3JpemVkIHVzZSBvbmx5MUUwQwYDVQQDEzxWZXJp\r\n"
			"U2lnbiBDbGFzcyAzIFB1YmxpYyBQcmltYXJ5IENlcnRpZmljYXRpb24gQXV0aG9y\r\n"
			"aXR5IC0gRzUwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQCvJAgIKXo1\r\n"
			"nmAMqudLO07cfLw8RRy7K+D+KQL5VwijZIUVJ/XxrcgxiV0i6CqqpkKzj/i5Vbex\r\n"
			"t0uz/o9+B1fs70PbZmIVYc9gDaTY3vjgw2IIPVQT60nKWVSFJuUrjxuf6/WhkcIz\r\n"
			"SdhDY2pSS9KP6HBRTdGJaXvHcPaz3BJ023tdS1bTlr8Vd6Gw9KIl8q8ckmcY5fQG\r\n"
			"BO+QueQA5N06tRn/Arr0PO7gi+s3i+z016zy9vA9r911kTMZHRxAy3QkGSGT2RT+\r\n"
			"rCpSx4/VBEnkjWNHiDxpg8v+R70rfk/Fla4OndTRQ8Bnc+MUCH7lP59zuDMKz10/\r\n"
			"NIeWiu5T6CUVAgMBAAGjgbIwga8wDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8E\r\n"
			"BAMCAQYwbQYIKwYBBQUHAQwEYTBfoV2gWzBZMFcwVRYJaW1hZ2UvZ2lmMCEwHzAH\r\n"
			"BgUrDgMCGgQUj+XTGoasjY5rw8+AatRIGCx7GS4wJRYjaHR0cDovL2xvZ28udmVy\r\n"
			"aXNpZ24uY29tL3ZzbG9nby5naWYwHQYDVR0OBBYEFH/TZafC3ey78DAJ80M5+gKv\r\n"
			"MzEzMA0GCSqGSIb3DQEBBQUAA4IBAQCTJEowX2LP2BqYLz3q3JktvXf2pXkiOOzE\r\n"
			"p6B4Eq1iDkVwZMXnl2YtmAl+X6/WzChl8gGqCBpH3vn5fJJaCGkgDdk+bW48DW7Y\r\n"
			"5gaRQBi5+MHt39tBquCWIMnNZBU4gcmU7qKEKQsTb47bDN0lAtukixlE0kF6BWlK\r\n"
			"WE9gyn6CagsCqiUXObXbf+eEZSqVir2G3l6BFoMtEMze/aiCKm0oHw0LxOXnGiYZ\r\n"
			"4fQRbxC1lfznQgUy286dUV4otp6F01vvpX1FQHKOtw5rDgb7MzVIcbidJ4vEZV8N\r\n"
			"hnacRHr2lVz2XTIIM6RUthg/aFzyQkqFOFSDX9HoLPKsEdao7WNq\r\n"
			"-----END CERTIFICATE-----\r\n";

// mqtt client handle
mqtt_client_t* pClientHandle = NULL;

// mqtt client parameters
mqtt_client_config_t clientConfig;

//typedef struct _mqtt_tls_param_t {
//	const unsigned char *ca_cert;	/* CA certificate, common between client and MQTT Broker */
//	const unsigned char *cert;	/* Client certificate */
//	const unsigned char *key;	/* Client private key */
//	int ca_cert_len;			/* the length of CA certificate  */
//	int cert_len;				/* the length of Client certificate */
//	int key_len;				/* the length of key */
//} mqtt_tls_param_t;

mqtt_tls_param_t clientTls;

//int blinkerValue = 0;
int currentLED = 0;

const unsigned char *get_ca_cert(void) {
	return (const unsigned char*)mqtt_ca_cert_str;
}

// mqtt client on connect callback
void onConnect(void* client, int result) {
    printf("mqtt client connected to the server\n");
}

// mqtt client on disconnect callback
void onDisconnect(void* client, int result) {
    printf("mqtt client disconnected from the server\n");
}

// mqtt client on publish callback
void onPublish(void* client, int result) {
   printf("mqtt client Published message\n");
}

// Write the value of given gpio port.
void gpio_write(int port, int value) {
    char str[4];
    static char devpath[16];
    snprintf(devpath, 16, "/dev/gpio%d", port);
    int fd = open(devpath, O_RDWR);

    ioctl(fd, GPIOIOC_SET_DIRECTION, GPIO_DIRECTION_OUT);
    write(fd, str, snprintf(str, 4, "%d", value != 0) + 1);

    close(fd);
}

void ledSignal(void) {
    switch (currentLED) {
    case RED_LED: {
        gpio_write(GREEN_LED,0);
        gpio_write(BLUE_LED, 0);
        gpio_write(RED_LED, 1);
    }
    break;
    case GREEN_LED: {
        gpio_write(BLUE_LED,0);
        gpio_write(RED_LED, 0);
        gpio_write(GREEN_LED, 1);
    }
    break;
    case BLUE_LED: {
        gpio_write(RED_LED,0);
        gpio_write(GREEN_LED, 0);
        gpio_write(BLUE_LED, 1);
    }
    break;
    default: {
        gpio_write(RED_LED, 0);
        gpio_write(BLUE_LED, 0);
        gpio_write(GREEN_LED, 0);
    }
    break;
    }
    return;
}

// mqtt client on message callback
void onMessage(void *client, mqtt_msg_t *msg) {
    printf("Received message\n");
    printf("Topic: %s\n", msg->topic);
    printf("Message: %s\n", msg->payload);

    // Check if the message received is RED
    int result = strcmp(msg->payload, RED);
    if (result == 0) {
        if (currentLED != RED_LED) {
            currentLED = RED_LED;
            ledSignal();
        }
        printf("Red LED\n");
        return;
    }

    // Check if the message received is GREEN
    result = strcmp(msg->payload, GREEN);
    if (result == 0) {
        if (currentLED != GREEN_LED) {
            currentLED = GREEN_LED;
            ledSignal();
        }
        printf("Green LED\n");
        return;
    }

    // Check if the message received is BLUE
    result = strcmp(msg->payload, BLUE);
    if (result == 0) {
        if (currentLED != BLUE_LED) {
            currentLED = BLUE_LED;
            ledSignal();
        }
        printf("Blue LED\n");
        return;
    }

    currentLED = 0;
    ledSignal();    // Turn off all LEDs
}

// Utility function to configure mqtt client
void initializeConfigUtil(void) {
    uint8_t macId[IFHWADDRLEN];
    int result = netlib_getmacaddr("wl1", macId);
    if (result < 0) {
        printf("Get MAC Address failed. Assigning \
                Client ID as 123456789");
        clientConfig.client_id =
                DEFAULT_CLIENT_ID; // MAC id Artik 053
    } else {
    printf("MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
            ((uint8_t *) macId)[0],
            ((uint8_t *) macId)[1], ((uint8_t *) macId)[2],
            ((uint8_t *) macId)[3], ((uint8_t *) macId)[4],
            ((uint8_t *) macId)[5]);
    char buf[12];
    sprintf(buf, "%02x%02x%02x%02x%02x%02x",
            ((uint8_t *) macId)[0],
            ((uint8_t *) macId)[1], ((uint8_t *) macId)[2],
            ((uint8_t *) macId)[3], ((uint8_t *) macId)[4],
            ((uint8_t *) macId)[5]);
    clientConfig.client_id = buf; // MAC id Artik 053
    printf("Registering mqtt client with id = %s\n", buf);
    }

    clientConfig.user_name = device_id;
    clientConfig.password = device_token;
    clientConfig.debug = true;
    clientConfig.on_connect = (void*) onConnect;
    clientConfig.on_disconnect = (void*) onDisconnect;
    clientConfig.on_message = (void*) onMessage;
    clientConfig.on_publish = (void*) onPublish;

    clientConfig.protocol_version = MQTT_PROTOCOL_VERSION_311;
    clientConfig.clean_session = true;

    clientTls.ca_cert = get_ca_cert();
    clientTls.ca_cert_len = sizeof(mqtt_ca_cert_str);
    clientTls.cert = NULL;
    clientTls.cert_len = 0;
    clientTls.key = NULL;
    clientTls.key_len = 0;

    clientConfig.tls = &clientTls;
}



//int main(int argc, char *argv[])
#ifdef CONFIG_BUILD_KERNEL
int main(int argc, FAR char *argv[])
#else
int mqtt_client_main(int argc, FAR char *argv[])
#endif
{
    bool wifiConnected = false;
    gpio_write(RED_ON_BOARD_LED, 1); // Turn on on board Red LED to indicate no WiFi connection is established

    char *strTopicMsg = (char*)malloc(sizeof(char)*256);
    char *strTopicAct = (char*)malloc(sizeof(char)*256);
    sprintf(strTopicMsg, "/v1.1/messages/%s", DEVICE_ID);
    sprintf(strTopicAct, "/v1.1/actions/%s", DEVICE_ID);

    memset(&clientConfig, 0, sizeof(clientConfig));
    memset(&clientTls, 0, sizeof(clientTls));

#ifdef CONFIG_CTRL_IFACE_FIFO
    int ret;

    while(!wifiConnected) {
        ret = mkfifo(CONFIG_WPA_CTRL_FIFO_DEV_REQ, CONFIG_WPA_CTRL_FIFO_MK_MODE);
        if (ret != 0 && ret != -EEXIST) {
            printf("mkfifo error for %s: %s", CONFIG_WPA_CTRL_FIFO_DEV_REQ, strerror(errno));
        }
        ret = mkfifo(CONFIG_WPA_CTRL_FIFO_DEV_CFM, CONFIG_WPA_CTRL_FIFO_MK_MODE);
        if (ret != 0 && ret != -EEXIST) {
            printf("mkfifo error for %s: %s", CONFIG_WPA_CTRL_FIFO_DEV_CFM, strerror(errno));
        }

        ret = mkfifo(CONFIG_WPA_MONITOR_FIFO_DEV, CONFIG_WPA_CTRL_FIFO_MK_MODE);
        if (ret != 0 && ret != -EEXIST) {
            printf("mkfifo error for %s: %s", CONFIG_WPA_MONITOR_FIFO_DEV, strerror(errno));
        }
    #endif

        if (start_wifi_interface() == SLSI_STATUS_ERROR) {
            printf("Connect Wi-Fi failed. Try Again.\n");
        }
        else {
            wifiConnected = true;
            gpio_write(RED_ON_BOARD_LED, 0); // Turn off Red LED to indicate WiFi connection is established
        }
    }

    printf("Connect to Wi-Fi success\n");

    bool mqttConnected = false;
    bool ipObtained = false;
    printf("Get IP address\n");

    struct dhcpc_state state;
    void *dhcp_handle;

    while(!ipObtained) {
        dhcp_handle = dhcpc_open(NET_DEVNAME);
        ret = dhcpc_request(dhcp_handle, &state);
        dhcpc_close(dhcp_handle);

        if (ret != OK) {
            printf("Failed to get IP address\n");
            printf("Try again\n");
            sleep(1);
        }
        else {
            ipObtained = true;
        }
    }
    netlib_set_ipv4addr(NET_DEVNAME, &state.ipaddr);
    netlib_set_ipv4netmask(NET_DEVNAME, &state.netmask);
    netlib_set_dripv4addr(NET_DEVNAME, &state.default_router);

    printf("IP address  %s\n", inet_ntoa(state.ipaddr));

    sleep(2);

    // Connect to the WiFi network for Internet connectivity
    printf("mqtt client tutorial\n");

    // Initialize mqtt client
    initializeConfigUtil();

    pClientHandle = mqtt_init_client(&clientConfig);
    if (pClientHandle == NULL) {
        printf("mqtt client handle initialization fail\n");
        return 0;
    }

    while (mqttConnected == false ) {
        sleep(2);
        // Connect mqtt client to server
        int result = mqtt_connect(pClientHandle, SERVER_ADDR, SERVER_PORT, 60);
        if (result < 0) {
            printf("mqtt client connect to server fail\n");
            continue;
        } else if (result == 0) {
        	mqttConnected = true;
        	printf("mqtt client connected to server\n");
        	break;
        }
    }

    bool mqttSubscribe = false;

    // Subscribe to topic of interest
    while (mqttSubscribe == false ) {
        sleep(2);
        int result = mqtt_subscribe(pClientHandle, strTopicAct, 0); //topic - color, QOS - 0
        if (result < 0) {
            printf("mqtt client subscribe to topic failed\n");
            continue;
        }
        mqttSubscribe = true;
        printf("mqtt client Subscribed to the topic successfully\n");
    }

    // Read analog signal using ADC
    int fd;
    struct adc_msg_s sample;
    size_t readsize;
    ssize_t nbytes;
    int publishedVal = -1;

    fd = open("/dev/adc0", O_RDONLY);
    if (fd < 0) {
        printf("%s: open failed: %d\n", __func__, errno);
        return 0;
    }

    while(1) {
    	usleep(10000);
    	ret = ioctl(fd, ANIOC_TRIGGER, 0);
    	if (ret < 0) {
    	    printf("%s: ioctl failed: %d\n", __func__, errno);
    	    close(fd);
    	    return 0;
    	}

    	readsize = sizeof(struct adc_msg_s);

    	// adc_msg_s structure returns the data structure containing analog readings from ADC pins
    	nbytes = read(fd, &sample, readsize);
    	if (nbytes < 0) {
    	    if (errno != EINTR) {
    	        printf("%s: read failed: %d\n", __func__, errno);
    	        close(fd);
    	        return 0;
    	    }
    	} else if (nbytes == 0) {
    	    printf("%s: No data read, Ignoring\n");
    	} else {
    	    int nsamples = nbytes / sizeof(struct adc_msg_s);
    	    if (nsamples * sizeof(struct adc_msg_s) != nbytes) {
    	        printf("%s: read size=%ld is not a multiple of sample size=%d, Ignoring\n", __func__, (long)nbytes, sizeof(struct adc_msg_s));
    	    } else {
    	        // Filter the data obtained from only ADC pin connected to the sensor A0 - 0, A1 - 1, A2 - 2, A3 - 3
    	        if ((sample.am_channel == 0) && ((sample.am_data > (publishedVal + 100)) || (sample.am_data < (publishedVal - 100)))) // A0 ADC
    	        {
    	            printf("Publish message for signal A0\n");
    	            char buf[4];
    	            sprintf(buf,"{\"brightness\" : %d}",sample.am_data);
    	            printf("CdS Value: %d\n", sample.am_data);
    	            char* msg = buf;
    	            // construct the mqtt message to be published
    	            mqtt_msg_t message;
    	            message.payload = msg;
    	            message.payload_len = strlen(buf);
    	            message.topic = strTopicMsg;
    	            message.qos = 0;
    	            message.retain = 0;
    	            ret = mqtt_publish(pClientHandle, message.topic, (char*)message.payload, message.payload_len, message.qos, message.retain);
    	            if (ret < 0) {
    	                printf("Error publishing \n");
    	            }
    	            else {
    	                publishedVal = sample.am_data;
    	            }
    	        }
    	    }
    	}
    }

    close(fd);
}

int main(int argc, FAR char *argv[]){
	tash_cmd_install("mqtt_client", mqtt_client_main, 1);
	return 0;
}

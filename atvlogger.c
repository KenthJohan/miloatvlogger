#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <inttypes.h>
#include <time.h>
#include <mosquitto.h>

#include "csc/csc_crossos.h"
#include "csc/csc_argv.h"

#define ARG_HELP             UINT32_C(0x00000001)
#define ARG_VERBOSE          UINT32_C(0x00000002)
#define ARG_STDIN            UINT32_C(0x00000010)


#define GHOSTIME_START 100
#define GHOSTIME_STOP  700


static int run = 1;

void handle_signal (int s)
{
	run = 0;
}


static void on_connect (struct mosquitto *mosq, void *obj, int reason_code)
{
	UNUSED (obj);
	printf("on_connect: %s\n", mosquitto_connack_string (reason_code));
	if (reason_code != 0)
	{
		mosquitto_disconnect (mosq);
	}
}

static void message_callback (struct mosquitto *mosq, void *obj, const struct mosquitto_message *message)
{
	int match = 0;
	printf ("got message '%.*s' for topic '%s'\n", message->payloadlen, (char*) message->payload, message->topic);
	mosquitto_topic_matches_sub ("/devices/wb-adc/controls/+", message->topic, &match);
	if (match)
	{
		printf("got message for ADC topic\n");
	}
}


static int mqtt_send(struct mosquitto *mosq, char * topic, char *msg)
{
  return mosquitto_publish(mosq, NULL, topic, strlen(msg), msg, 0, 0);
}

static int mqtt_send_i32(struct mosquitto *mosq, char * topic, int32_t val)
{
	char buf[33];
	itoa(val, buf, 10);
	return mosquitto_publish(mosq, NULL, topic, strlen(buf), buf, 0, 0);
}







int main (int argc, char const * argv[])
{
	UNUSED (argc);
	csc_crossos_enable_ansi_color();
	int rc = 0;
	time_t rawtime1;
	time_t rawtime2;
	signal (SIGINT, handle_signal);
	signal (SIGTERM, handle_signal);

	{
		int r = mosquitto_lib_init();
		ASSERT (r == MOSQ_ERR_SUCCESS);
	}

	uint32_t arg_flags = 0;
	char const * arg_filename = NULL;
	char const * arg_mqtt_address = "192.168.1.195";
	uint32_t arg_mqtt_port = 1883;
	uint32_t arg_mqtt_keepalive = 60;
	uint32_t arg_mqtt_qos = 2;
	uint32_t arg_udelay = 0;

	struct csc_argv_option option[] =
	{
	{CSC_ARGV_DEFINE_GROUP("General:")},
	{'h', "help",       CSC_TYPE_U32,    &arg_flags,          ARG_HELP,    "Show help"},
	{'v', "verbose",    CSC_TYPE_U32,    &arg_flags,          ARG_VERBOSE, "Show verbose"},
	{'i', "stdin",      CSC_TYPE_U32,    &arg_flags,          ARG_STDIN,   "Read pointcloud from stdin"},
	{'f', "filename",   CSC_TYPE_STRING, &arg_filename,       0,           "Read pointcloud from a file"},
	{'I', "interval",   CSC_TYPE_U32,    &arg_udelay,         0,           "Interval in microseconds"},
	{CSC_ARGV_DEFINE_GROUP("MQTT:")},
	{'a', "address",    CSC_TYPE_STRING, &arg_mqtt_address,   0,           "MQTT address"},
	{'p', "port",       CSC_TYPE_U32,    &arg_mqtt_port,      0,           "MQTT port"},
	{'k', "keepalive",  CSC_TYPE_U32,    &arg_mqtt_keepalive, 0,           "MQTT keepalive"},
	{'q', "qos",        CSC_TYPE_U32,    &arg_mqtt_qos,       0,           "MQTT integer value 0, 1 or 2 indicating the Quality of Service to be used for the message."},
	{CSC_ARGV_END}};
	csc_argv_parseall (argv+1, option);

	if (arg_flags & ARG_HELP)
	{
		csc_argv_description0 (option, stdout);
		csc_argv_description1 (option, stdout);
		return 0;
	}

	char clientid[24];
	memset(clientid, 0, 24);
	snprintf(clientid, 23, "mysql_log_%d", getpid());
	struct mosquitto * mosq = mosquitto_new (clientid, 1, NULL);
	if (mosq == NULL)
	{
		fprintf (stderr, "Error: Out of memory.\n");
		rc = 1;
		goto cleanexit;
	}


	mosquitto_connect_callback_set (mosq, on_connect);
	mosquitto_message_callback_set (mosq, message_callback);
	mosquitto_subscribe (mosq, NULL, "/banana", 0);



	{
		rc = mosquitto_connect (mosq, arg_mqtt_address, arg_mqtt_port, arg_mqtt_keepalive);
		if (rc != MOSQ_ERR_SUCCESS)
		{
			mosquitto_destroy (mosq);
			fprintf (stderr, "Error: %s\n", mosquitto_strerror(rc));
			goto cleanexit;
		}
	}

	/*
	{
		int rc = mosquitto_loop_start (mosq);
		if (rc != MOSQ_ERR_SUCCESS)
		{
			mosquitto_destroy (mosq);
			fprintf (stderr, "Error: %s\n", mosquitto_strerror (rc));
			return 1;
		}
	}
	*/

	{
		struct tm * timeinfo;
		time (&rawtime1);
		timeinfo = localtime (&rawtime1);
		printf ( "Current local time and date: %s", asctime (timeinfo));

		timeinfo->tm_sec =

		timeinfo->tm_hour ++;
		rawtime2 = mktime (timeinfo);
		timeinfo = localtime (&rawtime2);
		printf ( "Ne local time and date: %s", asctime (timeinfo) );

	}

	return 0;



	while (run)
	{

		int rc = mosquitto_loop (mosq, -1, 1);

		time_t rawtime;


		//rc = mqtt_send (mosq, "/hello", asctime (timeinfo));
		rc = mqtt_send_i32 (mosq, "/hello", timeinfo->tm_hour);
		printf ("%i\n", rc);
		if (run && rc)
		{
			printf ("connection error!\n");
			sleep(10);
			mosquitto_reconnect (mosq);
		}
	}


cleanexit:
	mosquitto_lib_cleanup();

	return rc;
}

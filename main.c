/*
 * main.c
 * 
 * Copyright The GRS IQ Receiver Contributors.
 * 
 * This file is part of GRS IQ Receiver.
 * 
 * GRS IQ Receiver is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * GRS IQ Receiver is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with GRS IQ Receiver. If not, see <http://www.gnu.org/licenses/>.
 * 
 */

/**
 * \brief GRS IQ samples receiver.
 * 
 * \author Gabriel Mariano Marcelino <gabriel.mm8@gmail.com>
 * 
 * \version 0.0.0
 * 
 * \date 2025/06/17
 * 
 * \defgroup grs-iq-rx GRS IQ Receiver
 * \{
 */

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <string.h>

#include <rtl-sdr.h>
#include <zmq.h>

#include "version.h"

/* Default values */
#define GRS_IQ_RX_DEFAULT_SAMPLE_RATE       500000
#define GRS_IQ_RX_DEFAULT_DEV_IDX           0
#define GRS_IQ_RX_DEFAULT_BUFFER_SIZE       (1024*2)
#define GRS_IQ_RX_DEFAULT_FREQ              100000000UL
#define GRS_IQ_RX_DEFAULT_GAIN              0
#define GRS_IQ_RX_DEFAULT_BUF_LEN           (16*16384)

bool do_exit = false;
static rtlsdr_dev_t *dev = NULL;
void *zmq_context;
void *zmq_publisher;

/**
 * \brief .
 *
 * \param[in] signum .
 *
 * \return None.
 */
void sigint_handler(int signum);

/**
 * \brief Print usage instructions.
 * 
 * \param[in] argv .
 *
 * \return None.
 */
void print_usage(char *argv);

void cleanup(void);

static void rtlsdr_callback(unsigned char *buf, uint32_t len, void *ctx)
{
    if (do_exit) {
        return;
    }

    /* Create ZMQ message */
    zmq_msg_t message;
    zmq_msg_init_size(&message, len);
    memcpy(zmq_msg_data(&message), buf, len);

    /* Send via pub socket */
    if (zmq_msg_send(&message, zmq_publisher, 0) == -1) {
        fprintf(stderr, "Error sending ZMQ message: %s\n", zmq_strerror(errno));
    }

    /* Release message */
    zmq_msg_close(&message);
}

int main(int argc, char *argv[])
{
    bool help = false;

    /* RTL-SDR parameters */
    uint32_t dev_idx        = GRS_IQ_RX_DEFAULT_DEV_IDX;
    uint32_t sample_rate    = GRS_IQ_RX_DEFAULT_SAMPLE_RATE;
    uint32_t freq           = GRS_IQ_RX_DEFAULT_FREQ;
    int gain                = GRS_IQ_RX_DEFAULT_GAIN;

    /* Register signal handler for clean exit */
    signal(SIGINT, sigint_handler);
    signal(SIGTERM, sigint_handler);
    signal(SIGQUIT, sigint_handler);

	unsigned int i;

	/* Handles arguments */
	for(i = 1; i < argc; i++)
    {
		if ((strcmp(argv[i], "-h") == 0) || (strcmp(argv[i], "--help") == 0))
        {
			help = true;
		}
        else if ((strcmp(argv[i], "-s") == 0) || (strcmp(argv[i], "--sample-rate") == 0))
        {
            sample_rate = atoi(argv[i + 1]);
        }
        else if ((strcmp(argv[i], "-f") == 0) || (strcmp(argv[i], "--freq") == 0))
        {
            freq = atoi(argv[i + 1]);
        }
        else if ((strcmp(argv[i], "-g") == 0) || (strcmp(argv[i], "--gain") == 0))
        {
            gain = atoi(argv[i + 1]);
        }
        else if ((strcmp(argv[i], "-d") == 0) || (strcmp(argv[i], "--dev") == 0))
        {
            dev_idx = atoi(argv[i + 1]);
        }
	}

	/* Handles help argument */
	if (help)
    {
        print_usage(*argv);

        exit(EXIT_SUCCESS);
	}

    /* Initialize ZMQ */
    zmq_context = zmq_ctx_new();
    if (!zmq_context)
    {
        fprintf(stderr, "Failed to create ZMQ context\n");

        exit(EXIT_FAILURE);
    }

    zmq_publisher = zmq_socket(zmq_context, ZMQ_PUB);
    if (!zmq_publisher)
    {
        fprintf(stderr, "Failed to create ZMQ publisher socket: %s\n", zmq_strerror(errno));
        cleanup();

        exit(EXIT_FAILURE);
    }

    /* Bind to port 5556 (subscribers will connect to this) */
    if (zmq_bind(zmq_publisher, "tcp://*:5556") != 0) {
        fprintf(stderr, "Failed to bind ZMQ publisher: %s\n", zmq_strerror(errno));
        cleanup();

        exit(EXIT_FAILURE);
    }

    if (rtlsdr_get_device_count() - 1 < dev_idx)
    {
        fprintf(stderr, "Device %d not found!\n\t", dev_idx);

        print_usage(*argv);

        exit(EXIT_FAILURE);
    }

    /* Open the device */
    if (rtlsdr_open(&dev, dev_idx) != 0)
    {
        fprintf(stderr, "Error opening device %d: %s!\n\r", dev_idx, strerror(errno));

        exit(EXIT_FAILURE);
    }

    /* Set sample rate */
    rtlsdr_set_sample_rate(dev, sample_rate);

    /* Set center frequency */
    rtlsdr_set_center_freq(dev, freq);

    /* Set gain mode */
    if (gain == 0)
    {
        rtlsdr_set_tuner_gain_mode(dev, 0); /* Automatic gain */
    }
    else
    {
        rtlsdr_set_tuner_gain_mode(dev, 1); /* Manual gain */
        rtlsdr_set_tuner_gain(dev, gain);
    }

    /* Automatic gain control */
    rtlsdr_set_agc_mode(dev, 0);

    /* Flush the buffer */
    rtlsdr_reset_buffer(dev);

    /* Start async reading */
    int r = rtlsdr_read_async(dev, rtlsdr_callback, NULL, 0, GRS_IQ_RX_DEFAULT_BUF_LEN);

    if (r < 0)
    {
        fprintf(stderr, "Async read failed with error %d\n", r);
    }

    /* Cleanup */
    rtlsdr_close(dev);

    exit(EXIT_SUCCESS);
}

void sigint_handler(int signum)
{
    do_exit = true;
    fprintf(stderr, "\nSignal caught, exiting!\n");
}

void print_usage(char *sw_name)
{
    printf("Usage:\n");
    printf("\t%s\t\t\tExecutes with default parameters\n",          sw_name);
    printf("\t%s -h, --help\t\tShows this text\n",                  sw_name);
    printf("\t%s -s, --sample-rate\tRate of incoming samples\n",    sw_name);
    printf("\t%s -f, --freq\t\tRF center frequency in Hz\n",        sw_name);
    printf("\t%s -g, --gain\t\tGain for the RF chain\n",            sw_name);
    printf("\t%s -d, --dev\t\tDevice index\n",                      sw_name);
}

void cleanup(void)
{
    if (dev)
    {
        rtlsdr_close(dev);
        dev = NULL;
    }

    if (zmq_publisher)
    {
        zmq_close(zmq_publisher);
    }

    if (zmq_context)
    {
        zmq_ctx_destroy(zmq_context);
    }
}

/** \} End of grs-iq-rx group */

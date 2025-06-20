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
 * \version 0.0.1
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
#define GRS_IQ_RX_DEFAULT_SAMPLE_RATE       2048000
#define GRS_IQ_RX_DEFAULT_BANDWIDTH         0               /* Automatic bandwidth */
#define GRS_IQ_RX_DEFAULT_BUF_LENGTH        (16 * 16384)
#define GRS_IQ_RX_DEFAULT_DEV_IDX           0
#define GRS_IQ_RX_DEFAULT_FREQ              100000000UL
#define GRS_IQ_RX_DEFAULT_GAIN              0

#define GRS_IQ_RX_MIN_BUF_LENGTH            512
#define GRS_IQ_RX_MAX_BUF_LENGTH            (256 * 16384)

bool do_exit = false;
bool verbose = false;
static uint32_t iq_frames_to_read = 0U;
static rtlsdr_dev_t *dev = NULL;

void *zmq_context;
void *zmq_publisher;

/**
 * \brief IQ sample type.
 */
typedef struct
{
    float i;    /**< In-phase sample. */
    float q;    /**< Quadrature sample. */
} iq_t;

/**
 * \brief .
 *
 * \param[in] signum .
 *
 * \return None.
 */
void sig_handler(int signum);

/**
 * \brief Print usage instructions.
 * 
 * \param[in] argv .
 *
 * \return None.
 */
void print_usage(char *argv);

/**
 * \brief .
 *
 * \return None.
 */
void cleanup(void);

static void rtlsdr_callback(unsigned char *buf, uint32_t len, void *ctx)
{
//    if (ctx)
//    {
        if (do_exit)
        {
            exit(EXIT_SUCCESS);
        }

        if ((iq_frames_to_read) && (iq_frames_to_read < len/2))
        {
            len = 2U * iq_frames_to_read;
            iq_frames_to_read = 0;
            do_exit = true;
            rtlsdr_cancel_async(dev);
        }

        uint32_t j = 0;
        for(j = 0; j < len; j += 2)
        {
            iq_t iq = {0};

            iq.i = (buf[j] - 127.5f) / 127.5f;
            iq.q = (buf[j + 1] - 127.5f) / 127.5f;

            /* Send via pub socket */
            if (zmq_send(zmq_publisher, &iq, sizeof(iq_t), 0) != sizeof(iq_t))
            {
                if (verbose)
                {
                    fprintf(stderr, "Error sending ZMQ message: %s\n\r", zmq_strerror(errno));
                }
            }
        }

        if (iq_frames_to_read)
        {
            if (iq_frames_to_read > len/2)
            {
                iq_frames_to_read -= len/2;
            }
            else
            {
                do_exit = true;
                rtlsdr_cancel_async(dev);
			}
		}
//    }
}

int main(int argc, char *argv[])
{
    struct sigaction sigact;

    bool help = false;

    int n_read;
    int gain                = GRS_IQ_RX_DEFAULT_GAIN;
    int ppm_error           = 0;
    bool sync_mode          = false;
    int dithering           = 1;
    uint8_t *buffer;
    int dev_index           = GRS_IQ_RX_DEFAULT_DEV_IDX;
    int dev_given           = 0;
    uint64_t frequency      = GRS_IQ_RX_DEFAULT_FREQ;
    uint32_t bandwidth      = GRS_IQ_RX_DEFAULT_BANDWIDTH;
    uint32_t samp_rate      = GRS_IQ_RX_DEFAULT_SAMPLE_RATE;
    uint32_t out_block_size = GRS_IQ_RX_DEFAULT_BUF_LENGTH;

	unsigned int i;

	/* Handles arguments */
	for(i = 1; i < argc; i++)
    {
		if ((strcmp(argv[i], "-h") == 0) || (strcmp(argv[i], "--help") == 0))
        {
			help = true;
		}
        else if ((strcmp(argv[i], "-d") == 0) || (strcmp(argv[i], "--device") == 0))
        {
            dev_index = atoi(argv[i + 1]);
        }
        else if ((strcmp(argv[i], "-f") == 0) || (strcmp(argv[i], "--frequency") == 0))
        {
            frequency = (uint64_t)atof(argv[i + 1]);
        }
        else if ((strcmp(argv[i], "-g") == 0) || (strcmp(argv[i], "--gain") == 0))
        {
            gain = atoi(argv[i + 1]);
        }
        else if ((strcmp(argv[i], "-s") == 0) || (strcmp(argv[i], "--sample-rate") == 0))
        {
            samp_rate = (uint32_t)atof(argv[i + 1]);
        }
        else if ((strcmp(argv[i], "-w") == 0) || (strcmp(argv[i], "--bandwidth") == 0))
        {
            bandwidth = (uint32_t)atof(argv[i + 1]);
        }
        else if ((strcmp(argv[i], "-p") == 0) || (strcmp(argv[i], "--ppm-error") == 0))
        {
            ppm_error = atoi(argv[i + 1]);
        }
        else if ((strcmp(argv[i], "-b") == 0) || (strcmp(argv[i], "--block-size") == 0))
        {
            out_block_size = (uint32_t)atof(argv[i + 1]);
        }
        else if ((strcmp(argv[i], "-n") == 0) || (strcmp(argv[i], "--frames") == 0))
        {
            iq_frames_to_read = (uint32_t)atof(argv[i + 1]);
        }
        else if ((strcmp(argv[i], "-S") == 0) || (strcmp(argv[i], "--sync-mode") == 0))
        {
            sync_mode = true;
        }
        else if ((strcmp(argv[i], "-N") == 0) || (strcmp(argv[i], "--dithering") == 0))
        {
            dithering = 0;
        }
        else if ((strcmp(argv[i], "-v") == 0) || (strcmp(argv[i], "--verbose") == 0))
        {
            verbose = true;
        }
	}

	/* Handles help argument */
	if (help)
    {
        print_usage(*argv);

        exit(EXIT_SUCCESS);
	}

    /* Check block size */
    if ((out_block_size < GRS_IQ_RX_MIN_BUF_LENGTH) || (out_block_size > GRS_IQ_RX_MAX_BUF_LENGTH))
    {
		out_block_size = GRS_IQ_RX_DEFAULT_BUF_LENGTH;

        if (verbose)
        {
            fprintf(stderr, "Output block size wrong value, falling back to default!\n\r");
            fprintf(stderr, "Minimal length: %u\n\r", GRS_IQ_RX_MIN_BUF_LENGTH);
            fprintf(stderr, "Maximal length: %u\n\r", GRS_IQ_RX_MAX_BUF_LENGTH);
        }
    }

    buffer = malloc(out_block_size * sizeof(uint8_t));

    if (dev_index < 0)
    {
        exit(EXIT_FAILURE);
    }

    /* Open the device */
    int r = rtlsdr_open(&dev, (uint32_t)dev_index);
    if (r < 0)
    {
        if (verbose)
        {
            fprintf(stderr, "Error to open RTL-SDR device #%d: %s!\n\r", dev_index, strerror(errno));
        }

        exit(EXIT_FAILURE);
    }

    /* Register signal handler for clean exit */
    sigact.sa_handler = sig_handler;
	sigemptyset(&sigact.sa_mask);
	sigact.sa_flags = 0;
	sigaction(SIGINT, &sigact, NULL);
	sigaction(SIGTERM, &sigact, NULL);
	sigaction(SIGQUIT, &sigact, NULL);
	sigaction(SIGPIPE, &sigact, NULL);

//    if (!dithering)
//    {
//        if (verbose)
//        {
//            fprintf(stderr, "Disabling dithering...");
//        }
//
//		r = rtlsdr_set_dithering(dev, dithering);
//
//        if (verbose)
//        {
//		    if (r)
//            {
//		    	fprintf(stderr, "FAIL!\n\r");
//		    }
//            else
//            {
//		    	fprintf(stderr, "SUCCESS!\n\r");
//		    }
//        }
//    }

    /* Set sample rate */
    rtlsdr_set_sample_rate(dev, samp_rate);

    /* Set the tuner bandwidth */
    rtlsdr_set_tuner_bandwidth(dev, bandwidth);

    /* Set the frequency */
    rtlsdr_set_center_freq(dev, frequency);

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

    /* Set frequency correction */
    rtlsdr_set_freq_correction(dev, ppm_error);

    if (rtlsdr_get_device_count() - 1 < dev_index)
    {
        fprintf(stderr, "Device %d not found!\n\t", dev_index);

        print_usage(*argv);

        exit(EXIT_FAILURE);
    }

    /* Initialize ZMQ */
    zmq_context = zmq_ctx_new();
    if (!zmq_context)
    {
        if (verbose)
        {
            fprintf(stderr, "Failed to create ZMQ context\n");
        }

        exit(EXIT_FAILURE);
    }

    zmq_publisher = zmq_socket(zmq_context, ZMQ_PUB);
    if (!zmq_publisher)
    {
        if (verbose)
        {
            fprintf(stderr, "Failed to create ZMQ publisher socket: %s\n", zmq_strerror(errno));
        }

        cleanup();

        exit(EXIT_FAILURE);
    }

    /* Bind to port 5556 (subscribers will connect to this) */
    if (zmq_bind(zmq_publisher, "tcp://*:5556") != 0)
    {
        if (verbose)
        {
            fprintf(stderr, "Failed to bind ZMQ publisher: %s\n", zmq_strerror(errno));
        }

        cleanup();

        exit(EXIT_FAILURE);
    }

    /* Flush the buffer */
    rtlsdr_reset_buffer(dev);

    if (sync_mode)
    {
        if (verbose)
        {
            fprintf(stderr, "Reading samples in sync mode...\n\r");
        }

		while(!do_exit)
        {
			r = rtlsdr_read_sync(dev, buffer, out_block_size, &n_read);
			if (r < 0)
            {
                if (verbose)
                {
				    fprintf(stderr, "WARNING: sync read failed!\n\r");
                }

				break;
			}

			if ((iq_frames_to_read) && (iq_frames_to_read < ((uint32_t)n_read /2)))
            {
				n_read = 2U * iq_frames_to_read;
				do_exit = true;
			}

			if ((uint32_t)n_read < out_block_size)
            {
                if (verbose)
                {
				    fprintf(stderr, "Short read, samples lost, exiting!\n\r");
                }

				break;
			}

			if (iq_frames_to_read)
            {
				if (iq_frames_to_read > ((uint32_t)n_read /2))
                {
					iq_frames_to_read -= n_read / 2;
                }
				else
                {
					do_exit = true;
                }
			}
		}
    }
    else
    {
        if (verbose)
        {
            fprintf(stderr, "Reading samples in async mode...\n\r");
        }

        /* Start async reading */
        r = rtlsdr_read_async(dev, rtlsdr_callback, NULL, 0, out_block_size);

        if (r < 0)
        {
            if (verbose)
            {
                fprintf(stderr, "Async read failed with error %d!\n\r", r);
            }
        }
    }

    if (verbose)
    {
        if (do_exit)
        {
            fprintf(stderr, "\n\rUser cancel, exiting...\n\r");
        }
        else
        {
            fprintf(stderr, "\n\rLibrary error %d, exiting...\n\r", r);
        }
    }

    /* Cleanup */
    cleanup();

    return r >= 0 ? r : -r;
}

void sig_handler(int signum)
{
    if (verbose)
    {
        fprintf(stderr, "Signal caught, exiting!\n\r");
    }

    do_exit = true;
    rtlsdr_cancel_async(dev);
}

void print_usage(char *sw_name)
{
    printf("GRS IQ Receiver v%s\n\n\r", GRS_IQ_RX_VERSION);
    printf("Usage:\n\r");
    printf("\t%s\t\t\t\tExecutes with default parameters\n\r",                          sw_name);
    printf("\t%s -h, --help\t\tShows this text\n\r",                                    sw_name);
    printf("\t%s -d, --device\tDevice index (default: %d)\n\r",                         sw_name, GRS_IQ_RX_DEFAULT_DEV_IDX);
    printf("\t%s -f, --frequency\tRF center frequency in Hz (default: %ld Hz)\n\r",     sw_name, GRS_IQ_RX_DEFAULT_FREQ);
    printf("\t%s -g, --gain\t\tGain for the RF chain (default: 0 for auto)\n\r",        sw_name);
    printf("\t%s -s, --sample-rate\tRate of incoming samples (default: %d S/s)\n\r",    sw_name, GRS_IQ_RX_DEFAULT_SAMPLE_RATE);
    printf("\t%s -w, --bandwidth\tTuner bandwidth (default: auto)\n\r",                 sw_name);
    printf("\t%s -p, --ppm-error\tPPM error (default: 0)\n\r",                          sw_name);
    printf("\t%s -b, --block-size\tBlock size (default: %d)\n\r",                       sw_name, GRS_IQ_RX_DEFAULT_BUF_LENGTH);
    printf("\t%s -n, --frames\tNumber of frames to read (default: 0, infinite)\n\r",    sw_name);
    printf("\t%s -S, --sync-mode\tForce sync output (default: async)\n\r",              sw_name);
    printf("\t%s -N, --dithering\tNo dithering (default: use dithering)\n\r",           sw_name);
    printf("\t%s -v, --verbose\tVerbose (default: disabled)\n\r",                       sw_name);
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

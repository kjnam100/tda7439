/*
 * Copyright (C) 2016 kjnam100@yahoo.co.kr
 *
 * TDA7439 module for Raspberry pi
 * http://blog.naver.com/kjnam100/220722517162
 *
 * compile with gcc -lm -lwiringPi -o tda7439 tda7439.c
 */
#include <wiringPi.h> 
#include <wiringPiI2C.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <math.h>
#include <string.h>
#include <ctype.h>
#include <libgen.h>
#include <sys/stat.h>

#define TDA7439_SAVED_INFO "/var/local/TDA7439_saved_info"

int fd;
int dID = 0x44; // i2c Channel the device is on
char *prog_name;

/* sub-address: Input selector
 *
 * 0 Input selector: IN1 (IN1 ~ IN4)
 * 1 Input gain: 0dB (0dB ~ 30dB)
 * 2 Volume: 0dB (0dB ~ -40dB, Mute)
 * 3 Bass gain: 0dB (-14dB ~ 14dB)
 * 4 Mid-range gain: 0dB (-14dB ~ 14dB)
 * 5 Treble gain: 0dB (-14dB ~ 14dB)
 * 6 Speaker attenuation L: 0dB (0dB ~ 72dB, Mute)
 * 7 Speaker attenuation R: 0dB (0dB ~ 72dB, Mute)
 */
unsigned char tda7439_data[9] = {0x10, 0x03, 0x00, 0x00, 0x07, 0x07, 0x07, 0x00, 0x00};
int write_ok = 0;
int mute = 0;

int t_tbl[15] = {-14, -12, -10, -8, -6, -4, -2, 0, 14, 12, 10, 8, 6, 4, 2};
int r_tbl[15] = {0, 1, 2, 3, 4, 5, 6, 7, 14, 13, 12, 11, 10, 9, 8};

int get_tda7439_stat()
{
	FILE *fd;
	int i;

	if ((fd = fopen(TDA7439_SAVED_INFO, "r")) == NULL)
		return -1;

	if (fscanf(fd, "%d\n", &write_ok) == EOF) return 0;
	if (fscanf(fd, "%d\n", &mute) == EOF) return 0;
	for (i = 0; i < 9; i++)
		if (fscanf(fd, "%d\n", (unsigned int *)&tda7439_data[i]) == EOF)
			break;

	fclose(fd);
	return 0;
}

int save_tda7439_stat()
{
	FILE *fd;
	int i;

	if ((fd = fopen(TDA7439_SAVED_INFO, "w")) == NULL)
        return -1;

	fprintf(fd, "%d\n", write_ok); 
	fprintf(fd, "%d\n", mute); 
	for (i = 0; i < 9; i++) 
		fprintf(fd, "%d\n", tda7439_data[i]); 

	fclose(fd);
	return 0;
}

int chk_write(int fd, unsigned char *data, int len)
{
	if (write(fd, data, len) < 0) write_ok = 0;
	else write_ok = 1;

	return write_ok;
}

void write_all(int fd, int flag)
{
	int i;
	unsigned char data[9];

	for (i = 0; i < 9; i++)
		data[i] = tda7439_data[i];

	if (mute)  
		data[3] = data[7] = data[8] = 0xff;

	if (write(fd, data, 9) < 0) write_ok = 0;
	else write_ok = 1;

	save_tda7439_stat();
	if (flag)
		system("/home/pi/bin/ssd1306_info.py");

	if (write_ok == 0) {
		fprintf(stderr, "TDA7439 write fail\n");
		exit(0);
	}
}

void usage() 
{
	fprintf(stderr, "Usage: %s [command] [value]\n", prog_name);
	fprintf(stderr, "  mute (toggle)\n");
	fprintf(stderr, "  selector <1~4>\n");
	fprintf(stderr, "  volume [up|down] <-40~0> (1dB steps)\n");
	fprintf(stderr, "  gain [up|down] <0~30> (2dB steps)\n");
	fprintf(stderr, "  base [up|down] <-14~14> (2dB steps)\n");
	fprintf(stderr, "  mid-range [up|down] <-14~14> (2dB steps)\n");
	fprintf(stderr, "  treble [up|down] <-14~14> (2dB steps)\n");
	fprintf(stderr, "  speaker-attenuation [up|down] <0~72> (1dB steps)\n");
	fprintf(stderr, "  reset (reset to default)\n");
	fprintf(stderr, "  status (print current status)\n");
	fprintf(stderr, "  help (this message)\n");
	exit(1);
}

//
// =========================================================
//
int main(int argc, char *argv[])
{
	int val;
	size_t len;

	prog_name = basename(argv[0]);

	//open access to the board, send error msg if fails
	if((fd = wiringPiI2CSetup(dID)) < 0) {
		fprintf(stderr, "error opening i2c channel\n");
		exit(1);
	}

	if (argc == 2 && (len = strlen(argv[1])) && !strncmp(argv[1], "reset", len)) {
		write_all(fd, 1);
		exit(0);
	}	

	get_tda7439_stat();

	if (argc < 2) {
		write_all(fd, 0);
		exit(0);
	}

	len = strlen(argv[1]);
	if (!strncmp(argv[1], "selector", len)) {
		if (argc > 2) {
			val = atoi(argv[2]);
			if (val < 1 || val > 4)
				usage();

			tda7439_data[1] = 4 - val;
			write_all(fd, 0);
		}
		else
			printf("Current Input selector = %d\n", 4 - tda7439_data[1]);
	}
	else if (argc == 2 && !strncmp(argv[1], "mute", len)) {
		mute = (mute + 1) % 2;
		write_all(fd, 1);
	}
	else if (!strncmp(argv[1], "gain", len)) {
		if (argc > 2) {
			len = strlen(argv[2]);
			if (!strncmp(argv[2], "up", len)) {
				if (tda7439_data[2] < 15)
					tda7439_data[2]++;
			}
			else if (!strncmp(argv[2], "down", len)) {
				if (tda7439_data[2] > 0)
					tda7439_data[2]--;
			}
			else {
				val = atoi(argv[2]);
				if (val >= 0 && val <= 30)
					tda7439_data[2] = val / 2;
			}
			write_all(fd, 1);
		}
		else 
			printf("Current Input gain = %d dB\n", tda7439_data[2] * 2);
	}
	else if (!strncmp(argv[1], "volume", len)) {
		if (argc > 2) {
			len = strlen(argv[2]);
			if (!strncmp(argv[2], "up", len)) {
				if (tda7439_data[3] > 0)
					tda7439_data[3]--;
			}
			else if (!strncmp(argv[2], "down", len)) {
				if (tda7439_data[3] < 40)
					tda7439_data[3]++;
			}
			else {
				val = atoi(argv[2]);
				if (val >= -40 && val <= 0)
					tda7439_data[3] = -val;
				else
					usage();
			}
			write_all(fd, 1);
		}
		else 
			printf("Volume = %c%d dB\n", tda7439_data[3]?'-':0, tda7439_data[3]);
	}
	else if (!strncmp(argv[1], "base", len)) {
		if (argc > 2) {
			len = strlen(argv[2]);
			if (!strncmp(argv[2], "up", len)) {
				val = t_tbl[tda7439_data[4]] / 2 + 7;
				if (val < 14)
					tda7439_data[4] = r_tbl[val + 1];
			}
			else if (!strncmp(argv[2], "down", len)) {
				val = t_tbl[tda7439_data[4]] / 2 + 7;
				if (val > 0)
					tda7439_data[4] = r_tbl[val - 1];
			}
			else {
				val = atoi(argv[2]);
				if (val >= -14 && val <= 14) {
					val = atoi(argv[2]) / 2 + 7;
					tda7439_data[4] = r_tbl[val];
				}
				else
					usage();
			}
			write_all(fd, 1);
		}
		else 
			printf("Current Bass gain = %d dB\n", t_tbl[tda7439_data[4]]);
	}
	else if (!strncmp(argv[1], "mid-range", len)) {
		if (argc > 2) {
			len = strlen(argv[2]);
			if (!strncmp(argv[2], "up", len)) {
				val = t_tbl[tda7439_data[5]] / 2 + 7;
				if (val < 14)
					tda7439_data[5] = r_tbl[val + 1];
			}
			else if (!strncmp(argv[2], "down", len)) {
				val = t_tbl[tda7439_data[5]] / 2 + 7;
				if (val > 0)
					tda7439_data[5] = r_tbl[val - 1];
			}
			else {
				val = atoi(argv[2]);
				if (val >= -14 && val <= 14) {
					val = atoi(argv[2]) / 2 + 7;
					tda7439_data[5] = r_tbl[val];
				}
				else
					usage();
			}
			write_all(fd, 1);
		}
		else 
			printf("Current Mid-range gain = %d dB\n", t_tbl[tda7439_data[5]]);
	}
	else if (!strncmp(argv[1], "treble", len)) {
		if (argc > 2) {
			len = strlen(argv[2]);
			if (!strncmp(argv[2], "up", len)) {
				val = t_tbl[tda7439_data[6]] / 2 + 7;
				if (val < 14)
					tda7439_data[6] = r_tbl[val + 1];
			}
			else if (!strncmp(argv[2], "down", len)) {
				val = t_tbl[tda7439_data[6]] / 2 + 7;
				if (val > 0)
					tda7439_data[6] = r_tbl[val - 1];
			}
			else {
				val = atoi(argv[2]);
				if (val >= -14 && val <= 14) {
					val = atoi(argv[2]) / 2 + 7;
					tda7439_data[6] = r_tbl[val];
				}
				else
					usage();
			}
			write_all(fd, 1);
		}
		else 
			printf("Current Treble gain = %d dB\n", t_tbl[tda7439_data[6]]);
	}
	else if (!strncmp(argv[1], "speaker", len)) {
		if (argc > 2) {
			len = strlen(argv[2]);
			if (!strncmp(argv[2], "up", len)) {
				if (tda7439_data[7] > 0)
					tda7439_data[7]--;
			}
			else if (!strncmp(argv[2], "down", len)) {
				if (tda7439_data[7] < 72)
					tda7439_data[7]++;
			}
			else {
				val = atoi(argv[2]);
				if (val >= 0 && val <= 72)
					tda7439_data[7] = val;
				else
					usage();
			}

			tda7439_data[8] = tda7439_data[7];
			write_all(fd, 1);
		}
		else {
			printf("Current Speaker attenuation = %d dB\n", tda7439_data[7]);
			if (tda7439_data[7] != tda7439_data[8])
				printf("Current Speaker attenuation R = %d dB\n", tda7439_data[8]);
		}
	}
	else if (!strncmp(argv[1], "status", len)) {
		printf("TDA7436 Power : %s\n", write_ok ? "on" : "off");
		printf("Mute : %s\n", mute ? "on" : "off");
		printf("Input selector = %d\n", 4 - tda7439_data[1]);
		printf("Input Gain = %d dB\n", tda7439_data[2] * 2);
		printf("Volume = %c%d dB\n", tda7439_data[3]?'-':0, tda7439_data[3]);
		printf("Bass gain = %d dB\n", t_tbl[tda7439_data[4]]);
		printf("Mid-range gain = %d dB\n", t_tbl[tda7439_data[5]]);
		printf("Treble gain = %d dB\n", t_tbl[tda7439_data[6]]);
		printf("Speaker attenuation = %d dB\n", tda7439_data[7]);
		if (tda7439_data[7] != tda7439_data[8])
			printf("Current Speaker attenuation R = %d dB\n", tda7439_data[8]);
	}
	else {
		usage();
	}

	return 0;
}


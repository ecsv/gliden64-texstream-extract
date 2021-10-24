// SPDX-License-Identifier: GPL-3.0-or-later
/* gliden64_texstream_extract, GLideN64 TexCache Extraction tool for debugging
 *
 * SPDX-FileCopyrightText: Sven Eckelmann <sven@narfation.org>
 */

/**
 * Example usage:
 * ./gliden64_texstream_extract --input MUPEN64PLUS.hts -vv -p MUPEN64PLUS > mupen64plus.tar
 */

#include "gliden64_texstream_extract.h"
#include <errno.h>
#include <getopt.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

struct _globals globals;

static int convert_input(void)
{
	int ret;
	uint32_t config;
	uint64_t checksum;
	uint64_t texpos;
	uint64_t storage_pos;
	uint32_t storage_size;
	uint32_t i;
	long pos;

	ret = get_item(config);
	if (ret < 0) {
		fprintf(stderr, "Failed to read config header\n");
		return ret;
	}

	ret = parse_config(config);
	if (ret < 0) {
		fprintf(stderr, "Failed to parse config header\n");
		return ret;
	}

	if (!(config & FILE_CACHE_MASK)) {
		fprintf(stderr, "TexCache format not supported, please use gliden64-cache-extract\n");
		return ret;
	}

	ret = get_item(storage_pos);
	if (ret < 0) {
		fprintf(stderr, "Failed to read index storage offset\n");
		return -EINVAL;
	}

	ret = fseek(globals.in, storage_pos, SEEK_SET);
	if (ret < 0) {
		fprintf(stderr, "Failed to switch to storage index offset %#"PRIx64"\n",
			storage_pos);
		return ret;
	}

	ret = get_item(storage_size);
	if (ret < 0) {
		fprintf(stderr, "Failed to read index storage size\n");
		return -EINVAL;
	}

	for (i = 0; i < storage_size; i++) {
		ret = get_item(checksum);
		if (ret < 0) {
			fprintf(stderr, "Failed to read checksum\n");
			return -EINVAL;
		}

		ret = get_item(texpos);
		if (ret < 0) {
			fprintf(stderr, "Failed to texture position\n");
			return -EINVAL;
		}

		pos = ftell(globals.in);
		if (pos < 0) {
			fprintf(stderr, "Failed to get storage index %u file position\n", i);
			return -EINVAL;
		}

		ret = convert_file(texpos, checksum);
		if (ret < 0)
			return ret;

		ret = fseek(globals.in, pos, SEEK_SET);
		if (ret < 0) {
			fprintf(stderr, "Failed to switch back to storage index\n");
			return ret;
		}
	}

	ret = write_tarblock(tarblock, sizeof(tarblock), 0);
	if (ret < 0) {
		fprintf(stderr, "Failed to write first EOF tar record\n");
		return ret;
	}

	ret = write_tarblock(tarblock, sizeof(tarblock), 0);
	if (ret < 0) {
		fprintf(stderr, "Failed to write second EOF tar record\n");
		return ret;
	}

	return 0;
}

static void usage(int argc, char *argv[])
{
	const char *cmd = "gliden64_texstream_extract";

	if (argc > 1)
		cmd = argv[0];

	printf("Usage: %s [options]\n\n", cmd);
	printf("options:\n");
	printf("\t -i,--input FILE                   Use FILE as uncompressed input file\n");
	printf("\t -o,--output FILE                  Use FILE as output file (default: stdout)\n");
	printf("\t -p,--prefix NAME                  Add prefix to each file\n");
	printf("\t -t,--type [hires|tex]             Type of the input\n");
	printf("\t -v,--verbose                      Print extra information on stderr (repeat for more verbosity)\n");
	printf("\t -e,--ignore-error                 Skip current file when an conversion error is detected\n");
	printf("\t -b,--bitmapv5                     Use V5 Windows Bitmap files with ImageMagick compatible alpha channels\n");
	printf("\t -h,--help                         Show this message and exit\n");
}

static int init(int argc, char *argv[])
{
	int o;
	int options_index;

	static const struct option long_options[] = {
		{"verbose",		no_argument,		NULL, 'v'},
		{"prefix",		required_argument,	NULL, 'p'},
		{"type",		required_argument,	NULL, 'p'},
		{"help",		no_argument,		NULL, 'h'},
		{"ignore-error",	no_argument,		NULL, 'e'},
		{"bitmapv5",		no_argument,		NULL, 'b'},
		{"input",		required_argument,	NULL, 'i'},
		{"output",		required_argument,	NULL, 'o'},
		{NULL,			0,			NULL,  0 },
	};

	memset(&globals, 0, sizeof(globals));
	memset(tarblock, 0, sizeof(tarblock));

	globals.in = NULL;
	globals.out = stdout;

	while ((o = getopt_long(argc, argv, "vp:t:ebhi:o:", long_options, &options_index)) != -1) {
		switch (o) {
		case 'v':
			globals.verbose++;
			break;
		case 'p':
			globals.prefix = strdup(optarg);
			if (!globals.prefix) {
				fprintf(stderr, "Could not save prefix\n");
				return -ENOMEM;
			}
			break;
		case 'h':
			usage(argc, argv);
			exit(0);
			break;
		case 't':
			if (strcasecmp(optarg, "hires") == 0) {
				globals.type = INPUT_HIRES;
			} else if (strcasecmp(optarg, "tex") == 0) {
				globals.type = INPUT_TEX;
			} else {
				fprintf(stderr, "Invalid type %s\n", optarg);
				return -EINVAL;
			}
			break;
		case 'e':
			globals.ignore_error = 1;
			break;
		case 'b':
			globals.bitmapv5 = 1;
			break;
		case 'i':
			if (globals.in)
				fclose(globals.in);

			globals.in = fopen(optarg, "rb");
			if (!globals.in) {
				fprintf(stderr, "Could not open input file %s\n", optarg);
				return -ENOENT;
			}
			break;
		case 'o':
			if (globals.out != stdout)
				fclose(globals.out);

			globals.out = fopen(optarg, "wb");
			if (!globals.out) {
				fprintf(stderr, "Could not open output file %s\n", optarg);
				return -ENOENT;
			}
			break;
		default:
			usage(argc, argv);
			return -EINVAL;
		}
	}

	if (!globals.in) {
		fprintf(stderr, "No input file specified\n");
		return -EINVAL;
	}

	return 0;
}

int main(int argc, char *argv[])
{
	int ret;

	ret = init(argc, argv);
	if (ret < 0) {
		usage(argc, argv);
		return 1;
	}

	ret = convert_input();
	if (ret < 0)
		return 2;

	return 0;
}

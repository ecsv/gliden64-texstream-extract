// SPDX-License-Identifier: GPL-3.0-or-later
/* gliden64_texstream_extract, GLideN64 TexCache Extraction tool for debugging
 *
 * SPDX-FileCopyrightText: Sven Eckelmann <sven@narfation.org>
 */

#include "gliden64_texstream_extract.h"
#include <errno.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static int get_buffer(void *buffer, size_t size, int print_error)
{
	size_t ret;

	ret = fread(buffer, 1, size, globals.in);

	if (ret == size)
		return 0;

	if (ferror(globals.in) && print_error)
		fprintf(stderr, "Error while reading input\n");

	if (feof(globals.in) && print_error)
		fprintf(stderr, "File stream ended to early\n");

	return -EIO;
}

int get_buffer_endian(void *buffer, size_t size, int print_error)
{
	int ret;

	ret = get_buffer(buffer, size, print_error);
	if (ret < 0)
		return ret;


	switch (size) {
	case 1:
		/* no endian problems */
		break;
	case 2:
		*(uint16_t *)buffer = le16toh(*(uint16_t *)buffer);
		break;
	case 4:
		*(uint32_t *)buffer = le32toh(*(uint32_t *)buffer);
		break;
	case 8:
		*(uint64_t *)buffer = le64toh(*(uint64_t *)buffer);
		break;
	default:
		fprintf(stderr, "Invalid item size %u for endianness conversion\n", (unsigned int)size);
		return -EINVAL;
	}

	return 0;
}

int convert_file(long pos, uint64_t checksum)
{
	struct gliden64_file file;
	int ret;

	ret = fseek(globals.in, pos, SEEK_SET);
	if (ret < 0) {
		fprintf(stderr, "Failed to switch to file position %#lx\n",
			pos);
		return ret;
	}

	file.checksum = checksum;

	ret = get_item(file.width);
	if (ret < 0) {
		fprintf(stderr, "Failed to read file width\n");
		return ret;
	}

	ret = get_item(file.height);
	if (ret < 0) {
		fprintf(stderr, "Failed to read file height\n");
		return ret;
	}

	ret = get_item(file.format);
	if (ret < 0) {
		fprintf(stderr, "Failed to read file format\n");
		return ret;
	}

	ret = get_item(file.texture_format);
	if (ret < 0) {
		fprintf(stderr, "Failed to read file texture_format\n");
		return ret;
	}

	ret = get_item(file.pixel_type);
	if (ret < 0) {
		fprintf(stderr, "Failed to read file pixel_type\n");
		return ret;
	}

	ret = get_item(file.is_hires_tex);
	if (ret < 0) {
		fprintf(stderr, "Failed to read file is_hires_tex\n");
		return ret;
	}

	ret = get_item(file.size);
	if (ret < 0) {
		fprintf(stderr, "Failed to read filesize\n");
		return ret;
	}

	if (globals.verbose >= VERBOSITY_FILE_HEADER) {
		fprintf(stderr, "Offset: %#lx\n", pos);
		fprintf(stderr, "File header:\n");
		fprintf(stderr, "\tchecksum: 0x%016"PRIX64"\n", file.checksum);
		fprintf(stderr, "\twidth: %"PRIu32"\n", file.width);
		fprintf(stderr, "\theight: %"PRIu32"\n", file.height);
		fprintf(stderr, "\tformat: %#"PRIx32"\n", file.format);
		fprintf(stderr, "\ttexture_format: %#"PRIx16"\n", file.texture_format);
		fprintf(stderr, "\tpixel_type: %#"PRIx16"\n", file.pixel_type);
		fprintf(stderr, "\tis_hires_tex: %"PRIu8"\n", file.is_hires_tex);
		fprintf(stderr, "\tsize: %"PRIu32"\n", file.size);
		fprintf(stderr, "\n");
	}

	if (file.size <= 0) {
		fprintf(stderr, "Invalid filesize\n");
		return ret;
	}

	file.data = malloc(file.size);
	if (!file.data) {
		fprintf(stderr, "Could not allocate memory for file content\n");
		return -ENOMEM;
	}
	ret = get_buffer(file.data, file.size, 1);
	if (ret < 0) {
		free(file.data);
		fprintf(stderr, "Failed to read file content\n");
		return ret;
	}

	ret = prepare_file(&file);
	if (ret < 0) {
		free(file.data);
		fprintf(stderr, "Failed to prepare file for export\n");
		if (globals.ignore_error)
			return 0;
		else
			return ret;
	}

	ret = write_file(&file);
	free(file.data);
	if (ret < 0) {
		fprintf(stderr, "Could not write file content\n");
		return ret;
	}

	return 0;
}

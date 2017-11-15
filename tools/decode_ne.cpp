
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

static void TO_LE16(uint8_t *dst, uint16_t value) {
	for (int i = 0; i < 2; ++i) {
		dst[i] = value & 255; value >>= 8;
	}
}

static void TO_LE32(uint8_t *dst, uint32_t value) {
	for (int i = 0; i < 4; ++i) {
		dst[i] = value & 255; value >>= 8;
	}
}

static bool freadTag(FILE *fp, const char *tag) {
	assert(tag);
	while (*tag) {
		if (fgetc(fp) != *tag) {
			return false;
		}
		++tag;
	}
	return true;
}

static uint16_t freadUint16LE(FILE *fp) {
	uint16_t val = fgetc(fp);
	return (fgetc(fp) << 8) + val;
}

enum {
	kResTypeCursor = 0x8001,
	kResTypeBitmap = 0x8002,
	kResTypeIcon = 0x8003,
	kResTypeMenu = 0x8004,
	kResTypeDialog = 0x8005,
	kResTypeString = 0x8006,
	kResTypeGroupFont = 0x8007,
	kResTypeFont = 0x8008,
	kResTypeData = 0x800A,
	kResTypeGroupCursor = 0x800C,
	kResTypeGroupIcon = 0x800E,
	kResTypeVersion = 0x8010,
};

const char *resourceTypeName(int type) {
	switch (type) {
	case kResTypeCursor:
		return "cursor";
	case kResTypeBitmap:
		return "bitmap";
	case kResTypeIcon:
		return "icon";
	case kResTypeMenu:
		return "menu";
	case kResTypeString:
		return "string";
	case kResTypeData:
		return "data";
	case kResTypeGroupCursor:
		return "group_cursor";
	case kResTypeGroupIcon:
		return "group_icon";
	default:
		return "";
	}
}

struct ResourceEntry {
	uint32_t offset;
	uint32_t size;
	uint16_t flags;
	uint16_t id;
};

#define BITMAPFILEHEADER_SIZE 14
#define BITMAPINFOHEADER_SIZE 40

static void dumpResource(FILE *fp, int type, int index, const ResourceEntry *re) {
	fprintf(stdout, "Resource size %d flags 0x%x id 0x%x type 0x%x\n", re->size, re->flags, re->id, type);
	FILE *out;

	char name[32];
	snprintf(name, sizeof(name), "%s%d", resourceTypeName(type), index);

	const int pos = ftell(fp);
	fseek(fp, re->offset, SEEK_SET);

	uint8_t *buf = (uint8_t *)malloc(re->size);
	if (buf) {
		fread(buf, 1, re->size, fp);

		switch (type) {
		case kResTypeIcon:
			out = fopen(name, "wb");
			if (out) {
					// icons are stored with DIB header
				uint8_t header[BITMAPFILEHEADER_SIZE];

				header[0] = 'B'; header[1] = 'M';
				TO_LE32(header + 2, BITMAPFILEHEADER_SIZE + re->size);
				TO_LE16(header + 6, 0);
				TO_LE16(header + 8, 0);
				TO_LE32(header + 10, BITMAPFILEHEADER_SIZE + BITMAPINFOHEADER_SIZE + 16 * 4);

				fwrite(header, 1, sizeof(header), out);

				// Fixup dimensions to 32x32, the lower 32 rows contain garbage
				if (buf[0] == 40 && buf[4] == 32 && buf[8] == 64) {
					fprintf(stdout, "Fixing up dimensions to 32x32\n");
					buf[8] = 32;
				}

				fwrite(buf, 1, re->size, out);
				fclose(out);
			}
			break;
		case kResTypeData:
			out = fopen(name, "wb");
			if (out) {
				fwrite(buf, 1, re->size, out);
				fclose(out);
			}
			break;
		}
		free(buf);
	}
	fseek(fp, pos, SEEK_SET);
}

static void decodeResourceTable(FILE *fp) {
	const int align = 1 << freadUint16LE(fp);
	int type = freadUint16LE(fp);
	while (type != 0) {
		assert((type & 0x8000) != 0);

		// DW Number of resources for this type
		int count = freadUint16LE(fp);
		fprintf(stdout, "Resource type 0x%x '%s' count %d\n", type, resourceTypeName(type), count);

		// DD Reserved.
		fseek(fp, 4, SEEK_CUR);

		for (int i = 0; i < count; ++i) {
			ResourceEntry re;

			// DW File offset to the contents of the resource data
			re.offset = freadUint16LE(fp) * align;

			// DW Length of resource in the file
			re.size = freadUint16LE(fp) * align;

			// DW Flag word.
			re.flags = freadUint16LE(fp);

			// DW Resource ID
			re.id = freadUint16LE(fp);

			// DD Reserved.
			fseek(fp, 4, SEEK_CUR);

			dumpResource(fp, type, i, &re);
		}

		type = freadUint16LE(fp);
	}
}

int main(int argc, char *argv[]) {
	if (argc == 2) {
		FILE *fp = fopen(argv[1], "rb");
		if (fp) {
			if (!freadTag(fp, "MZ")) {
				fprintf(stdout, "Failed to read MZ tag\n");
			} else {
				fseek(fp, 60, SEEK_SET);
				uint16_t executableOffset = freadUint16LE(fp);
				fseek(fp, executableOffset, SEEK_SET);
				if (!freadTag(fp, "NE")) {
					fprintf(stdout, "Failed to read NE tag\n");
				} else {
					fseek(fp, executableOffset + 36, SEEK_SET);
					uint16_t resourceOffset = freadUint16LE(fp);
					if (resourceOffset != 0) {
						fseek(fp, executableOffset + resourceOffset, SEEK_SET);
						decodeResourceTable(fp);
					}
				}
			}
			fclose(fp);
		}
	}
	return 0;
}

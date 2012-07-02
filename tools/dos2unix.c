#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void check_io(const char *f, size_t count1, size_t count2) {
	if (count1 != count2) {
		printf("I/O error on %s %d,%d\n", f, count1, count2);
		exit(-1);
	}
}

static void convert_rn_n(char *buf, FILE *fp) {
	char *p;
	size_t len, count;
	while ((p = strstr(buf, "\r\n"))) {
		len = p - buf + 1;
		p[0] = '\n';
		count = fwrite(buf, 1, len, fp);
		check_io("fwrite", count, len);
		buf = &p[2];
	}
	len = strlen(buf);
	count = fwrite(buf, 1, len, fp);
	check_io("fwrite", count, len);
}

int main(int argc, char *argv[]) {
	int i;
	long file_size, count;
	for (i = 1; i < argc; ++i) {
		FILE *fp = fopen(argv[i], "rb");
		if (fp) {
			fseek(fp, 0, SEEK_END);
			file_size = ftell(fp);
			fseek(fp, 0, SEEK_SET);
			if (file_size > 0) {
				char *file_buf = (char *)malloc(file_size + 1);
				if (file_buf) {
					count = fread(file_buf, 1, file_size, fp);
					check_io("fread", count, file_size);
					fclose(fp);
					file_buf[file_size] = 0;
					
					fp = fopen(argv[i], "wb");
					if (fp) {
						convert_rn_n(file_buf, fp);
						fclose(fp);
					}
					free(file_buf);
				}
			}
		}
	}	
	return 0;
}

#include<stdio.h>
#include<stdlib.h>
#include<fcntl.h>
#include<unistd.h>
#include<ctype.h>

typedef unsigned long long wc_count_t;
wc_count_t tot_line_cnt = 0, tot_word_cnt = 0, tot_char_cnt = 0;
int rval = 0;

void counts(char* file);
void print(wc_count_t, wc_count_t, wc_count_t, char*);

int main(int argc, char *argv[]) {
	if (argc == 1) {
		counts(NULL);
		return rval;
	}
	int flag = (argc > 2);
	while (argc-- > 1)
		counts(*++argv);
	if (flag) print(tot_line_cnt, tot_word_cnt, tot_char_cnt, "total");
	return rval;
}

void counts(char* file) {
	wc_count_t line_cnt = 0, word_cnt = 0, char_cnt = 0;
	int fd; char *name;
	if (file) {
		if ((fd = open(file, O_RDONLY, 0)) == -1) {
			fprintf(stderr, "wc: %s: No such file or directory\n", file);
			rval = 1;
			return;
		}
		name = file;
	} else {
		fd = STDIN_FILENO;
		name = "<stdin>";
	}
	int len = 0, has_space = 1;
	char ch;
	while ((len = read(fd, &ch, 1)) == 1) {
		char_cnt++;
		if (isspace(ch)) {
			has_space = 1;
			if (ch == '\n') line_cnt++;
		} else if(has_space){
			word_cnt++;
			has_space = 0;
		}
	}
	
	print(line_cnt, word_cnt, char_cnt, name);
	tot_line_cnt += line_cnt;
	tot_word_cnt += word_cnt;
	tot_char_cnt += char_cnt;
	if (close(fd)) {
		fprintf(stderr, "wc: error in file %s", name);
		rval = 1;
	}
}

void print(wc_count_t lines, wc_count_t words, wc_count_t chars, char* name) {
	printf("%llu %llu %llu", lines, words, chars);
	name ? printf(" %s\n", name) : printf("\n");
}

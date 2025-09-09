#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <string.h>
#include <signal.h>

#define MIN_WIDTH 80
#define MIN_HEIGHT 15

#define MAX_WIDTH 500
#define MAX_HEIGHT 200

struct termios original_termios; 

int r = 0;
int g = 0;
int b = 0;
int sel = 0;

int clamp(int min, int max, int x) {
	if (x < min) return min;
	if (x > max) return max;
	return x;
}

void drawbar(int r, int g, int b, int width) {
	for (int i = 0; i < width; i++) {
		int lerp = i * 255 / width;
		int pr = (r == -1) ? lerp : r;
		int pg = (g == -1) ? lerp : g;
		int pb = (b == -1) ? lerp : b;
		printf("\e[48;2;%d;%d;%dm ", pr, pg, pb);
	}
	printf("\e[m");
}

void drawslider(int color) {
	printf("\e[%dm\e[A \e[1B\e[1D \e[1B\e[1D \e[m", color);
}

void drawbox(int height, int width, int color) {
	printf("\e[48;5;%dm", color);
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) printf(" ");
		printf("\e[%dD\e[B", width);
	}
	printf("\e[m");
}

void draw() {
	struct winsize ws;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
	int width = clamp(0, MAX_WIDTH, ws.ws_col);
	int height = clamp(0, MAX_HEIGHT, ws.ws_row);
	printf("\e[H\e[J");
	if (width < MIN_WIDTH || height < MIN_HEIGHT) {
		char msg[256];
		sprintf(msg, "terminal too small: %d x %d", width, height);
		printf("\e[%d;%dH%s", height / 2, (int)(width / 2 - (strlen(msg) / 2)), msg);
		fflush(stdout);
		return;
	}

	printf("\e[%d;%dH", height / 2 - 4, width / 8);
	drawbar(-1, g * 51, b * 51, width / 2);
	printf("\e[%d;%dH", height / 2 - 4, width / 8 + (r * (width / 2 - 1) / 5));
	drawslider(sel == 0 ? 107 : 47);
	
	printf("\e[%d;%dH", height / 2 + 0, width / 8);
	drawbar(r * 51, -1, b * 51, width / 2);
	printf("\e[%d;%dH", height / 2 + 0, width / 8 + (g * (width / 2 - 1) / 5));
	drawslider(sel == 1 ? 107 : 47);
	
	printf("\e[%d;%dH", height / 2 + 4, width / 8);
	drawbar(r * 51, g * 51, -1, width / 2);
	printf("\e[%d;%dH", height / 2 + 4, width / 8 + (b * (width / 2 - 1) / 5));
	drawslider(sel == 2 ? 107 : 47);

	int color = 16 + r * 36 + g * 6 + b;

	printf("\e[%d;%dH", height / 2 - 5, width * 3 / 4);
	drawbox(9, 20, color);

	printf("\e[%d;%dHescape: \\e[38;5;%dm", height / 2 + 5, width * 3 / 4, color);
	
	fflush(stdout);
}

void sigwinch(int sg) {
	(void)sg;
	draw();
}

void leave() {
	printf("\e[?1049l\e[?25h");
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios);
}

void enter() {
	tcgetattr(STDIN_FILENO, &original_termios);

	struct termios tios = original_termios;
	tios.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG); 
	tios.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON); 
	tios.c_oflag &= ~(OPOST); 
	tios.c_cflag |= (CS8); 

	tios.c_cc[VMIN] = 0; 
	tios.c_cc[VTIME] = 1; 

	tcsetattr(STDIN_FILENO, TCSAFLUSH, &tios);
	printf("\e[?1049h\e[?25l\e[H");
	fflush(stdout);
}

int main() {
	signal(SIGWINCH, sigwinch);

	enter();
	draw();

	char c;
	char o;
	while (c != 'q') {
		o = read(STDIN_FILENO, &c, 1);
		if (o != 1) continue;
		if (c == 'r') r = g = b = 0;
		if (c == 'j') sel = (sel + 1) % 3;
		if (c == 'k') sel = (sel + 2) % 3;
		if (c == 'l') {
			if (sel == 0) r = clamp(0, 5, r+1);
			if (sel == 1) g = clamp(0, 5, g+1);
			if (sel == 2) b = clamp(0, 5, b+1);
		}
		if (c == 'h') {
			if (sel == 0) r = clamp(0, 5, r-1);
			if (sel == 1) g = clamp(0, 5, g-1);
			if (sel == 2) b = clamp(0, 5, b-1);
		}
		draw();
	}

	leave(); 
}


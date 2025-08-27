#ifndef TUTUI_H
#define TUTUI_H

#include <stdio.h>
#include <wchar.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <stdlib.h>
#include <locale.h>


typedef struct tu_position {
	int x;
	int y;
} tu_position;

typedef struct tu_window {
	char *title;
	struct winsize size; //ws_row, ws_col
	tu_position position;
	wchar_t *draw_buffer;
} tu_window;

tu_window* tu_create_window(char *title);


#ifdef TUTUI_IMPLEMENTATION

tu_window* tu_create_window(char *title){
	tu_window *window = malloc(sizeof(*window));
	window->title = malloc(strlen(title));
	strncpy(window->title, title, strlen(title));

	if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &window->size) == -1) return NULL;
	window->position = (tu_position){0, 0};

	size_t buffer_size = sizeof(wchar_t) * window->size.ws_row * window->size.ws_col;
	window->draw_buffer = malloc(buffer_size);
	wmemset(window->draw_buffer, L' ', buffer_size/sizeof(wchar_t));

	return window;
}

void tu_run(tu_window *window){
	setlocale(LC_ALL, "");
	const wchar_t border[6] = {L'╔', L'═', L'╗', L'║', L'╝', L'╚'};
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &window->size);

	for(int y = window->position.y; y < window->size.ws_row; y++){
		for(int x = window->position.x; x < window->size.ws_col; x++){
			int linear_index = x + y * window->size.ws_col;

			if(x % (window->size.ws_col-2) == 0)
				window->draw_buffer[linear_index] = border[3];
			if(y % (window->size.ws_row-1) == 0 && x != window->size.ws_col-1)
				window->draw_buffer[linear_index] = border[1];
			if(x == window->position.x && y == window->position.y)
				window->draw_buffer[linear_index] = border[0];
			if(x == window->position.x && y == window->size.ws_row-1)
				window->draw_buffer[linear_index] = border[5];
			if(x == window->size.ws_col-2 && y == window->size.ws_row-1)
				window->draw_buffer[linear_index] = border[4];
			if(x == window->size.ws_col-2 && y == window->position.y)
				window->draw_buffer[linear_index] = border[2];
			if(x == window->size.ws_col-1 && y != window->size.ws_row-1) 
				window->draw_buffer[linear_index] = L'\n';
		}
	}
	window->draw_buffer[window->size.ws_row * window->size.ws_col] = L'\0';

	wprintf(L"\r%ls", window->draw_buffer); // Formatted output >>
	fflush(stdout);
	usleep((1000 * 1000) / 60);

	wmemset(window->draw_buffer, L' ', window->size.ws_row * window->size.ws_col);
}

#endif
#endif

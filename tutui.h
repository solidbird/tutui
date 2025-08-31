#ifndef TUTUI_H
#define TUTUI_H

#include <stdio.h>
#include <wchar.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <stdlib.h>
#include <locale.h>

/*void *draw_elements[] = {
	tu_draw_textlabel,
	tu_draw_textfield,
	tu_draw_textlabel,
	tu_draw_textlabel,
	tu_draw_textlabel,
};*/

typedef enum tu_element_type {
	TU_WINDOW,
	TU_TEXTLABEL,
	TU_TEXTFIELD,
	TU_TEXTAREA,
	TU_TEXTAREPANEL,
	TU_TEXTAREBUTTON,
	TU_TEXTARECHECKBOX,
	// ...
} tu_element_type;

typedef struct tu_element {
	void *ref;
	tu_element_type type;
	size_t child_count;
	struct tu_element* children;
	struct tu_element* parent;	
} tu_element;

typedef struct tu_position {
	int x;
	int y;
} tu_position;

typedef struct tu_window {
	char *title;
	struct winsize size; //ws_row, ws_col
	tu_position position;

	wchar_t *draw_buffer;
	tu_element main_element;
} tu_window;

tu_window* tu_create_window(char *title);

#ifdef TUTUI_IMPLEMENTATION

tu_window* tu_create_window(char *title){
	tu_window *window = malloc(sizeof(*window));
	window->title = malloc(strlen(title));
	strncpy(window->title, title, strlen(title));

	if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &window->size) == -1) return NULL;
	window->position = (tu_position){0, 0};
	window->main_element = (tu_element){0};

	size_t buffer_size = sizeof(wchar_t) * (window->size.ws_row * window->size.ws_col + 1);
	window->draw_buffer = malloc(buffer_size);
	wmemset(window->draw_buffer, L' ', buffer_size/sizeof(wchar_t));

	return window;
}

static void __prep_main_window(tu_window **window){
	setlocale(LC_ALL, "");
	const wchar_t border[6] = {L'╔', L'═', L'╗', L'║', L'╝', L'╚'};

	for(int y = (*window)->position.y; y < (*window)->size.ws_row; y++){
		for(int x = (*window)->position.x; x < (*window)->size.ws_col; x++){
			int linear_index = x + y * (*window)->size.ws_col;

			if(x % ((*window)->size.ws_col-2) == 0)
				(*window)->draw_buffer[linear_index] = border[3];
			if(y % ((*window)->size.ws_row-1) == 0 && x != (*window)->size.ws_col-1)
				(*window)->draw_buffer[linear_index] = border[1];
			if(x == (*window)->position.x && y == (*window)->position.y)
				(*window)->draw_buffer[linear_index] = border[0];
			if(x == (*window)->position.x && y == (*window)->size.ws_row-1)
				(*window)->draw_buffer[linear_index] = border[5];
			if(x == (*window)->size.ws_col-2 && y == (*window)->size.ws_row-1)
				(*window)->draw_buffer[linear_index] = border[4];
			if(x == (*window)->size.ws_col-2 && y == (*window)->position.y)
				(*window)->draw_buffer[linear_index] = border[2];
			if(x == (*window)->size.ws_col-1 && y != (*window)->size.ws_row-1) 
				(*window)->draw_buffer[linear_index] = L'\n';
		}
	}
}

void tu_run(tu_window *window) {
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &window->size);
	size_t area = window->size.ws_row * window->size.ws_col;

	wchar_t *tmp = realloc(window->draw_buffer, sizeof(wchar_t) * (area + 1));
	if (tmp == NULL) {
		perror("realloc failed");
		free(window->draw_buffer);
		exit(1);
	}
	window->draw_buffer = tmp;

	wmemset(window->draw_buffer, L' ', area);
	window->draw_buffer[area] = L'\0';

	__prep_main_window(&window);

	wprintf(L"\r%ls", window->draw_buffer);
	fflush(stdout);
	usleep((1000 * 1000) / 60);  // 60 FPS
}


#endif
#endif

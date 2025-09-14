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

#define FG_COLOR(R, G, B) "\x1b[38;2;" #R ";" #G ";" #B "m"
#define BG_COLOR(R, G, B) "\x1b[48;2;" #R ";" #G ";" #B "m"
#define END_COLOR "\x1b[0m"


const wchar_t border[6] = {L'╔', L'═', L'╗', L'║', L'╝', L'╚'};

typedef enum tu_element_type {
	TU_WINDOW,
	TU_CONTAINER,
	TU_TEXTLABEL,
	TU_TEXTFIELD,
	TU_TEXTAREA,
	TU_TEXTAREPANEL,
	TU_TEXTAREBUTTON,
	TU_TEXTARECHECKBOX,
	// ...
} tu_element_type;

typedef struct tu_position {
	int x;
	int y;
} tu_position;

typedef struct tu_element {
	void *specifics;

	tu_element_type type;
	tu_position position;
	struct winsize size; //ws_row, ws_col

	size_t child_count;
	struct tu_element** children;
	struct tu_element* parent;	
} tu_element;

typedef struct tu_window {
	char *title;
	wchar_t *draw_buffer;
	
	tu_element main_element;
} tu_window;

typedef struct tu_textlabel {
	char *text;
	tu_element element;
} tu_textlabel;

void tu_add(tu_element *parent_element, tu_element *child_element);
tu_window* tu_create_window(char *title);
tu_textlabel* tu_create_textlabel(char *text, int x, int y, int width, int height);

#ifdef TUTUI_IMPLEMENTATION

tu_textlabel* tu_create_textlabel(char *text, int x, int y, int width, int height){
	tu_textlabel *tl = malloc(sizeof(*tl));
	tl->text = text;
	tl->element.position = (tu_position){x, y};
	tl->element.size = (struct winsize) {width, height};

	return tl;
}

void tu_add(tu_element *parent_element, tu_element *child_element){
	if(parent_element->child_count > 0){
		parent_element->children = realloc(parent_element->children, sizeof(*child_element) * ++(parent_element->child_count));
	} else {
		parent_element->children = malloc(sizeof(*child_element) * ++(parent_element->child_count));
	}
	parent_element->children[parent_element->child_count - 1] = child_element;
}

tu_window* tu_create_window(char *title){
	tu_window *window = malloc(sizeof(*window));
	window->title = malloc(strlen(title));
	strncpy(window->title, title, strlen(title));

	if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &window->main_element.size) == -1) return NULL;
	window->main_element.position = (tu_position){0, 0};
	window->main_element = (tu_element){0};

	size_t buffer_size = sizeof(wchar_t) * (window->main_element.size.ws_row * window->main_element.size.ws_col + 1);
	window->draw_buffer = malloc(buffer_size);
	wmemset(window->draw_buffer, L' ', buffer_size/sizeof(wchar_t));

	return window;
}

static void __general_border(wchar_t *draw_buffer, int pos_x, int pos_y, int row, int col){

	for(int y = pos_y; y < row; y++){
		for(int x = pos_x; x < col; x++){
			int linear_index = x + y * col;

			if(x % (col-2) == 0)
				draw_buffer[linear_index] = border[3];
			if(y % (row-1) == 0 && x != col-1)
				draw_buffer[linear_index] = border[1];
			if(x == pos_x && y == pos_y)
				draw_buffer[linear_index] = border[0];
			if(x == pos_x && y == row-1)
				draw_buffer[linear_index] = border[5];
			if(x == col-2 && y == row-1)
				draw_buffer[linear_index] = border[4];
			if(x == col-2 && y == pos_y)
				draw_buffer[linear_index] = border[2];
			if(x == col-1 && y != row-1) 
				draw_buffer[linear_index] = L'\n';
		}
	}
}

static void __element_border(tu_window **window, int pos_x, int pos_y, int row, int col){
	int window_col = (*window)->main_element.size.ws_col;

	for(int y = pos_y; y < (*window)->main_element.size.ws_row; y++){
		for(int x = pos_x; x < (*window)->main_element.size.ws_col; x++){
			int linear_index = x + y * window_col;

			if(x == pos_x && y >= pos_y && y < pos_y + row)
				(*window)->draw_buffer[linear_index] = border[3];
			if(x == pos_x + col && y >= pos_y && y < pos_y + row)
				(*window)->draw_buffer[linear_index] = border[3];
			if(y == pos_y && x >= pos_x && x < pos_x + col)
				(*window)->draw_buffer[linear_index] = border[1];
			if(y == pos_y + row && x >= pos_x && x < pos_x + col)
				(*window)->draw_buffer[linear_index] = border[1];
			if(x == pos_x && y == pos_y)
				(*window)->draw_buffer[linear_index] = border[0];
			if(x == pos_x + row && y == pos_y)
				(*window)->draw_buffer[linear_index] = border[2];
			if(x == pos_x + row && y == pos_y + col)
				(*window)->draw_buffer[linear_index] = border[4];
			if(x == pos_x && y == pos_y + col)
				(*window)->draw_buffer[linear_index] = border[5];
		}
	}
}

static void __prep_main_window(tu_window **window){
	setlocale(LC_ALL, "");

	__general_border(
		(*window)->draw_buffer,
		(*window)->main_element.position.x,
		(*window)->main_element.position.y,
		(*window)->main_element.size.ws_row,
		(*window)->main_element.size.ws_col
	);


	for(int i = 0; i < (*window)->main_element.child_count; i++){
		__element_border(
			window,
			(*window)->main_element.children[i]->position.x,
			(*window)->main_element.children[i]->position.y,
			(*window)->main_element.children[i]->size.ws_row,
			(*window)->main_element.children[i]->size.ws_col
		);
	}
}

void tu_run(tu_window *window) {
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &window->main_element.size);
	size_t area = window->main_element.size.ws_row * window->main_element.size.ws_col;

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

	printf(BG_COLOR(0,200, 0));
	printf(FG_COLOR(200,0, 0));
	printf("\r%ls", window->draw_buffer);
	printf(END_COLOR);
	printf(END_COLOR);
	fflush(stdout);
	usleep((1000 * 1000) / 60);  // 60 FPS
}

#endif
#endif

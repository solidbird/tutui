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


typedef enum tu_border_index {
	TU_BORDER_TOP_LEFT_CORNER,
	TU_BORDER_HORIZONTAL,
	TU_BORDER_TOP_RIGHT_CORNER,
	TU_BORDER_VERTICAL,
	TU_BORDER_BOTTOM_RIGT_CORNER,
	TU_BORDER_BOTTOM_LEFT_CORNER,
} tu_border_index;
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

typedef struct Base {
	tu_element element;
} Base;

typedef struct tu_window {
	tu_element main_element;

	char *title;
	wchar_t *draw_buffer;
} tu_window;

typedef struct tu_textlabel {
	tu_element element;

	char *text;
} tu_textlabel;

void tu_add(void *parent, void *child);
tu_window* tu_create_window(char *title);
tu_textlabel* tu_create_textlabel(char *text, int x, int y, int width, int height);

#ifdef TUTUI_IMPLEMENTATION

tu_textlabel* tu_create_textlabel(char *text, int x, int y, int width, int height){
	tu_textlabel *tl = malloc(sizeof(*tl));
	tl->text = text;
	tl->element.position = (tu_position){x, y};
	tl->element.size = (struct winsize) {width, height};
	tl->element.specifics = tl;
	tl->element.type = TU_TEXTLABEL;

	return tl;
}

void tu_add(void *parent, void *child){
	tu_element *parent_element = &((Base*)parent)->element;
	tu_element *child_element = &((Base*)child)->element;
	child_element->specifics = child;

	if(parent_element->child_count > 0){
		parent_element->children = realloc(parent_element->children, sizeof(*child_element) * ++(parent_element->child_count));
	} else {
		parent_element->children = malloc(sizeof(*child_element) * ++(parent_element->child_count));
	}
	parent_element->children[parent_element->child_count - 1] = child_element;
	child_element->parent = parent_element;
}

tu_window* tu_create_window(char *title){
	tu_window *window = malloc(sizeof(*window));
	window->title = malloc(strlen(title));
	strncpy(window->title, title, strlen(title));

	if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &window->main_element.size) == -1) return NULL;
	window->main_element = (tu_element){0};
	window->main_element.position = (tu_position){0, 0};
	window->main_element.specifics = window;
	window->main_element.type = TU_WINDOW;

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
				draw_buffer[linear_index] = border[TU_BORDER_VERTICAL];
			if(y % (row-1) == 0 && x != col-1)
				draw_buffer[linear_index] = border[TU_BORDER_HORIZONTAL];
			if(x == pos_x && y == pos_y)
				draw_buffer[linear_index] = border[TU_BORDER_TOP_LEFT_CORNER];
			if(x == pos_x && y == row-1)
				draw_buffer[linear_index] = border[TU_BORDER_BOTTOM_LEFT_CORNER];
			if(x == col-2 && y == row-1)
				draw_buffer[linear_index] = border[TU_BORDER_BOTTOM_RIGT_CORNER];
			if(x == col-2 && y == pos_y)
				draw_buffer[linear_index] = border[TU_BORDER_TOP_RIGHT_CORNER];
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

void update_position(tu_element *node, tu_position pos){
}

void travel_child_tree(tu_window **window, tu_element *parent){
	for(int i = 0; i < parent->child_count; i++){
		if(parent->children[i]->child_count == 0)
			__element_border(
				window,
				parent->children[i]->parent->position.x + parent->children[i]->position.x,
				parent->children[i]->parent->position.y + parent->children[i]->position.y,
				parent->children[i]->size.ws_row,
				parent->children[i]->size.ws_col
			);
		else
			travel_child_tree(window, parent->children[i]);
	}
	if(&(*window)->main_element != parent)
		__element_border(
			window,
			parent->parent->position.x + parent->position.x,
			parent->parent->position.y + parent->position.y,
			parent->size.ws_row,
			parent->size.ws_col
		);
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

	travel_child_tree(window, &((*window)->main_element));
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

	printf(BG_COLOR(80, 80, 230));
	printf(FG_COLOR(255, 255, 255));
	printf("\r%ls", window->draw_buffer);
	printf(END_COLOR);
	printf(END_COLOR);
	fflush(stdout);
	usleep((1000 * 1000) / 60);  // 60 FPS
}

#endif
#endif

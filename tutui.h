#ifndef TUTUI_H
#define TUTUI_H

#include <stdio.h>
#include <wchar.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <stdlib.h>
#include <locale.h>
#include <stdbool.h>

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

#define tu_layout(node, ...) \
	tu_layout_(node, (tu_layout_params) { __VA_ARGS__ })

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

typedef enum  {
	TU_LAYOUT_FIXED,
	TU_LAYOUT_FLOW,
	TU_LAYOUT_VBOX,
	TU_LAYOUT_HBOX,
	TU_LAYOUT_GRID,
} tu_layout_type;


typedef struct tu_position {
	int x;
	int y;
} tu_position;

typedef struct {
	tu_layout_type type;

	int row;
	int col;

} tu_layout_params;

typedef struct tu_element {
	void *specifics;

	tu_element_type type;
	tu_position position;
	struct winsize size; //ws_row, ws_col
	tu_layout_params layout;

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
void apply_layout_on_children(tu_element *parent);
void reshape_based_on_layout(tu_element *parent);
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

// Try to keep default values for tu_layout_params 0 so we can pretend to overwrite default values here
void tu_layout_(void *node, tu_layout_params layout_params){
	tu_element *element = &((Base*)node)->element;
	element->layout = layout_params;
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

bool check_point_within_bounds(tu_position pos, tu_position bound_pos, struct winsize bound_size){
	if(bound_pos.x < pos.x &&
	bound_pos.y < pos.y &&
	bound_size.ws_col + bound_pos.x > pos.x &&
	bound_size.ws_row + bound_pos.y > pos.y) return true;

	return false;
}

bool check_rect_within_bounds(tu_position pos, struct winsize size, tu_position bound_pos, struct winsize bound_size){
	if(bound_pos.x < pos.x &&
	bound_pos.y < pos.y &&
	bound_size.ws_col + bound_pos.x > pos.x + size.ws_col &&
	bound_size.ws_row + bound_pos.y > pos.y + size.ws_row) return true;

	return false;
}

static void __element_border(tu_window **window, tu_element *parent, int pos_x, int pos_y, int row, int col){
	int window_col = (*window)->main_element.size.ws_col;

	for(int y = pos_y; y < (*window)->main_element.size.ws_row; y++){
		for(int x = pos_x; x < (*window)->main_element.size.ws_col; x++){
			int linear_index = x + y * window_col;


			if(x == pos_x && y >= pos_y && y < pos_y + row){ // Draw topleft -> bottomleft
				if(!check_point_within_bounds((tu_position){x, y}, parent->position, parent->size)) continue;
				(*window)->draw_buffer[linear_index] = border[TU_BORDER_VERTICAL];
			}if(x == pos_x + col && y >= pos_y && y < pos_y + row){ // topright -> bottomright
				if(!check_point_within_bounds((tu_position){x, y}, parent->position, parent->size)) continue;
				(*window)->draw_buffer[linear_index] = border[TU_BORDER_VERTICAL];
			}if(y == pos_y && x >= pos_x && x < pos_x + col){ // topleft -> topright
				if(!check_point_within_bounds((tu_position){x, y}, parent->position, parent->size)) continue;
				(*window)->draw_buffer[linear_index] = border[TU_BORDER_HORIZONTAL];
			}if(y == pos_y + row && x >= pos_x && x < pos_x + col){ // bottomleft -> bottomright
				if(!check_point_within_bounds((tu_position){x, y}, parent->position, parent->size)) continue;
				(*window)->draw_buffer[linear_index] = border[TU_BORDER_HORIZONTAL];
			}if(x == pos_x && y == pos_y){ // topleft
				if(!check_point_within_bounds((tu_position){x, y}, parent->position, parent->size)) continue;
				(*window)->draw_buffer[linear_index] = border[TU_BORDER_TOP_LEFT_CORNER];
			}if(x == pos_x + col && y == pos_y){ // topright
				if(!check_point_within_bounds((tu_position){x, y}, parent->position, parent->size)) continue;
				(*window)->draw_buffer[linear_index] = border[TU_BORDER_TOP_RIGHT_CORNER];
			}if(x == pos_x + col && y == pos_y + row){ // bottomright
				if(!check_point_within_bounds((tu_position){x, y}, parent->position, parent->size)) continue;
				(*window)->draw_buffer[linear_index] = border[TU_BORDER_BOTTOM_RIGT_CORNER];
			}if(x == pos_x && y == pos_y + row){ // bottomleft
				if(!check_point_within_bounds((tu_position){x, y}, parent->position, parent->size)) continue;
				(*window)->draw_buffer[linear_index] = border[TU_BORDER_BOTTOM_LEFT_CORNER];
			}
		}
	}
}

/*
- FLOW:
(a bit more complex)
	A B C D
	E F G H

	A - (0,0)
	B - (A.width, 0)
	C - (B.width, 0)
	D - (C.width, 0)
	E - (0, A.height)
	F - (A.width, B.height)
	G - (B.width, C.height)
	H - (C.width, D.height)
	
- VBOX:
	A
	B
	C
	D
	E
	F
	G
	H

	(container.width, container.height / container.child_count)

- HBOX:
	(container.width / container.child_count, container.height)

- GRID:
	4x2

	A B C D
	E F G H

	A - (0,0)
	B - (1 * container.width/4, 0)
	C - (2 * ", 0)
	D - (3 * ", 0)
	E - (0, container. 1 * height/2)
	F - (1 * ", container. 1 * height/2)
	G - (2 * ", container. 1 * height/2)
	H - (3 * ", container. 1 * height/2)

*/

void tu_init(tu_window **win){
	reshape_based_on_layout(&(*win)->main_element);
}

void apply_layout_on_children(tu_element *parent){

	for(int i = 0; i < parent->child_count; i++){
		parent->children[i]->position.x += parent->position.x;
		parent->children[i]->position.y += parent->position.y;
	}

	switch(parent->layout.type){
		case TU_LAYOUT_FIXED:
		break;
		case TU_LAYOUT_FLOW:
		break;
		case TU_LAYOUT_VBOX:
			for(int i = 0; i < parent->child_count; i++){
				parent->children[i]->position =
					(tu_position){
						parent->position.x + 1,
						(parent->children[i]->position.y) + i * (parent->size.ws_row / parent->child_count)
					};
				parent->children[i]->size =
					(struct winsize){
						.ws_row = parent->size.ws_row / parent->child_count - 2,
						.ws_col = parent->size.ws_col - 2
					};
			}
		break;
		case TU_LAYOUT_HBOX:
			for(int i = 0; i < parent->child_count; i++){
				parent->children[i]->position =
					(tu_position){
						parent->children[i]->position.x + i * (parent->size.ws_col / parent->child_count),
						parent->position.y + 1
					};
				parent->children[i]->size =
					(struct winsize){
						.ws_row = parent->size.ws_row - 2,
						.ws_col = parent->size.ws_col / parent->child_count - 2
					};
			}
		break;
		case TU_LAYOUT_GRID:
			for(int y = 0; y < parent->layout.col; y++){
				for(int x = 0; x < parent->layout.row; x++){
					int linear_index = y * parent->layout.col + x;
					if(linear_index >= parent->child_count) return;
					
					parent->children[linear_index]->position = 
						(tu_position){
							x * (parent->size.ws_col / parent->layout.col),
							y * (parent->size.ws_row / parent->layout.row)
						};
					parent->children[linear_index]->size =
						(struct winsize){
							.ws_row = parent->size.ws_row / parent->layout.row,
							.ws_col = parent->size.ws_col / parent->layout.col
						};
				}
			}
		break;
	}

	for(int i = 0; i < parent->child_count; i++){
		
	}
}

void reshape_based_on_layout(tu_element *parent){

	// 0. If child_count of parent is 0 then just return
	// 1. Reshape on all children of the current parent (based on layout choosen)
	// 2. Interate through each child and give them as next parent into
	// this function

	if(parent->child_count == 0) return;

	apply_layout_on_children(parent);
	
	for(int i = 0; i < parent->child_count; i++){
		reshape_based_on_layout(parent->children[i]);
	}
}

void travel_child_tree(tu_window **window, tu_element *parent){
	for(int i = 0; i < parent->child_count; i++){
		if(parent->children[i]->child_count == 0)
			__element_border(
				window,
				parent,
				//parent->children[i]->parent->position.x + parent->children[i]->position.x,
				//parent->children[i]->parent->position.y + parent->children[i]->position.y,
				parent->children[i]->position.x,
				parent->children[i]->position.y,
				parent->children[i]->size.ws_row,
				parent->children[i]->size.ws_col
			);
		else
			travel_child_tree(window, parent->children[i]);
	}
	if(&(*window)->main_element != parent)
		__element_border(
			window,
			parent->parent,
			//parent->parent->position.x + parent->position.x,
			//parent->parent->position.y + parent->position.y,
			parent->position.x,
			parent->position.y,
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

#pragma once

#include <pax_gfx.h>
#include <pax_shapes.h>
#include <pax_fonts.h>

// A number of inputs supported by the PAX keyboard.
typedef enum {
	// Represents no input being pressed.
	PKB_NO_INPUT,
	// Movement of the cursor.
	PKB_UP, PKB_DOWN, PKB_LEFT, PKB_RIGHT,
	// Delete to the left or the selection. Backspace key.
	PKB_DELETE_BEFORE,
	// Delete to the right or the selection. Delete key.
	PKB_DELETE_AFTER,
	// Switch between lower case, upper case and symbols.
	PKB_MODESELECT,
	// Enter a character.
	PKB_CHARSELECT,  
	// The same thing as the shift key.
	// Goes between PKB_LOWERCASE and PKB_UPPERCASE or PKB_NUMBERS and PKB_SYMBOLS.
	PKB_SHIFT,
	PKB_ENTER,
	PKB_ACCEPT,
	PKB_SUPER,
	PKB_FN,
	PKB_Q,
	PKB_W,
	PKB_E,
	PKB_R,
	PKB_T,
	PKB_Y,
	PKB_U,
	PKB_I,
	PKB_O,
	PKB_P,
	PKB_A,
	PKB_S,
	PKB_D,
	PKB_F,
	PKB_G,
	PKB_H,
	PKB_J,
	PKB_K,
	PKB_L,
	PKB_Z,
	PKB_X,
	PKB_C,
	PKB_V,
	PKB_B,
	PKB_N,
	PKB_M,
} pkb_input_t;

// The type of keyboard currently selected.
typedef enum {
	// Lowercase and .,
	PKB_LOWERCASE,
	// Uppercase and <>
	PBK_UPPERCASE,
	// Numbers and symbols 1/2
	PKB_NUMBERS,
	// Symbols 2/2
	PKB_SYMBOLS,
} pkb_keyboard_t;

// The PAX keyboard context used for drawing and alike.
typedef struct {
	// Position on screen of the keyboard.
	int            x, y;
	// Maximum size of the keyboard.
	int            width, height;
	
	// Content of the keyboard.
	char          *content;
	// Size in bytes of capacity of the content buffer.
	size_t         content_cap;
	
	// Starting position of the selection in the text box.
	int            selection;
	// Cursor position of the text box.
	int            cursor;
	
	// Cursor position of the keyboard.
	int            key_x, key_y;
	// The currently held input.
	pkb_input_t    held;
	// The time that holding the input started.
	int64_t        hold_start;
	// The last time pkb_press was called.
	int64_t        last_press;
	
	// Whether the keyboard is multi-line.
	bool           multiline;
	// Whether the keyboard is in insert mode.
	bool           insert;
	// The board that is currently selected.
	pkb_keyboard_t board_sel;
	
	// The font to use for the keyboard.
	const pax_font_t *kb_font;
	// The font size to use for the keyboard.
	float          kb_font_size;
	// The font to use for the text.
	const pax_font_t *text_font;
	// The font size to use for the text.
	float          text_font_size;
	// The text color to use.
	pax_col_t      text_col;
	// The text color to use when a character is being held down.
	pax_col_t      sel_text_col;
	// The selection color to use.
	pax_col_t      sel_col;
	// The background color to use.
	pax_col_t      bg_col;
	
	// Whether something has changed since last draw.
	bool           dirty;
	// Whether the text has changed since last draw.
	bool           text_dirty;
	// Whether the keyboard has changed since last draw.
	bool           kb_dirty;
	// Whether just the selected character has changed since last draw.
	bool           sel_dirty;
	// Previous cursor position of the keyboard.
	// Used for sel_dirty.
	int            last_key_x, last_key_y;
	
	// Indicates that the input has been accepted.
	bool           input_accepted;
} pkb_ctx_t;

// Initialise the context with default settings.
void pkb_init   (pax_buf_t *buf, pkb_ctx_t *ctx, size_t buffer_cap);
// Free any memory associated with the context.
void pkb_destroy(pkb_ctx_t *ctx);
// Replaces the text in the keyboard with the given text.
// Makes a copy of the given text.
void pkb_set_content(pkb_ctx_t *ctx, const char *content);

// Redraw the complete on-screen keyboard.
void pkb_render (pax_buf_t *buf, pkb_ctx_t *ctx);
// Redraw only the changed parts of the on-screen keyboard.
void pkb_redraw (pax_buf_t *buf, pkb_ctx_t *ctx);

// The loop that allows input repeating.
void pkb_loop   (pkb_ctx_t *ctx);

// A pressing of the input.
void pkb_press  (pkb_ctx_t *ctx, pkb_input_t input);
// A relealing of the input.
void pkb_release(pkb_ctx_t *ctx, pkb_input_t input);

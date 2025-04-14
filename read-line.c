#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#define MAX_BUFFER_LINE 2048
#define MAX_HISTORY 100  // Maximum number of history entries

extern void tty_raw_mode(void);

// Editor state
int line_length;
int cursor_position;
char line_buffer[MAX_BUFFER_LINE];

// Dynamic history
static char *history[MAX_HISTORY];
static int history_count = 0;     // total commands in history
static int history_index = 0;     // which command in history is currently displayed

// Print usage info
void read_line_print_usage()
{
  const char * usage = "\n"
    " ctrl-?       Print usage\n"
    " Backspace    Deletes char before cursor\n"
    " up arrow     Previous command in history\n"
    " down arrow   Next command in history\n"
    " left arrow   Move cursor left\n"
    " right arrow  Move cursor right\n"
    " ctrl-A       Move cursor to start (Home)\n"
    " ctrl-E       Move cursor to end\n";
  write(1, usage, strlen(usage));
}

/**
 * Add the command to history if it's not empty.
 */
void add_to_history(const char *cmd) {
  if (!cmd || cmd[0] == '\0') {
    return; // ignore empty lines
  }

  // Duplicate the string
  char *dup = strdup(cmd);
  if (!dup) return; // out of memory?

  // If we have room, just add
  if (history_count < MAX_HISTORY) {
    history[history_count++] = dup;
  } else {
    // Full: remove the oldest
    free(history[0]);
    // Shift everything left
    for (int i = 0; i < MAX_HISTORY - 1; i++) {
      history[i] = history[i+1];
    }
    // Put new command at end
    history[MAX_HISTORY - 1] = dup;
  }
  // Reset history_index to "after last"
  history_index = history_count;
}

/**
 * Clear the entire current line from the terminal
 * and move cursor to start (carriage return).
 */
void clear_line_on_screen() {
  // Move to start
  char cr = 13; // carriage return
  write(1, &cr, 1);

  // Overwrite the line + extra space
  for (int i = 0; i < line_length + 1; i++) {
    write(1, " ", 1);
  }
  // Move back again
  write(1, &cr, 1);
}

/**
 * Redraw the line from scratch with the cursor at the correct position.
 */
void redraw_line() {
  clear_line_on_screen();
  write(1, line_buffer, line_length);

  // Move cursor back from end to cursor_position
  int dist = line_length - cursor_position;
  while (dist > 0) {
    char bs = 8;  // backspace
    write(1, &bs, 1);
    dist--;
  }
}

/**
 * Move the cursor left by 1 (if possible).
 */
void move_cursor_left() {
  if (cursor_position > 0) {
    cursor_position--;
    char bs = 8;
    write(1, &bs, 1);  // physically move left
  }
}

/**
 * Move the cursor right by 1 (if possible).
 */
void move_cursor_right() {
  if (cursor_position < line_length) {
    // "print" the character at cursor to move visually
    char c = line_buffer[cursor_position];
    write(1, &c, 1);
    cursor_position++;
  }
}

/**
 * Insert a character c at the cursor, shifting the rest right.
 */
void insert_char(char c) {
  if (line_length >= MAX_BUFFER_LINE - 2) {
    // buffer full
    return;
  }
  // shift right
  memmove(&line_buffer[cursor_position + 1],
          &line_buffer[cursor_position],
          line_length - cursor_position);

  // place c
  line_buffer[cursor_position] = c;
  line_length++;
  cursor_position++;

  redraw_line();
}

/**
 * Backspace: remove char before cursor (if any).
 */
void backspace_char() {
  if (cursor_position > 0) {
    // shift everything left from cursor_position-1
    memmove(&line_buffer[cursor_position - 1],
            &line_buffer[cursor_position],
            line_length - (cursor_position - 1));
    cursor_position--;
    line_length--;
    redraw_line();
  }
}

/**
 * Delete at cursor (ctrl-D).
 */
void delete_char_at_cursor() {
  if (cursor_position < line_length) {
    // shift left
    memmove(&line_buffer[cursor_position],
            &line_buffer[cursor_position + 1],
            line_length - cursor_position - 1);
    line_length--;
    redraw_line();
  }
}

/**
 * Get a command from history at index, or blank if out of range.
 * Put into line_buffer, set line_length/cursor_position, then redraw.
 */
void get_history_line(int idx) {
  if (idx < 0) idx = 0;
  if (idx >= history_count) {
    // beyond newest => blank line
    line_buffer[0] = '\0';
    line_length = 0;
    cursor_position = 0;
  } else {
    strcpy(line_buffer, history[idx]);
    line_length = strlen(line_buffer);
    cursor_position = line_length;
  }
  redraw_line();
}

/**
 * read_line: read an entire line with basic editing and history.
 */
char *read_line() {
  // set raw mode
  tty_raw_mode();

  line_length = 0;
  cursor_position = 0;
  memset(line_buffer, 0, MAX_BUFFER_LINE);

  // Start from "blank" after last command
  history_index = history_count;

  while (1) {
    char ch;
    int n = read(0, &ch, 1);
    if (n <= 0) {
      // read error or EOF
      break;
    }

    // normal printable
    if (ch >= 32 && ch < 127) {
      insert_char(ch);
    }
    else if (ch == 10) {
      // ENTER
      write(1, &ch, 1);
      // Null-terminate
      line_buffer[line_length] = 0;
      // Add to history if not empty
      if (line_length > 0) {
        add_to_history(line_buffer);
      }
      break;
    }
    else if (ch == 31) {
      // ctrl-?
      read_line_print_usage();
      line_buffer[0] = 0;
      line_length = 0;
      cursor_position = 0;
      redraw_line();
    }
    else if (ch == 8 || ch == 127) {
      // backspace
      backspace_char();
    }
    else if (ch == 4) {
      // ctrl-D => delete at cursor
      delete_char_at_cursor();
    }
    else if (ch == 1) {
      // ctrl-A => move to start
      while (cursor_position > 0) {
        move_cursor_left();
      }
    }
    else if (ch == 5) {
      // ctrl-E => move to end
      while (cursor_position < line_length) {
        move_cursor_right();
      }
    }
    else if (ch == 27) {
      // ESC sequence
      char ch1, ch2;
      if (read(0, &ch1, 1) == 0) break;
      if (read(0, &ch2, 1) == 0) break;

      // Up arrow
      if (ch1 == 91 && ch2 == 65) {
        // previous command
        if (history_index > 0) {
          history_index--;
          clear_line_on_screen();
          get_history_line(history_index);
        }
      }
      // Down arrow
      else if (ch1 == 91 && ch2 == 66) {
        // next command
        if (history_index < history_count) {
          history_index++;
          clear_line_on_screen();
          get_history_line(history_index);
        } else {
          // user may want blank line if at newest
          history_index = history_count; 
          clear_line_on_screen();
          line_buffer[0] = '\0';
          line_length = 0;
          cursor_position = 0;
        }
      }
      // Left arrow
      else if (ch1 == 91 && ch2 == 68) {
        move_cursor_left();
      }
      // Right arrow
      else if (ch1 == 91 && ch2 == 67) {
        move_cursor_right();
      }
    }
  }

  // Add newline, and null terminate
  line_buffer[line_length] = 10; 
  line_length++;
  line_buffer[line_length] = 0;
  return line_buffer;
}

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#define MAX_BUFFER_LINE 2048

extern void tty_raw_mode(void);

// Buffer and cursor
int line_length;
int cursor_position; 
char line_buffer[MAX_BUFFER_LINE];

// A simple, static history
int history_index = 0;
char * history [] = {
  "ls -al | grep x", 
  "ps -e",
  "cat read-line-example.c",
  "vi hello.c",
  "make",
  "ls -al | grep xxx | grep yyy"
};
int history_length = sizeof(history)/sizeof(char *);

void read_line_print_usage()
{
  char * usage = "\n"
    " ctrl-?       Print usage\n"
    " Backspace    Deletes last character before cursor\n"
    " up arrow     See last command in the history\n"
    " left arrow   Move cursor left\n"
    " right arrow  Move cursor right\n";

  write(1, usage, strlen(usage));
}

/**
 * Redraw the entire line on screen from scratch, 
 * placing the cursor where `cursor_position` indicates.
 */
void refreshLine() {
  // Move cursor all the way back to the start
  // by sending backspaces = cursor_position times
  int i;
  for (i = 0; i < cursor_position; i++) {
    char ch = 8; // ASCII backspace
    write(1, &ch, 1);
  }
  
  // Now overwrite the entire line with spaces
  for (i = 0; i < line_length; i++) {
    char ch = ' ';
    write(1, &ch, 1);
  }

  // Move back again
  for (i = 0; i < line_length; i++) {
    char ch = 8;
    write(1, &ch, 1);
  }

  // Print the updated buffer
  write(1, line_buffer, line_length);

  // Finally, move the cursor from end to the current position
  int dist = line_length - cursor_position;
  while (dist > 0) {
    char ch = 8;
    write(1, &ch, 1);
    dist--;
  }
}

/**
 * Insert a character into line_buffer at `cursor_position`,
 * shifting the rest of the line to the right.
 */
void insertChar(char c) {
  if (line_length < MAX_BUFFER_LINE - 2) {
    // shift everything to the right from cursor_position
    int i;
    for (i = line_length; i >= cursor_position; i--) {
      line_buffer[i+1] = line_buffer[i];
    }
    // place the new char
    line_buffer[cursor_position] = c;
    line_length++;
    cursor_position++;
    refreshLine();
  }
}

/**
 * Backspace: remove the char before the cursor, shift left
 */
void backspaceChar() {
  if (cursor_position > 0) {
    // shift everything left from cursor_position-1
    int i;
    for (i = cursor_position-1; i < line_length; i++) {
      line_buffer[i] = line_buffer[i+1];
    }
    cursor_position--;
    line_length--;
    refreshLine();
  }
}

/* 
 * read_line: Input a line with some basic editing.
 */
char * read_line() {
  // Set terminal in raw mode
  tty_raw_mode();

  line_length = 0;
  cursor_position = 0;
  memset(line_buffer, 0, MAX_BUFFER_LINE);

  while (1) {
    // Read one character
    char ch;
    int n = read(0, &ch, 1);
    if (n <= 0) {
      // read error or EOF
      break;
    }

    // Printable?
    if (ch >= 32 && ch < 127) {
      // Insert at cursor
      insertChar(ch);
    }
    else if (ch == 10) {
      // <Enter>
      // Print newline
      write(1, &ch, 1);
      break;
    }
    else if (ch == 31) {
      // ctrl-?
      read_line_print_usage();
      line_buffer[0] = 0;
      line_length = 0;
      cursor_position = 0;
      break;
    }
    else if (ch == 8 || ch == 127) {
      // Backspace
      backspaceChar();
    }
    else if (ch == 27) {
      // Escape sequence
      char ch1, ch2;
      read(0, &ch1, 1);
      read(0, &ch2, 1);

      // Left arrow: ESC [ D => 27,91,68
      if (ch1 == 91 && ch2 == 68) {
        if (cursor_position > 0) {
          cursor_position--;
          refreshLine();
        }
      }
      // Right arrow: ESC [ C => 27,91,67
      else if (ch1 == 91 && ch2 == 67) {
        if (cursor_position < line_length) {
          cursor_position++;
          refreshLine();
        }
      }
      // Up arrow: ESC [ A => 27,91,65
      else if (ch1==91 && ch2==65) {
        // Erase old line visually
        for (int i = 0; i < cursor_position; i++) {
          char bs = 8;
          write(1, &bs, 1);
        }
        for (int i = 0; i < line_length; i++) {
          write(1, " ", 1);
        }
        for (int i = 0; i < line_length; i++) {
          char bs = 8;
          write(1, &bs, 1);
        }	

        // Copy line from history
        strcpy(line_buffer, history[history_index]);
        line_length = strlen(line_buffer);
        history_index = (history_index + 1) % history_length;
        cursor_position = line_length;

        // echo line
        write(1, line_buffer, line_length);
      }
    }
  }

  // Add newline + null
  line_buffer[line_length] = 10;
  line_length++;
  line_buffer[line_length] = 0;

  return line_buffer;
}

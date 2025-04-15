#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define MAX_BUFFER_LINE 2048
#define MAX_HISTORY     50

extern void tty_raw_mode(void);

// Buffer where line is stored
int  line_length;
char line_buffer[MAX_BUFFER_LINE];
int  cursor_position; 

// --- DYNAMIC HISTORY TABLE ---

// We'll store pointers to command strings here:
char *history[MAX_HISTORY];
int   history_length   = 0;  // Number of actual commands stored
int   history_index    = 0;  // Current position when navigating history

// Add a command to the history
// (We copy the command string so we don't lose it later.)
void add_history(const char *command) {
    if (command == NULL || command[0] == '\0') {
        // Don't store empty commands
        return;
    }

    // If we still have space
    if (history_length < MAX_HISTORY) {
        history[history_length++] = strdup(command);
    } else {
        // If history is full, drop the oldest entry
        free(history[0]);
        // Shift everything left
        memmove(&history[0], &history[1], sizeof(char*) * (MAX_HISTORY - 1));
        // Put the new command at the end
        history[MAX_HISTORY - 1] = strdup(command);
        // history_length stays at MAX_HISTORY
    }

    // Whenever we add something new, set the navigation index to the 'end'
    // (meaning one past the last stored command).
    history_index = history_length;
}

// Print usage message
void read_line_print_usage()
{
  const char * usage = 
    "\n"
    " ctrl-?       Print usage\n"
    " Backspace    Deletes last character\n"
    " up arrow     See previous command in the history\n"
    " down arrow   See next command in the history\n";

  write(1, usage, strlen(usage));
}

// A helper to refresh line display after insertions/deletions in the middle
void refresh_display(int pos, int len) {
  int i;
  char ch;
  
  // Save the current cursor position
  write(1, "\033[s", 3);
  
  // Write the portion of the buffer from cursor to end
  if (pos < len) {
    write(1, &line_buffer[pos], len - pos);
  }
  
  // Write a space at the end (to erase any potential leftover character)
  ch = ' ';
  write(1, &ch, 1);
  
  // Restore cursor position
  write(1, "\033[u", 3);
}

/* 
 * Read a line with some basic editing.
 */
char * read_line() {

  // Set terminal in raw mode
  tty_raw_mode();

  line_length = 0;
  cursor_position = 0;

  memset(line_buffer, 0, MAX_BUFFER_LINE);

  while (1) {

    // Read one character in raw mode.
    char ch;
    read(0, &ch, 1);

    if (ch >= 32 && ch != 127) {
      // It is a printable character (except DEL=127).
      
      if (line_length == MAX_BUFFER_LINE-2) {
        // No space left in the buffer; ignore the character.
        continue;
      }
      
      // If cursor is at the end of line, just add char
      if (cursor_position == line_length) {
        // Echo the character
        write(1, &ch, 1);

        // Add char to buffer
        line_buffer[cursor_position] = ch;
        cursor_position++;
        line_length++;
      } 
      else {
        // Inserting in the middle of the line
        // Shift existing text to the right
        memmove(&line_buffer[cursor_position + 1], 
                &line_buffer[cursor_position], 
                line_length - cursor_position);

        // Place the new character
        line_buffer[cursor_position] = ch;
        cursor_position++;
        line_length++;

        // Echo the new character
        write(1, &ch, 1);
        // Refresh display for everything after the cursor
        refresh_display(cursor_position, line_length);
      }
    }
    else if (ch == 10) {
      // <Enter> was typed. Return the line
      
      // Print newline
      write(1, &ch, 1);

      // At this point, line_buffer has the user command
      // but it ends with '\0'. We store it in the history
      // without the trailing newline.
      //  - The code currently appends newline after user typed <Enter>,
      //    so let's remove it from the stored version.
      line_buffer[line_length] = '\0';
      add_history(line_buffer);

      // Now add the trailing newline in line_buffer so the caller
      // sees a consistent string ending in '\n\0'
      line_buffer[line_length] = 10;  // newline
      line_length++;
      line_buffer[line_length] = 0;

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
    else if ((ch == 8) || (ch == 127)) {
      // Backspace or DEL
      if (cursor_position > 0) {
        // Shift everything left by one
        memmove(&line_buffer[cursor_position - 1],
                &line_buffer[cursor_position],
                line_length - cursor_position);

        cursor_position--;
        line_length--;

        // Move cursor back one position on screen
        char bs = 8;
        write(1, &bs, 1);

        // Refresh what's after the cursor
        refresh_display(cursor_position, line_length);
      }
    }
    else if (ch == 4) {
      // ctrl-D (forward delete)
      if (cursor_position < line_length) {
        // Shift everything left from cursor
        memmove(&line_buffer[cursor_position],
                &line_buffer[cursor_position + 1],
                line_length - cursor_position - 1);
        line_length--;

        // Refresh what's after the cursor
        refresh_display(cursor_position, line_length);
      }
    }
    else if (ch == 1) {
      // ctrl-A (Home key)
      while (cursor_position > 0) {
        // Move cursor back
        char bs = 8;
        write(1, &bs, 1);
        cursor_position--;
      }
    }
    else if (ch == 5) {
      // ctrl-E (End key)
      while (cursor_position < line_length) {
        char forward_char = line_buffer[cursor_position];
        write(1, &forward_char, 1);
        cursor_position++;
      }
    }
    else if (ch == 27) {
      // Escape sequence. Read two more chars.
      char ch1, ch2;
      read(0, &ch1, 1);
      read(0, &ch2, 1);

      // Left arrow: ESC [ D
      if (ch1 == 91 && ch2 == 68) {
        if (cursor_position > 0) {
          // Move cursor left
          char bs = 8;
          write(1, &bs, 1);
          cursor_position--;
        }
      }
      // Right arrow: ESC [ C
      else if (ch1 == 91 && ch2 == 67) {
        if (cursor_position < line_length) {
          // Move cursor right
          char forward_char = line_buffer[cursor_position];
          write(1, &forward_char, 1);
          cursor_position++;
        }
      }
      // Up arrow: ESC [ A
      else if (ch1 == 91 && ch2 == 65) {
        // Show previous command in history
        if (history_length > 0) {
          // If we're not already at the oldest command
          if (history_index > 0) {
            history_index--;
          }
          
          // Erase the old line
          char cr = 13;
          write(1, &cr, 1);
          // Write spaces over the old content
          int i;
          for (i = 0; i < line_length + 1; i++) {
            write(1, " ", 1);
          }
          // Move back to line start
          write(1, &cr, 1);

          // Copy the history line into our buffer
          strcpy(line_buffer, history[history_index]);
          line_length = strlen(line_buffer);
          cursor_position = line_length;

          // Echo line
          write(1, line_buffer, line_length);
        }
      }
      // Down arrow: ESC [ B
      else if (ch1 == 91 && ch2 == 66) {
        // Show next command in history (or empty if at end)
        if (history_length > 0) {
          // If we're not already on the "newest + 1" position
          if (history_index < history_length) {
            history_index++;
          }
          // Erase the old line
          char cr = 13;
          write(1, &cr, 1);
          int i;
          for (i = 0; i < line_length + 1; i++) {
            write(1, " ", 1);
          }
          write(1, &cr, 1);

          if (history_index == history_length) {
            // If we've gone "below" the last command,
            // just show an empty line
            line_buffer[0] = '\0';
            line_length = 0;
            cursor_position = 0;
          }
          else {
            // Copy the history line
            strcpy(line_buffer, history[history_index]);
            line_length = strlen(line_buffer);
            cursor_position = line_length;
            write(1, line_buffer, line_length);
          }
        }
      }
    }
    // Ignore all other control characters
  }

  return line_buffer;
}

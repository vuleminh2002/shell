/*
 * CS252: Systems Programming
 * Purdue University
 * Example that shows how to read one line with simple editing
 * using raw terminal.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define MAX_BUFFER_LINE 2048

extern void tty_raw_mode(void);

// Buffer where line is stored
int line_length;
char line_buffer[MAX_BUFFER_LINE];
int cursor_position; 
// Simple history array
// This history does not change. 
// Yours have to be updated.
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
    " Backspace    Deletes last character\n"
    " up arrow     See last command in the history\n";

  write(1, usage, strlen(usage));
}


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
 * Input a line with some basic editing.
 */
char * read_line() {

  // Set terminal in raw mode
  tty_raw_mode();

  line_length = 0;
  cursor_position = 0;
  //clear buffer
  memset(line_buffer, 0, MAX_BUFFER_LINE);

  // Read one line until enter is typed
  while (1) {

    // Read one character in raw mode.
    char ch;
    read(0, &ch, 1);

    if (ch>=32) {
      // It is a printable character.
      
      // If max number of character reached return.
      if (line_length == MAX_BUFFER_LINE-2) continue;
      
      // If cursor is at the end of line, just add char
      if (cursor_position == line_length) {
        // Do echo
        write(1, &ch, 1);
        
        // Add char to buffer
        line_buffer[cursor_position] = ch;
        cursor_position++;
        line_length++;
      } 
      else {
        // Inserting in the middle of the line
        // First, make space by shifting characters
        memmove(&line_buffer[cursor_position + 1], 
                &line_buffer[cursor_position], 
                line_length - cursor_position);
        
        // Insert the character
        line_buffer[cursor_position] = ch;
        cursor_position++;
        line_length++;
        
        // Reprint the line from current position
        write(1, &ch, 1); // Print the character just inserted
        refresh_display(cursor_position, line_length);
      }
    }
    else if (ch==10) {
      // <Enter> was typed. Return line
      
      // Print newline
      write(1,&ch,1);

      break;
    }
    else if (ch == 31) {
      // ctrl-?
      read_line_print_usage();
      line_buffer[0]=0;
      break;
    }
    else if (ch == 8) {
      // <backspace> was typed. Remove previous character read.
      if (cursor_position > 0) {
        // Move content after cursor one position left
        memmove(&line_buffer[cursor_position - 1], 
                &line_buffer[cursor_position], 
                line_length - cursor_position);
        
        cursor_position--;
        line_length--;
        
        // Move cursor back
        ch = 8;
        write(1, &ch, 1);
        
        // Reprint the line from current position
        refresh_display(cursor_position, line_length);
      }
    }
    else if (ch==27) {
      // Escape sequence. Read two chars more
      //
      // HINT: Use the program "keyboard-example" to
      // see the ascii code for the different chars typed.
      //
      char ch1; 
      char ch2;
      read(0, &ch1, 1);
      read(0, &ch2, 1);

      // Left arrow: ESC [ D => 27,91,68
      if (ch1 == 91 && ch2 == 68) {
        if (cursor_position > 0) {
          // Move cursor back
          ch = 8;
          write(1, &ch, 1);
          cursor_position--;
        }
      }
     
      else if (ch1 == 91 && ch2 == 67) {
        if (cursor_position < line_length) {
          // Move cursor forward
          ch = line_buffer[cursor_position];
          write(1, &ch, 1);
          cursor_position++;
        }
      }
      if (ch1==91 && ch2==65) {
	// Up arrow. Print next line in history.

	// Erase old line
	// Print backspaces
	int i = 0;
	for (i =0; i < line_length; i++) {
	  ch = 8;
	  write(1,&ch,1);
	}

	// Print spaces on top
	for (i =0; i < line_length; i++) {
	  ch = ' ';
	  write(1,&ch,1);
	}

	// Print backspaces
	for (i =0; i < line_length; i++) {
	  ch = 8;
	  write(1,&ch,1);
	}	

	// Copy line from history
	strcpy(line_buffer, history[history_index]);
	line_length = strlen(line_buffer);
	history_index=(history_index+1)%history_length;

	// echo line
	write(1, line_buffer, line_length);
      }
      
    }

  }

  // Add eol and null char at the end of string
  line_buffer[line_length]=10;
  line_length++;
  line_buffer[line_length]=0;

  return line_buffer;
}



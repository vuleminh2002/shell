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
 
 #define HISTORY_SIZE 16
 
 char *history[HISTORY_SIZE];
 int history_index = 0;
 int history_index_rev = 0;
 int history_full = 0;
 int history_length = HISTORY_SIZE;
 
 
 extern void tty_raw_mode(void);
 
 // Buffer where the current line is stored
 int line_length;
 char line_buffer[MAX_BUFFER_LINE];
 int cursor_position; 
 
 // Forward declarations
 void read_line_print_usage();
 void refresh_display(int pos, int len);
 
 // Print usage message (unchanged)
 void read_line_print_usage()
 {
   char * usage = "\n"
     " ctrl-?       Print usage\n"
     " Backspace    Deletes last character\n"
     " up arrow     See last command in the history\n"
     " down arrow   See next command in the history\n"
     " ctrl-A       Move cursor to beginning of line\n"
     " ctrl-E       Move cursor to end of line\n";
 
   write(1, usage, strlen(usage));
 }
 
 // A helper to refresh line display after insertions/deletions in the middle
 void refresh_display(int pos, int len) {
   // Save the current cursor position
   write(1, "\033[s", 3);
 
   if (pos < len) {
     write(1, &line_buffer[pos], len - pos);
   }
   char ch = ' ';
   write(1, &ch, 1);
 
   write(1, "\033[u", 3);
 }
 
 
 static void clear_current_line_on_screen(int length)
 {
   char ch;
   // Move cursor to beginning (carriage return)
   ch = 13; 
   write(1, &ch, 1);
   // Print spaces to cover the old line
   for (int i = 0; i < length; i++) {
     ch = ' ';
     write(1, &ch, 1);
   }
   // Move to beginning again
   ch = 13; 
   write(1, &ch, 1);
 }
 
 /*
  * Store the current line_buffer into history (if non-empty).
  * This matches the ring-buffer approach in your snippet.
  */
 static void store_line_in_history()
 {
   if (line_length == 0) {
     return; // don't store empty commands
   }
 
   // If we haven't allocated a slot for this ring position yet, do so
   if (history[history_index] == NULL) {
     history[history_index] = (char *)malloc(MAX_BUFFER_LINE);
   }
 
   // Copy the new command into history
   strcpy(history[history_index], line_buffer);
 
   // Update the "reverse index" to point here
   history_index_rev = history_index;
 
   // Advance history_index (the next new command will go there)
   history_index++;
   if (history_index >= history_length) {
     history_index = 0;
     history_full = 1; 
   }
 }

 static void handle_history_navigation(int direction)
 {
   // If no commands stored yet, do nothing
   int stored_count = history_full ? history_length : history_index;
   if (stored_count == 0) {
     return;
   }
 
   // Clear old line from screen
   clear_current_line_on_screen(line_length);
 
   // Copy the command from history
   strcpy(line_buffer, history[history_index_rev]);
   line_length = strlen(line_buffer);
   cursor_position = line_length;
 
   // Echo it to the screen
   write(1, line_buffer, line_length);
 
   int tmp = stored_count; // # commands actually in the ring

   history_index_rev = (history_index_rev + direction) % tmp;
   // If negative, wrap around
   if (history_index_rev < 0) {
     history_index_rev = tmp - 1;
   }
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
 
   // Read one line until enter is typed
   while (1) {
 
     // Read one character in raw mode.
     char ch;
     read(0, &ch, 1);

     if (ch >= 32 && ch != 127) {
       // If max number of characters reached, ignore
       if (line_length == MAX_BUFFER_LINE-2) {
         continue;
       }
       
       // If cursor is at the end of line, just append
       if (cursor_position == line_length) {
         // Echo the character
         write(1, &ch, 1);
 
         // Add char to buffer
         line_buffer[cursor_position] = ch;
         cursor_position++;
         line_length++;
       } 
       else {

         memmove(&line_buffer[cursor_position + 1], 
                 &line_buffer[cursor_position], 
                 line_length - cursor_position);
         // Insert the new character
         line_buffer[cursor_position] = ch;
         cursor_position++;
         line_length++;
 
         // Echo the new character
         write(1, &ch, 1);
 
         // Refresh what's after the cursor
         refresh_display(cursor_position, line_length);
       }
     }

     else if (ch == 10) {
       // Print newline
       write(1, &ch, 1);
 
       // Store this command in history (if non-empty)
       line_buffer[line_length] = '\0';
       store_line_in_history();
 
       // Add the newline + null so the caller sees it
       line_buffer[line_length] = 10;
       line_length++;
       line_buffer[line_length] = 0;
 
       break;
     }

     else if (ch == 31) {
       // Print usage
       read_line_print_usage();
       // Clear line buffer
       line_length = 0;
       cursor_position = 0;
       line_buffer[0] = '\0';
       break;
     }
      //ctrl h
     else if (ch == 8 ) {
       if (cursor_position > 0) {
         // Shift text left by one
         memmove(&line_buffer[cursor_position - 1],
                 &line_buffer[cursor_position],
                 line_length - cursor_position);
         cursor_position--;
         line_length--;
         
         // Move cursor back on screen
         char bs = 8;
         write(1, &bs, 1);
 
         // Refresh the rest
         refresh_display(cursor_position, line_length);
       }
     }
     //ctrl d
     else if (ch == 4) {
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
       // Move cursor all the way to the left
       while (cursor_position > 0) {
         char bs = 8;
         write(1, &bs, 1);
         cursor_position--;
       }
     }

     else if (ch == 5) {
       // Move cursor to end of line
       while (cursor_position < line_length) {
         char forward_char = line_buffer[cursor_position];
         write(1, &forward_char, 1);
         cursor_position++;
       }
     }

     else if (ch == 27) {
       // Read the next two chars
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
 
       else if (ch1 == 91 && ch2 == 65) {
         // direction = -1 (move to older command)
         handle_history_navigation(-1);
       }
       // Down arrow:
       else if (ch1 == 91 && ch2 == 66) {
         // direction = +1 (move to newer command)
         handle_history_navigation(+1);
       }
    
     }
 
   }
 
   return line_buffer;
 }
 
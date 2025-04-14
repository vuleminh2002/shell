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
 #define MAX_HISTORY 100  // Maximum number of history entries
 
 extern void tty_raw_mode(void);
 
 // Buffer where line is stored
 int line_length;
 char line_buffer[MAX_BUFFER_LINE];
 int cursor_position; 
 
 // Dynamic history implementation
 char* history[MAX_HISTORY];      // Array of pointers to history strings
 int history_length = 0;          // Current number of items in history
 int history_index = 0;           // Current position in history when navigating
 int history_position = 0;        // Position in history for adding new items
 
 // Temporary buffer to store the current line when navigating history
 char temp_buffer[MAX_BUFFER_LINE];
 int temp_length = 0;
 int in_history = 0;              // Flag to indicate if we're currently viewing history
 
 // Function to add a command to history
 void add_to_history(const char* line) {
     // Don't add empty lines or duplicates of the last command
     if (line[0] == '\n' || (history_length > 0 && strcmp(line, history[(history_position + MAX_HISTORY - 1) % MAX_HISTORY]) == 0)) {
         return;
     }
 
     // If we've reached max history entries, free the oldest one
     if (history_length == MAX_HISTORY) {
         free(history[history_position]);
     } else {
         history_length++;
     }
     
     // Allocate memory and copy the command (without newline and null terminator)
     int len = strlen(line);
     if (line[len-1] == '\n') len--; // Don't include newline in history
     
     history[history_position] = (char*)malloc(len + 1);
     strncpy(history[history_position], line, len);
     history[history_position][len] = '\0';
     
     // Update position for next history item
     history_position = (history_position + 1) % MAX_HISTORY;
     history_index = history_position; // Reset index for navigation
 }
 
 // Function to refresh the display from cursor to end of line
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
 
 // Function to reprint the entire line and position cursor correctly
 void reprint_line() {
     int i;
     char ch;
     
     // Erase old line - first move to beginning
     ch = 13; // Carriage return
     write(1, &ch, 1);
     
     // Print spaces
     for (i = 0; i < line_length + 1; i++) {
         ch = ' ';
         write(1, &ch, 1);
     }
     
     // Move to beginning again
     ch = 13;
     write(1, &ch, 1);
     
     // Print the current buffer content
     write(1, line_buffer, line_length);
     
     // Move cursor to correct position
     if (cursor_position < line_length) {
         // Move cursor back to the right position
         for (i = 0; i < (line_length - cursor_position); i++) {
             ch = 8; // Backspace
             write(1, &ch, 1);
         }
     }
 }
 
 // Function to clear the current line and display a history entry
 void show_history_entry(int index) {
     char ch;
     int i;
     
     // Erase current line - move to beginning
     ch = 13; // Carriage return
     write(1, &ch, 1);
     
     // Print spaces to erase the line
     for (i = 0; i < line_length + 1; i++) {
         ch = ' ';
         write(1, &ch, 1);
     }
     
     // Move to beginning again
     ch = 13;
     write(1, &ch, 1);
     
     // Get history entry
     char* hist_entry = history[index];
     
     // Copy to line buffer
     strcpy(line_buffer, hist_entry);
     line_length = strlen(line_buffer);
     cursor_position = line_length;
     
     // Echo the history line
     write(1, line_buffer, line_length);
 }
 
 void read_line_print_usage()
 {
     char * usage = "\n"
         " ctrl-?       Print usage\n"
         " Backspace    Deletes last character\n"
         " Up arrow     Show previous command in history\n"
         " Down arrow   Show next command in history\n"
         " Left arrow   Move cursor to the left\n"
         " Right arrow  Move cursor to the right\n"
         " ctrl-D       Delete character at cursor\n"
         " ctrl-H       Backspace (delete character before cursor)\n"
         " ctrl-A       Move cursor to beginning of line\n"
         " ctrl-E       Move cursor to end of line\n";
 
     write(1, usage, strlen(usage));
 }
 
 /* 
  * Input a line with some basic editing.
  */
 char * read_line() {
     // Set terminal in raw mode
     tty_raw_mode();
 
     line_length = 0;
     cursor_position = 0;
     
     // Clear buffer
     memset(line_buffer, 0, MAX_BUFFER_LINE);
     
     // Reset history navigation
     if (history_length > 0) {
         history_index = history_position;
     }
     
     in_history = 0;
     temp_length = 0;
     
     // Read one line until enter is typed
     while (1) {
         // Read one character in raw mode.
         char ch;
         read(0, &ch, 1);
 
         if (ch >= 32) {
             // It is a printable character.
             
             // If max number of character reached return.
             if (line_length == MAX_BUFFER_LINE-2) continue;
             
             // If we were navigating history, we're now making changes
             if (in_history) {
                 in_history = 0;
             }
             
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
         else if (ch == 10) {
             // <Enter> was typed. Return line
             
             // Print newline
             write(1, &ch, 1);
             
             // If line isn't empty, add to history
             if (line_length > 0) {
                 // Add null termination temporarily for add_to_history
                 line_buffer[line_length] = '\0';
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
             break;
         }
         else if (ch == 8 || ch == 127) {
             // <backspace> was typed (ctrl-H is 8, backspace key often sends 127)
             // Remove character before cursor
             
             if (cursor_position > 0) {
                 // If we were navigating history, we're now making changes
                 if (in_history) {
                     in_history = 0;
                 }
                 
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
         else if (ch == 4) {
             // ctrl-D (Delete key)
             if (cursor_position < line_length) {
                 // If we were navigating history, we're now making changes
                 if (in_history) {
                     in_history = 0;
                 }
                 
                 // Move content after cursor one position left
                 memmove(&line_buffer[cursor_position], 
                         &line_buffer[cursor_position + 1], 
                         line_length - cursor_position - 1);
                 
                 line_length--;
                 
                 // Reprint the line from current position
                 refresh_display(cursor_position, line_length);
             }
         }
         else if (ch == 1) {
             // ctrl-A (Home key)
             while (cursor_position > 0) {
                 // Move cursor back
                 ch = 8;
                 write(1, &ch, 1);
                 cursor_position--;
             }
         }
         else if (ch == 5) {
             // ctrl-E (End key)
             // Move cursor to end of line
             while (cursor_position < line_length) {
                 // Output character at current position
                 ch = line_buffer[cursor_position];
                 write(1, &ch, 1);
                 cursor_position++;
             }
         }
         else if (ch == 27) {
             // Escape sequence. Read two chars more
             char ch1; 
             char ch2;
             read(0, &ch1, 1);
             read(0, &ch2, 1);
             
             if (ch1 == 91) {
                 if (ch2 == 65) {
                     // Up arrow - show previous command in history
                     if (history_length == 0) {
                         continue; // No history available
                     }
                     
                     // Save current line in temp buffer if we're not already in history mode
                     if (!in_history) {
                         memcpy(temp_buffer, line_buffer, line_length);
                         temp_buffer[line_length] = '\0';
                         temp_length = line_length;
                         in_history = 1;
                     }
                     
                     // Calculate previous history index
                     int prev_index = (history_index + MAX_HISTORY - 1) % MAX_HISTORY;
                     
                     // Only navigate if there's history and we're not at the oldest entry
                     if (history_length > 0 && (prev_index != ((history_position + MAX_HISTORY - history_length) % MAX_HISTORY) || 
                                                history_index == history_position)) {
                         // Update history index
                         history_index = prev_index;
                         
                         // Show the history entry
                         show_history_entry(history_index);
                     }
                 }
                 else if (ch2 == 66) {
                     // Down arrow - show next command in history
                     if (!in_history) {
                         continue; // Not navigating history
                     }
                     
                     // Calculate next history index
                     int next_index = (history_index + 1) % MAX_HISTORY;
                     
                     // Check if we've reached current position in history
                     if (next_index == history_position) {
                         // Restore original line
                         // Erase current line
                         ch = 13; // Carriage return
                         write(1, &ch, 1);
                         
                         // Print spaces to erase the line
                         int i;
                         for (i = 0; i < line_length + 1; i++) {
                             ch = ' ';
                             write(1, &ch, 1);
                         }
                         
                         // Move to beginning again
                         ch = 13;
                         write(1, &ch, 1);
                         
                         // Restore temp buffer
                         memcpy(line_buffer, temp_buffer, temp_length);
                         line_length = temp_length;
                         cursor_position = line_length;
                         
                         // Echo line
                         write(1, line_buffer, line_length);
                         
                         in_history = 0;
                     } else {
                         // Show next history entry
                         history_index = next_index;
                         show_history_entry(history_index);
                     }
                 }
                 else if (ch2 == 68) {
                     // Left arrow
                     if (cursor_position > 0) {
                         // Move cursor back
                         ch = 8;
                         write(1, &ch, 1);
                         cursor_position--;
                     }
                 }
                 else if (ch2 == 67) {
                     // Right arrow
                     if (cursor_position < line_length) {
                         // Move cursor forward
                         ch = line_buffer[cursor_position];
                         write(1, &ch, 1);
                         cursor_position++;
                     }
                 }
                 else if (ch2 == 72) {
                     // Home key (some terminals)
                     while (cursor_position > 0) {
                         // Move cursor back
                         ch = 8;
                         write(1, &ch, 1);
                         cursor_position--;
                     }
                 }
                 else if (ch2 == 70) {
                     // End key (some terminals)
                     while (cursor_position < line_length) {
                         // Output character at current position
                         ch = line_buffer[cursor_position];
                         write(1, &ch, 1);
                         cursor_position++;
                     }
                 }
             }
         }
     }
 
     // Add eol and null char at the end of string
     line_buffer[line_length] = 10;
     line_length++;
     line_buffer[line_length] = 0;
 
     return line_buffer;
 }
 
 // Function to free history when program exits
 void free_history() {
     int i;
     for (i = 0; i < history_length; i++) {
         int index = (history_position - history_length + i + MAX_HISTORY) % MAX_HISTORY;
         free(history[index]);
     }
 }
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
 char *history[MAX_HISTORY];  // Array of history strings
 int history_length = 0;      // Current number of entries in history
 int history_index = 0;       // Current position in history when navigating
 int history_position = 0;    // Position to add new commands (end of history)
 int temp_history_index = 0;  // Used for temporary position when navigating
 
 // Add command to history
 void add_to_history(char *line) {
     // Don't add empty lines or duplicates of last command
     if (line_length <= 1 || (history_length > 0 && 
         strcmp(line, history[(history_position + MAX_HISTORY - 1) % MAX_HISTORY]) == 0)) {
         return;
     }
     
     // Allocate space and copy the command
     char *entry = strdup(line);
     
     // Remove newline character
     int len = strlen(entry);
     if (len > 0 && entry[len-1] == '\n') {
         entry[len-1] = '\0';
     }
     
     // Free old entry if we're replacing one
     if (history_length == MAX_HISTORY && history[history_position] != NULL) {
         free(history[history_position]);
     }
     
     // Add to history
     history[history_position] = entry;
     
     // Update history position
     history_position = (history_position + 1) % MAX_HISTORY;
     
     // Update history length
     if (history_length < MAX_HISTORY) {
         history_length++;
     }
     
     // Reset history navigation index
     history_index = history_position;
     temp_history_index = history_index;
 }
 
 void read_line_print_usage()
 {
     char * usage = "\n"
         " ctrl-?       Print usage\n"
         " Backspace    Deletes last character\n"
         " up arrow     See previous command in the history\n"
         " down arrow   See next command in the history\n";
 
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
 
 // Clear the current line
 void clear_line(int length) {
     char ch;
     
     // Move to beginning
     ch = 13; // Carriage return
     write(1, &ch, 1);
     
     // Print spaces
     for (int i = 0; i < length + 1; i++) {
         ch = ' ';
         write(1, &ch, 1);
     }
     
     // Move to beginning again
     ch = 13;
     write(1, &ch, 1);
 }
 
 // Display a command from history
 void display_history_command(int index) {
     if (history_length == 0) {
         return;
     }
     
     // Clear current line
     clear_line(line_length);
     
     // Get command from history
     char *cmd = history[index];
     
     // Copy to line buffer
     strcpy(line_buffer, cmd);
     line_length = strlen(line_buffer);
     cursor_position = line_length;
     
     // Display the command
     write(1, line_buffer, line_length);
 }
 
 /* 
  * Input a line with some basic editing.
  */
 char * read_line() {
     // Set terminal in raw mode
     tty_raw_mode();
 
     line_length = 0;
     cursor_position = 0;
     temp_history_index = history_index;
 
     memset(line_buffer, 0, MAX_BUFFER_LINE);
     
     // Read one line until enter is typed
     while (1) {
         // Read one character in raw mode.
         char ch;
         read(0, &ch, 1);
 
         if (ch >= 32) {
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
             write(1, &ch, 1);
             
             // Add command to history before returning
             line_buffer[line_length] = '\0';  // Temporarily null-terminate
             add_to_history(line_buffer);
             line_buffer[line_length] = ch;    // Restore newline
 
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
         else if (ch == 4) {
             // ctrl-D (Delete key)
             if (cursor_position < line_length) {
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
         else if (ch==27) {
             // Escape sequence. Read two chars more
             char ch1; 
             char ch2;
             read(0, &ch1, 1);
             read(0, &ch2, 1);
 
             if (ch1 == 91 && ch2 == 68) {
                 // Left arrow
                 if (cursor_position > 0) {
                     // Move cursor back
                     ch = 8;
                     write(1, &ch, 1);
                     cursor_position--;
                 }
             }
             else if (ch1 == 91 && ch2 == 67) {
                 // Right arrow
                 if (cursor_position < line_length) {
                     // Move cursor forward
                     ch = line_buffer[cursor_position];
                     write(1, &ch, 1);
                     cursor_position++;
                 }
             }
             else if (ch1 == 91 && ch2 == 65) {
                 // Up arrow. Show previous command in history.
                 if (history_length > 0) {
                     // Calculate previous entry in history
                     temp_history_index = (temp_history_index + MAX_HISTORY - 1) % MAX_HISTORY;
                     
                     // Don't go past beginning of history
                     if (temp_history_index == history_position && history_length < MAX_HISTORY) {
                         temp_history_index = (temp_history_index + 1) % MAX_HISTORY;
                         continue;
                     }
                     
                     // Display the command
                     display_history_command(temp_history_index);
                 }
             }
             else if (ch1 == 91 && ch2 == 66) {
                 // Down arrow. Show next command in history.
                 if (history_length > 0) {
                     // If we're already at the end of history, clear line
                     if (temp_history_index == history_position) {
                         clear_line(line_length);
                         line_length = 0;
                         cursor_position = 0;
                         line_buffer[0] = '\0';
                         continue;
                     }
                     
                     // Calculate next entry in history
                     temp_history_index = (temp_history_index + 1) % MAX_HISTORY;
                     
                     // Display the command
                     display_history_command(temp_history_index);
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
 
 // Initialize history - call this before using read_line
 void init_history() {
     for (int i = 0; i < MAX_HISTORY; i++) {
         history[i] = NULL;
     }
     history_length = 0;
     history_position = 0;
     history_index = 0;
 }
 
 // Free allocated history memory - call this before exiting
 void free_history() {
     for (int i = 0; i < MAX_HISTORY; i++) {
         if (history[i] != NULL) {
             free(history[i]);
             history[i] = NULL;
         }
     }
 }
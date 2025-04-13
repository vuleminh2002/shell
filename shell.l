/*
 *
 * CS-252 
 * shell.l: lexical analyzer for shell
 * You have to extend it.
 *
 */

%{
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <stack>
#include "y.tab.hh"
#include <unistd.h>      
#include <sys/types.h>
#include <sys/wait.h> 
extern int yyparse();
#include "shell.hh"  
extern FILE *yyin;

typedef struct yy_buffer_state *YY_BUFFER_STATE;
extern YY_BUFFER_STATE yy_create_buffer(FILE *, int);
extern void yy_switch_to_buffer(YY_BUFFER_STATE);
extern void yy_delete_buffer(YY_BUFFER_STATE);

std::stack<YY_BUFFER_STATE> bufferStack;
std::string expandAllEnv(const std::string &input); 
static  void yyunput (int c,char *buf_ptr  );

void myunputc(int c) {
  unput(c);
}

%}

%option noyywrap

%%

\n {
  return NEWLINE;
}

[ \t] {
  /* Discard spaces and tabs */
}

"|" {
	return PIPE;
}

"&"	{
	return AMPERSAND;
}

"<" {
	return LESS;
}

">" {
  return GREAT;
}

">&" {
	return GREATAMP;
}

">>" {
	return GREATGREAT;
}

">>&" {
	return GREATGREATAMP;
}

"2>" {
  return GREAT2;
}



source[ \t]+[^ \t\n]+ {
    std::string filename(yytext + 7); // Skip "source "
    FILE *fp = fopen(filename.c_str(), "r");

    if (!fp) {
        fprintf(stderr, "source: cannot open file %s\n", filename.c_str());
    } else {
        // Save current buffer
        bufferStack.push(YY_CURRENT_BUFFER);

        // Create a new buffer for the source file
        YY_BUFFER_STATE newBuffer = yy_create_buffer(fp, YY_BUF_SIZE);
        yy_switch_to_buffer(newBuffer);
        //Shell::_isSubshell = true;
        // Parse the file
        yyparse();

        // After parsing, return to old buffer
        yy_switch_to_buffer(bufferStack.top());
        bufferStack.pop();
        //Shell::_isSubshell = false;
        fclose(fp);
    }

    return NEWLINE; // simulate a newline after running source
}

\"([^\"\n]*)\" {
  /*matching quote*/
  yylval.cpp_string = new std::string(yytext + 1, yyleng - 2);
  return WORD;
}

["][^\n\"]*["] {
	/* 2.4: Quotes */
	
	yylval.cpp_string = new std::string(yytext);
	*yylval.cpp_string = yylval.cpp_string->substr(1, yylval.cpp_string->length()-2);

	/*printf("after dequote, string = %s\n",yylval.cpp_string);*/
	return WORD;
}

[^ \t\n|><&]*\\[^ \t\n]* {
    /*2.5 escaping */

    char *src = strdup(yytext);
    int length = strlen(src);

    char *dest = (char *)malloc(length + 1);  // Enough space for worst case
    int idx = 0;

    // Iterate over each char in src
    for (int j = 0; j < length; j++) {
        if (src[j] == '\\') {
            // Check if next char is also a backslash
            if ((j + 1) < length && src[j + 1] == '\\') {
                // Two backslashes => produce one '\'
                dest[idx++] = '\\';
                j++;   // Skip the next '\'
            } else {
                // Single backslash => skip it, output the next char
                j++;
                if (j < length) {
                    dest[idx++] = src[j];
                }
            }
        } else {
            // Normal character
            dest[idx++] = src[j];
        }
    }

    // Null-terminate
    dest[idx] = '\0';

    // Assign to yylval and return
    yylval.cpp_string = new std::string(dest);

    free(src);
    free(dest);

    return WORD;
}

\`[^\n\`]*\`|$\([^\n]*\) {
  /* 2.8 subshell */
  // Strip the outer $(...) or backticks
  std::string str(yytext);
  if (str[0] == '$') {
    str = str.substr(2, str.size()-3); //remove $()
  }
  else {
    str = str.substr(1, str.size()-2); //remove `..`
  }
  str += "\nexit\n";

  int tempin = dup(0);
  int tempout = dup(1);

  //create two pipes
  int pipeIn[2], pipeOut[2];
  pipe(pipeIn);
  pipe(pipeOut);

  dup2(pipeIn[0], 0);
  dup2(pipeOut[1], 1);
  close(pipeIn[0]);
  close(pipeOut[1]);

  write(pipeIn[1], str.c_str(), str.length());
  close(pipeIn[1]);

  Shell::_isSubshell = true;
  int pid = fork();
  if (pid == 0) {
    //child process
    char *args[] = { (char*)"/proc/self/exe", nullptr };
    execvp(args[0], args);
    perror("execvp (subshell)");
    exit(1);
  }
  else if (pid < 0) {
    perror("execvup (subshell)");
    exit(1);
  } else {
    Shell::_isSubshell = false;
    //parent process read the output
    waitpid(pid, nullptr, 0);

     // Restore original stdin/stdout
    dup2(tempin, 0);
    dup2(tempout, 1);
    close(tempin);
    close(tempout);

    // Read from pout[0]
    char buffer[4096] = {0};
    int n = read(pipeOut[0], buffer, sizeof(buffer));
    //fprintf(stderr, "Subshell output inserted: [%s]\n", buffer);
    close(pipeOut[0]);

    // Push the output back into the lexer input buffer
    for (int i = n - 1; i >= 0; --i) {
      if (buffer[i] == '\n') {
        //fprintf(stderr, "Subshell output inserted: %c\n", buffer[i]);
        buffer[i] = '\t';  // Avoid newlines breaking parsing
      }
      myunputc(buffer[i]);
    }
  }
}

[ \t\n]+  { /* do nothing */ }

[^ \t\n|><][^ \t\n|><]*  {
    std::string raw(yytext);
    // Expand environment variables in raw
    std::string expanded = expandAllEnv(raw);
    yylval.cpp_string = new std::string(expanded);
    return WORD;
}




.  {
    return NOTOKEN;
}




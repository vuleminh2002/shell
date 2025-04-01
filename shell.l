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
#include <unistd.h>       // for fork, execvp, pipe, dup2, etc.
#include <sys/types.h>
#include <sys/wait.h> 
extern int yyparse();
extern FILE *yyin;

typedef struct yy_buffer_state *YY_BUFFER_STATE;
extern YY_BUFFER_STATE yy_create_buffer(FILE *, int);
extern void yy_switch_to_buffer(YY_BUFFER_STATE);
extern void yy_delete_buffer(YY_BUFFER_STATE);

std::stack<YY_BUFFER_STATE> bufferStack;

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

        // Parse the file
        yyparse();

        // After parsing, return to old buffer
        yy_switch_to_buffer(bufferStack.top());
        bufferStack.pop();
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
  std::string str = std::string(yytext);
  if (str.at(0) == '$') {
    str = str.substr(1,str.size()-1);
  }
  str = str.substr(1,str.size()-2);
  str += "\nexit\n";
  //printf("str : %s",str.c_str());

  int pin[2], pout[2], tmpin, tmpout;
  tmpin = dup(0); tmpout = dup(1);
  pipe(pin); pipe(pout);

  dup2(pin[0], 0);
  dup2(pout[1], 1);
  close(pin[0]);
  close(pout[1]);

  write(pin[1], str.c_str(), str.size());
  close(pin[1]);

  int ret = fork();
  if (ret==0) {
    char ** args = new char*[2];
    args[0] = (char*)"/proc/self/exe";
    args[1] = NULL;
    execvp(args[0], args);
    perror("execvp(subshell)");
    exit(1);
  } else if (ret < 0) {
    perror("fork");
    exit(1);
  } else {

    waitpid(ret, NULL, 0);

    dup2(tmpin, 0);
    dup2(tmpout, 1);
    close(tmpin);
    close(tmpout);
  
    int BUFSIZE = 1024;
    char* c = new char[BUFSIZE];
    int rd = read(pout[0], c, BUFSIZE);
    close(pout[0]);

    //printf("output is %d, %d, %s", BUFSIZE, rd, c);
    for (int i = rd - 12; i >= 0; --i ) {
      if (c[i] == '\n') {
        c[i] = '\t';
      }
      myunputc( c[i] );
    }
    delete c;
  }
}

[^ \t\n|><][^ \t\n|><]*  {
  /* Assume that file names have only alpha chars */
  yylval.cpp_string = new std::string(yytext);
  return WORD;
}

.  {
    return NOTOKEN;
}




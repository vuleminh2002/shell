/*
 *
 * CS-252 
 * shell.l: lexical analyzer for shell
 * You have to extend it.
 *
 */

%{

#include <cstring>
#include "y.tab.hh"

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


[^ \t\n|><][^ \t\n|><]*  {
  /* Assume that file names have only alpha chars */
  yylval.cpp_string = new std::string(yytext);
  return WORD;
}

.  {
    return NOTOKEN;
}


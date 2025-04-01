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

[^ \t\n|><&]*\\.+ {
	/* 2.5 Escaping */
char *str = strdup(yytext);
char *newstr = (char *)malloc(128);  // Adjust size as needed
int i = 0;                           // Index for newstr
int len = strlen(str);

for (int j = 0; j < len; j++) {
    if (str[j] == '\\') {
        // Check if the next character is also a backslash
        if ((j + 1) < len && str[j + 1] == '\\') {
            // Two backslashes => produce ONE literal '\'
            newstr[i++] = '\\';
            j++;  // Skip the next '\'
        } else {
            // Single backslash => skip it, take next char as literal
            j++;
            if (j < len) {
                newstr[i++] = str[j];
            }
        }
    } else {
        newstr[i++] = str[j];
    }
}


newstr[i] = '\0';


free(str); // if you don't need 'str' anymore
}


[^ \t\n|><][^ \t\n|><]*  {
  /* Assume that file names have only alpha chars */
  yylval.cpp_string = new std::string(yytext);
  return WORD;
}

.  {
    return NOTOKEN;
}


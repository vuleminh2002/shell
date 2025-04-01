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

([^ \t\n|><&"]|\\.)+ {
     int len = strlen(str);
    std::string newstr;
    newstr.reserve(len); // optional optimization

    for (int j = 0; j < len; j++) {
        if (str[j] == '\\') {
            // Check if the next character exists
            if (j + 1 < len) {
                // Case: two backslashes => produce one '\'
                if (str[j + 1] == '\\') {
                    newstr.push_back('\\');
                    j++; // skip next '\'
                } else {
                    // Single backslash => skip it, add next char
                    j++;
                    if (j < len) {
                        newstr.push_back(str[j]);
                    }
                }
            } else {
                // single '\' at end => literal
                newstr.push_back('\\');
            }
        } else {
            // Normal character, copy as-is
            newstr.push_back(str[j]);
        }
    }

    yylval.cpp_string = new std::string(newstr);
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


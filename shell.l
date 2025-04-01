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

([^ \t\n|><&\\"]*\\[^ \t\n])+ {
  /*2.5 Escape*/
  std::string result;
  for (int i = 0; i < yyleng; i++) {
    if (yytext[i] == '\\' && i + 1 < yyleng && yytext[i + 1] != ' ' && yytext[i + 1] != '\t' && yytext[i + 1] != '\n') {
      result += yytext[++i];  // unescape character
    } else {
      result += yytext[i];
    }
  }
  yylval.cpp_string = new std::string(result);
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


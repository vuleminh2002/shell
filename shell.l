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
	/* 2.5 Escaping */

	char * str = strdup(yytext);
	char * newstr = (char*) malloc (100);
	//printf("before deescaping, string = %s\n", str);

	int i = 0;
	while (*str){
		if (*str == '\\'){
			if (*(str+1) == '\\'){
				str = str +2;
				newstr[i++] = '\\';
			}else{
				newstr[i++] = *(++str);
			}
		}else{
			newstr[i++] = *str;
		}
		str++;
	}
	newstr[i] = '\0';
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


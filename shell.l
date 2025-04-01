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

	//printf("after deescaping, string = %s\n", newstr);
//	std::string *newstr;

//	int i;
//	for(i = 0; i <yytext->length(); i++){
//		if (yytext[i] == '\\' && i < yytext->length()-1 && yytext[i+1] == '\\'){
//			newstr->append('\\');
//			i = i+1;
//		}else{
//			newstr->append(yytext[i]);
//		}
//	}
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


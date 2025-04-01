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

([^ \t\n|><&"]*\\[^ \t\n])+ {
  /*2.5 Escape*/
 std::string result;
for (int i = 0; i < yyleng; ++i) {
    if (yytext[i] == '\\') {
        int count = 0;
        // Count consecutive backslashes
        while (i < yyleng && yytext[i] == '\\') {
            count++;
            i++;
        }

        if (i < yyleng && yytext[i] != ' ' && yytext[i] != '\t' && yytext[i] != '\n') {
            // Escaping the next character
            result.append(count / 2, '\\');  // Every 2 slashes → one literal
            if (count % 2 == 1) {
                result += yytext[i];         // Odd one left → escape next char
                continue; // Already moved `i` forward above
            } else {
                i--; // step back so outer loop catches this non-escaped char
            }
        } else {
            // Backslashes before space/tab/newline → treat as literal
            result.append(count, '\\');
            i--; // so outer loop sees the next char (space, etc.)
        }
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


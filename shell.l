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
    result.reserve(length);

    for (int i = 0; i < length; /* increment inside loop */) {
        // If current char is not a backslash, copy as-is
        if (text[i] != '\\') {
            result.push_back(text[i]);
            i++;
            continue;
        }

        // Otherwise, we have one or more backslashes in a row
        int count = 0;
        // Count consecutive backslashes
        while (i < length && text[i] == '\\') {
            count++;
            i++;
        }

        // If we reached the end of string => put all backslashes literal
        if (i == length) {
            // Just put them all as literal
            // each pair => one '\', leftover => literal single '\'
            int pairs = count / 2;
            int leftover = count % 2;
            // pairs => produce that many '\'
            for (int p = 0; p < pairs; p++) {
                result.push_back('\\');
            }
            // leftover => produce 1 '\'
            if (leftover == 1) {
                result.push_back('\\');
            }
            break; // done
        }

        // If next char is whitespace => treat all backslashes as literal
        if (isspace((unsigned char)text[i])) {
            // Put 'count' many literal backslashes
            for (int c = 0; c < count; c++) {
                result.push_back('\\');
            }
            // We haven't consumed text[i], so we keep going
            // i not incremented => the loop will handle that character next time
            continue;
        }

        // Next char is non-whitespace => maybe we do an "escape"
        // Example:  \&
        // We'll produce (count/2) literal backslashes, 
        // if leftover=1 => that leftover means we "escape" the next char
        int pairs = count / 2;
        int leftover = count % 2;

        // produce pairs many literal backslashes
        for (int p = 0; p < pairs; p++) {
            result.push_back('\\');
        }

        if (leftover == 1) {
            // leftover means we consume the next char as escaped
            // i is at next char
            result.push_back(text[i]);
            i++;  // skip next char
        } 
        // if leftover == 0 => do not skip next char, so outer loop sees it
    }

    return result;
}


[^ \t\n|><][^ \t\n|><]*  {
  /* Assume that file names have only alpha chars */
  yylval.cpp_string = new std::string(yytext);
  return WORD;
}

.  {
    return NOTOKEN;
}


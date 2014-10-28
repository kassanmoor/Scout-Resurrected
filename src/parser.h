#ifndef PARSER_H
#define PARSER_H

/* HTML parsing tokens */
#define TOKENS " ="
#define TOKENS_PLUS " =\""

void *parse_text( char *path, char *html_text );

#endif/*PARSER_H*/

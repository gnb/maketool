#ifndef _UTIL_H_
#define _UTIL_H_

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

typedef struct
{
    char *data;
    int length;		/* string length not including nul */
    int available;	/* total bytes available, >= length+1 */
} estring;

void estring_init(estring *e);
void estring_append_string(estring *e, const char *str);
void estring_append_char(estring *e, char c);


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/* Result needs to be free()d */
char *expand_string(const char *in, const char *expands[256]);

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#endif /* _UTIL_H_ */

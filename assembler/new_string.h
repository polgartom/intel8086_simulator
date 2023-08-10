#ifndef H_NEW_STRING
#define H_NEW_STRING

#include <string.h>
#include <assert.h>

typedef struct {
    char *data;
    int count;
} String;

#define SFMT "%.*s"
#define SARG(__s) (int) (__s).count, (__s).data

inline String str_make(char *data)
{
    String s;
    s.data  = data;
    s.count = strlen(data);
    
    return s;
}

inline String str_make_alloc(unsigned int size)
{
    String s;
    s.data  = memset(((char *)malloc(size+1)), 0, (size+1));
    s.count = size;
    
    return s;
}

inline String str_advance(String s, unsigned int step)
{
    assert(s.count >= step && step >= 0);

    String new_s = s;
    new_s.data   = s.data + step; 
    new_s.count -= step;
        
    return new_s;
}

inline char *string_to_cstr(String s)
{
    // @Leak:
    char *c_str = memset(((char *)malloc(s.count+1)), 0, (s.count+1));
    memcpy(c_str, s.data, s.count);
    return c_str;
}

#endif
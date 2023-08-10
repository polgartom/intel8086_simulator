#ifndef H_NEW_STRING
#define H_NEW_STRING

#include <stdbool.h>
#include <string.h>
#include <assert.h>

typedef struct {
    char *data;
    int count;
} String;

#define SFMT "%.*s"
#define SARG(__s) (int) (__s).count, (__s).data

inline String string_create(char *data)
{
    String s;
    s.data  = data;
    s.count = strlen(data);
    
    return s;
}

inline String string_make_alloc(unsigned int size)
{
    String s;
    s.data  = memset(((char *)malloc(size+1)), 0, (size+1));
    s.count = size;
    
    return s;
}

inline String string_advance(String s, unsigned int step)
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

inline bool string_equal(String a, String b)
{
    if (a.count != b.count) return false;
    int max_count = a.count > b.count ? a.count : b.count;
    for (int i = 0; i < max_count; i++) {
        if (a.data[i] != b.data[i]) return false;
    }
    
    return true;
}

inline bool *string_equal_nocase(String a, String b)
{
    // @Todo
    assert(0);
}


#endif
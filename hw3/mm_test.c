#include <assert.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

/* Function pointers to hw3 functions */
void* (*mm_malloc)(size_t);
void* (*mm_realloc)(void*, size_t);
void (*mm_free)(void*);

void load_alloc_functions() {
    void *handle = dlopen("hw3lib.so", RTLD_NOW);
    if (!handle) {
        fprintf(stderr, "%s\n", dlerror());
        exit(1);
    }

    char* error;
    mm_malloc = dlsym(handle, "mm_malloc");
    if ((error = dlerror()) != NULL)  {
        fprintf(stderr, "%s\n", dlerror());
        exit(1);
    }

    mm_realloc = dlsym(handle, "mm_realloc");
    if ((error = dlerror()) != NULL)  {
        fprintf(stderr, "%s\n", dlerror());
        exit(1);
    }

    mm_free = dlsym(handle, "mm_free");
    if ((error = dlerror()) != NULL)  {
        fprintf(stderr, "%s\n", dlerror());
        exit(1);
    }
}

int main() {
    load_alloc_functions();

    int *data = (int*) mm_malloc(sizeof(int));
    assert(data != NULL);
    data[0] = 0x162;
    printf("%d malloc test successful!\n", *data);
    printf("%p malloc test successful!\n", data);
    //mm_free(data);
    char *data2 = (char*) mm_malloc(sizeof(char)*100);
    assert(data2 != NULL);
    for (int i = 0; i < 99; i++) data2[i] = 'a';
    data2[99] = 0;
    printf("%s\n", data2);
    printf("%p\n", data2);
    mm_free(data2);
    int *data3 = (int*) mm_malloc(sizeof(int));
    assert(data3 != NULL);
    data[0] = 0x162;
    printf("%d malloc test successful!\n", *data3);
    printf("%p malloc test successful!\n", data3);
    mm_free(data3);
    mm_free(data);
    return 0;
}

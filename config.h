#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <regex.h>

#define BUF_SIZE 512
#define STDIN 0


struct Configuration {
    int tempochegada;
    int simdias;    
};


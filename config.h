#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <string.h>
#include <regex.h>

#define BUF_SIZE 512
#define STDIN 0

struct SimConfig {
    int tempochegada;
    int simdias;    
};

struct MonConfig {
    char* nomeParque;
};

char** carregarConfiguracao(char* filename) {

    FILE* conf = fopen(filename, "r");

    if (conf == NULL) {
        printf("Ocorreu um erro na abertura do ficheiro: %s\n", strerror(errno));
        return NULL;
    }

    regex_t regex;
    int ret;
    regmatch_t rm[2];
    const char* pattern = ":(\\w+)";
    char* line = (char*)malloc(BUF_SIZE * sizeof(char));
    int result_count = 0;
    char** results = (char**)malloc(BUF_SIZE * sizeof(char*));
    char* buffer = (char*)malloc(BUF_SIZE * BUF_SIZE * sizeof(char));
    for (int i = 0; i < BUF_SIZE; i++) 
        results[i] = &buffer[i * BUF_SIZE];
        
    if (regcomp(&regex, pattern, REG_EXTENDED) != 0) {
        fprintf(stderr, "Could not compile regex\n");
        return NULL;
    }

    while (fgets(line, BUF_SIZE, conf) != NULL) {
        if ((ret = regexec(&regex, line, 2, rm, 0)) == 0) {
            strncpy(results[result_count], line + rm[1].rm_so, (size_t)(rm[1].rm_eo - rm[1].rm_so));
            results[result_count][rm[1].rm_eo - rm[1].rm_so] = '\0';
            result_count++;  
        }
    }

    regfree(&regex);
    fclose(conf);

    return results;
};


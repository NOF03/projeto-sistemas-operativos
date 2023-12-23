#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <string.h>
#include <regex.h>
#include <time.h>
#include <stdbool.h>

#define BUF_SIZE 512
#define STDIN 0

#define FALSE 0
#define TRUE 1

#define THREAD_SIZE 50000

#define UNIXSTR_PATH "/tmp/s.2082021"

#define RESET "\x1B[0m"
#define VERMELHO "\x1B[31m"
#define VERDE "\x1B[32m"
#define AMARELO "\x1B[33m"
#define AZUL "\x1B[34m"
#define MAGENTA "\x1B[35m"
#define CIANO "\x1B[36m"
#define BRANCO "\x1B[37m"

struct simConfig {
    int tempoChegada;
    int simDias;
	int tamMaxFilaAtracoes;
	float probSairFilaEntrada;
	float probSairAtracoes;
	bool tobogansFunci;
	bool piscinaFunci;
	bool pistasFunci;
	float probSairSemUmaAtracao;
	int lotEstacionamento;
	int lotParque;
	float probSairSemEstacionamento;
};

struct monConfig {
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
    const char* pattern = ":(.*)$";
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

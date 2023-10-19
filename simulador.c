#include "config.h"

#define CONFFILE "simulador.conf"

void carregarConfiguracao(char* filename, struct Configuration *simConfiguration) {

    FILE* conf;

    if (strcmp(filename, CONFFILE) != 0) {
        printf("Nome do ficheiro de configuracao incorreto. %s\n", filename);
        return;
}

    conf = fopen(filename, "r");
    if (conf == NULL) {
        printf("Ocorreu um erro na abertura do ficheiro: %s\n", strerror(errno));
        return;
    }

    regex_t regex;
    int ret;
    regmatch_t rm[2];
    const char* pattern = ":(\\w+)";
    char line[BUF_SIZE];
    char* results[BUF_SIZE][BUF_SIZE];
    int result_count = 0;
    
    if (regcomp(&regex, pattern, REG_EXTENDED) != 0) {
        fprintf(stderr, "Could not compile regex\n");
        return;
    }


    while (fgets(line, sizeof(line), conf) != NULL) {
        line[strcspn(line, "\n")] = '\0';
        if ((ret = regexec(&regex, line, 2, rm, 0)) == 0) {
            strncpy(results[result_count], line + rm[1].rm_so, (size_t)(rm[1].rm_eo - rm[1].rm_so));
            results[result_count][rm[1].rm_eo - rm[1].rm_so] = '\0'; // Null-terminate the string
            result_count++;  
        }
        
    }
    regfree(&regex);

    printf("Ficheiro lido com sucesso\n");

    fclose(conf);

    for (int i = 0; i < result_count; i++) {
        printf("Matched Value %d: %s\n", i, results[i]);
    }

    simConfiguration->simdias = atoi(results[0]);
    simConfiguration->tempochegada = atoi(results[1]);

    return;
};

int main(int argc, char **argv) {

    struct Configuration simConfiguration;

    carregarConfiguracao(argv[1], &simConfiguration);
    printf("Dias: %d\n", simConfiguration.simdias);
    printf("Tempo de chegada: %d\n", simConfiguration.tempochegada);
    return 1;
}

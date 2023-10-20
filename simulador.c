#include "config.h"

#define CONFFILE "simulador.conf"


void atribuirConfiguracao(struct SimConfig *simConfiguration, char** results) {

    simConfiguration->simdias = atoi(results[0]);
    simConfiguration->tempochegada = atoi(results[1]);
    return;
};

int main(int argc, char **argv) {

    struct SimConfig simConfiguration;

    if (strcmp(argv[1], CONFFILE) != 0) {
        printf("Nome do ficheiro de configuracao incorreto. %s\n", argv[1]);
        return 1;
    }
    
    atribuirConfiguracao(&simConfiguration, carregarConfiguracao(argv[1]));
    printf("Dias: %d\n", simConfiguration.simdias);
    printf("Tempo de chegada: %d\n", simConfiguration.tempochegada);
    return 0;
};

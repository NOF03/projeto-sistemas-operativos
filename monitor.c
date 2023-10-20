#include "config.h"

#define CONFFILE "monitor.conf"
#define REPFILE  "relatorio.txt"

void atribuirConfiguracao(struct MonConfig *monConfiguration, char** results) {
    monConfiguration->nomeParque = results[0];
    return;
}

void escreveRelatorio (char* phrase) {
    FILE* report = fopen(REPFILE, "w");

    fprintf(report, "--------------------------------");
    fprintf(report, "\nBem vindo ao %s", phrase);
    fprintf(report, "\n--------------------------------");
    fclose(report);
}

int main(int argc, char **argv) {

    struct MonConfig monConfiguration;
    
    if (strcmp(argv[1], CONFFILE) != 0) {
        printf("Nome do ficheiro de configuracao incorreto. %s\n", argv[1]);
        return 1;
    }

    atribuirConfiguracao(&monConfiguration, carregarConfiguracao(argv[1]));
    printf("%s", monConfiguration.nomeParque);
    escreveRelatorio(monConfiguration.nomeParque);
    return 0;
}

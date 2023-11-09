#include "config.h"

#define CONFFILE "monitor.conf"
#define REPFILE  "relatorio.txt"

void atribuirConfiguracao(struct monConfig *monConfiguration, char** results) {
    monConfiguration->nomeParque = results[0];
    return;
}

void ligacaoSocket() {

    int sockfd, servlen;
	struct sockaddr_un serv_addr;

	/* Cria socket stream */

	if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
		err_dump("client: can't open stream socket");

	/* Primeiro uma limpeza preventiva!
	   Dados para o socket stream: tipo + nome do ficheiro.
		 O ficheiro identifica o servidor */

	bzero((char *)&serv_addr, sizeof(serv_addr));
	serv_addr.sun_family = AF_UNIX;
	strcpy(serv_addr.sun_path, UNIXSTR_PATH);
	servlen = strlen(serv_addr.sun_path) + sizeof(serv_addr.sun_family);

	/* Tenta estabelecer uma ligação. Só funciona se o servidor tiver
		 sido lançado primeiro (o servidor tem de criar o ficheiro e associar
		 o socket ao ficheiro) */

	if (connect(sockfd, (struct sockaddr *)&serv_addr, servlen) < 0)
		err_dump("client: can't connect to server");

	/* Fecha o socket e termina */

	close(sockfd);
}

void escreveRelatorio (char* phrase) {
    FILE* report = fopen(REPFILE, "w");

    fprintf(report, "--------------------------------");
    fprintf(report, "\nBem vindo ao %s", phrase);
    fprintf(report, "\n--------------------------------");
    fclose(report);
}

int main(int argc, char **argv) {

    struct monConfig monConfiguration;
    
    if (strcmp(argv[1], CONFFILE) != 0) {
        printf("Nome do ficheiro de configuracao incorreto. %s\n", argv[1]);
        return 1;
    }

    atribuirConfiguracao(&monConfiguration, carregarConfiguracao(argv[1]));
    escreveRelatorio(monConfiguration.nomeParque);
    ligacaoSocket();
    return 0;
}

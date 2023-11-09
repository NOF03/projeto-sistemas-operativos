#include "config.h"
#include <pthread.h>
#include <time.h>

#define CONFFILE "simulador.conf"

// GLOBAL VARIABLES
int sockfd = 0;
int idPessoa = 0;

struct timeval tv;

struct Pessoa {
	int id;
	bool prioritario;
	bool membroVip;
	bool noParque;
};

enum Diversao {
	PISTASRAPIDAS,
	TOBOGAN,
	PISCINA,
	RIOLENTO,
	ESCORREGA
};

enum Sitios {
	BALNEARIOS,
	RESTAURANTE,
	CACIFOS,
	CABANAS,
	PARQUE,
	ENFERMARIA,
	TICKETS
};

void atribuirConfiguracao(struct simConfig *simConfiguration, char** results) {

    simConfiguration->simDias = atoi(results[0]);
    simConfiguration->tempoChegada = atoi(results[1]);
	simConfiguration->tamMaxFilaAtracoes = atoi(results[2]);
	simConfiguration->probSairFilaEntrada = strtof(results[3], NULL);
	simConfiguration->probSairAtracoes = strtof(results[4], NULL);
	simConfiguration->tobogansFunci = (strcmp(results[5], "Sim") > 0) ? true : false;
	simConfiguration->piscinaFunci = (strcmp(results[6], "Sim") > 0) ? true : false;
	simConfiguration->pistasFunci = (strcmp(results[7], "Sim") > 0) ? true : false;
	simConfiguration->probSairSemUmaAtracao = strtof(results[8], NULL);
	simConfiguration->lotEstacionamento = atoi(results[9]);
	simConfiguration->lotParque = atoi(results[10]);
	simConfiguration->probSairSemEstacionamento = strtof(results[11], NULL);

    return;
};

void ligacaoSocket() {
    int sockfd, newsockfd, clilen, childpid, servlen;
	struct sockaddr_un cli_addr, serv_addr;
	
	if ((sockfd = socket(AF_UNIX,SOCK_STREAM,0)) < 0)
		err_dump("server: can't open stream socket");

	bzero((char *)&serv_addr, sizeof(serv_addr));
	serv_addr.sun_family = AF_UNIX;
	strcpy(serv_addr.sun_path, UNIXSTR_PATH);

	servlen = strlen(serv_addr.sun_path) + sizeof(serv_addr.sun_family);
	unlink(UNIXSTR_PATH);
	if (bind(sockfd, (struct sockaddr *) &serv_addr, servlen) < 0)
		err_dump("server, can't bind local address");

	listen(sockfd, 1);

	for (;;) {

		clilen = sizeof(cli_addr);
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr,
				   &clilen);
		if (newsockfd < 0)
			err_dump("server: accept error");

		if ((childpid = fork()) < 0)
			err_dump("server: fork error");

		else if (childpid == 0) {
			close(sockfd);
			exit(0);
		}

		close(newsockfd);
	}
}

int numeroAleatorio(int numeroMaximo,
                    int numeroMinimo) { // Retorna um numero aleatorio entre
                                        // numero minimo e numero maximo
    return rand() % (numeroMaximo + 1 - numeroMinimo) + numeroMinimo;
}

void sendMessage(char* sendingMessage) {
	int sizeMessage;
    char mensagem[BUF_SIZE];
    if (strcpy(mensagem, sendingMessage) != 0) {
        sizeMessage = strlen(mensagem) + 1;
        if (write(sockfd, mensagem, sizeMessage) != sizeMessage) {
            printf("Erro no write!\n");
        }
    }
}

struct Pessoa Person() {

	struct Pessoa person;

	person.id = idPessoa;
	person.membroVip = numeroAleatorio(1, 0) == 0 ? false : true;
	person.prioritario = numeroAleatorio(1, 0) == 0 ? false : true;

	
}

void *simulation() {
	Person();
}

void createPerson() {

	pthread_t personThread;
	
	pthread_create(&personThread, NULL, simulation, NULL);

}

int main(int argc, char **argv) {

    struct simConfig simConfiguration;

    if (strcmp(argv[1], CONFFILE) != 0) {
        printf("Nome do ficheiro de configuracao incorreto. %s\n", argv[1]);
        return 1;
    }
    
    atribuirConfiguracao(&simConfiguration, carregarConfiguracao(argv[1]));
	ligacaoSocket();
    return 0;
};

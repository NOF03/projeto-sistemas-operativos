#include "config.h"
#include <pthread.h>
#include <time.h>

#define CONFFILE "simulador.conf"

// GLOBAL VARIABLES
int sockfd = 0;
int newsockfd = 0;
int idPessoa = 0;
struct simConfig simConfiguration;

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

void atribuirConfiguracao(char** results) {

    simConfiguration.simDias = atoi(results[0]);
    simConfiguration.tempoChegada = atoi(results[1]);
	simConfiguration.tamMaxFilaAtracoes = atoi(results[2]);
	simConfiguration.probSairFilaEntrada = strtof(results[3], NULL);
	simConfiguration.probSairAtracoes = strtof(results[4], NULL);
	simConfiguration.tobogansFunci = (strcmp(results[5], "Sim") > 0) ? true : false;
	simConfiguration.piscinaFunci = (strcmp(results[6], "Sim") > 0) ? true : false;
	simConfiguration.pistasFunci = (strcmp(results[7], "Sim") > 0) ? true : false;
	simConfiguration.probSairSemUmaAtracao = strtof(results[8], NULL);
	simConfiguration.lotEstacionamento = atoi(results[9]);
	simConfiguration.lotParque = atoi(results[10]);
	simConfiguration.probSairSemEstacionamento = strtof(results[11], NULL);

    return;
};

void sendMessage(char* sendingMessage) {
	send(newsockfd, sendingMessage, strlen(sendingMessage), 0);
}

void serverCreation() {
    int clilen, childpid, servlen;
	struct sockaddr_un cli_addr, serv_addr;
	
	if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
		printf("server: can't open stream socket");

	bzero((char *)&serv_addr, sizeof(serv_addr));
	serv_addr.sun_family = AF_UNIX;
	strcpy(serv_addr.sun_path, UNIXSTR_PATH);

	servlen = strlen(serv_addr.sun_path) + sizeof(serv_addr.sun_family);
	unlink(UNIXSTR_PATH);
	if (bind(sockfd, (struct sockaddr *) &serv_addr, servlen) < 0)
		printf("server, can't bind local address");

	printf("Esperando pelo monitor...\n");
	listen(sockfd, 1);

	clilen = sizeof(cli_addr);
	newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr,
				&clilen);
	if (newsockfd < 0)
		printf("server: accept error");

	if ((childpid = fork()) < 0)
		printf("server: fork error");

	else if (childpid == 0) {
		close(sockfd);
		iniciarSimulacao();
	}

	close(newsockfd);

}

int numeroAleatorio(int numeroMaximo,
                    int numeroMinimo) { // Retorna um numero aleatorio entre
                                        // numero minimo e numero maximo
    return rand() % (numeroMaximo + 1 - numeroMinimo) + numeroMinimo;
}


struct Pessoa Person() {

	struct Pessoa person;

	person.id = idPessoa;
	person.membroVip = numeroAleatorio(1, 0) == 0 ? false : true;
	person.prioritario = numeroAleatorio(1, 0) == 0 ? false : true;

	
}

void iniciarSimulacao() {
	createPerson();
}

void *simulation() {
	Person();
}

void createPerson() {

	//pthread_t personThread;
	
	//pthread_create(&personThread, NULL, simulation, NULL);
	sendMessage("1");
}

int main(int argc, char **argv) {

    struct simConfig simConfiguration;

    if (strcmp(argv[1], CONFFILE) != 0) {
        printf("Nome do ficheiro de configuracao incorreto. %s\n", argv[1]);
        return -1;
    }
    
    atribuirConfiguracao(carregarConfiguracao(argv[1]));
	
	serverCreation();
	
    return 0;
};

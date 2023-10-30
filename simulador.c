#include "config.h"

#define CONFFILE "simulador.conf"

void atribuirConfiguracao(struct SimConfig *simConfiguration, char** results) {

    simConfiguration->simdias = atoi(results[0]);
    simConfiguration->tempochegada = atoi(results[1]);
    return;
};

void ligacaoSocket() {
    int sockfd, newsockfd, clilen, childpid, servlen;
	struct sockaddr_un cli_addr, serv_addr;
	
	/* Cria socket stream */

	if ((sockfd = socket(AF_UNIX,SOCK_STREAM,0)) < 0)
		err_dump("server: can't open stream socket");

	/* Primeiro uma limpeza preventiva!
	   Dados para o socket stream: tipo + nome do ficheiro.
         O ficheiro serve para que os clientes possam identificar o servidor */

	bzero((char *)&serv_addr, sizeof(serv_addr));
	serv_addr.sun_family = AF_UNIX;
	strcpy(serv_addr.sun_path, UNIXSTR_PATH);

      /* O servidor é quem cria o ficheiro que identifica o socket.
         Elimina o ficheiro, para o caso de algo ter ficado pendurado.
         Em seguida associa o socket ao ficheiro. 
         A dimensão a indicar ao bind não é a da estrutura, pois depende
         do nome do ficheiro */

	servlen = strlen(serv_addr.sun_path) + sizeof(serv_addr.sun_family);
	unlink(UNIXSTR_PATH);
	if (bind(sockfd, (struct sockaddr *) &serv_addr, servlen) < 0)
		err_dump("server, can't bind local address");

	/* Servidor pronto a aceitar 5 clientes para o socket stream */

	listen(sockfd, 5);

	for (;;) {

		/* Não esquecer que quando o servidor aceita um cliente cria um
		   socket para comunicar com ele. O primeiro socket (sockfd) fica 
		   à espera de mais clientes */

		clilen = sizeof(cli_addr);
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr,
				   &clilen);
		if (newsockfd < 0)
			err_dump("server: accept error");

		/* Lança processo filho para lidar com o cliente */

		if ((childpid = fork()) < 0)
			err_dump("server: fork error");

		else if (childpid == 0) {

			/* Processo filho que vai atender o cliente. 
			   Fechar sockfd é sanitário, já que não é
			   utilizado pelo processo filho.
			   Os dados recebidos do cliente são reenviados 
			   para o cliente */

			close(sockfd);
			str_echo(newsockfd);
			exit(0);
		}

		/* Processo pai.
		   Fechar newsockfd é sanitário, já que não é
		   utilizado pelo processo pai */

		close(newsockfd);
	}
}

int main(int argc, char **argv) {

    struct SimConfig simConfiguration;

    if (strcmp(argv[1], CONFFILE) != 0) {
        printf("Nome do ficheiro de configuracao incorreto. %s\n", argv[1]);
        return 1;
    }
    
    atribuirConfiguracao(&simConfiguration, carregarConfiguracao(argv[1]));
    printf("Dias: %d\n", simConfiguration.simdias);
    printf("Tempo de chegada: %d\n", simConfiguration.tempochegada);
    ligacaoSocket();
    return 0;
};

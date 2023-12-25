#include "config.h"

#define CONFFILE "monitor.conf"
#define REPFILE "relatorio.txt"

struct monConfig monConfiguration;
int sockfd = 0;
int numPessoasMortas = 0, numPessoasParque = 0;
bool simulacaoAtiva = TRUE;

void atribuirConfiguracao(char **results)
{
	monConfiguration.nomeParque = results[0];
	return;
}

void trataMensagem(int mensagem)
{

	switch (mensagem)
	{
	case 49:
		numPessoasParque++;
		printf("Pessoas no parque: %d\n", numPessoasParque);
		break;
	case 50:
		simulacaoAtiva = FALSE;
		break;
	default:
		break;
	}
}

void readMessage()
{
	int size = 0;
	char buffer[BUF_SIZE];
	size = read(sockfd, buffer, BUF_SIZE);
	if (size == 0)
	{
		return;
	}
	else if (size > 0)
	{
		buffer[size] = '\0';
		for (int i = 0; i < size; i++)
		{
			printf("Mensagem recebida do servidor: %d\n", buffer[i]);
			trataMensagem(buffer[i]);
		}
	}
	else
	{
		printf("Erro: Nao leu do socket \n");
	}
}

void ligacaoSocket()
{

	int servlen;
	struct sockaddr_un serv_addr;

	/* Cria socket stream */

	if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
		printf("client: can't open stream socket");

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
	int varprint = 0;
	while (connect(sockfd, (struct sockaddr *)&serv_addr, servlen) < 0)
	{
		if (varprint == 0)
		{
			printf("Espera pelo simulador...\n");
			varprint = 1;
		}
	}
	printf("Monitor pronto. Esperando pelo início da simulação...\n");
}

void escreveRelatorio(FILE *report)
{
	fprintf(report, "\nNumero de pessoas que entraram no Parque: %d", numPessoasParque);
}

void escreveTitulo(char *phrase, FILE *report)
{
	struct tm *data_hora_atual;
	time_t segundos;
	time(&segundos);
	data_hora_atual = localtime(&segundos);

	fprintf(report, "--------------------------------");
	fprintf(report, "\nRELATORIO DO %s A %d/%d", phrase, data_hora_atual->tm_mday, data_hora_atual->tm_mon + 1);
	fprintf(report, "\n--------------------------------");
}

int simulacao()
{

	int i = 0;
	while (simulacaoAtiva)
	{
		readMessage();
	}
	FILE *report = fopen(REPFILE, "w");

	escreveTitulo(monConfiguration.nomeParque, report);

	escreveRelatorio(report);
	numPessoasParque = 0;
	fclose(report);
}

int main(int argc, char **argv)
{

	if (strcmp(argv[1], CONFFILE) != 0)
	{
		printf("Nome do ficheiro de configuracao incorreto. %s\n", argv[1]);
		return 1;
	}

	int opcao;

	for (;;)
	{
		printf("++++++++++++ Bem vindo ++++++++++++\n");
		printf("1: Comecar simulacao\n");

		scanf("%d", &opcao);

		switch (opcao)
		{
		case 1:
			simulacaoAtiva = true;
			atribuirConfiguracao(carregarConfiguracao(argv[1]));
			ligacaoSocket();
			simulacao();
			close(sockfd);
			break;

		default:
			return 0;
		}
	}

	return 0;
}

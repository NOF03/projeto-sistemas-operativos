#include "config.h"

#define CONFFILE "monitor.conf"
#define REPFILE "relatorio.txt"

struct monConfig monConfiguration;
int sockfd = 0;
int numPessoasFeridas = 0, numPessoasParque = 0, numPessoasEstacionamento = 0, numPessoasQueriamEntrarParque = 0, numPessoasFilaParque = 0, numPessoasFilaEstacionamento = 0, numCacifosOcupados = 0, numPessoasEntraramTobogan = 0, numPessoasEntraramPistasRapidas = 0, numPessoasEntraramPiscina = 0, numPessoasEntraramEscorrega = 0, numPessoasEntraramRioLento = 0, numCabanasOcupadas = 0, numPessoasBalnearios = 0;
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
		numPessoasQueriamEntrarParque++;
		break;
	case 50:
		numPessoasQueriamEntrarParque--;
		break;
	case 51:
		numPessoasEstacionamento++;
		break;
	case 52:
		numPessoasEstacionamento--;
		break;
	case 53:
		numPessoasFilaParque++;
		break;
	case 54:
		numPessoasFilaParque--;
		break;
	case 55:
		numPessoasFilaEstacionamento++;
		break;
	case 56:
		numPessoasFilaEstacionamento--;
		break;
	case 57:
		numPessoasParque++;
		break;
	case 58:
		numPessoasParque--;
		break;
	case 65:
		numCacifosOcupados++;
		break;
	case 66:
		numCacifosOcupados--;
		break;
	case 67:
		numPessoasEntraramTobogan++;
		break;
	case 68:
		numPessoasEntraramPistasRapidas++;
		break;
	case 69:
		numPessoasEntraramPiscina++;
		break;
	case 70:
		numPessoasEntraramEscorrega++;
		break;
	case 71:
		numPessoasEntraramRioLento++;
		break;
	case 72:
		numCabanasOcupadas++;
		break;
	case 73:
		numCabanasOcupadas--;
		break;
	case 74:
		numPessoasFeridas++;
		break;
	case 75:
		numPessoasBalnearios++;
		break;
	case 33:
		simulacaoAtiva = FALSE;
		break;
	default:
		break;
	}
	printf("Estado atual => Simulacao a decorrer!\n");
	printf("Pessoas que queriam entrar no parque: %d\n", numPessoasQueriamEntrarParque);
	printf("Pessoas na fila de espera do parque: %d\n", numPessoasFilaParque);
	printf("Pessoas na fila de espera do estacionamento: %d\n", numPessoasFilaEstacionamento);
	printf("Pessoas no estacionamento do parque: %d\n", numPessoasEstacionamento);
	printf("Pessoas no parque: %d\n", numPessoasParque);
	printf("Cacifos Ocupados: %d\n", numCacifosOcupados);
	printf("Cabanas Ocupadas: %d\n", numCabanasOcupadas);
	printf("Vezes que foram no Tobogan: %d\n", numPessoasEntraramTobogan);
	printf("Vezes que foram nas Pistas Rápidas: %d\n", numPessoasEntraramPistasRapidas);
	printf("Vezes que foram na Piscina: %d\n", numPessoasEntraramPiscina);
	printf("Vezes que foram na Escorrega: %d\n", numPessoasEntraramEscorrega);
	printf("Vezes que foram no Rio Lento: %d\n", numPessoasEntraramRioLento);
	printf("Vezes que foram na Enfermaria: %d\n", numPessoasFeridas);
	printf("Vezes que foram nos Balneários: %d\n", numPessoasBalnearios);
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
			// printf("Mensagem recebida do servidor: %d\n", buffer[i]);
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
	fprintf(report, "\nNumero de pessoas que quiseram entrar no Parque: %d", numPessoasQueriamEntrarParque);
	
	
	fprintf(report, "\n\nNumero de pessoas que entraram no Parque: %d", numPessoasParque);
	fprintf(report, "\nNumero de pessoas no estacionamento do Parque: %d", numPessoasEstacionamento);
	fprintf(report, "\nNumero de cacifos alugados: %d", numCacifosOcupados);
	fprintf(report, "\nNumero de cabanas alugadas: %d", numCabanasOcupadas);

	fprintf(report, "\n\nNumero de pessoas que utilizaram os balneários: %d", numPessoasBalnearios);

	fprintf(report, "\n\nNumero de pessoas que utilizaram cada atração:");
	fprintf(report, "\n	Tobogans: %d", numPessoasEntraramTobogan);
	fprintf(report, "\n	Escorrega: %d", numPessoasEntraramEscorrega);
	fprintf(report, "\n	Pistas Rápidas: %d", numPessoasEntraramPistasRapidas);
	fprintf(report, "\n	Piscina: %d", numPessoasEntraramPiscina);
	fprintf(report, "\n	Rio Lento: %d", numPessoasEntraramRioLento);
	
	fprintf(report, "\n\nPessoas que foram à enfermaria: %d", numPessoasFeridas);
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

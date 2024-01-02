#include "config.h"

#define CONFFILE "monitor.conf"
#define REPFILE "relatorio.out"

struct monConfig monConfiguration;
int sockfd = 0;
int numPessoasFeridas = 0, numPessoasParque = 0, numPessoasEstacionamento = 0, numPessoasQueriamEntrarParque = 0, numPessoasFilaParque = 0, numPessoasFilaEstacionamento = 0, numCacifosOcupados = 0, numPessoasEntraramTobogan = 0, numPessoasEntraramPistasRapidas = 0, numPessoasEntraramPiscina = 0, numPessoasEntraramEscorrega = 0, numPessoasEntraramRioLento = 0, numCabanasOcupadas = 0, numPessoasBalnearios = 0;
int totalTempoEntrarNoEstacionamento = 1, medioTempoEntrarNoEstacionamento = 1, totalTempoEntrarNoParque = 1, medioTempoEntrarNoParque = 1, totalTempoEntrarNoToboga = 1, medioTempoEntrarNoToboga = 1, totalTempoEntrarNoPistasRapidas = 1, medioTempoEntrarNoPistasRapidas = 1, totalTempoEntrarNoPiscina = 1, medioTempoEntrarNoPiscina = 1, totalTempoEntrarNoEscorrega = 1, medioTempoEntrarNoEscorrega = 1, totalTempoEntrarNoRioLento = 1, medioTempoEntrarNoRioLento = 1;
bool simulacaoAtiva = TRUE;

void atribuirConfiguracao(char **results)
{
	monConfiguration.nomeParque = results[0];
	return;
}

void trataMensagem(char *mensagem)
{

	char *token;
	char *eptr;
	int timeDifference;

	char *newMessage = malloc(strlen(mensagem) + 1);

	strcpy(newMessage, mensagem);
	token = strchr(mensagem, '|');
	if (token != NULL) {
		token++;
		timeDifference = atoi(token);
	}
	

	free(newMessage);

	switch (mensagem[0])
	{
	case 49:
		numPessoasQueriamEntrarParque++;
		break;
	case 50:
		numPessoasQueriamEntrarParque--;
		break;
	case 51:
		numPessoasEstacionamento++;

		totalTempoEntrarNoEstacionamento += timeDifference * 20;
		medioTempoEntrarNoEstacionamento = totalTempoEntrarNoEstacionamento / numPessoasEstacionamento;

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
		
		totalTempoEntrarNoParque += timeDifference * 20;
		medioTempoEntrarNoParque = totalTempoEntrarNoParque / numPessoasParque;

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

		totalTempoEntrarNoToboga += timeDifference * 20;
		medioTempoEntrarNoToboga = totalTempoEntrarNoToboga / numPessoasEntraramTobogan;

		break;
	case 68:
		numPessoasEntraramPistasRapidas++;

		totalTempoEntrarNoPistasRapidas += timeDifference;
		medioTempoEntrarNoPistasRapidas = totalTempoEntrarNoPistasRapidas / numPessoasEntraramPistasRapidas;

		break;
	case 69:
		numPessoasEntraramPiscina++;

		totalTempoEntrarNoPiscina += timeDifference * 20;
		medioTempoEntrarNoPiscina = totalTempoEntrarNoPiscina / numPessoasEntraramPiscina;

		break;
	case 70:
		numPessoasEntraramEscorrega++;

		totalTempoEntrarNoEscorrega += timeDifference * 20;
		medioTempoEntrarNoEscorrega = totalTempoEntrarNoEscorrega / numPessoasEntraramEscorrega;

		break;
	case 71:
		numPessoasEntraramRioLento++;

		totalTempoEntrarNoRioLento += timeDifference * 20;
		medioTempoEntrarNoRioLento = totalTempoEntrarNoRioLento / numPessoasEntraramRioLento;

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
	case 90:
		simulacaoAtiva = FALSE;
		break;
	default:
		break;
	}
	printf("Estado atual => Simulacao a decorrer!\n");
	printf("Pessoas que queriam entrar no parque: %d\n", numPessoasQueriamEntrarParque);
	printf("Pessoas na fila de espera do parque: %d || Tempo médio de espera: %d min\n", numPessoasFilaParque, medioTempoEntrarNoParque);
	printf("Pessoas na fila de espera do estacionamento: %d || Tempo médio de espera: %d min\n", numPessoasFilaEstacionamento, medioTempoEntrarNoEstacionamento);
	printf("Pessoas no estacionamento do parque: %d\n", numPessoasEstacionamento);
	printf("Pessoas no parque: %d\n", numPessoasParque);
	printf("Cacifos Ocupados: %d\n", numCacifosOcupados);
	printf("Cabanas Ocupadas: %d\n", numCabanasOcupadas);
	printf("Vezes que foram no Tobogan: %d || Tempo médio de espera: %d min\n", numPessoasEntraramTobogan, medioTempoEntrarNoToboga);
	printf("Vezes que foram nas Pistas Rápidas: %d || Tempo médio de espera: %d min\n", numPessoasEntraramPistasRapidas, medioTempoEntrarNoPistasRapidas);
	printf("Vezes que foram na Piscina: %d || Tempo médio de espera: %d min\n", numPessoasEntraramPiscina, medioTempoEntrarNoPiscina);
	printf("Vezes que foram na Escorrega: %d || Tempo médio de espera: %d min\n", numPessoasEntraramEscorrega, medioTempoEntrarNoEscorrega);
	printf("Vezes que foram no Rio Lento: %d || Tempo médio de espera: %d min\n", numPessoasEntraramRioLento, medioTempoEntrarNoRioLento);
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
		char *response = strtok(buffer, "-");
		while (response != NULL)
		{
			trataMensagem(response);
			response = strtok(NULL, "-");
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

void initializeVariables()
{
	numPessoasFeridas = 0;
	numPessoasParque = 0;
	numPessoasEstacionamento = 0;
	numPessoasQueriamEntrarParque = 0;
	numPessoasFilaParque = 0;
	numPessoasFilaEstacionamento = 0;
	numCacifosOcupados = 0;
	numPessoasEntraramTobogan = 0;
	numPessoasEntraramPistasRapidas = 0;
	numPessoasEntraramPiscina = 0;
	numPessoasEntraramEscorrega = 0;
	numPessoasEntraramRioLento = 0;
	numCabanasOcupadas = 0;
	numPessoasBalnearios = 0;
	totalTempoEntrarNoEstacionamento = 1;
	medioTempoEntrarNoEstacionamento = 1;
	totalTempoEntrarNoParque = 1;
	medioTempoEntrarNoParque = 1;
	totalTempoEntrarNoToboga = 1;
	medioTempoEntrarNoToboga = 1;
	totalTempoEntrarNoPistasRapidas = 1; 
	medioTempoEntrarNoPistasRapidas = 1;
	totalTempoEntrarNoPiscina = 1; 
	medioTempoEntrarNoPiscina = 1;
	totalTempoEntrarNoEscorrega = 1; 
	medioTempoEntrarNoEscorrega = 1;
	totalTempoEntrarNoRioLento = 1; 
	medioTempoEntrarNoRioLento = 1;
	simulacaoAtiva = true;
	system("clear");
}

int simulacao()
{

	int i = 0;
	while (simulacaoAtiva)
	{
		readMessage();
	}
	printf("\nEstado atual => Simulacao realizada!\n");
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
		printf("0: Sair\n");

		scanf("%d", &opcao);

		switch (opcao)
		{
		case 1:
			initializeVariables();
			atribuirConfiguracao(carregarConfiguracao(argv[1]));
			ligacaoSocket();
			simulacao();
			close(sockfd);
			break;
		case 0:
			return 0;
		default:
			break;
		}
	}

	return 0;
}

#include "config.h"

#define CONFFILE "simulador.conf"

// GLOBAL VARIABLES
int sockfd = 0;
int newsockfd = 0;
int idPessoa = 0;
int minutosDecorridos = 0;
struct simConfig simConfiguration;

pthread_t IDthread[THREAD_SIZE];

struct Person *createdPeople[THREAD_SIZE];
struct Parque parque;

struct Person
{
	int id;
	int numeroPessoasAFrenteParaDesistir;
	int tempoChegadaFilaEspera;
	int tempoMaximoEspera;
	bool prioritario;
	bool membroVip;
	bool noParque;
	bool desistiu;
	sem_t semaforoPessoa;
};

struct Parque
{
	int numeroPessoaEspera;
};

enum Diversao
{
	PISTASRAPIDAS,
	TOBOGAN,
	PISCINA,
	RIOLENTO,
	ESCORREGA
};

enum Sitios
{
	BALNEARIOS,
	RESTAURANTE,
	CACIFOS,
	CABANAS,
	ESTACIONAMENTO,
	TICKETS,
	ENFERMARIA
};

// TRINCOS E SEMAFOROS
pthread_mutex_t mutexCriarPessoa;
pthread_mutex_t mutexPessoasParque;
pthread_mutex_t sendMessageThread;
sem_t semCriarPessoa;

void atribuirConfiguracao(char **results)
{

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

void startSemaphoresAndLatches()
{
	if (pthread_mutex_init(&mutexCriarPessoa, NULL) != 0)
	{
		printf(VERMELHO "Inicialização do trinco de criar pessoas falhou!.\n");
	}
	if (pthread_mutex_init(&sendMessageThread, NULL) != 0)
	{
		printf(VERMELHO "Inicialização do trinco de criar pessoas falhou!.\n");
	}

	if (pthread_mutex_init(&mutexPessoasParque, NULL) != 0) 
	{
		printf(VERMELHO "Inicialização do trinco de criar pessoas falhou!.\n");
	}

	sem_init(&mutexMensagens, 0, 1);
	sem_init(&semCriarPessoa, 0, 1);
}

int numeroAleatorio(int numeroMaximo, int numeroMinimo)
{
	return rand() % (numeroMaximo + 1 - numeroMinimo) + numeroMinimo;
}

long long current_timestamp()
{
	struct timeval te;
	gettimeofday(&te, NULL);										 // get current time
	long long milliseconds = te.tv_sec * 1000LL + te.tv_usec / 1000; // calculate milliseconds
	return milliseconds;
}

void sendMessage(char *sendingMessage)
{
	write(newsockfd, sendingMessage, strlen(sendingMessage));
}
struct Person createPerson()
{

	struct Person person;

	person.id = idPessoa;
	person.membroVip = numeroAleatorio(1, 0) == 0 ? false : true;
	person.prioritario = numeroAleatorio(1, 0) == 0 ? false : true;
	person.numeroPessoasAFrenteParaDesistir = numeroAleatorio(simConfiguration.lotParque, (simConfiguration.lotParque * 3) / 4);
	person.noParque = false;
	person.desistiu = false;
	sem_wait(&semCriarPessoa);
	idPessoa++;
	sem_post(&semCriarPessoa);

	return person;
}

void WaitingList(struct Person *pessoa)
{
	char mensagem[BUF_SIZE];
	long timeStamp = minutosDecorridos;
	int index, tempoEspera;
	int valorSemaforo;
	if (!pessoa->noParque)
	{
		pthread_mutex_lock(&mutexPessoasParque);
		int pessoasNaFila = parque.numeroPessoaEspera;
		pthread_mutex_unlock(&mutexPessoasParque);	
		if (pessoasNaFila < simConfiguration.lotParque)
		{
			if (pessoa->numeroPessoasAFrenteParaDesistir < pessoasNaFila)
			{
				printf(VERMELHO "%d desistiu da fila do parque porque tinha muita gente a frente. \n", pessoa->id);
				pessoa->desistiu = TRUE;
			}
			else
			{
				pessoa->tempoChegadaFilaEspera = timeStamp;
				pthread_mutex_lock(&mutexPessoasParque);
				parque.numeroPessoaEspera++;
				pthread_mutex_unlock(&mutexPessoasParque);
			}
		}
	}
}

void *Person()
{
	struct Person onePerson = createPerson();
	createdPeople[onePerson.id] = &onePerson;
	char buffer[BUF_SIZE];
	printf(VERDE "Pessoa criada %d\n", onePerson.id);
	sendMessage("1");
	while (TRUE)
	{

		WaitingList(&onePerson);
		if (!onePerson.desistiu)
		{
		}
		break;
	}
}

void Simulation()
{
	srand(time(NULL));
	startSemaphoresAndLatches();
	char buffer[BUF_SIZE];
	int lastTimeStamp = current_timestamp();
	int auxTimeStamp, numeroDia = 1;
	int tempoLimite = simConfiguration.simDias * DIA;
	int idx;
	char sendingMessage[BUF_SIZE];

	for (idx = 0; idx < simConfiguration.lotParque; idx++)
	{
		if (pthread_create(&IDthread[idPessoa], NULL, Person, NULL))
		{
			printf("Erro na criação da tarefa \n");
			exit(1);
		}
	}
	sendMessage("2");
}

void serverCreation()
{
	int clilen, childpid, servlen;
	struct sockaddr_un cli_addr, serv_addr;

	if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
		printf("server: can't open stream socket");

	bzero((char *)&serv_addr, sizeof(serv_addr));
	serv_addr.sun_family = AF_UNIX;
	strcpy(serv_addr.sun_path, UNIXSTR_PATH);

	servlen = strlen(serv_addr.sun_path) + sizeof(serv_addr.sun_family);
	unlink(UNIXSTR_PATH);
	if (bind(sockfd, (struct sockaddr *)&serv_addr, servlen) < 0)
		printf("server, can't bind local address");

	printf("Esperando pelo monitor...\n");
	listen(sockfd, 1);

	clilen = sizeof(cli_addr);
	newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr,
					   &clilen);
	if (newsockfd < 0)
		printf("server: accept error");

	if ((childpid = fork()) < 0)
		printf("server: fork error");

	else if (childpid == 0)
	{
		close(sockfd);
		Simulation();
	}
	close(newsockfd);
}

int main(int argc, char **argv)
{

	if (strcmp(argv[1], CONFFILE) != 0)
	{
		printf("Nome do ficheiro de configuracao incorreto. %s\n", argv[1]);
		return -1;
	}

	atribuirConfiguracao(carregarConfiguracao(argv[1]));

	serverCreation();
	return 0;
};

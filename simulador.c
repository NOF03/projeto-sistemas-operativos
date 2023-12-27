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
	long long tempoChegadaFilaEspera;
	bool precisaEstacionamento;
	bool prioritario;
	bool noParque;
	bool noEstacionamento;
	bool desistiu;
	sem_t semaforoPessoa;
	int diversao;
	int sitio;
	int tempoMaximoEspera;
};

struct Parque
{
	int numeroPessoaEspera;
	int numeroPessoasNoParque;
	int numeroPessoaEsperaNoEstacionamento;
	int numeroPessoasNoEstacionamento;
	sem_t filaParque;
	sem_t filaEstacionamento;
	int numeroEntradaDisponivel;
	int numeroEstacionamentoDisponivel;
};

enum Diversao
{
	NENHUMA,
	PISTASRAPIDAS,
	TOBOGAN,
	PISCINA,
	RIOLENTO,
	ESCORREGA
};

enum Sitios
{
	NENHUM,
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
pthread_mutex_t mutexPessoasEstacionamento;
sem_t semCriarPessoa;

pthread_mutex_t mutexVariaveisSimulacao;

void atribuirConfiguracao(char **results)
{

	simConfiguration.simDias = atoi(results[0]);
	simConfiguration.tempoMedioDeEspera = atoi(results[1]);
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

	if (pthread_mutex_init(&mutexPessoasParque, NULL) != 0)
	{
		printf(VERMELHO "Inicialização do trinco de criar pessoas falhou!.\n");
	}

	if (pthread_mutex_init(&mutexPessoasEstacionamento, NULL) != 0)
	{
		printf(VERMELHO "Inicialização do trinco de criar pessoas falhou!.\n");
	}

	sem_init(&semCriarPessoa, 0, 1);
	sem_init(&parque.filaParque, 0, 1);
	sem_init(&parque.filaEstacionamento, 0, 1);

	parque.numeroPessoaEspera = 0;
	parque.numeroPessoaEsperaNoEstacionamento = 0;
	parque.numeroPessoasNoEstacionamento = 0;
	parque.numeroPessoasNoParque = 0;
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

void WaitingListParking(struct Person *pessoa)
{
	char mensagem[BUF_SIZE];
	int index, tempoEspera;
	int valorSemaforo;
	if (pessoa->precisaEstacionamento)
	{
		if (!pessoa->noEstacionamento)
		{
			pthread_mutex_lock(&mutexPessoasEstacionamento);
			int pessoasNoEstacionamento = parque.numeroPessoasNoEstacionamento;
			pthread_mutex_unlock(&mutexPessoasEstacionamento);
			if (pessoasNoEstacionamento < simConfiguration.lotEstacionamento)
			{
				if (numeroAleatorio(100, 0) < simConfiguration.probSairSemEstacionamento * parque.numeroPessoaEsperaNoEstacionamento)
				{
					printf(VERMELHO "O visitante %d desistiu. Muita gente a sua frente para estacionar.\n", pessoa->id);
					pessoa->desistiu = TRUE;
				}
				else
				{
					pthread_mutex_lock(&mutexPessoasEstacionamento);
					parque.numeroPessoaEsperaNoEstacionamento++;
					pthread_mutex_unlock(&mutexPessoasEstacionamento);
					pessoa->tempoChegadaFilaEspera = current_timestamp();
					sendMessage("7");
					sem_wait(&parque.filaEstacionamento);
					pthread_mutex_lock(&mutexVariaveisSimulacao);
					tempoEspera = current_timestamp() - pessoa->tempoChegadaFilaEspera; // TEMPO MAX DE ESPERA DA PESSOA
					pthread_mutex_unlock(&mutexVariaveisSimulacao);
					while (tempoEspera <= pessoa->tempoMaximoEspera && parque.numeroPessoasNoEstacionamento >= simConfiguration.lotEstacionamento)
					{
						pthread_mutex_lock(&mutexVariaveisSimulacao);
						tempoEspera = current_timestamp() - pessoa->tempoChegadaFilaEspera; // TEMPO MAX DE ESPERA DA PESSOA
						pthread_mutex_unlock(&mutexVariaveisSimulacao);
					};
					if (tempoEspera > pessoa->tempoMaximoEspera)
					{

						pessoa->desistiu = TRUE;
						pthread_mutex_lock(&mutexPessoasEstacionamento);
						parque.numeroPessoaEsperaNoEstacionamento--;
						sem_post(&parque.filaEstacionamento);
						pthread_mutex_unlock(&mutexPessoasEstacionamento);
						printf(VERMELHO "O visitante %d desistiu. Passou muito tempo a espera no estacionamento.\n", pessoa->id);
						sendMessage("8");
					}
					else
					{
						sendMessage("8");
						sendMessage("3");
						pessoa->noEstacionamento = TRUE;
						printf(MAGENTA "O visitante %d conseguiu estacionar.\n", pessoa->id);
						pthread_mutex_lock(&mutexPessoasEstacionamento);
						parque.numeroPessoaEsperaNoEstacionamento--;
						parque.numeroPessoasNoEstacionamento++;
						pthread_mutex_unlock(&mutexPessoasEstacionamento);
						sem_post(&parque.filaEstacionamento);
						WaitingListWaterPark(pessoa);
					}
				}
			}
			else
			{
				pessoa->desistiu = TRUE;
				printf(VERMELHO "O visitante %d desistiu. PARQUE CHEIO.\n", pessoa->id);
			}
		}
	}
	else
	{
		WaitingListWaterPark(pessoa);
	}
}

void WaitingListWaterPark(struct Person *pessoa)
{
	char mensagem[BUF_SIZE] = "";
	int index, tempoEspera;
	int valorSemaforo;
	if (!pessoa->noParque)
	{
		pthread_mutex_lock(&mutexPessoasParque);
		int pessoasNoParque = parque.numeroPessoasNoParque;
		pthread_mutex_unlock(&mutexPessoasParque);
		if (pessoasNoParque < simConfiguration.lotParque)
		{
			if (numeroAleatorio(100, 0) < simConfiguration.probSairFilaEntrada * parque.numeroPessoaEspera)
			{
				printf(VERMELHO "O visitante %d desistiu. Muita gente a sua frente na fila.\n", pessoa->id);
				pessoa->desistiu = TRUE;
				if (pessoa->noEstacionamento)
				{
					sendMessage("4");
					pthread_mutex_lock(&mutexPessoasParque);
					parque.numeroPessoasNoEstacionamento--;
					pthread_mutex_unlock(&mutexPessoasParque);
				}
			}
			else
			{
				pessoa->tempoChegadaFilaEspera = current_timestamp();
				pthread_mutex_lock(&mutexPessoasParque);
				parque.numeroPessoaEspera++;
				pthread_mutex_unlock(&mutexPessoasParque);
				sendMessage("5");
				sem_wait(&parque.filaParque);
				pthread_mutex_lock(&mutexVariaveisSimulacao);
				tempoEspera = current_timestamp() - pessoa->tempoChegadaFilaEspera; // TEMPO MAX DE ESPERA DA PESSOA
				pthread_mutex_unlock(&mutexVariaveisSimulacao);

				while (tempoEspera <= pessoa->tempoMaximoEspera && parque.numeroPessoasNoParque >= simConfiguration.lotParque)
					{
						pthread_mutex_lock(&mutexVariaveisSimulacao);
						tempoEspera = current_timestamp() - pessoa->tempoChegadaFilaEspera; // TEMPO MAX DE ESPERA DA PESSOA
						pthread_mutex_unlock(&mutexVariaveisSimulacao);
						strcpy(mensagem, "PARQUE CHEIO.");
					};
				if (tempoEspera > pessoa->tempoMaximoEspera)
				{
					pessoa->desistiu = TRUE;
					pthread_mutex_lock(&mutexPessoasParque);
					parque.numeroPessoaEspera--;
					sem_post(&parque.filaParque);
					pthread_mutex_unlock(&mutexPessoasParque);
					printf(VERMELHO "O visitante %d desistiu. Passou muito tempo a espera. %s\n", pessoa->id, mensagem);
					sendMessage("6");
				}
				else
				{
					pessoa->noParque = TRUE;
					sendMessage("9");
					sendMessage("6");
					printf(VERDE "O visitante %d entrou no parque.\n", pessoa->id);
					pthread_mutex_lock(&mutexPessoasParque);
					parque.numeroPessoaEspera--;
					parque.numeroPessoasNoParque++;
					pthread_mutex_unlock(&mutexPessoasParque);
					sem_post(&parque.filaParque);
				}
			}
		}
		else
		{
			pthread_mutex_unlock(&mutexPessoasParque);
			pessoa->desistiu = TRUE;
		}
	}
}

struct Person createPerson()
{
	struct Person person;

	person.id = idPessoa;
	person.prioritario = numeroAleatorio(1, 0) == 0 ? false : true;
	person.precisaEstacionamento = numeroAleatorio(1, 0) == 0 ? false : true;
	person.noParque = false;
	person.noEstacionamento = false;
	person.desistiu = false;
	person.diversao = 0;
	person.sitio = 0;
	person.tempoMaximoEspera = numeroAleatorio(simConfiguration.tempoMedioDeEspera, (simConfiguration.tempoMedioDeEspera * 3) / 4);
	sem_wait(&semCriarPessoa);
	idPessoa++;
	sem_post(&semCriarPessoa);

	return person;
}

void *Person()
{
	struct Person onePerson = createPerson();
	createdPeople[onePerson.id] = &onePerson;
	char buffer[BUF_SIZE];
	printf(RESET "Um visitante quer entrar no parque. Numero %d.\n", onePerson.id);
	sendMessage("1");
	while (TRUE)
	{
		WaitingListParking(&onePerson);

		if (!onePerson.desistiu)
		{
			break;
		}
		else
		{
			break;
		}
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

	for (idx = 0; idx < 300; idx++)
	{
		if (pthread_create(&IDthread[idPessoa], NULL, Person, NULL))
		{
			printf("Erro na criação da tarefa \n");
			exit(1);
		}
	}

	usleep(2000000);
	sendMessage("A");
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

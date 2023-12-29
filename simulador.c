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
	bool precisaCacifo;
	bool alugouCacifo;
};

bool pistasRapidasEmAndamento;

struct Parque
{
	int numeroPessoaEspera;
	int numeroPessoasNoParque;
	int numeroPessoaEsperaNoEstacionamento;
	int numeroPessoasNoEstacionamento;
	int numeroCacifosOcupados;
	sem_t filaParque;
	sem_t filaEstacionamento;
	int numeroEntradaDisponivel;
	int numeroEstacionamentoDisponivel;
	sem_t filaSitios[BUF_SIZE];
	int capacidadeSitios[BUF_SIZE];
	int capacidadeAtualSitios[BUF_SIZE];
};

enum Sitios
{
	NENHUM,
	BALNEARIOS,
	CACIFOS,
	RESTAURANTE,
	CABANAS,
	ENFERMARIA,
	PISTASRAPIDAS,
	TOBOGAN,
	PISCINA,
	RIOLENTO,
	ESCORREGA,
	TOTAL
};

// TRINCOS E SEMAFOROS
pthread_mutex_t mutexCriarPessoa;
pthread_mutex_t mutexPessoasParque;
pthread_mutex_t mutexPessoasEstacionamento;
pthread_mutex_t mutexVariaveisSimulacao;
pthread_mutex_t mutexPessoasCacifos;
pthread_mutex_t mutexPessoasPistasRapidas;
sem_t mutexMensagens;
sem_t semCriarPessoa;

void atribuirConfiguracao(char **results)
{

	simConfiguration.simDias = atoi(results[0]);
	simConfiguration.tempoMedioDeEspera = atoi(results[1]);
	simConfiguration.capAtracoes = atoi(results[2]);
	simConfiguration.capBalnearios = atoi(results[3]);
	simConfiguration.capCacifos = atoi(results[4]);
	simConfiguration.probSairFilaEntrada = strtof(results[5], NULL);
	simConfiguration.probSairAtracoes = strtof(results[6], NULL);
	simConfiguration.tobogansFunci = (strcmp(results[7], "Sim") > 0) ? true : false;
	simConfiguration.piscinaFunci = (strcmp(results[8], "Sim") > 0) ? true : false;
	simConfiguration.pistasFunci = (strcmp(results[9], "Sim") > 0) ? true : false;
	simConfiguration.escorregaFunci = (strcmp(results[10], "Sim") > 0) ? true : false;
	simConfiguration.rioLentoFunci = (strcmp(results[11], "Sim") > 0) ? true : false;
	simConfiguration.probEntrarNumaAtracao = strtof(results[12], NULL);
	simConfiguration.probSairSemUmaAtracao = strtof(results[13], NULL);
	simConfiguration.lotEstacionamento = atoi(results[14]);
	simConfiguration.lotParque = atoi(results[15]);
	simConfiguration.probSairSemEstacionamento = strtof(results[16], NULL);

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

	if (pthread_mutex_init(&mutexPessoasCacifos, NULL) != 0)
	{
		printf(VERMELHO "Inicialização do trinco de criar cacifos falhou!.\n");
	}

	if (pthread_mutex_init(&mutexVariaveisSimulacao, NULL) != 0)
	{
		printf(VERMELHO "Inicialização do trinco de criar cacifos falhou!.\n");
	}

	if (pthread_mutex_init(&mutexPessoasPistasRapidas, NULL) != 0)
	{
		printf(VERMELHO "Inicialização do trinco de criar cacifos falhou!.\n");
	}

	sem_init(&semCriarPessoa, 0, 1);
	sem_init(&parque.filaParque, 0, 1);
	sem_init(&parque.filaEstacionamento, 0, 1);
	sem_init(&mutexMensagens, 0, 1);

	parque.numeroPessoaEspera = 0;
	parque.numeroPessoaEsperaNoEstacionamento = 0;
	parque.numeroPessoasNoEstacionamento = 0;
	parque.numeroPessoasNoParque = 0;
	parque.numeroCacifosOcupados = 0;
	sem_init(&parque.filaSitios[BALNEARIOS], 0, simConfiguration.capBalnearios);
	sem_init(&parque.filaSitios[CACIFOS], 0, simConfiguration.capCacifos);
	sem_init(&parque.filaSitios[RESTAURANTE], 0, 30);
	sem_init(&parque.filaSitios[CABANAS], 0, 1);
	sem_init(&parque.filaSitios[ENFERMARIA], 0, 10);
	sem_init(&parque.filaSitios[PISTASRAPIDAS], 0, 4);
	sem_init(&parque.filaSitios[TOBOGAN], 0, 2);
	sem_init(&parque.filaSitios[RIOLENTO], 0, simConfiguration.capAtracoes);
	sem_init(&parque.filaSitios[PISCINA], 0, simConfiguration.capAtracoes);
	sem_init(&parque.filaSitios[ESCORREGA], 0, 1);

	pistasRapidasEmAndamento = false;
	parque.capacidadeAtualSitios[PISTASRAPIDAS] = 0;
}

int numeroAleatorio(int numeroMaximo, int numeroMinimo)
{
	return rand() % (numeroMaximo + 1 - numeroMinimo) + numeroMinimo;
}

int probabilidade(float prob)
{ // Função probabilidade, retorna true ou false
	return numeroAleatorio(100, 0) < (prob * 100);
}

long long current_timestamp()
{
	struct timeval te;
	gettimeofday(&te, NULL);										 // get current time
	long long milliseconds = te.tv_sec * 1000LL + te.tv_usec / 1000; // calculate milliseconds
	return milliseconds;
}

char *distinguirPrioritario(struct Person *person)
{
	return (person->prioritario) ? "PRIORITÁRIO" : "";
}

void sendMessage(char *sendingMessage)
{
	write(newsockfd, sendingMessage, strlen(sendingMessage));
}

void UsePark(struct Person *pessoa)
{
	char mensagem[BUF_SIZE] = "";

	if (pessoa->precisaCacifo && parque.numeroCacifosOcupados < simConfiguration.capCacifos && !pessoa->alugouCacifo)
	{
		sem_wait(&parque.filaSitios[CACIFOS]);
		pthread_mutex_lock(&mutexPessoasCacifos);
		parque.numeroCacifosOcupados++;
		pthread_mutex_unlock(&mutexPessoasCacifos);
		sem_post(&parque.filaSitios[CACIFOS]);
		printf(CIANO "O visitante %s %d alugou cacifo.\n", distinguirPrioritario(pessoa), pessoa->id);
		sendMessage("A");
		pessoa->alugouCacifo = true;
	}

	switch (pessoa->sitio)
	{
	case NENHUM:
	case TOBOGAN:
		if (simConfiguration.tobogansFunci)
		{
			if (probabilidade(simConfiguration.probEntrarNumaAtracao))
			{
				pessoa->sitio = TOBOGAN;
				sem_wait(&parque.filaSitios[TOBOGAN]);
				printf(AMARELO "O visitante %d divertiu-se nos TOBOGANS.\n" RESET, pessoa->id);
				sendMessage("C");
				sem_post(&parque.filaSitios[TOBOGAN]);
			}
		}
		else
		{
			if (probabilidade(simConfiguration.probSairSemUmaAtracao))
			{
				pessoa->desistiu = TRUE;
				printf(VERMELHO "O visitante %d saiu. Não tinha TOBOGANS. %s\n", pessoa->id, mensagem);
				sendMessage(":");
				if (pessoa->noEstacionamento)
				{

					pthread_mutex_lock(&mutexPessoasEstacionamento);
					parque.numeroPessoasNoEstacionamento--;
					pthread_mutex_unlock(&mutexPessoasEstacionamento);
					sendMessage("4");
				}
			}
		}
	case PISTASRAPIDAS:
		if (simConfiguration.pistasFunci)
		{
			if (probabilidade(simConfiguration.probEntrarNumaAtracao))
			{

				int semValue;
				pessoa->sitio = PISTASRAPIDAS;
				sem_wait(&parque.filaSitios[PISTASRAPIDAS]);
				pthread_mutex_lock(&mutexPessoasPistasRapidas);
				int capacidadeAtual = parque.capacidadeAtualSitios[PISTASRAPIDAS];
				pthread_mutex_unlock(&mutexPessoasPistasRapidas);
				while (pistasRapidasEmAndamento || capacidadeAtual == 4)
				{
				}
				pthread_mutex_lock(&mutexPessoasPistasRapidas);
				parque.capacidadeAtualSitios[PISTASRAPIDAS]++;
				if (parque.capacidadeAtualSitios[PISTASRAPIDAS] == 4) {
					pistasRapidasEmAndamento = true;
				}
				pthread_mutex_unlock(&mutexPessoasPistasRapidas);
				while (!pistasRapidasEmAndamento) {}
				printf(AMARELO "O visitante %d divertiu-se nas PISTAS RÁPIDAS.\n" RESET, pessoa->id);
				sendMessage("D");
				
				pthread_mutex_lock(&mutexPessoasPistasRapidas);
				
				parque.capacidadeAtualSitios[PISTASRAPIDAS]--;
				if (parque.capacidadeAtualSitios[PISTASRAPIDAS] == 0) {
					pistasRapidasEmAndamento = false;
				}
				pthread_mutex_unlock(&mutexPessoasPistasRapidas);
				
				while (pistasRapidasEmAndamento) {};
				
				sem_post(&parque.filaSitios[PISTASRAPIDAS]);
				
			}
		}
		else
		{
			if (probabilidade(simConfiguration.probSairSemUmaAtracao))
			{
				pessoa->desistiu = TRUE;
				printf(VERMELHO "O visitante %d saiu. Não tinha TOBOGANS. %s\n", pessoa->id, mensagem);
				sendMessage(":");
				if (pessoa->noEstacionamento)
				{

					pthread_mutex_lock(&mutexPessoasEstacionamento);
					parque.numeroPessoasNoEstacionamento--;
					pthread_mutex_unlock(&mutexPessoasEstacionamento);
					sendMessage("4");
				}
			}
		}
	case PISCINA:
		if (simConfiguration.piscinaFunci)
		{
			if (probabilidade(simConfiguration.probEntrarNumaAtracao))
			{
				pessoa->sitio = PISCINA;
				sem_wait(&parque.filaSitios[PISCINA]);
				printf(AMARELO "O visitante %d divertiu-se na PISCINA.\n" RESET, pessoa->id);
				sendMessage("E");
				sem_post(&parque.filaSitios[PISCINA]);
			}
		}
		else
		{
			if (probabilidade(simConfiguration.probSairSemUmaAtracao))
			{
				pessoa->desistiu = TRUE;
				printf(VERMELHO "O visitante %d saiu. Não tinha PISCINA. %s\n", pessoa->id, mensagem);
				sendMessage(":");
				if (pessoa->noEstacionamento)
				{

					pthread_mutex_lock(&mutexPessoasEstacionamento);
					parque.numeroPessoasNoEstacionamento--;
					pthread_mutex_unlock(&mutexPessoasEstacionamento);
					sendMessage("4");
				}
			}
		}
	case ESCORREGA:
		if (simConfiguration.escorregaFunci)
		{
			if (probabilidade(simConfiguration.probEntrarNumaAtracao))
			{
				pessoa->sitio = ESCORREGA;
				sem_wait(&parque.filaSitios[ESCORREGA]);
				printf(AMARELO "O visitante %d divertiu-se nos ESCORREGAS.\n" RESET, pessoa->id);
				sendMessage("F");
				sem_post(&parque.filaSitios[ESCORREGA]);
			}
		}
		else
		{
			if (probabilidade(simConfiguration.probSairSemUmaAtracao))
			{
				pessoa->desistiu = TRUE;
				printf(VERMELHO "O visitante %d saiu. Não tinha ESCORREGAS. %s\n", pessoa->id, mensagem);
				sendMessage(":");
				if (pessoa->noEstacionamento)
				{

					pthread_mutex_lock(&mutexPessoasEstacionamento);
					parque.numeroPessoasNoEstacionamento--;
					pthread_mutex_unlock(&mutexPessoasEstacionamento);
					sendMessage("4");
				}
			}
		}
	case RIOLENTO:
		if (simConfiguration.escorregaFunci)
		{
			if (probabilidade(simConfiguration.probEntrarNumaAtracao))
			{

				pessoa->sitio = RIOLENTO;
				sem_wait(&parque.filaSitios[RIOLENTO]);
			}
		}
		else
		{
			if (probabilidade(simConfiguration.probSairSemUmaAtracao))
			{
				pessoa->desistiu = TRUE;
				printf(VERMELHO "O visitante %d saiu. Não tinha RIOLENTO. %s\n", pessoa->id, mensagem);
				sendMessage(":");
				if (pessoa->noEstacionamento)
				{

					pthread_mutex_lock(&mutexPessoasEstacionamento);
					parque.numeroPessoasNoEstacionamento--;
					pthread_mutex_unlock(&mutexPessoasEstacionamento);
					sendMessage("4");
				}
			}
		}
	default:
		break;
	}
}

void WaitingListWaterPark(struct Person *pessoa)
{
	char mensagem[BUF_SIZE] = "";
	int index, tempoEspera;
	int valorSemaforo;

	pthread_mutex_lock(&mutexPessoasParque);
	int pessoasNoParque = parque.numeroPessoasNoParque;
	pthread_mutex_unlock(&mutexPessoasParque);
	if (pessoasNoParque < simConfiguration.lotParque)
	{
		if (!pessoa->prioritario)
		{
			if (numeroAleatorio(100, 0) < simConfiguration.probSairFilaEntrada * parque.numeroPessoaEspera)
			{
				printf(VERMELHO "O visitante %d desistiu. Muita gente a sua frente na fila.\n", pessoa->id);
				pessoa->desistiu = TRUE;
				if (pessoa->noEstacionamento)
				{

					pthread_mutex_lock(&mutexPessoasEstacionamento);
					parque.numeroPessoasNoEstacionamento--;
					pthread_mutex_unlock(&mutexPessoasEstacionamento);
					sendMessage("4");
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
					if (pessoa->noEstacionamento)
					{
						pthread_mutex_lock(&mutexPessoasEstacionamento);
						parque.numeroPessoasNoEstacionamento--;
						pthread_mutex_unlock(&mutexPessoasEstacionamento);
						sendMessage("4");
					}
				}
				else
				{
					pessoa->noParque = TRUE;

					printf(VERDE "O visitante %d entrou no parque.\n", pessoa->id);
					pthread_mutex_lock(&mutexPessoasParque);
					parque.numeroPessoaEspera--;
					parque.numeroPessoasNoParque++;
					pthread_mutex_unlock(&mutexPessoasParque);
					sendMessage("6");
					sendMessage("9");
					sem_post(&parque.filaParque);
				}
			}
		}
		else
		{
			pessoa->noParque = TRUE;

			printf(VERDE "O visitante PRIORITÁRIO %d entrou no parque.\n", pessoa->id);
			pthread_mutex_lock(&mutexPessoasParque);
			parque.numeroPessoasNoParque++;
			pthread_mutex_unlock(&mutexPessoasParque);
			sendMessage("9");
		}
	}
	else
	{
		pessoa->desistiu = TRUE;
		printf(VERMELHO "O visitante %d desistiu. PARQUE CHEIO.\n", pessoa->id);
	}
}

void WaitingListParking(struct Person *pessoa)
{
	char mensagem[BUF_SIZE] = "";
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
					sendMessage("7");
					pessoa->tempoChegadaFilaEspera = current_timestamp();

					sem_wait(&parque.filaEstacionamento);
					pthread_mutex_lock(&mutexVariaveisSimulacao);
					tempoEspera = current_timestamp() - pessoa->tempoChegadaFilaEspera; // TEMPO MAX DE ESPERA DA PESSOA
					pthread_mutex_unlock(&mutexVariaveisSimulacao);

					while (tempoEspera <= pessoa->tempoMaximoEspera && parque.numeroPessoasNoEstacionamento >= simConfiguration.lotEstacionamento)
					{
						pthread_mutex_lock(&mutexVariaveisSimulacao);
						tempoEspera = current_timestamp() - pessoa->tempoChegadaFilaEspera; // TEMPO MAX DE ESPERA DA PESSOA
						pthread_mutex_unlock(&mutexVariaveisSimulacao);
						strcpy(mensagem, "ESTACIONAMENTO CHEIO.");
					};

					if (tempoEspera > pessoa->tempoMaximoEspera)
					{

						pessoa->desistiu = TRUE;
						pthread_mutex_lock(&mutexPessoasEstacionamento);
						parque.numeroPessoaEsperaNoEstacionamento--;
						sem_post(&parque.filaEstacionamento);
						pthread_mutex_unlock(&mutexPessoasEstacionamento);
						printf(VERMELHO "O visitante %d desistiu. Passou muito tempo a espera no estacionamento. %s\n", pessoa->id, mensagem);
						sendMessage("8");
					}
					else
					{

						pessoa->noEstacionamento = TRUE;
						printf(MAGENTA "O visitante %d conseguiu estacionar.\n", pessoa->id);
						pthread_mutex_lock(&mutexPessoasEstacionamento);
						parque.numeroPessoaEsperaNoEstacionamento--;
						parque.numeroPessoasNoEstacionamento++;
						pthread_mutex_unlock(&mutexPessoasEstacionamento);
						sendMessage("8");
						sendMessage("3");
						sem_post(&parque.filaEstacionamento);
						WaitingListWaterPark(pessoa);
					}
				}
			}
			else
			{
				pessoa->desistiu = TRUE;
				printf(VERMELHO "O visitante %d desistiu. ESTACIONAMENTO CHEIO.\n", pessoa->id);
			}
		}
	}
	else
	{
		WaitingListWaterPark(pessoa);
	}
}

struct Person createPerson()
{
	struct Person person;

	person.id = idPessoa++;

	person.prioritario = numeroAleatorio(10, 0) == 0 ? true : false;
	person.precisaEstacionamento = numeroAleatorio(1, 0) == 0 ? true : false;
	person.noParque = false;
	person.noEstacionamento = false;
	person.desistiu = false;
	person.sitio = NENHUM;
	person.tempoMaximoEspera = numeroAleatorio(simConfiguration.tempoMedioDeEspera, (simConfiguration.tempoMedioDeEspera * 3) / 4);
	person.precisaCacifo = numeroAleatorio(1, 0) == 0 ? true : false;

	return person;
}

void *Person()
{

	sem_wait(&semCriarPessoa);
	struct Person onePerson = createPerson();
	printf(RESET "Um visitante quer entrar no parque. Numero %d.\n", onePerson.id);
	sendMessage("1");
	sem_post(&semCriarPessoa);
	createdPeople[onePerson.id] = &onePerson;
	char buffer[BUF_SIZE];
	WaitingListParking(&onePerson);

	while (TRUE)
	{
		if (!onePerson.desistiu)
		{
			UsePark(&onePerson);
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
	int semValue;
	for (idx = 0; idx < numeroAleatorio(1000, 700); idx++)
	{
		if (pthread_create(&IDthread[idPessoa], NULL, Person, NULL))
		{

			printf("Erro na criação da tarefa \n");
			exit(1);
		}
	}

	usleep(2000000);
	sendMessage("!");
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

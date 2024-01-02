#include "config.h"

#define CONFFILE "simulador.conf"

// GLOBAL VARIABLES
int sockfd = 0;
int newsockfd = 0;
int idPessoa = 0;
int minutosDecorridos = 0;
int tempoLimite;
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
	bool precisaCabana;
	bool alugouCabana;
};

bool pistasRapidasEmAndamento;

struct Parque
{
	int numeroPessoaEspera;
	int numeroPessoasNoParque;
	int numeroPessoaEsperaNoEstacionamento;
	int numeroPessoasNoEstacionamento;
	int numeroCacifosOcupados;
	int numeroCabanasOcupadas;
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
	CACIFOS,
	CABANAS,
	PISTASRAPIDAS,
	TOBOGAN,
	PISCINA,
	RIOLENTO,
	ESCORREGA,
	BALNEARIOS,
	ENFERMARIA,
	TOTAL
};

// TRINCOS E SEMAFOROS
pthread_mutex_t mutexCriarPessoa = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexPessoasParque = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexPessoasEstacionamento = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexVariaveisSimulacao = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexPessoasCacifos = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexPessoasCabanas = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexPessoasPistasRapidas = PTHREAD_MUTEX_INITIALIZER;
sem_t mutexMensagens;
sem_t semCriarPessoa;

void atribuirConfiguracao(char **results)
{

	simConfiguration.tempoMedioDeEspera = atoi(results[0]);
	simConfiguration.capAtracoes = atoi(results[1]);
	simConfiguration.capBalnearios = atoi(results[2]);
	simConfiguration.capCacifos = atoi(results[3]);
	simConfiguration.capCabanas = atoi(results[4]);
	simConfiguration.probSairFilaEntrada = strtof(results[5], NULL);
	simConfiguration.tobogansFunci = (strcmp(results[6], "Sim") > 0) ? true : false;
	simConfiguration.piscinaFunci = (strcmp(results[7], "Sim") > 0) ? true : false;
	simConfiguration.pistasFunci = (strcmp(results[8], "Sim") > 0) ? true : false;
	simConfiguration.escorregaFunci = (strcmp(results[9], "Sim") > 0) ? true : false;
	simConfiguration.rioLentoFunci = (strcmp(results[10], "Sim") > 0) ? true : false;
	simConfiguration.probEntrarNumaAtracao = strtof(results[11], NULL);
	simConfiguration.probSairSemUmaAtracao = strtof(results[12], NULL);
	simConfiguration.probPessoaFerir = strtof(results[13], NULL);
	simConfiguration.lotEstacionamento = atoi(results[14]);
	simConfiguration.lotParque = atoi(results[15]);
	simConfiguration.probSairSemEstacionamento = strtof(results[16], NULL);

	return;
};

long long current_timestamp()
{
	struct timeval te;
	gettimeofday(&te, NULL);										 // get current time
	long long milliseconds = te.tv_sec * 1000LL + te.tv_usec / 1000; // calculate milliseconds
	return milliseconds;
}

void startSemaphoresAndLatches()
{

	sem_init(&semCriarPessoa, 0, 1);
	sem_init(&parque.filaParque, 0, 1);
	sem_init(&parque.filaEstacionamento, 0, 1);
	sem_init(&mutexMensagens, 0, 1);

	parque.numeroPessoaEspera = 0;
	parque.numeroPessoaEsperaNoEstacionamento = 0;
	parque.numeroPessoasNoEstacionamento = 0;
	parque.numeroPessoasNoParque = 0;
	parque.numeroCacifosOcupados = 0;
	parque.numeroCabanasOcupadas = 0;
	sem_init(&parque.filaSitios[BALNEARIOS], 0, simConfiguration.capBalnearios);
	sem_init(&parque.filaSitios[CACIFOS], 0, simConfiguration.capCacifos);
	sem_init(&parque.filaSitios[CABANAS], 0, simConfiguration.capCabanas);
	sem_init(&parque.filaSitios[ENFERMARIA], 0, 10);
	sem_init(&parque.filaSitios[PISTASRAPIDAS], 0, 4);
	sem_init(&parque.filaSitios[TOBOGAN], 0, 2);
	sem_init(&parque.filaSitios[RIOLENTO], 0, simConfiguration.capAtracoes);
	sem_init(&parque.filaSitios[PISCINA], 0, simConfiguration.capAtracoes);
	sem_init(&parque.filaSitios[ESCORREGA], 0, 1);

	pistasRapidasEmAndamento = false;
	parque.capacidadeAtualSitios[PISTASRAPIDAS] = 0;

	minutosDecorridos = current_timestamp();
	tempoLimite = current_timestamp() + HORA * 7;
}

int numeroAleatorio(int numeroMaximo, int numeroMinimo)
{
	return rand() % (numeroMaximo + 1 - numeroMinimo) + numeroMinimo;
}

int probabilidade(float prob)
{ // Função probabilidade, retorna true ou false
	return numeroAleatorio(100, 0) < (prob * 100);
}

char *distinguirPrioritario(struct Person *person)
{
	return (person->prioritario) ? "PRIORITÁRIO" : "";
}

void sendMessage(char *sendingMessage, long long timestamp)
{
	char timestampStr[BUF_SIZE];
	snprintf(timestampStr, sizeof(timestampStr), "%lld", timestamp);

	// Concatenate the timestamp and the original message
	char combinedMessage[BUF_SIZE * 2];
	snprintf(combinedMessage, sizeof(combinedMessage), "%s|%s-", sendingMessage, timestampStr);

	// Send the combined message over the socket
	write(newsockfd, combinedMessage, strlen(combinedMessage));
}

void UsePark(struct Person *pessoa)
{
	char mensagem[BUF_SIZE] = "";

	int sitioEscolhido = numeroAleatorio(TOTAL - 2, 1);
	if (sitioEscolhido == BALNEARIOS && probabilidade(0.7))
	{
		sitioEscolhido = numeroAleatorio(TOTAL - 2, 1);
	}
	long long enterTimeStamp = current_timestamp();
	switch (pessoa->sitio)
	{
	case NENHUM:
		if (minutosDecorridos >= tempoLimite)
		{
			printf(AZUL "O visitante %d divirtiu-se e saiu quando o parque fechou!\n", pessoa->id);
			break;
		}
		pessoa->sitio = sitioEscolhido;
		break;
	case TOBOGAN:
		if (simConfiguration.tobogansFunci)
		{
			if (probabilidade(simConfiguration.probEntrarNumaAtracao))
			{
				pessoa->sitio = TOBOGAN;
				long long timestamp = current_timestamp();
				sem_wait(&parque.filaSitios[TOBOGAN]);
				if (minutosDecorridos >= tempoLimite)
				{
					sem_post(&parque.filaSitios[TOBOGAN]);
					break;
				}
				printf(AMARELO "O visitante %d divertiu-se nos TOBOGANS.\n" RESET, pessoa->id);
				sendMessage("C", current_timestamp() - timestamp);
				sem_post(&parque.filaSitios[TOBOGAN]);
				if (probabilidade(simConfiguration.probPessoaFerir))
				{
					pessoa->sitio = ENFERMARIA;
					break;
				}
			}
		}
		else
		{
			if (probabilidade(simConfiguration.probSairSemUmaAtracao))
			{
				pessoa->desistiu = TRUE;
				printf(VERMELHO "O visitante %d saiu do parque. Não tinha TOBOGANS. %s\n", pessoa->id, mensagem);
				sendMessage(":", 0);
				if (pessoa->noEstacionamento)
				{

					pthread_mutex_lock(&mutexPessoasEstacionamento);
					parque.numeroPessoasNoEstacionamento--;
					pthread_mutex_unlock(&mutexPessoasEstacionamento);
					sendMessage("4", 0);
				}
				if (pessoa->alugouCacifo)
				{
					pthread_mutex_lock(&mutexPessoasCacifos);
					parque.numeroCacifosOcupados--;
					pthread_mutex_unlock(&mutexPessoasCacifos);
					sendMessage("B", 0);
				}
				if (pessoa->alugouCabana)
				{
					pthread_mutex_lock(&mutexPessoasCabanas);
					parque.numeroCabanasOcupadas--;
					pthread_mutex_unlock(&mutexPessoasCabanas);
					sendMessage("I", 0);
				}
			}
		}
		pessoa->sitio = NENHUM;
		break;
	case PISTASRAPIDAS:
		if (simConfiguration.pistasFunci)
		{
			if (probabilidade(simConfiguration.probEntrarNumaAtracao))
			{

				int semValue;
				pessoa->sitio = PISTASRAPIDAS;
				long long timestamp = current_timestamp();
				sem_wait(&parque.filaSitios[PISTASRAPIDAS]);
				if (minutosDecorridos >= tempoLimite)
				{
					sem_post(&parque.filaSitios[PISTASRAPIDAS]);
					break;
				}
				pthread_mutex_lock(&mutexPessoasPistasRapidas);
				int capacidadeAtual = parque.capacidadeAtualSitios[PISTASRAPIDAS];
				pthread_mutex_unlock(&mutexPessoasPistasRapidas);
				while (pistasRapidasEmAndamento || capacidadeAtual == 4)
				{
				}
				pthread_mutex_lock(&mutexPessoasPistasRapidas);
				parque.capacidadeAtualSitios[PISTASRAPIDAS]++;
				if (parque.capacidadeAtualSitios[PISTASRAPIDAS] == 4)
				{
					pistasRapidasEmAndamento = true;
				}
				pthread_mutex_unlock(&mutexPessoasPistasRapidas);
				while (!pistasRapidasEmAndamento)
				{
				}
				printf(AMARELO "O visitante %d divertiu-se nas PISTAS RÁPIDAS.\n" RESET, pessoa->id);
				sendMessage("D", current_timestamp() - timestamp);

				pthread_mutex_lock(&mutexPessoasPistasRapidas);

				parque.capacidadeAtualSitios[PISTASRAPIDAS]--;
				if (parque.capacidadeAtualSitios[PISTASRAPIDAS] == 0)
				{
					pistasRapidasEmAndamento = false;
				}
				pthread_mutex_unlock(&mutexPessoasPistasRapidas);

				while (pistasRapidasEmAndamento)
				{
				};

				sem_post(&parque.filaSitios[PISTASRAPIDAS]);

				if (probabilidade(simConfiguration.probPessoaFerir))
				{
					pessoa->sitio = ENFERMARIA;
					break;
				}
			}
		}
		else
		{
			if (probabilidade(simConfiguration.probSairSemUmaAtracao))
			{
				pessoa->desistiu = TRUE;
				printf(VERMELHO "O visitante %d saiu do parque. Não tinha TOBOGANS. %s\n", pessoa->id, mensagem);
				sendMessage(":", 0);
				if (pessoa->noEstacionamento)
				{

					pthread_mutex_lock(&mutexPessoasEstacionamento);
					parque.numeroPessoasNoEstacionamento--;
					pthread_mutex_unlock(&mutexPessoasEstacionamento);
					sendMessage("4", 0);
				}
				if (pessoa->alugouCacifo)
				{
					pthread_mutex_lock(&mutexPessoasCacifos);
					parque.numeroCacifosOcupados--;
					pthread_mutex_unlock(&mutexPessoasCacifos);
					sendMessage("B", 0);
				}
				if (pessoa->alugouCabana)
				{
					pthread_mutex_lock(&mutexPessoasCabanas);
					parque.numeroCabanasOcupadas--;
					pthread_mutex_unlock(&mutexPessoasCabanas);
					sendMessage("I", 0);
				}
			}
		}
		pessoa->sitio = NENHUM;
		break;
	case PISCINA:
		if (simConfiguration.piscinaFunci)
		{
			if (probabilidade(simConfiguration.probEntrarNumaAtracao))
			{
				pessoa->sitio = PISCINA;
				long long timestamp = current_timestamp();
				sem_wait(&parque.filaSitios[PISCINA]);
				if (minutosDecorridos >= tempoLimite)
				{
					sem_post(&parque.filaSitios[PISCINA]);
					break;
				}
				printf(AMARELO "O visitante %d divertiu-se na PISCINA.\n" RESET, pessoa->id);
				sendMessage("E", current_timestamp() - timestamp);
				sem_post(&parque.filaSitios[PISCINA]);

				if (probabilidade(simConfiguration.probPessoaFerir))
				{
					pessoa->sitio = ENFERMARIA;
					break;
				}
			}
		}
		else
		{
			if (probabilidade(simConfiguration.probSairSemUmaAtracao))
			{
				pessoa->desistiu = TRUE;
				printf(VERMELHO "O visitante %d saiu do parque. Não tinha PISCINA. %s\n", pessoa->id, mensagem);
				sendMessage(":", 0);
				if (pessoa->noEstacionamento)
				{

					pthread_mutex_lock(&mutexPessoasEstacionamento);
					parque.numeroPessoasNoEstacionamento--;
					pthread_mutex_unlock(&mutexPessoasEstacionamento);
					sendMessage("4", 0);
				}
				if (pessoa->alugouCacifo)
				{
					pthread_mutex_lock(&mutexPessoasCacifos);
					parque.numeroCacifosOcupados--;
					pthread_mutex_unlock(&mutexPessoasCacifos);
					sendMessage("B", 0);
				}
				if (pessoa->alugouCabana)
				{
					pthread_mutex_lock(&mutexPessoasCabanas);
					parque.numeroCabanasOcupadas--;
					pthread_mutex_unlock(&mutexPessoasCabanas);
					sendMessage("I", 0);
				}
			}
		}
		pessoa->sitio = NENHUM;
		break;
	case ESCORREGA:
		if (simConfiguration.escorregaFunci)
		{
			if (probabilidade(simConfiguration.probEntrarNumaAtracao))
			{
				pessoa->sitio = ESCORREGA;
				long long timestamp = current_timestamp();
				sem_wait(&parque.filaSitios[ESCORREGA]);
				if (minutosDecorridos >= tempoLimite)
				{
					sem_post(&parque.filaSitios[ESCORREGA]);
					break;
				}
				printf(AMARELO "O visitante %d divertiu-se nos ESCORREGAS.\n" RESET, pessoa->id);
				sendMessage("F", current_timestamp() - timestamp);
				sem_post(&parque.filaSitios[ESCORREGA]);

				if (probabilidade(simConfiguration.probPessoaFerir))
				{
					pessoa->sitio = ENFERMARIA;
					break;
				}
			}
		}
		else
		{
			if (probabilidade(simConfiguration.probSairSemUmaAtracao))
			{
				pessoa->desistiu = TRUE;
				printf(VERMELHO "O visitante %d saiu do parque. Não tinha ESCORREGAS. %s\n", pessoa->id, mensagem);
				sendMessage(":", 0);
				if (pessoa->noEstacionamento)
				{
					pthread_mutex_lock(&mutexPessoasEstacionamento);
					parque.numeroPessoasNoEstacionamento--;
					pthread_mutex_unlock(&mutexPessoasEstacionamento);
					sendMessage("4", 0);
				}
				if (pessoa->alugouCacifo)
				{
					pthread_mutex_lock(&mutexPessoasCacifos);
					parque.numeroCacifosOcupados--;
					pthread_mutex_unlock(&mutexPessoasCacifos);
					sendMessage("B", 0);
				}
				if (pessoa->alugouCabana)
				{
					pthread_mutex_lock(&mutexPessoasCabanas);
					parque.numeroCabanasOcupadas--;
					pthread_mutex_unlock(&mutexPessoasCabanas);
					sendMessage("I", 0);
				}
			}
		}
		pessoa->sitio = NENHUM;
		break;
	case RIOLENTO:
		if (simConfiguration.escorregaFunci)
		{
			if (probabilidade(simConfiguration.probEntrarNumaAtracao))
			{

				pessoa->sitio = RIOLENTO;
				long long timestamp = current_timestamp();
				sem_wait(&parque.filaSitios[RIOLENTO]);
				if (minutosDecorridos >= tempoLimite)
				{
					sem_post(&parque.filaSitios[RIOLENTO]);
					break;
				}
				printf(AMARELO "O visitante %d divertiu-se na RIOLENTO.\n" RESET, pessoa->id);
				sendMessage("G", current_timestamp() - timestamp);
				sem_post(&parque.filaSitios[RIOLENTO]);

				if (probabilidade(simConfiguration.probPessoaFerir))
				{
					pessoa->sitio = ENFERMARIA;
					break;
				}
			}
		}
		else
		{
			if (probabilidade(simConfiguration.probSairSemUmaAtracao))
			{
				pessoa->desistiu = TRUE;
				printf(VERMELHO "O visitante %d saiu do parque. Não tinha RIOLENTO. %s\n", pessoa->id, mensagem);
				sendMessage(":", 0);
				if (pessoa->noEstacionamento)
				{

					pthread_mutex_lock(&mutexPessoasEstacionamento);
					parque.numeroPessoasNoEstacionamento--;
					pthread_mutex_unlock(&mutexPessoasEstacionamento);
					sendMessage("4", 0);
				}
				if (pessoa->alugouCacifo)
				{
					pthread_mutex_lock(&mutexPessoasCacifos);
					parque.numeroCacifosOcupados--;
					pthread_mutex_unlock(&mutexPessoasCacifos);
					sendMessage("B", 0);
				}
				if (pessoa->alugouCabana)
				{
					pthread_mutex_lock(&mutexPessoasCabanas);
					parque.numeroCabanasOcupadas--;
					pthread_mutex_unlock(&mutexPessoasCabanas);
					sendMessage("I", 0);
				}
			}
		}
		pessoa->sitio = NENHUM;
		break;
	case ENFERMARIA:
		long long timestampEnf = current_timestamp();
		sem_wait(&parque.filaSitios[ENFERMARIA]);
		if (minutosDecorridos >= tempoLimite)
		{
			sem_post(&parque.filaSitios[ENFERMARIA]);
			break;
		}
		printf(MAGENTA "O visitante %d feriu-se e foi para a ENFERMARIA.\n", pessoa->id);
		sendMessage("J", current_timestamp() - timestampEnf);
		sem_post(&parque.filaSitios[ENFERMARIA]);
		pessoa->sitio = NENHUM;
		break;
	case BALNEARIOS:
		long long timestamp = current_timestamp();
		sem_wait(&parque.filaSitios[BALNEARIOS]);
		if (minutosDecorridos >= tempoLimite)
		{
			sem_post(&parque.filaSitios[BALNEARIOS]);
			break;
		}
		printf(CIANO "O visitante %d foi %s nos BALNEARIOS.\n", pessoa->id, probabilidade(0.33) ? "defecar" : probabilidade(0.5) ? "urinar"
																																 : "trocar de roupa");
		sendMessage("K", current_timestamp() - timestamp);
		sem_post(&parque.filaSitios[BALNEARIOS]);
		pessoa->sitio = NENHUM;
		break;
	default:
		pessoa->sitio = NENHUM;
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
					sendMessage("4", 0);
				}
			}
			else
			{
				pessoa->tempoChegadaFilaEspera = current_timestamp();
				pthread_mutex_lock(&mutexPessoasParque);
				parque.numeroPessoaEspera++;
				pthread_mutex_unlock(&mutexPessoasParque);
				sendMessage("5", 0);
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
					sendMessage("6", 0);
					if (pessoa->noEstacionamento)
					{
						pthread_mutex_lock(&mutexPessoasEstacionamento);
						parque.numeroPessoasNoEstacionamento--;
						pthread_mutex_unlock(&mutexPessoasEstacionamento);
						sendMessage("4", 0);
					}
					if (pessoa->alugouCacifo)
					{
						pthread_mutex_lock(&mutexPessoasCacifos);
						parque.numeroCacifosOcupados--;
						pthread_mutex_unlock(&mutexPessoasCacifos);
						sendMessage("B", 0);
					}
					if (pessoa->alugouCabana)
					{
						pthread_mutex_lock(&mutexPessoasCabanas);
						parque.numeroCabanasOcupadas--;
						pthread_mutex_unlock(&mutexPessoasCabanas);
						sendMessage("I", 0);
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
					sendMessage("6", 0);
					sendMessage("9", current_timestamp() - pessoa->tempoChegadaFilaEspera);
					sem_post(&parque.filaParque);
					if (pessoa->precisaCacifo && parque.numeroCacifosOcupados < simConfiguration.capCacifos && !pessoa->alugouCacifo)
					{
						sem_wait(&parque.filaSitios[CACIFOS]);
						pthread_mutex_lock(&mutexPessoasCacifos);
						parque.numeroCacifosOcupados++;
						pthread_mutex_unlock(&mutexPessoasCacifos);
						sem_post(&parque.filaSitios[CACIFOS]);
						printf(CIANO "O visitante %s %d alugou cacifo.\n", distinguirPrioritario(pessoa), pessoa->id);
						sendMessage("A", 0);
						pessoa->alugouCacifo = true;
					}

					if (pessoa->precisaCabana && parque.numeroCabanasOcupadas < simConfiguration.capCabanas && !pessoa->alugouCabana)
					{
						sem_wait(&parque.filaSitios[CABANAS]);
						pthread_mutex_lock(&mutexPessoasCabanas);
						parque.numeroCabanasOcupadas++;
						pthread_mutex_unlock(&mutexPessoasCabanas);
						sem_post(&parque.filaSitios[CABANAS]);
						printf(CIANO "O visitante %s %d alugou cabana.\n", distinguirPrioritario(pessoa), pessoa->id);
						sendMessage("H", 0);
						pessoa->alugouCabana = true;
					}
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
			sendMessage("9", 0);
			if (pessoa->precisaCacifo && parque.numeroCacifosOcupados < simConfiguration.capCacifos && !pessoa->alugouCacifo)
			{
				sem_wait(&parque.filaSitios[CACIFOS]);
				pthread_mutex_lock(&mutexPessoasCacifos);
				parque.numeroCacifosOcupados++;
				pthread_mutex_unlock(&mutexPessoasCacifos);
				sem_post(&parque.filaSitios[CACIFOS]);
				printf(CIANO "O visitante %s %d alugou cacifo.\n", distinguirPrioritario(pessoa), pessoa->id);
				sendMessage("A", 0);
				pessoa->alugouCacifo = true;
			}

			if (pessoa->precisaCabana && parque.numeroCabanasOcupadas < simConfiguration.capCabanas && !pessoa->alugouCabana)
			{
				sem_wait(&parque.filaSitios[CABANAS]);
				pthread_mutex_lock(&mutexPessoasCabanas);
				parque.numeroCabanasOcupadas++;
				pthread_mutex_unlock(&mutexPessoasCabanas);
				sem_post(&parque.filaSitios[CABANAS]);
				printf(CIANO "O visitante %s %d alugou cabana.\n", distinguirPrioritario(pessoa), pessoa->id);
				sendMessage("H", 0);
				pessoa->alugouCabana = true;
			}
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
					sendMessage("7", 0);
					pessoa->tempoChegadaFilaEspera = current_timestamp();
					printf("%lld", current_timestamp());
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
						sendMessage("8", 0);
					}
					else
					{

						pessoa->noEstacionamento = TRUE;
						printf(MAGENTA "O visitante %d conseguiu estacionar.\n", pessoa->id);
						pthread_mutex_lock(&mutexPessoasEstacionamento);
						parque.numeroPessoaEsperaNoEstacionamento--;
						parque.numeroPessoasNoEstacionamento++;
						pthread_mutex_unlock(&mutexPessoasEstacionamento);
						sendMessage("8", 0);
						printf("%lld", current_timestamp());
						sendMessage("3", current_timestamp() - pessoa->tempoChegadaFilaEspera);
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
	person.precisaCabana = numeroAleatorio(4, 0) == 0 ? true : false;

	return person;
}

void *Person()
{
	sem_wait(&semCriarPessoa);
	struct Person onePerson = createPerson();
	printf(RESET "Um visitante quer entrar no parque. Numero %d.\n", onePerson.id);
	sendMessage("1", 0);
	sem_post(&semCriarPessoa);
	createdPeople[onePerson.id] = &onePerson;
	char buffer[BUF_SIZE];
	WaitingListParking(&onePerson);
	if (onePerson.desistiu)
		return NULL;
	while (TRUE)
	{
		usleep(300000);
		if (!onePerson.desistiu)
		{
			if (minutosDecorridos >= tempoLimite)
			{
				printf(AZUL "O visitante %d divirtiu-se e saiu quando o parque fechou!\n", onePerson.id);
				break;
			}

			UsePark(&onePerson);
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
	int auxTimeStamp, numeroDia = 1;

	int idx;
	char sendingMessage[BUF_SIZE];
	int semValue;
	int numeroPessoas = numeroAleatorio(600, 200);

	printf("\nO PARQUE ESTÁ ABERTO. VAI COMEÇAR A SIMULAÇÃO.\n");
	usleep(5000000);

	for (idx = 0; idx < numeroPessoas; idx++)
	{
		if (pthread_create(&IDthread[idPessoa], NULL, Person, NULL))
		{

			printf("Erro na criação da tarefa \n");
			exit(1);
		}
	}

	while (minutosDecorridos <= tempoLimite)
	{
		minutosDecorridos = current_timestamp();
	}

	usleep(12000000);
	for (idx = 0; idx < numeroPessoas; idx++)
	{
		if (pthread_join(IDthread[numeroPessoas], NULL))
		{
		}
	}
	sendMessage("Z", 0);
	printf(RESET "\nO PARQUE ESTÁ ENCERRADO. SIMULAÇÃO TERMINADA!\n");
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
		system("clear");
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

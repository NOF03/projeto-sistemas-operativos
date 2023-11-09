#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <string.h>
#include <regex.h>
#include <stdbool.h>

#define BUF_SIZE 512
#define STDIN 0

#define FALSE 0
#define TRUE 1

#define UNIXSTR_PATH "/tmp/s.2082021"

struct simConfig {
    int tempoChegada;
    int simDias;
	int tamMaxFilaAtracoes;
	float probSairFilaEntrada;
	float probSairAtracoes;
	bool tobogansFunci;
	bool piscinaFunci;
	bool pistasFunci;
	float probSairSemUmaAtracao;
	int lotEstacionamento;
	int lotParque;
	float probSairSemEstacionamento;
};

struct monConfig {
    char* nomeParque;
};

char** carregarConfiguracao(char* filename) {

    FILE* conf = fopen(filename, "r");

    if (conf == NULL) {
        printf("Ocorreu um erro na abertura do ficheiro: %s\n", strerror(errno));
        return NULL;
    }

    regex_t regex;
    int ret;
    regmatch_t rm[2];
    const char* pattern = ":(.*)$";
    char* line = (char*)malloc(BUF_SIZE * sizeof(char));
    int result_count = 0;
    char** results = (char**)malloc(BUF_SIZE * sizeof(char*));
    char* buffer = (char*)malloc(BUF_SIZE * BUF_SIZE * sizeof(char));
    for (int i = 0; i < BUF_SIZE; i++) 
        results[i] = &buffer[i * BUF_SIZE];
        
    if (regcomp(&regex, pattern, REG_EXTENDED) != 0) {
        fprintf(stderr, "Could not compile regex\n");
        return NULL;
    }

    while (fgets(line, BUF_SIZE, conf) != NULL) {
        if ((ret = regexec(&regex, line, 2, rm, 0)) == 0) {
            strncpy(results[result_count], line + rm[1].rm_so, (size_t)(rm[1].rm_eo - rm[1].rm_so));
            results[result_count][rm[1].rm_eo - rm[1].rm_so] = '\0';
            result_count++;  
        }
    }

    regfree(&regex);
    fclose(conf);

    return results;
};


int readn(int fd, char *ptr, int nbytes)
{
	int nleft, nread;

	nleft = BUF_SIZE;
	while (nleft > 0)
	{
		nread = read(fd, ptr, nleft);
		if (nread < 0)
			return (nread);
		else if (nread == 0)
			break;

		nleft -= nread;
		ptr += nread;
	}
	return (nbytes - nleft);
}

/* Escreve nbytes num ficheiro/socket.
   Bloqueia até conseguir escrever os nbytes ou dar erro */
int writen(int fd, char *ptr, int nbytes)
{
	int nleft, nwritten;

	nleft = nbytes;
	while (nleft > 0)
	{
		nwritten = write(fd, ptr, nleft);
		if (nwritten <= 0)
			return (nwritten);

		nleft -= nwritten;
		ptr += nwritten;
	}
	return (nbytes - nleft);
}

/* Lê uma linha de um ficheiro/socket (até \n, maxlen ou \0).
   Bloqueia até ler a linha ou dar erro.
   Retorna quantos caracteres conseguiu ler */
int readline(int fd, char *ptr, int maxlen)
{
	int n, rc;
	char c;

	for (n = 1; n < maxlen; n++)
	{
		if ((rc = read(fd, &c, 1)) == 1)
		{
			*ptr++ = c;
			if (c == '\n')
				break;
		}
		else if (rc == 0)
		{
			if (n == 1)
				return (0);
			else
				break;
		}
		else
			return (-1);
	}

	/* Não esquecer de terminar a string */
	*ptr = 0;

	/* Note-se que n foi incrementado de modo a contar
	   com o \n ou \0 */
	return (n);
}

void err_dump(char *msg)
{
	perror(msg);
	exit(1);
}

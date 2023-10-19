#include "config.h"

#define CONFFILE "monitor.conf"

int carregarConfiguracao(char* filename) {
    char buf[BUF_SIZE];
    int bytes_read, bytes_written;
    int fd;

    if (filename != CONFFILE) {
        return -1;
    }

    bytes_read = read(STDIN, buf, BUF_SIZE);
    if (bytes_read < 0) {
        printf("Ocorreu um erro na leitura: %s\n", strerror(errno));
        return -1;
    }

    fd = open(filename, O_RDONLY, S_IRWXU);
    if (fd < 0) {
        printf("Ocorreu um erro na abertura do ficheiro: %s\n", strerror(errno));
        return -1;
    }

    close(fd);

    return 1;
}

int main(int argc, char *argv[]) {

    carregarConfiguracao(argv[0]);

}

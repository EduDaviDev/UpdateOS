#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <getopt.h>
#include <string.h>

#define UEX_HEADER_SIZE 11

typedef struct {
    uint32_t stack_size;
    uint32_t entry_offset;
    char *input_file;
    char *output_file;
} Options;

void print_usage(const char *prog) {
    fprintf(stderr, "Uso: %s --stack STACK --entry ENTRY --input BIN --output UEX\n", prog);
}

int parse_options(int argc, char **argv, Options *opts) {
    static struct option long_opts[] = {
        {"stack", required_argument, 0, 's'},
        {"entry", required_argument, 0, 'e'},
        {"input", required_argument, 0, 'i'},
        {"output", required_argument, 0, 'o'},
        {"help", no_argument, 0, 'h'},
        {0,0,0,0}
    };
    int c;
    opts->stack_size = 1024;
    opts->entry_offset = 0x10;
    opts->input_file = NULL;
    opts->output_file = NULL;
    while ((c = getopt_long(argc, argv, "s:e:i:o:h", long_opts, NULL)) != -1) {
        switch (c) {
            case 's': opts->stack_size = strtoul(optarg, NULL, 0); break;
            case 'e': opts->entry_offset = strtoul(optarg, NULL, 0); break;
            case 'i': opts->input_file = optarg; break;
            case 'o': opts->output_file = optarg; break;
            case 'h': print_usage(argv[0]); return 0;
            default: print_usage(argv[0]); return -1;
        }
    }
    if (!opts->input_file || !opts->output_file) {
        fprintf(stderr, "Erro: --input e --output são obrigatórios.\n");
        return -1;
    }
    return 1;
}

void write_uint32_le(FILE *f, uint32_t val) {
    fputc(val & 0xFF, f);
    fputc((val>>8)&0xFF, f);
    fputc((val>>16)&0xFF, f);
    fputc((val>>24)&0xFF, f);
}

int main(int argc, char **argv) {
    Options opts;
    int ret = parse_options(argc, argv, &opts);
    if (ret <= 0) return ret == 0 ? 0 : 1;

    // Lê o binário do app
    FILE *in = fopen(opts.input_file, "rb");
    if (!in) { perror("Erro ao abrir entrada"); return 1; }
    fseek(in, 0, SEEK_END);
    long bin_size = ftell(in);
    fseek(in, 0, SEEK_SET);
    unsigned char *bin_data = malloc(bin_size);
    if (!bin_data) { fclose(in); return 1; }
    fread(bin_data, 1, bin_size, in);
    fclose(in);

    // Calcula o padding necessário para que o código comece em entry_offset
    if (opts.entry_offset < UEX_HEADER_SIZE) {
        fprintf(stderr, "Aviso: entry_offset (%u) < cabeçalho (%d). Ajustando para %d.\n",
                opts.entry_offset, UEX_HEADER_SIZE, UEX_HEADER_SIZE);
        opts.entry_offset = UEX_HEADER_SIZE;
    }
    int padding = opts.entry_offset - UEX_HEADER_SIZE;

    // Abre saída
    FILE *out = fopen(opts.output_file, "wb");
    if (!out) { free(bin_data); perror("Erro ao criar saída"); return 1; }

    // Escreve cabeçalho (11 bytes)
    fwrite("UEX", 1, 3, out);
    write_uint32_le(out, opts.stack_size);
    write_uint32_le(out, opts.entry_offset);

    // Padding com zeros (se necessário)
    if (padding > 0) {
        unsigned char *zeros = calloc(1, padding);
        fwrite(zeros, 1, padding, out);
        free(zeros);
    }

    // Escreve o binário do app
    fwrite(bin_data, 1, bin_size, out);

    fclose(out);
    free(bin_data);

    printf("UEX gerado: %s (stack=%u, entry=%u, padding=%d, total_size=%ld)\n",
           opts.output_file, opts.stack_size, opts.entry_offset, padding,
           bin_size + UEX_HEADER_SIZE + padding);
    return 0;
}
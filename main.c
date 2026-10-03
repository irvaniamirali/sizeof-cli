#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

#define SOURCE_SIZE 16384
#define DEFAULT_HEADERS_FILE "headers.csrc"

static void die(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

static void append(char *source, size_t *used, const char *text)
{
    size_t len = strlen(text);

    if (*used > SOURCE_SIZE - len - 1) {
        fprintf(stderr, "generated source is too large\n");
        exit(EXIT_FAILURE);
    }

    memcpy(source + *used, text, len);
    *used += len;
    source[*used] = '\0';
}

static void include_header(char *source,
                           size_t *used,
                           const char *header,
                           int local)
{
    append(source, used, "#include ");

    if (local)
        append(source, used, "\"");
    else
        append(source, used, "<");

    append(source, used, header);

    if (local)
        append(source, used, "\"\n");
    else
        append(source, used, ">\n");
}

static void load_headers(char *source,
                         size_t *used,
                         const char *filename)
{
    FILE *file = fopen(filename, "r");

    if (file == NULL)
        die(filename);

    char line[4096];

    while (fgets(line, sizeof(line), file) != NULL) {
        line[strcspn(line, "\r\n")] = '\0';

        if (line[0] == '\0')
            continue;

        include_header(source, used, line, 0);
    }

    if (ferror(file)) {
        fclose(file);
        die("fgets");
    }

    fclose(file);
}

static void usage(const char *program)
{
    fprintf(stderr,
        "usage: %s [options] <type>\n"
        "\n"
        "Options:\n"
        "  -i, --include-system HEADER\n"
        "      Add #include <HEADER>\n"
        "\n"
        "  -I, --include-local HEADER\n"
        "      Add #include \"HEADER\"\n"
        "\n"
        "  -H, --add-headers-list FILE\n"
        "      Use FILE as the header list\n"
        "      (default: %s)\n",
        program,
        DEFAULT_HEADERS_FILE
    );
}

static void write_all(int fd, const char *buffer, size_t size)
{
    size_t written = 0;

    while (written < size) {
        ssize_t n = write(
            fd,
            buffer + written,
            size - written
        );

        if (n == -1) {
            if (errno == EINTR)
                continue;

            die("write");
        }

        written += (size_t)n;
    }
}

int main(int argc, char **argv)
{
    static const struct option long_options[] = {
        { "include-system",   required_argument, NULL, 'i' },
        { "include-local",    required_argument, NULL, 'I' },
        { "add-headers-list", required_argument, NULL, 'H' },
        { NULL,               0,                 NULL,  0  }
    };

    char option_headers[SOURCE_SIZE] = {0};
    size_t option_headers_used = 0;

    const char *headers_file = DEFAULT_HEADERS_FILE;

    int option;

    while ((option = getopt_long(
        argc,
        argv,
        "i:I:H:",
        long_options,
        NULL
    )) != -1) {

        switch (option) {
        case 'i':
            include_header(
                option_headers,
                &option_headers_used,
                optarg,
                0
            );
            break;

        case 'I':
            include_header(
                option_headers,
                &option_headers_used,
                optarg,
                1
            );
            break;

        case 'H':
            headers_file = optarg;
            break;

        default:
            usage(argv[0]);
            return EXIT_FAILURE;
        }
    }

    if (optind >= argc) {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    const char *type = argv[optind];

    char file_headers[SOURCE_SIZE] = {0};
    size_t file_headers_used = 0;

    load_headers(
        file_headers,
        &file_headers_used,
        headers_file
    );

    char final_source[SOURCE_SIZE] = {0};
    size_t final_used = 0;

    include_header(
        final_source,
        &final_used,
        "stdio.h",
        0
    );

    append(
        final_source,
        &final_used,
        file_headers
    );

    append(
        final_source,
        &final_used,
        option_headers
    );

    append(
        final_source,
        &final_used,
        "\n"
        "int main(void)\n"
        "{\n"
        "    printf(\"%zu\\n\", sizeof("
    );

    append(
        final_source,
        &final_used,
        type
    );

    append(
        final_source,
        &final_used,
        "));\n"
        "    return 0;\n"
        "}\n"
    );

    int input[2];

    if (pipe(input) == -1)
        die("pipe");

    int executable = memfd_create("sizeof", 0);

    if (executable == -1)
        die("memfd_create");

    pid_t pid = fork();

    if (pid == -1)
        die("fork");

    if (pid == 0) {
        if (dup2(input[0], STDIN_FILENO) == -1)
            die("dup2");

        close(input[0]);
        close(input[1]);

        if (dup2(executable, 3) == -1)
            die("dup2");

        close(executable);

        execlp(
            "cc",
            "cc",
            "-x", "c",
            "-",
            "-o", "/proc/self/fd/3",
            NULL
        );

        perror("cc");
        _exit(127);
    }

    close(input[0]);

    write_all(
        input[1],
        final_source,
        final_used
    );

    close(input[1]);

    int status;

    if (waitpid(pid, &status, 0) == -1)
        die("waitpid");

    if (!WIFEXITED(status) ||
        WEXITSTATUS(status) != 0) {
        close(executable);
        return EXIT_FAILURE;
    }

    char *const child_argv[] = {
        "sizeof",
        NULL
    };

    char *const child_envp[] = {
        NULL
    };

    fexecve(
        executable,
        child_argv,
        child_envp
    );

    die("fexecve");
}
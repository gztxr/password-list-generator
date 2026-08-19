#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/random.h>
#include <unistd.h>

#define DEFAULT_LENGTH 16
#define DEFAULT_COUNT 10
#define MIN_PASSWORD_LENGTH 1
#define MAX_PASSWORD_LENGTH 256
#define MAX_COUNT 1000000
#define MAX_OUTPUT_FILENAME 4096

#define LOWERCASE "abcdefghijklmnopqrstuvwxyz"
#define UPPERCASE "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define DIGITS "0123456789"
#define SPECIAL "!@#$%^&*()-_=+[]{}:,.?/"
#define AMBIGUOUS "il1Lo0O"

typedef struct {
    bool lowercase;
    bool uppercase;
    bool digits;
    bool special;
    bool require_lowercase;
    bool require_uppercase;
    bool require_digits;
    bool require_special;
    bool exclude_ambiguous;
    bool variable_length;
    int length_min;
    int length_max;
    int count;
    bool stdout_mode;
    char output_file[MAX_OUTPUT_FILENAME];
    bool unique;
} GeneratorConfig;

typedef struct {
    char **items;
    size_t count;
    size_t capacity;
} PasswordSet;

static void print_error(const char *message)
{
    fprintf(stderr, "Error: %s\n", message);
}

static bool parse_positive_int(
    const char *value,
    long min,
    long max,
    int *result)
{
    char *end = NULL;
    long parsed;

    if (value == NULL || *value == '\0')
        return false;

    errno = 0;
    parsed = strtol(value, &end, 10);

    if (errno == ERANGE ||
        end == value ||
        *end != '\0' ||
        parsed < min ||
        parsed > max)
        return false;

    *result = (int)parsed;
    return true;
}

static bool secure_random_bytes(void *buffer, size_t length)
{
    unsigned char *ptr = buffer;

    while (length > 0) {
        ssize_t result = getrandom(ptr, length, 0);

        if (result < 0) {
            if (errno == EINTR)
                continue;

            return false;
        }

        if (result == 0)
            return false;

        ptr += result;
        length -= (size_t)result;
    }

    return true;
}

static bool secure_random_bounded(
    size_t upper_bound,
    size_t *result)
{
    unsigned int random_value;
    unsigned int limit;

    if (upper_bound == 0 ||
        upper_bound > (size_t)UINT_MAX)
        return false;

    limit = UINT_MAX -
            (UINT_MAX % (unsigned int)upper_bound);

    do {
        if (!secure_random_bytes(
                &random_value,
                sizeof(random_value)))
            return false;
    } while (random_value >= limit);

    *result = random_value % upper_bound;

    return true;
}

static bool is_ambiguous(char c)
{
    return strchr(AMBIGUOUS, c) != NULL;
}

static bool character_allowed(
    char c,
    const GeneratorConfig *config)
{
    if (config->exclude_ambiguous &&
        is_ambiguous(c))
        return false;

    return true;
}

static size_t build_character_pool(
    const GeneratorConfig *config,
    char *pool,
    size_t pool_size)
{
    size_t length = 0;

    const char *sets[4];
    size_t set_count = 0;

    if (config->lowercase)
        sets[set_count++] = LOWERCASE;

    if (config->uppercase)
        sets[set_count++] = UPPERCASE;

    if (config->digits)
        sets[set_count++] = DIGITS;

    if (config->special)
        sets[set_count++] = SPECIAL;

    for (size_t i = 0; i < set_count; ++i) {
        const char *set = sets[i];

        for (size_t j = 0; set[j] != '\0'; ++j) {
            char c = set[j];

            if (!character_allowed(c, config))
                continue;

            if (length + 1 >= pool_size)
                return 0;

            pool[length++] = c;
        }
    }

    pool[length] = '\0';

    return length;
}

static bool random_character_from_set(
    const char *set,
    const GeneratorConfig *config,
    char *result)
{
    char filtered[256];
    size_t count = 0;
    size_t index;

    for (size_t i = 0; set[i] != '\0'; ++i) {
        if (character_allowed(set[i], config))
            filtered[count++] = set[i];
    }

    if (count == 0)
        return false;

    if (!secure_random_bounded(count, &index))
        return false;

    *result = filtered[index];

    return true;
}

static bool random_character_from_pool(
    const char *pool,
    size_t pool_length,
    char *result)
{
    size_t index;

    if (pool_length == 0)
        return false;

    if (!secure_random_bounded(pool_length, &index))
        return false;

    *result = pool[index];

    return true;
}

static bool get_password_length(
    const GeneratorConfig *config,
    int *length)
{
    if (!config->variable_length) {
        *length = config->length_min;
        return true;
    }

    size_t range =
        (size_t)(config->length_max -
                 config->length_min + 1);

    size_t offset;

    if (!secure_random_bounded(range, &offset))
        return false;

    *length = config->length_min + (int)offset;

    return true;
}

static bool generate_password(
    const GeneratorConfig *config,
    char *password,
    size_t password_size)
{
    char pool[512];
    size_t pool_length;
    int length;

    if (!get_password_length(config, &length))
        return false;

    if (length < MIN_PASSWORD_LENGTH ||
        length > MAX_PASSWORD_LENGTH ||
        password_size < (size_t)length + 1)
        return false;

    pool_length = build_character_pool(
        config,
        pool,
        sizeof(pool));

    if (pool_length == 0)
        return false;

    int required = 0;

    if (config->require_lowercase)
        required++;

    if (config->require_uppercase)
        required++;

    if (config->require_digits)
        required++;

    if (config->require_special)
        required++;

    if (required > length)
        return false;

    size_t position = 0;

    if (config->require_lowercase) {
        if (!random_character_from_set(
                LOWERCASE,
                config,
                &password[position++]))
            return false;
    }

    if (config->require_uppercase) {
        if (!random_character_from_set(
                UPPERCASE,
                config,
                &password[position++]))
            return false;
    }

    if (config->require_digits) {
        if (!random_character_from_set(
                DIGITS,
                config,
                &password[position++]))
            return false;
    }

    if (config->require_special) {
        if (!random_character_from_set(
                SPECIAL,
                config,
                &password[position++]))
            return false;
    }

    while (position < (size_t)length) {
        if (!random_character_from_pool(
                pool,
                pool_length,
                &password[position]))
            return false;

        position++;
    }

    password[length] = '\0';

    for (size_t i = (size_t)length - 1; i > 0; --i) {
        size_t j;

        if (!secure_random_bounded(i + 1, &j))
            return false;

        char temp = password[i];
        password[i] = password[j];
        password[j] = temp;
    }

    return true;
}

static void password_set_free(PasswordSet *set)
{
    if (set == NULL)
        return;

    for (size_t i = 0; i < set->count; ++i)
        free(set->items[i]);

    free(set->items);

    set->items = NULL;
    set->count = 0;
    set->capacity = 0;
}

static bool password_set_contains(
    const PasswordSet *set,
    const char *password)
{
    for (size_t i = 0; i < set->count; ++i) {
        if (strcmp(set->items[i], password) == 0)
            return true;
    }

    return false;
}

static bool password_set_add(
    PasswordSet *set,
    const char *password)
{
    if (password_set_contains(set, password))
        return false;

    if (set->count == set->capacity) {
        size_t new_capacity =
            set->capacity == 0
                ? 128
                : set->capacity * 2;

        char **new_items =
            realloc(
                set->items,
                new_capacity * sizeof(*new_items));

        if (new_items == NULL)
            return false;

        set->items = new_items;
        set->capacity = new_capacity;
    }

    set->items[set->count] = strdup(password);

    if (set->items[set->count] == NULL)
        return false;

    set->count++;

    return true;
}

static void print_help(const char *program)
{
    printf(
        "\n"
        "Secure Password Generator\n"
        "=========================\n\n"
        "Usage:\n"
        "  %s [OPTIONS]\n\n"
        "Length:\n"
        "  -l, --length N       Password length (default: %d)\n"
        "  -v, --variable       Random length between --min and --max\n"
        "      --min N          Minimum variable length (default: 8)\n"
        "      --max N          Maximum variable length (default: 32)\n\n"
        "Character classes:\n"
        "      --lowercase      Allow lowercase letters\n"
        "      --uppercase      Allow uppercase letters\n"
        "      --digits         Allow digits\n"
        "      --special        Allow special characters\n"
        "  -a, --all            Allow all character classes\n\n"
        "Required classes:\n"
        "      --require-lowercase\n"
        "      --require-uppercase\n"
        "      --require-digits\n"
        "      --require-special\n\n"
        "Other:\n"
        "  -c, --count N        Number of passwords (default: %d)\n"
        "      --no-ambiguous   Exclude il1Lo0O\n"
        "      --unique         Do not output duplicates\n"
        "      --stdout         Write passwords to stdout\n"
        "  -o, --output FILE    Write passwords to FILE\n"
        "  -h, --help           Show this help\n\n",
        program,
        DEFAULT_LENGTH,
        DEFAULT_COUNT
    );
}

static bool validate_config(
    const GeneratorConfig *config)
{
    if (config->length_min < MIN_PASSWORD_LENGTH ||
        config->length_min > MAX_PASSWORD_LENGTH) {
        print_error("invalid minimum password length");
        return false;
    }

    if (config->length_max < MIN_PASSWORD_LENGTH ||
        config->length_max > MAX_PASSWORD_LENGTH) {
        print_error("invalid maximum password length");
        return false;
    }

    if (config->length_max < config->length_min) {
        print_error(
            "--max must be greater than or equal to --min");
        return false;
    }

    if (config->count < 1 ||
        config->count > MAX_COUNT) {
        print_error("invalid count");
        return false;
    }

    if (!config->lowercase &&
        !config->uppercase &&
        !config->digits &&
        !config->special) {
        print_error(
            "at least one character class must be enabled");
        return false;
    }

    int required = 0;

    if (config->require_lowercase)
        required++;

    if (config->require_uppercase)
        required++;

    if (config->require_digits)
        required++;

    if (config->require_special)
        required++;

    if (config->length_min < required) {
        print_error(
            "minimum password length is too small for "
            "the required character classes");
        return false;
    }

    if (config->exclude_ambiguous) {
        char pool[512];

        if (build_character_pool(
                config,
                pool,
                sizeof(pool)) == 0) {
            print_error(
                "character set is empty after "
                "excluding ambiguous characters");
            return false;
        }
    }

    if (config->stdout_mode &&
        config->output_file[0] != '\0') {
        print_error(
            "--stdout and --output cannot be used together");
        return false;
    }

    return true;
}

static bool parse_args(
    int argc,
    char **argv,
    GeneratorConfig *config)
{
    static const struct option long_options[] = {
        {"length",             required_argument, 0, 'l'},
        {"count",              required_argument, 0, 'c'},
        {"variable",           no_argument,       0, 'v'},
        {"min",                required_argument, 0, 1},
        {"max",                required_argument, 0, 2},
        {"lowercase",          no_argument,       0, 3},
        {"uppercase",          no_argument,       0, 4},
        {"digits",             no_argument,       0, 5},
        {"special",            no_argument,       0, 6},
        {"all",                no_argument,       0, 'a'},
        {"require-lowercase",  no_argument,       0, 7},
        {"require-uppercase",  no_argument,       0, 8},
        {"require-digits",     no_argument,       0, 9},
        {"require-special",    no_argument,       0, 10},
        {"no-ambiguous",       no_argument,       0, 11},
        {"unique",             no_argument,       0, 12},
        {"stdout",             no_argument,       0, 13},
        {"output",             required_argument, 0, 'o'},
        {"help",               no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };

    int option_index = 0;
    int option;

    while ((option = getopt_long(
                argc,
                argv,
                "l:c:vao:h",
                long_options,
                &option_index)) != -1) {

        switch (option) {

        case 'l':
            if (!parse_positive_int(
                    optarg,
                    MIN_PASSWORD_LENGTH,
                    MAX_PASSWORD_LENGTH,
                    &config->length_min)) {
                print_error("invalid password length");
                return false;
            }

            config->length_max = config->length_min;
            break;

        case 'c':
            if (!parse_positive_int(
                    optarg,
                    1,
                    MAX_COUNT,
                    &config->count)) {
                print_error("invalid count");
                return false;
            }
            break;

        case 'v':
            config->variable_length = true;
            break;

        case 'a':
            config->lowercase = true;
            config->uppercase = true;
            config->digits = true;
            config->special = true;

            config->require_lowercase = true;
            config->require_uppercase = true;
            config->require_digits = true;
            config->require_special = true;
            break;

        case 'o':
            if (strlen(optarg) >= MAX_OUTPUT_FILENAME) {
                print_error("output filename is too long");
                return false;
            }

            strcpy(config->output_file, optarg);
            break;

        case 'h':
            print_help(argv[0]);
            exit(EXIT_SUCCESS);

        case 1:
            if (!parse_positive_int(
                    optarg,
                    MIN_PASSWORD_LENGTH,
                    MAX_PASSWORD_LENGTH,
                    &config->length_min)) {
                print_error("invalid minimum length");
                return false;
            }
            break;

        case 2:
            if (!parse_positive_int(
                    optarg,
                    MIN_PASSWORD_LENGTH,
                    MAX_PASSWORD_LENGTH,
                    &config->length_max)) {
                print_error("invalid maximum length");
                return false;
            }
            break;

        case 3:
            config->lowercase = true;
            break;

        case 4:
            config->uppercase = true;
            break;

        case 5:
            config->digits = true;
            break;

        case 6:
            config->special = true;
            break;

        case 7:
            config->lowercase = true;
            config->require_lowercase = true;
            break;

        case 8:
            config->uppercase = true;
            config->require_uppercase = true;
            break;

        case 9:
            config->digits = true;
            config->require_digits = true;
            break;

        case 10:
            config->special = true;
            config->require_special = true;
            break;

        case 11:
            config->exclude_ambiguous = true;
            break;

        case 12:
            config->unique = true;
            break;

        case 13:
            config->stdout_mode = true;
            break;

        case '?':
        default:
            print_error("invalid command-line option");
            return false;
        }
    }

    if (optind < argc) {
        fprintf(
            stderr,
            "Error: unexpected argument '%s'\n",
            argv[optind]);

        return false;
    }

    return true;
}

static FILE *open_output(
    const GeneratorConfig *config)
{
    if (config->stdout_mode)
        return stdout;

    const char *filename =
        config->output_file[0] != '\0'
            ? config->output_file
            : "passwords.txt";

    int fd = open(
        filename,
        O_WRONLY | O_CREAT | O_TRUNC,
        0600);

    if (fd < 0)
        return NULL;

    FILE *file = fdopen(fd, "w");

    if (file == NULL) {
        close(fd);
        return NULL;
    }

    return file;
}

int main(int argc, char **argv)
{
    GeneratorConfig config = {
        .lowercase = true,
        .uppercase = true,
        .digits = true,
        .special = false,

        .require_lowercase = false,
        .require_uppercase = false,
        .require_digits = false,
        .require_special = false,

        .exclude_ambiguous = false,

        .variable_length = false,
        .length_min = DEFAULT_LENGTH,
        .length_max = DEFAULT_LENGTH,

        .count = DEFAULT_COUNT,

        .stdout_mode = false,
        .output_file = {0},

        .unique = false
    };

    if (!parse_args(argc, argv, &config)) {
        fprintf(
            stderr,
            "Try '%s --help' for usage.\n",
            argv[0]);

        return EXIT_FAILURE;
    }

    if (!validate_config(&config))
        return EXIT_FAILURE;

    FILE *output = open_output(&config);

    if (output == NULL) {
        fprintf(
            stderr,
            "Error: could not open output file '%s': %s\n",
            config.output_file[0] != '\0'
                ? config.output_file
                : "passwords.txt",
            strerror(errno));

        return EXIT_FAILURE;
    }

    PasswordSet generated = {0};

    char password[MAX_PASSWORD_LENGTH + 1];

    int generated_count = 0;
    int attempts = 0;

    const int max_attempts =
        config.unique
            ? config.count * 100
            : config.count;

    while (generated_count < config.count) {

        if (attempts >= max_attempts) {
            print_error(
                "unable to generate enough unique passwords");

            password_set_free(&generated);

            if (output != stdout)
                fclose(output);

            return EXIT_FAILURE;
        }

        attempts++;

        if (!generate_password(
                &config,
                password,
                sizeof(password))) {

            print_error("password generation failed");

            password_set_free(&generated);

            if (output != stdout)
                fclose(output);

            return EXIT_FAILURE;
        }

        if (config.unique) {
            if (password_set_contains(
                    &generated,
                    password)) {
                continue;
            }

            if (!password_set_add(
                    &generated,
                    password)) {

                print_error(
                    "failed to maintain duplicate set");

                password_set_free(&generated);

                if (output != stdout)
                    fclose(output);

                return EXIT_FAILURE;
            }
        }

        if (fprintf(output, "%s\n", password) < 0) {
            print_error("failed to write output");

            password_set_free(&generated);

            if (output != stdout)
                fclose(output);

            return EXIT_FAILURE;
        }

        generated_count++;

        if (!config.stdout_mode &&
            generated_count % 1000 == 0) {

            fprintf(
                stderr,
                "Generated %d/%d\n",
                generated_count,
                config.count);
        }
    }

    if (fflush(output) != 0) {
        print_error("failed to flush output");

        password_set_free(&generated);

        if (output != stdout)
            fclose(output);

        return EXIT_FAILURE;
    }

    if (output != stdout) {
        if (fclose(output) != 0) {
            print_error("failed to close output file");

            password_set_free(&generated);

            return EXIT_FAILURE;
        }
    }

    password_set_free(&generated);

    volatile char *secure_clear = password;

    for (size_t i = 0; i < sizeof(password); ++i)
        secure_clear[i] = '\0';

    if (!config.stdout_mode) {
        printf(
            "Successfully generated %d password(s).\n",
            generated_count);

        printf(
            "Output: %s\n",
            config.output_file[0] != '\0'
                ? config.output_file
                : "passwords.txt");
    }

    return EXIT_SUCCESS;
}

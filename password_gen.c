#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>

#define DEFAULT_LENGTH 12
#define DEFAULT_COUNT 10
#define MAX_WORDS 1000
#define MAX_WORD_LENGTH 50
#define MAX_PASSWORD_LENGTH 256
#define MAX_FILENAME 256

typedef struct {
    bool use_lowercase;
    bool use_uppercase;
    bool use_digits;
    bool use_special;
    bool use_words;
    bool use_common_passwords;
    bool variable_length;
    int length_min;
    int length_max;
    int count;
    bool exclude_ambiguous;
    char words[MAX_WORDS][MAX_WORD_LENGTH];
    int word_count;
    char output_file[MAX_FILENAME];
} GeneratorConfig;

const char* FIRST_NAMES[] = {
    "alex", "john", "mike", "david", "james", "robert", "mark", "steven", "paul", "andrew",
    "chris", "jason", "ryan", "kevin", "brian", "daniel", "matthew", "anthony", "donald", "timothy",
    "sarah", "jessica", "emily", "rachel", "nicole", "amanda", "jennifer", "melissa", "laura", "kelly",
    "megan", "ashley", "taylor", "courtney", "lauren", "brittany", "alexis", "jordan", "madison", "hannah"
};

const char* LAST_NAMES[] = {
    "smith", "johnson", "williams", "brown", "jones", "garcia", "miller", "davis", "rodriguez", "martinez",
    "anderson", "taylor", "thomas", "jackson", "white", "harris", "martin", "thompson", "moore", "walker",
    "king", "wright", "lopez", "hill", "scott", "green", "adams", "baker", "gonzalez", "nelson",
    "carter", "mitchell", "perez", "roberts", "turner", "phillips", "campbell", "parker", "evans", "edwards"
};

const char* YEARS[] = {"1986", "1987", "1988", "1989", "1990", "1991", "1992", "1993", "1994", "1995",
    "1996", "1997", "1998", "1999", "2000", "2001", "2002", "2003", "2004", "2005",
    "2006", "2007", "2008", "2009", "2010", "2011", "2012", "2013", "2014", "2015",
    "2016", "2017", "2018", "2019", "2020", "2021", "2022", "2023", "2024", "2025"
};

const char* COMMON_WORDS[] = {
    "password", "admin", "root", "guest", "user", "support", "default", "changeme",
    "qwerty", "abc123", "letmein", "monkey", "dragon", "master", "login", "pass",
    "iloveyou", "princess", "rockyou", "sunshine", "love", "angel", "honey", "baby",
    "super", "star", "king", "queen", "lord", "sir", "lady", "dude", "bro", "mama", "papa"
};

const char* NUMBERS[] = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15",
    "16", "17", "18", "19", "20", "21", "22", "23", "24", "25", "26", "27", "28", "29", "30",
    "31", "32", "33", "34", "35", "36", "37", "38", "39", "40", "50", "60", "70", "80", "90", "100"
};

const char* COMMON_PASSWORDS[] = {
    "admin", "password", "123456", "12345678", "1234", "12345", 
    "root", "guest", "user", "support", "default", "changeme",
    "password123", "admin123", "adminadmin", "123456789", "qwerty", 
    "abc123", "letmein", "monkey", "dragon", "master", "login",
    "pass", "admin1", "administrator", "p@ssw0rd", "P@ssw0rd",
    "123123", "654321", "111111", "000000", "password1", "passw0rd",
    "adminpass", "root123", "guest123", "ubnt", "UBNT", "airport",
    "camera", "ipcam", "admin1234", "1234567890", "qwertyuiop",
    "asdfghjkl", "zxcvbnm", "1q2w3e4r", "1qaz2wsx", "qwerty123",
    "admin_password", "default_password", "null", "none", "toor",
    "raspberry", "pi", "alpine", "debian", "ubuntu", "centos",
    "oracle", "postgres", "mysql", "mongo", "redis", "elastic",
    "kibana", "logstash", "hadoop", "spark", "kafka", "zookeeper",
    "cassandra", "couchdb", "rabbitmq", "activemq", "openvpn",
    "cisco", "Cisco", "router", "switch", "ap", "wifi", "wireless",
    "network", "internet", "gateway", "firewall", "proxy", "vpn"
};

#define FIRST_NAMES_COUNT (sizeof(FIRST_NAMES) / sizeof(FIRST_NAMES[0]))
#define LAST_NAMES_COUNT (sizeof(LAST_NAMES) / sizeof(LAST_NAMES[0]))
#define YEARS_COUNT (sizeof(YEARS) / sizeof(YEARS[0]))
#define COMMON_WORDS_COUNT (sizeof(COMMON_WORDS) / sizeof(COMMON_WORDS[0]))
#define NUMBERS_COUNT (sizeof(NUMBERS) / sizeof(NUMBERS[0]))
#define COMMON_PASSWORDS_COUNT (sizeof(COMMON_PASSWORDS) / sizeof(COMMON_PASSWORDS[0]))

void print_help(const char* program_name);
void generate_realistic_password(char* password, int length);
void generate_password_with_words(const GeneratorConfig* config, char* password);
bool parse_args(int argc, char* argv[], GeneratorConfig* config);
int get_random_length(const GeneratorConfig* config);
void generate_filename(char* filename, size_t size);
char* get_absolute_path(const char* filename);

void print_help(const char* program_name) {
    printf("\nPassword List Generator v2.0\n");
    printf("Generates realistic passwords using names, years, and common patterns\n\n");
    
    printf("USAGE:\n");
    printf("  %s [OPTIONS]\n\n", program_name);
    
    printf("BASIC OPTIONS:\n");
    printf("  -l, --length N        Password length (default: %d)\n", DEFAULT_LENGTH);
    printf("  -c, --count N         Number of passwords to generate (default: %d)\n", DEFAULT_COUNT);
    printf("  -v, --variable        Generate variable length passwords\n");
    printf("  --min N               Minimum length when using -v (default: 8)\n");
    printf("  --max N               Maximum length when using -v (default: 20)\n");
    printf("  -o, --output FILE     Output file name (default: passwords_YYYYMMDD_HHMMSS.txt)\n");
    printf("  -h, --help            Show this help message\n\n");
    
    printf("CHARACTER SETS:\n");
    printf("  -a, --all             Use ALL character types\n");
    printf("  --lowercase           Include lowercase letters (a-z)\n");
    printf("  --uppercase           Include uppercase letters (A-Z)\n");
    printf("  --digits              Include digits (0-9)\n");
    printf("  --special             Include special characters (!@#$...)\n");
    printf("  --no-ambiguous        Exclude ambiguous chars (il1Lo0O)\n\n");
    
    printf("DICTIONARY & WORD OPTIONS:\n");
    printf("  -w, --words           Use dictionary words in passwords\n");
    printf("  -W, --add-word WORD   Add custom word to dictionary (can use multiple times)\n");
    printf("  -f, --word-file FILE  Load words from a file (one per line)\n");
    printf("  -C, --common          Include common/default passwords\n\n");
    
    printf("EXAMPLES:\n");
    printf("  %s                      Generate 10 realistic passwords\n", program_name);
    printf("  %s -c 100               Generate 100 realistic passwords\n", program_name);
    printf("  %s -l 16 -c 5          Generate 5 passwords length 16\n", program_name);
    printf("  %s -W myword --all -c 10\n", program_name);
    printf("  %s -v --min 8 --max 16 -c 20\n\n", program_name);
}

int get_random_length(const GeneratorConfig* config) {
    if (config->variable_length) {
        return config->length_min + (rand() % (config->length_max - config->length_min + 1));
    }
    return config->length_min;
}

void generate_realistic_password(char* password, int length) {
    char temp[MAX_PASSWORD_LENGTH];
    int pattern = rand() % 8;
    
    if (pattern == 0) {
        const char* first = FIRST_NAMES[rand() % FIRST_NAMES_COUNT];
        const char* last = LAST_NAMES[rand() % LAST_NAMES_COUNT];
        const char* year = YEARS[rand() % YEARS_COUNT];
        snprintf(temp, sizeof(temp), "%s%s%s", first, last, year);
    }
    else if (pattern == 1) {
        const char* first = FIRST_NAMES[rand() % FIRST_NAMES_COUNT];
        const char* last = LAST_NAMES[rand() % LAST_NAMES_COUNT];
        const char* year = YEARS[rand() % YEARS_COUNT];
        snprintf(temp, sizeof(temp), "%s_%s_%s", first, last, year);
    }
    else if (pattern == 2) {
        const char* word = COMMON_WORDS[rand() % COMMON_WORDS_COUNT];
        const char* number = NUMBERS[rand() % NUMBERS_COUNT];
        const char* year = YEARS[rand() % YEARS_COUNT];
        snprintf(temp, sizeof(temp), "%s%s%s", word, number, year);
    }
    else if (pattern == 3) {
        const char* first = FIRST_NAMES[rand() % FIRST_NAMES_COUNT];
        const char* word = COMMON_WORDS[rand() % COMMON_WORDS_COUNT];
        const char* number = NUMBERS[rand() % NUMBERS_COUNT];
        snprintf(temp, sizeof(temp), "%s%s%s", first, word, number);
    }
    else if (pattern == 4) {
        const char* word1 = COMMON_WORDS[rand() % COMMON_WORDS_COUNT];
        const char* word2 = COMMON_WORDS[rand() % COMMON_WORDS_COUNT];
        const char* number = NUMBERS[rand() % NUMBERS_COUNT];
        snprintf(temp, sizeof(temp), "%s%s%s", word1, word2, number);
    }
    else if (pattern == 5) {
        const char* first = FIRST_NAMES[rand() % FIRST_NAMES_COUNT];
        const char* year = YEARS[rand() % YEARS_COUNT];
        snprintf(temp, sizeof(temp), "%s%s", first, year);
    }
    else if (pattern == 6) {
        const char* word = COMMON_WORDS[rand() % COMMON_WORDS_COUNT];
        const char* year = YEARS[rand() % YEARS_COUNT];
        snprintf(temp, sizeof(temp), "%s%s", word, year);
    }
    else {
        const char* first = FIRST_NAMES[rand() % FIRST_NAMES_COUNT];
        const char* last = LAST_NAMES[rand() % LAST_NAMES_COUNT];
        snprintf(temp, sizeof(temp), "%s%s", first, last);
    }
    
    int temp_len = strlen(temp);
    
    if (temp_len < length) {
        int need = length - temp_len;
        int add_numbers = need / 2;
        int add_chars = need - add_numbers;
        
        if (add_numbers > 0) {
            for (int i = 0; i < add_numbers && temp_len < MAX_PASSWORD_LENGTH; i++) {
                temp[temp_len++] = '0' + (rand() % 10);
            }
        }
        
        if (add_chars > 0) {
            char* extra_chars = "abcdefghijklmnopqrstuvwxyz";
            for (int i = 0; i < add_chars && temp_len < MAX_PASSWORD_LENGTH; i++) {
                if (rand() % 3 == 0) {
                    temp[temp_len++] = 'A' + (rand() % 26);
                } else {
                    temp[temp_len++] = extra_chars[rand() % 26];
                }
            }
        }
        temp[temp_len] = '\0';
    }
    else if (temp_len > length) {
        temp[length] = '\0';
    }
    
    strcpy(password, temp);
}

void generate_password_with_words(const GeneratorConfig* config, char* password) {
    char temp_pass[MAX_PASSWORD_LENGTH];
    int target_length = get_random_length(config);
    
    char word_pool[MAX_WORDS][MAX_WORD_LENGTH];
    int pool_size = 0;
    
    for (int i = 0; i < config->word_count && pool_size < MAX_WORDS; i++) {
        strcpy(word_pool[pool_size++], config->words[i]);
    }
    
    if (config->use_common_passwords) {
        for (int i = 0; i < COMMON_PASSWORDS_COUNT && pool_size < MAX_WORDS; i++) {
            strcpy(word_pool[pool_size++], COMMON_PASSWORDS[i]);
        }
    }
    
    if (config->word_count > 0 || config->use_common_passwords) {
        int num_words = 1 + (rand() % 2);
        if (num_words > pool_size) num_words = pool_size;
        
        char selected_words[MAX_WORDS][MAX_WORD_LENGTH];
        int used_indices[MAX_WORDS] = {0};
        int total_word_len = 0;
        
        for (int i = 0; i < num_words; i++) {
            int idx;
            do {
                idx = rand() % pool_size;
            } while (used_indices[idx]);
            used_indices[idx] = 1;
            strcpy(selected_words[i], word_pool[idx]);
            total_word_len += strlen(word_pool[idx]);
        }
        
        int remaining_len = target_length - total_word_len;
        if (remaining_len < 0) {
            strcpy(temp_pass, selected_words[0]);
            temp_pass[target_length] = '\0';
            strcpy(password, temp_pass);
            return;
        }
        
        int pos = 0;
        
        for (int i = 0; i < num_words; i++) {
            strcpy(&temp_pass[pos], selected_words[i]);
            pos += strlen(selected_words[i]);
            
            if (i < num_words - 1 && rand() % 2 == 0) {
                temp_pass[pos++] = '_';
            }
        }
        
        if (remaining_len > 0 && config->use_digits) {
            int num_digits = 1 + (rand() % 4);
            if (num_digits > remaining_len) num_digits = remaining_len;
            
            for (int i = 0; i < num_digits; i++) {
                temp_pass[pos++] = '0' + (rand() % 10);
            }
            remaining_len -= num_digits;
        }
        
        if (remaining_len > 0 && config->use_uppercase) {
            int num_upper = 1 + (rand() % 2);
            if (num_upper > remaining_len) num_upper = remaining_len;
            
            for (int i = 0; i < num_upper; i++) {
                temp_pass[pos++] = 'A' + (rand() % 26);
            }
            remaining_len -= num_upper;
        }
        
        if (remaining_len > 0 && config->use_special) {
            int num_special = 1 + (rand() % 2);
            if (num_special > remaining_len) num_special = remaining_len;
            
            char* special_chars = "!@#$%^&*";
            for (int i = 0; i < num_special; i++) {
                temp_pass[pos++] = special_chars[rand() % 8];
            }
            remaining_len -= num_special;
        }
        
        if (remaining_len > 0) {
            for (int i = 0; i < remaining_len; i++) {
                temp_pass[pos++] = 'a' + (rand() % 26);
            }
        }
        
        temp_pass[pos] = '\0';
        
        int len = strlen(temp_pass);
        if (len > target_length) {
            temp_pass[target_length] = '\0';
        }
        
        strcpy(password, temp_pass);
    } else {
        generate_realistic_password(password, target_length);
    }
}

void generate_filename(char* filename, size_t size) {
    time_t t;
    struct tm* tm_info;
    time(&t);
    tm_info = localtime(&t);
    strftime(filename, size, "passwords_%Y%m%d_%H%M%S.txt", tm_info);
}

char* get_absolute_path(const char* filename) {
    static char path[1024];
    char cwd[1024];
    
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        snprintf(path, sizeof(path), "%s/%s", cwd, filename);
        return path;
    }
    return (char*)filename;
}

bool parse_args(int argc, char* argv[], GeneratorConfig* config) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_help(argv[0]);
            return false;
        } else if (strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--length") == 0) {
            if (i + 1 < argc) {
                config->length_min = atoi(argv[++i]);
                config->length_max = config->length_min;
                if (config->length_min < 1) {
                    printf("Error: Length must be at least 1\n");
                    return false;
                }
            }
        } else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--count") == 0) {
            if (i + 1 < argc) {
                config->count = atoi(argv[++i]);
                if (config->count < 1) {
                    printf("Error: Count must be at least 1\n");
                    return false;
                }
            }
        } else if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0) {
            if (i + 1 < argc) {
                strcpy(config->output_file, argv[++i]);
            }
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--variable") == 0) {
            config->variable_length = true;
        } else if (strcmp(argv[i], "--min") == 0) {
            if (i + 1 < argc) {
                config->length_min = atoi(argv[++i]);
                if (config->length_min < 1) {
                    printf("Error: Minimum length must be at least 1\n");
                    return false;
                }
            }
        } else if (strcmp(argv[i], "--max") == 0) {
            if (i + 1 < argc) {
                config->length_max = atoi(argv[++i]);
                if (config->length_max < config->length_min) {
                    printf("Error: Maximum length must be >= minimum length\n");
                    return false;
                }
            }
        } else if (strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--all") == 0) {
            config->use_lowercase = true;
            config->use_uppercase = true;
            config->use_digits = true;
            config->use_special = true;
        } else if (strcmp(argv[i], "--lowercase") == 0) {
            config->use_lowercase = true;
        } else if (strcmp(argv[i], "--uppercase") == 0) {
            config->use_uppercase = true;
        } else if (strcmp(argv[i], "--digits") == 0) {
            config->use_digits = true;
        } else if (strcmp(argv[i], "--special") == 0) {
            config->use_special = true;
        } else if (strcmp(argv[i], "--no-ambiguous") == 0) {
            config->exclude_ambiguous = true;
        } else if (strcmp(argv[i], "-w") == 0 || strcmp(argv[i], "--words") == 0) {
            config->use_words = true;
        } else if (strcmp(argv[i], "-W") == 0 || strcmp(argv[i], "--add-word") == 0) {
            if (i + 1 < argc) {
                if (config->word_count < MAX_WORDS) {
                    strcpy(config->words[config->word_count++], argv[++i]);
                } else {
                    printf("Warning: Max words reached, ignoring '%s'\n", argv[i]);
                }
            }
        } else if (strcmp(argv[i], "-C") == 0 || strcmp(argv[i], "--common") == 0) {
            config->use_common_passwords = true;
        } else if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--word-file") == 0) {
            if (i + 1 < argc) {
                FILE* file = fopen(argv[++i], "r");
                if (file) {
                    char line[MAX_WORD_LENGTH];
                    while (fgets(line, sizeof(line), file) && config->word_count < MAX_WORDS) {
                        line[strcspn(line, "\n")] = 0;
                        if (strlen(line) > 0) {
                            strcpy(config->words[config->word_count++], line);
                        }
                    }
                    fclose(file);
                    printf("Loaded %d words from file\n", config->word_count);
                } else {
                    printf("Error: Could not open file '%s'\n", argv[i]);
                    return false;
                }
            }
        } else {
            printf("Error: Unknown option '%s'\n", argv[i]);
            printf("Try '%s --help' for usage information\n", argv[0]);
            return false;
        }
    }
    return true;
}

int main(int argc, char* argv[]) {
    srand(time(NULL));
    
    GeneratorConfig config = {
        .use_lowercase = true,
        .use_uppercase = false,
        .use_digits = false,
        .use_special = false,
        .use_words = false,
        .use_common_passwords = false,
        .variable_length = false,
        .length_min = DEFAULT_LENGTH,
        .length_max = DEFAULT_LENGTH,
        .count = DEFAULT_COUNT,
        .exclude_ambiguous = false,
        .word_count = 0,
        .output_file = {0}
    };
    
    if (!parse_args(argc, argv, &config)) {
        return 1;
    }
    
    if (strlen(config.output_file) == 0) {
        generate_filename(config.output_file, sizeof(config.output_file));
    }
    
    FILE* file = fopen(config.output_file, "w");
    if (!file) {
        printf("Error: Could not create file '%s'\n", config.output_file);
        return 1;
    }
    
    printf("\nGenerating %d realistic passwords...\n", config.count);
    printf("  Length: ");
    if (config.variable_length) {
        printf("%d to %d (variable)\n", config.length_min, config.length_max);
    } else {
        printf("%d\n", config.length_min);
    }
    
    if (config.word_count > 0) printf("  Custom words: %d\n", config.word_count);
    if (config.use_common_passwords) printf("  Including common passwords\n");
    printf("\n");
    
    char password[MAX_PASSWORD_LENGTH];
    
    for (int i = 0; i < config.count; i++) {
        if (config.word_count > 0 || config.use_common_passwords) {
            generate_password_with_words(&config, password);
        } else {
            generate_realistic_password(password, get_random_length(&config));
        }
        fprintf(file, "%s\n", password);
        
        if ((i + 1) % 100 == 0) {
            printf("  Progress: %d/%d passwords generated\n", i + 1, config.count);
        }
    }
    
    fclose(file);
    
    char* abs_path = get_absolute_path(config.output_file);
    printf("\nSuccessfully generated %d passwords\n", config.count);
    printf("File saved to: %s\n\n", abs_path);
    
    return 0;
}

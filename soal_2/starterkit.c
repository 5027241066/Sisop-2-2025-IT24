#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <signal.h>

#define STARTER_KIT "starter_kit"
#define QUARANTINE "quarantine"
#define LOG_FILE "activity.log"
#define PID_FILE ".pid"

void create_directory_if_not_exists(const char *dir_name);
void create_log_if_not_exists();
void write_log(const char *message);
void quarantine_files();
void return_files();
void eradicate_files();
void decrypt_daemon();
void shutdown_daemon();
char* base64_decode(const char* input);

void create_directory_if_not_exists(const char *dir_name) {
    struct stat st = {0};
    if (stat(dir_name, &st) == -1) {
        mkdir(dir_name, 0755);
    }
}

void create_log_if_not_exists() {
    FILE *f = fopen(LOG_FILE, "a");
    if (f == NULL) {
        perror("Could not create log file");
        exit(EXIT_FAILURE);
    }
    fclose(f);
}

void write_log(const char *message) {
    FILE *f = fopen(LOG_FILE, "a");
    if (f == NULL) {
        perror("Could not write to log file");
        return;
    }

    time_t rawtime;
    struct tm *timeinfo;
    char buffer[80];

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(buffer, sizeof(buffer), "[%d-%m-%Y][%H:%M:%S]", timeinfo);
    fprintf(f, "%s - %s\n", buffer, message);
    fclose(f);
}

void quarantine_files() {
    DIR *d = opendir(STARTER_KIT);
    if (!d) {
        perror("Could not open starter_kit directory");
        return;
    }

    struct dirent *dir;
    while ((dir = readdir(d)) != NULL) {
        if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0) continue;

        char src_path[512], dest_path[512];
        snprintf(src_path, sizeof(src_path), "%s/%s", STARTER_KIT, dir->d_name);
        snprintf(dest_path, sizeof(dest_path), "%s/%s", QUARANTINE, dir->d_name);

        if (rename(src_path, dest_path) == 0) {
            char log_msg[600];
            snprintf(log_msg, sizeof(log_msg), "%s - Successfully moved to quarantine directory.", dir->d_name);
            write_log(log_msg);
        }
    }

    closedir(d);
}

void return_files() {
    DIR *d = opendir(QUARANTINE);
    if (!d) {
        perror("Could not open quarantine directory");
        return;
    }

    struct dirent *dir;
    while ((dir = readdir(d)) != NULL) {
        if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0) continue;

        char src_path[512], dest_path[512];
        snprintf(src_path, sizeof(src_path), "%s/%s", QUARANTINE, dir->d_name);
        snprintf(dest_path, sizeof(dest_path), "%s/%s", STARTER_KIT, dir->d_name);

        if (rename(src_path, dest_path) == 0) {
            char log_msg[600];
            snprintf(log_msg, sizeof(log_msg), "%s - Successfully returned to starter kit directory.", dir->d_name);
            write_log(log_msg);
        }
    }

    closedir(d);
}

void eradicate_files() {
    DIR *d = opendir(QUARANTINE);
    if (!d) {
        perror("Could not open quarantine directory");
        return;
    }

    struct dirent *dir;
    while ((dir = readdir(d)) != NULL) {
        if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0) continue;

        char filepath[512];
        snprintf(filepath, sizeof(filepath), "%s/%s", QUARANTINE, dir->d_name);

        if (remove(filepath) == 0) {
            char log_msg[600];
            snprintf(log_msg, sizeof(log_msg), "%s - Successfully deleted.", dir->d_name);
            write_log(log_msg);
        }
    }

    closedir(d);
}

void decrypt_daemon() {
    pid_t pid = fork();
    if (pid < 0) {
        perror("Failed to fork");
        exit(EXIT_FAILURE);
    }

    if (pid > 0) {
        printf("Daemon started with PID %d\n", pid);
        FILE *pf = fopen(PID_FILE, "w");
        if (pf) {
            fprintf(pf, "%d\n", pid);
            fclose(pf);
        }

        char msg[128];
        snprintf(msg, sizeof(msg), "Successfully started decryption process with PID %d.", pid);
        write_log(msg);
        exit(0);
    }

    setsid(); // detach
    chdir("/");
    fclose(stdin);
    fclose(stdout);
    fclose(stderr);

    while (1) {
        DIR *d = opendir(QUARANTINE);
        if (d) {
            struct dirent *dir;
            while ((dir = readdir(d)) != NULL) {
                if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0) continue;

                char old_path[512], new_path[512];
                snprintf(old_path, sizeof(old_path), "%s/%s", QUARANTINE, dir->d_name);
                char *decoded = base64_decode(dir->d_name);
                snprintf(new_path, sizeof(new_path), "%s/%s", QUARANTINE, decoded);

                rename(old_path, new_path);
                free(decoded);
            }
            closedir(d);
        }

        sleep(10); // scan tiap 10 detik
    }
}

void shutdown_daemon() {
    FILE *f = fopen(PID_FILE, "r");
    if (!f) {
        printf("No PID file found.\n");
        return;
    }

    int pid;
    fscanf(f, "%d", &pid);
    fclose(f);

    if (kill(pid, SIGTERM) == 0) {
        char msg[128];
        snprintf(msg, sizeof(msg), "Successfully shut off decryption process with PID %d.", pid);
        write_log(msg);
        remove(PID_FILE);
    } else {
        perror("Failed to shut down process");
    }
}

#include <openssl/bio.h>
#include <openssl/evp.h>

char* base64_decode(const char* input) {
    BIO *bio, *b64;
    int len = strlen(input);
    char *buffer = (char*)malloc(len);
    memset(buffer, 0, len);

    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new_mem_buf((void*)input, len);
    bio = BIO_push(b64, bio);
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
    int decoded_size = BIO_read(bio, buffer, len);
    buffer[decoded_size] = '\0';
    BIO_free_all(bio);
    return buffer;
}

int main(int argc, char *argv[]) {
    create_directory_if_not_exists(STARTER_KIT);
    create_directory_if_not_exists(QUARANTINE);
    create_log_if_not_exists();

    if (argc != 2) {
        printf("Usage:\n");
        printf("  ./starterkit --decrypt\n");
        printf("  ./starterkit --quarantine\n");
        printf("  ./starterkit --return\n");
        printf("  ./starterkit --eradicate\n");
        printf("  ./starterkit --shutdown\n");
        return 1;
    }

    if (strcmp(argv[1], "--quarantine") == 0) {
        quarantine_files();
    } else if (strcmp(argv[1], "--return") == 0) {
        return_files();
    } else if (strcmp(argv[1], "--eradicate") == 0) {
        eradicate_files();
    } else if (strcmp(argv[1], "--decrypt") == 0) {
        decrypt_daemon();
    } else if (strcmp(argv[1], "--shutdown") == 0) {
        shutdown_daemon();
    } else {
        printf("Invalid option: %s\n", argv[1]);
        return 1;
    }

    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

int syarat(char *name) {
    int len = strlen(name);
    if (len != 5) return 0;
    if (strcmp(&name[1], ".txt") != 0) return 0;
    char c = name[0];
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) return 1;
    return 0;
}

void filter() {
    mkdir("Filtered",0777);
    char *folders[] = {"Clues/ClueA", "Clues/ClueB", "Clues/ClueC", "Clues/ClueD"};
    for (int i = 0; i < 4; i++) {
        DIR *dir = opendir(folders[i]);
        if (dir == NULL) continue;
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_name[0] != '.') {
                if (syarat(entry->d_name)) {
                    char from[100], to[100];
                    sprintf(from, "%s/%s", folders[i], entry->d_name);
                    sprintf(to, "Filtered/%s", entry->d_name);
                    rename(from, to);
                } else {
                    char path[100];
                    sprintf(path, "%s/%s", folders[i], entry->d_name);
                    remove(path);
                }
            }
        }
        closedir(dir);
    }
}

int txt(char *name) {
    return strlen(name) == 5 && strcmp(&name[1], ".txt") == 0;
}

int nomer(char c) {
    return c >= '0' && c <= '9';
}

int huruf(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

void swap(char arr[][20], int i, int j) {
    char temp[20];
    strcpy(temp, arr[i]);
    strcpy(arr[i], arr[j]);
    strcpy(arr[j], temp);
}

void sort(char arr[][20], int n) {
    for (int i = 0; i < n - 1; i++) {
        for (int j = 0; j < n - i - 1; j++) {
            if (arr[j][0] > arr[j + 1][0]) {
                swap(arr, j, j + 1);
            }
        }
    }
}

void combine() {
    char angka[100][20];
    char huruf_arr[100][20];
    int a = 0, h = 0;

    DIR *dir = opendir("Filtered");
    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] != '.' && txt(entry->d_name)) {
            if (nomer(entry->d_name[0])) {
                strcpy(angka[a++], entry->d_name);
            } else if (huruf(entry->d_name[0])) {
                strcpy(huruf_arr[h++], entry->d_name);
            }
        }
    }
    closedir(dir);

    sort(angka, a);
    sort(huruf_arr, h);

    FILE *gabung = fopen("Combined.txt", "w");
    int i = 0, j = 0;
    while (i < a || j < h) {
        if (i < a) {
            char path[100];
            sprintf(path, "Filtered/%s", angka[i]);
            FILE *f = fopen(path, "r");
            if (f) {
                char ch;
                while ((ch = fgetc(f)) != EOF) {
                    fputc(ch, gabung);
                }
                fclose(f);
                remove(path);
            }
            i++;
        }
        if (j < h) {
            char path[100];
            sprintf(path, "Filtered/%s", huruf_arr[j]);
            FILE *f = fopen(path, "r");
            if (f) {
                char ch;
                while ((ch = fgetc(f)) != EOF) {
                    fputc(ch, gabung);
                }
                fclose(f);
                remove(path);
            }
            j++;
        }
    }
    fclose(gabung);
}

char rot13(char c) {
    if (c >= 'a' && c <= 'z') return ((c - 'a' + 13) % 26) + 'a';
    if (c >= 'A' && c <= 'Z') return ((c - 'A' + 13) % 26) + 'A';
    return c;
}

void decode() {
    FILE *input = fopen("Combined.txt", "r");
    FILE *output = fopen("Decoded.txt", "w");

    if (input && output) {
        char ch;
        while ((ch = fgetc(input)) != EOF) {
            fputc(rot13(ch), output);
        }
        fclose(input);
        fclose(output);
    }
}

int main(int argc, char *argv[]) {
    int status;

    DIR *cek = opendir("Clues");
    if (cek != NULL) {
        closedir(cek);
    } else {
        pid_t pid1 = fork();
        if (pid1 == 0) {
            char *curl_argv[] = {
                "/usr/bin/curl",
                "-L",
                "-o",
                "Clues.zip",
                "https://drive.usercontent.google.com/u/0/uc?id=1xFn1OBJUuSdnApDseEczKhtNzyGekauK&export=download",
                NULL
            };
            execve("/usr/bin/curl", curl_argv, NULL);
            perror("Gagal menjalankan curl");
            exit(1);
        }
        waitpid(pid1, &status, 0);

        pid_t pid2 = fork();
        if (pid2 == 0) {
            char *unzip_argv[] = {
                "/usr/bin/unzip",
                "-o",
                "Clues.zip",
                NULL
            };
            execve("/usr/bin/unzip", unzip_argv, NULL);
            perror("Gagal menjalankan unzip");
            exit(1);
        }
        waitpid(pid2, &status, 0);

        pid_t pid3 = fork();
        if (pid3 == 0) {
            char *rm_argv[] = {
                "/usr/bin/rm",
                "-f",
                "Clues.zip",
                NULL
            };
            execve("/usr/bin/rm", rm_argv, NULL);
            perror("Gagal remove");
            exit(1);
        }
        waitpid(pid3, &status, 0);
    }

    if (argc == 3 && strcmp(argv[1], "-m") == 0) {
        if (strcmp(argv[2], "Filter") == 0) {
            filter();
        } else if (strcmp(argv[2], "Combine") == 0) {
            combine();
        } else if (strcmp(argv[2], "Decode") == 0) {
            decode();
        } else {
            printf("Command tidak tersedia\n");
        }
    } else if (argc > 1) {
        printf("Command tidak tersedia\n");
    }
    return 0;
}

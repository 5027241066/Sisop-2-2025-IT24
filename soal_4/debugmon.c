#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <pwd.h>

void write_custom_log(const char *procname, const char *status) {
    FILE *log = fopen("debugmon.log", "a");
    if (!log) {
        perror("Failed to open debugmon.log");
        return;
    }

    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    fprintf(log, "[%02d:%02d:%d]-[%02d:%02d:%02d]_%s_%s\n",
            tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900,
            tm.tm_hour, tm.tm_min, tm.tm_sec,
            procname, status);

    fclose(log);
}

void run_fail_daemon(const char *user) {
    pid_t pid = fork();
    if (pid > 0) return;
    if (pid < 0) {
        perror("Failed to fork in run_fail_daemon");
        exit(1);
    }

    setsid();

    char failfile[64];
    snprintf(failfile, sizeof(failfile), "fail_%s.flag", user);

    while (1) {
        if (access(failfile, F_OK) == 0) {
            pid_t kill_pid = fork();
            if (kill_pid == 0) {
                FILE *ps = popen("ps -u $(whoami) -o pid --no-headers", "r");
                if (!ps) {
                    perror("Failed to run ps in run_fail_daemon");
                    exit(1);
                }

                char line[256];
                while (fgets(line, sizeof(line), ps)) {
                    int pid_to_kill = atoi(line);
                    if (pid_to_kill <= 0) continue;

                    char proc_path[256];
                    snprintf(proc_path, sizeof(proc_path), "/proc/%d/comm", pid_to_kill);
                    FILE *comm_file = fopen(proc_path, "r");
                    if (!comm_file) continue;

                    char comm[256];
                    if (fgets(comm, sizeof(comm), comm_file)) {
                        comm[strcspn(comm, "\n")] = 0;
                        if (strcmp(comm, "debugmon") != 0) {
                            kill(pid_to_kill, SIGTERM);
                        }
                    }
                    fclose(comm_file);
                }
                pclose(ps);
                exit(0);
            } else if (kill_pid < 0) {
                perror("Failed to fork for killing in run_fail_daemon");
            }
            wait(NULL);
        }
        sleep(5);
    }
}

void write_log(const char *user) {
    while (1) {
        char failfile[64];
        snprintf(failfile, sizeof(failfile), "fail_%s.flag", user);
        if (access(failfile, F_OK) == 0) {
            write_custom_log("daemon", "FAILED");
        } else {
            write_custom_log("daemon", "RUNNING");
        }
        sleep(5);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage:\n");
        printf("  %s list <user>\n", argv[0]);
        printf("  %s daemon <user>\n", argv[0]);
        printf("  %s stop <user>\n", argv[0]);
        printf("  %s fail <user>\n", argv[0]);
        printf("  %s revert <user>\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "list") == 0) {
        pid_t pid = fork();
        if (pid == 0) {
            char *args[] = {
                "ps", "-u", argv[2],
                "-o", "pid,comm,%cpu,%mem",
                "--no-headers", NULL
            };
            execvp("ps", args);
            perror("Failed to exec ps");
            exit(1);
        } else if (pid < 0) {
            perror("Failed to fork for ps");
        }
        wait(NULL);
        write_custom_log("list", "RUNNING");

    } else if (strcmp(argv[1], "daemon") == 0) {
        char pidfile[64];
        snprintf(pidfile, sizeof(pidfile), "debugmon_%s.pid", argv[2]);
        FILE *pf_check = fopen(pidfile, "r");
        if (pf_check) {
            int existing_pid;
            if (fscanf(pf_check, "%d", &existing_pid) == 1) {
                if (kill(existing_pid, 0) == 0) {
                    printf("Daemon for user %s is already running with PID %d.\n", argv[2], existing_pid);
                    fclose(pf_check);
                    return 1;
                }
            }
            fclose(pf_check);
            remove(pidfile);
        }

        pid_t pid = fork();
        if (pid > 0) {
            printf("Daemon started.\n");
            exit(0);
        }
        if (pid < 0) {
            perror("Failed to fork for daemon");
            exit(1);
        }

        setsid();

        FILE *pf = fopen(pidfile, "w");
        if (pf) {
            fprintf(pf, "%d", getpid());
            fclose(pf);
        } else {
            perror("Failed to write PID file");
        }

        write_custom_log("daemon", "RUNNING");
        write_log(argv[2]);

    } else if (strcmp(argv[1], "stop") == 0) {
        char pidfile[64];
        snprintf(pidfile, sizeof(pidfile), "debugmon_%s.pid", argv[2]);

        FILE *pf = fopen(pidfile, "r");
        if (!pf) {
            printf("No running daemon for user %s.\n", argv[2]);
            return 1;
        }

        int daemon_pid;
        if (fscanf(pf, "%d", &daemon_pid) != 1) {
            fclose(pf);
            printf("Failed to read daemon PID.\n");
            return 1;
        }
        fclose(pf);

        if (kill(daemon_pid, SIGTERM) == 0) {
            printf("Daemon for user %s stopped.\n", argv[2]);
            remove(pidfile);
            write_custom_log("stop", "RUNNING");
        } else {
            perror("Failed to stop daemon");
        }

    } else if (strcmp(argv[1], "fail") == 0) {
        if (getuid() == 0) {
            printf("Error: 'fail' command is not allowed for root user.\n");
            return 1;
        }

        struct passwd *pw = getpwuid(getuid());
        if (!pw) {
            perror("Failed to get current user information");
            return 1;
        }
        if (strcmp(pw->pw_name, argv[2]) != 0) {
            printf("Error: 'fail' command can only target the current user (%s).\n", pw->pw_name);
            return 1;
        }

        pid_t current_pid = getpid();

        pid_t pid = fork();
        if (pid == 0) {
            FILE *ps = popen("ps -u $(whoami) -o pid --no-headers", "r");
            if (!ps) {
                perror("Failed to run ps in fail");
                exit(1);
            }

            char line[256];
            while (fgets(line, sizeof(line), ps)) {
                int pid_to_kill = atoi(line);
                if (pid_to_kill > 0 && pid_to_kill != current_pid) {
                    kill(pid_to_kill, SIGTERM);
                }
            }
            pclose(ps);
            exit(0);
        } else if (pid < 0) {
            perror("Failed to fork for killing in fail");
        }
        wait(NULL);

        char failfile[64];
        snprintf(failfile, sizeof(failfile), "fail_%s.flag", argv[2]);
        FILE *ff = fopen(failfile, "w");
        if (ff) {
            fprintf(ff, "User %s is blocked.\n", argv[2]);
            fclose(ff);
        } else {
            perror("Failed to create fail flag file");
        }

        write_custom_log("fail", "FAILED");
        printf("User %s failed. Blocked from running processes.\n", argv[2]);

        run_fail_daemon(argv[2]);

    } else if (strcmp(argv[1], "revert") == 0) {
        char failfile[64];
        snprintf(failfile, sizeof(failfile), "fail_%s.flag", argv[2]);

        if (access(failfile, F_OK) == 0) {
            if (remove(failfile) != 0) {
                perror("Failed to remove fail flag file");
            } else {
                write_custom_log("revert", "RUNNING");
                printf("User %s unblocked.\n", argv[2]);
            }
        } else {
            printf("User %s is not blocked.\n", argv[2]);
        }

    } else {
        printf("Unknown command: %s\n", argv[1]);
    }

    return 0;
}

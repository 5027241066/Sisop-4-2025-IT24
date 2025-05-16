#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>

void createfolder() {
    struct stat st = {0};
    if (stat("anomali/image", &st) == -1) {
        if (mkdir("anomali/image", 0777) == -1) {
            perror("Gagal membuat folder anomali/image");
            exit(1);
        }
    }
}

int hexIMG(const char *hex_filename) {
    FILE *hex_file = fopen(hex_filename, "r");
    if (!hex_file) {
        perror("Gagal membuka file hex");
        return 1;
    }

    const char *filename = strrchr(hex_filename, '/');
    filename = filename ? filename + 1 : hex_filename;

    char id[64];
    strncpy(id, filename, sizeof(id));
    char *dot = strrchr(id, '.');
    if (dot) *dot = '\0';

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d_%H:%M:%S", t);

    char output_filename[128];
    snprintf(output_filename, sizeof(output_filename), "%s_image_%s.png", id, timestamp);

    char output_path[256];
    snprintf(output_path, sizeof(output_path), "anomali/image/%s", output_filename);

    FILE *img_file = fopen(output_path, "wb");
    if (!img_file) {
        perror("Gagal membuat file gambar");
        fclose(hex_file);
        return 1;
    }

    char *hex_data = NULL;
    size_t size = 0, cap = 4096;
    hex_data = malloc(cap);
    if (!hex_data) {
        perror("Gagal alokasi memori");
        fclose(hex_file);
        fclose(img_file);
        return 1;
    }

    int c;
    while ((c = fgetc(hex_file)) != EOF) {
        if ((c >= '0' && c <= '9') ||
            (c >= 'a' && c <= 'f') ||
            (c >= 'A' && c <= 'F')) {
            if (size + 1 >= cap) {
                cap *= 2;
                hex_data = realloc(hex_data, cap);
                if (!hex_data) {
                    perror("Gagal realloc");
                    fclose(hex_file);
                    fclose(img_file);
                    return 1;
                }
            }
            hex_data[size++] = (char)c;
        }
    }

    if (size % 2 != 0) size--;

    for (size_t i = 0; i < size; i += 2) {
        char byte_str[3] = { hex_data[i], hex_data[i+1], '\0' };
        unsigned char byte = (unsigned char) strtol(byte_str, NULL, 16);
        fwrite(&byte, 1, 1, img_file);
    }

    fclose(hex_file);
    fclose(img_file);
    free(hex_data);

    char log_path[256];
    strncpy(log_path, hex_filename, sizeof(log_path));
    char *last_slash = strrchr(log_path, '/');
    if (last_slash) {
        *last_slash = '\0';
    } else {
        strcpy(log_path, ".");
    }
    strncat(log_path, "/conversion.log", sizeof(log_path) - strlen(log_path) - 1);

    FILE *log_file = fopen(log_path, "a");
    if (log_file) {
        fprintf(log_file, "[%04d-%02d-%02d][%02d:%02d:%02d]: Successfully converted hexadecimal text %s to %s.\n",
            t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
            t->tm_hour, t->tm_min, t->tm_sec,
            filename, output_filename
        );
        fclose(log_file);
    }

    printf("Gambar berhasil dibuat: %s\n", output_path);
    return 0;
}

int main() {
    int status;

    pid_t pid1 = fork();
    if (pid1 == 0) {
        char *curl_argv[] = {
            "/usr/bin/curl", "-L", "-o", "anomali.zip",
            "https://drive.usercontent.google.com/u/0/uc?id=1hi_GDdP51Kn2JJMw02WmCOxuc3qrXzh5&export=download",
            NULL
        };
        execve("/usr/bin/curl", curl_argv, NULL);
        perror("Gagal download");
        exit(1);
    }
    waitpid(pid1, &status, 0);

    pid_t pid2 = fork();
    if (pid2 == 0) {
        char *unzip_argv[] = { "/usr/bin/unzip", "-o", "anomali.zip", NULL };
        execve("/usr/bin/unzip", unzip_argv, NULL);
        perror("Gagal unzip");
        exit(1);
    }
    waitpid(pid2, &status, 0);

    pid_t pid3 = fork();
    if (pid3 == 0) {
        char *rm_argv[] = { "/usr/bin/rm", "-f", "anomali.zip", NULL };
        execve("/usr/bin/rm", rm_argv, NULL);
        perror("Gagal remove");
        exit(1);
    }
    waitpid(pid3, &status, 0);

    createfolder();

    DIR *dir;
    struct dirent *entry;
    dir = opendir("anomali");
    if (dir == NULL) {
        perror("Gagal membuka direktori anomali");
        return 1;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strstr(entry->d_name, ".txt")) {
            char hex_file_path[256];
            snprintf(hex_file_path, sizeof(hex_file_path), "anomali/%s", entry->d_name);
            hexIMG(hex_file_path);
        }
    }

    closedir(dir);
    return 0;
}

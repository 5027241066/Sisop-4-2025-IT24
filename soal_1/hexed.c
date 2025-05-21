#define FUSE_USE_VERSION 35

#include <fuse3/fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <curl/curl.h>
#include <pthread.h>
#include <ctype.h>

#define BASE_DIR "anomali"
#define IMAGE_DIR "anomali/image"
#define LOG_FILE "anomali/conversion.log"
#define ZIP_URL "https://drive.usercontent.google.com/u/0/uc?id=1hi_GDdP51Kn2JJMw02WmCOxuc3qrXzh5&export=download"
#define ZIP_NAME "anomali.zip"

static const char *dirpath = BASE_DIR;
pthread_mutex_t convert_mutex = PTHREAD_MUTEX_INITIALIZER;

struct file_info {
    char *data;
    size_t size;
};

size_t curlwrite(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    return fwrite(ptr, size, nmemb, stream);
}

void download_zip() {
    printf("Downloading zip file...\n");
    FILE *fp = fopen(ZIP_NAME, "wb");
    if (!fp) {
        perror("fopen");
        return;
    }

    CURL *curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, ZIP_URL);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlwrite);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        
        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }
        curl_easy_cleanup(curl);
    }
    fclose(fp);

    if (access(ZIP_NAME, F_OK) == 0) {
        char cmd[256];
        snprintf(cmd, sizeof(cmd), "unzip -o %s > /dev/null", ZIP_NAME);
        int status = system(cmd);
        if (status == 0) {
            remove(ZIP_NAME);
            printf("Files extracted successfully to %s/\n", BASE_DIR);
        } else {
            fprintf(stderr, "Unzip failed\n");
        }
    }
}

int checkhex(const char *str) {
    for (int i = 0; str[i] != '\0'; i++) {
        if (!isxdigit(str[i])) {
            return 0;
        }
    }
    return 1;
}

void converthex(const char *filename) {
    pthread_mutex_lock(&convert_mutex);
    
    char filepath[512];
    snprintf(filepath, sizeof(filepath), "%s/%s", BASE_DIR, filename);
    printf("Processing file: %s\n", filepath);

    FILE *fp = fopen(filepath, "r");
    if (!fp) {
        perror("Failed to open file");
        pthread_mutex_unlock(&convert_mutex);
        return;
    }

    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (fsize <= 0) {
        fclose(fp);
        pthread_mutex_unlock(&convert_mutex);
        return;
    }

    char *hex_data = malloc(fsize + 1);
    size_t bytes_read = fread(hex_data, 1, fsize, fp);
    fclose(fp);
    
    if (bytes_read != fsize) {
        free(hex_data);
        pthread_mutex_unlock(&convert_mutex);
        return;
    }
    hex_data[fsize] = 0;

    char *clean_hex = malloc(fsize + 1);
    int clean_len = 0;
    for (int i = 0; i < fsize; i++) {
        if (isxdigit(hex_data[i])) {
            clean_hex[clean_len++] = hex_data[i];
        }
    }
    clean_hex[clean_len] = '\0';
    
    if (!checkhex(clean_hex) || clean_len % 2 != 0) {
        free(hex_data);
        free(clean_hex);
        pthread_mutex_unlock(&convert_mutex);
        return;
    }

    int len = clean_len / 2;
    unsigned char *img_data = malloc(len);
    for (int i = 0; i < len; i++) {
        sscanf(&clean_hex[i * 2], "%2hhx", &img_data[i]);
    }

    mkdir(IMAGE_DIR, 0755);

    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    char log_timestamp[32];
    strftime(log_timestamp, sizeof(log_timestamp), "%Y-%m-%d", tm_info);

    char log_time[32];
    strftime(log_time, sizeof(log_time), "%H:%M:%S", tm_info);

    char file_timestamp[32];
    strftime(file_timestamp, sizeof(file_timestamp), "%Y-%m-%d_%H:%M:%S", tm_info);

    char img_filename[256];
    snprintf(img_filename, sizeof(img_filename), "%.*s_image_%s.png", 
         (int)(strlen(filename) - 4), filename, file_timestamp);

    char output_path[512];
    snprintf(output_path, sizeof(output_path), "%s/%s", IMAGE_DIR, img_filename);

    FILE *img = fopen(output_path, "wb");
    if (img) {
        fwrite(img_data, 1, len, img);
        fclose(img);
        printf("Created image: %s\n", output_path);
    }

    FILE *log = fopen(LOG_FILE, "a");
    if (log) {
        fprintf(log, "[%s][%s]: Successfully converted hexadecimal text %s to %s.\n", 
        log_timestamp, log_time, filename, img_filename);
        fclose(log);
    }

    free(hex_data);
    free(clean_hex);
    free(img_data);
    pthread_mutex_unlock(&convert_mutex);
}

static int xmp_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi) {
    char fpath[512];
    snprintf(fpath, sizeof(fpath), "%s%s", dirpath, path);
    return lstat(fpath, stbuf);
}

static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                       off_t offset, struct fuse_file_info *fi,
                       enum fuse_readdir_flags flags) {
    char fpath[512];
    snprintf(fpath, sizeof(fpath), "%s%s", dirpath, path);

    DIR *dp = opendir(fpath);
    if (dp == NULL)
        return -errno;

    struct dirent *de;
    while ((de = readdir(dp)) != NULL) {
        struct stat st = {0};
        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;
        if (filler(buf, de->d_name, &st, 0, 0))
            break;
    }

    closedir(dp);
    return 0;
}

static int xmp_open(const char *path, struct fuse_file_info *fi) {
    char fpath[512];
    snprintf(fpath, sizeof(fpath), "%s%s", dirpath, path);

    int res = open(fpath, fi->flags);
    if (res == -1)
        return -errno;

    close(res);

    const char *fname = strrchr(path, '/') ? strrchr(path, '/') + 1 : path;
    if (strstr(fname, ".txt") != NULL) {
        converthex(fname);
    }

    return 0;
}

static int xmp_read(const char *path, char *buf, size_t size, off_t offset,
                    struct fuse_file_info *fi) {
    char fpath[512];
    snprintf(fpath, sizeof(fpath), "%s%s", dirpath, path);
    int fd = open(fpath, O_RDONLY);
    if (fd == -1)
        return -errno;

    int res = pread(fd, buf, size, offset);
    if (res == -1)
        res = -errno;

    close(fd);
    return res;
}

static const struct fuse_operations xmp_oper = {
    .getattr = xmp_getattr,
    .readdir = xmp_readdir,
    .open    = xmp_open,
    .read    = xmp_read,
};

int main(int argc, char *argv[]) {
    mkdir(BASE_DIR, 0755);
    mkdir(IMAGE_DIR, 0755);

    if (access("anomali/1.txt", F_OK) != 0) {
        download_zip();
    }

    DIR *dir = opendir(BASE_DIR);
    if (dir) {
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_type == DT_REG && strstr(entry->d_name, ".txt") != NULL) {
                converthex(entry->d_name);
            }
        }
        closedir(dir);
    }

    return fuse_main(argc, argv, &xmp_oper, NULL);
}

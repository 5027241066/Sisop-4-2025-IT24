#define FUSE_USE_VERSION 31
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <time.h>
#include <errno.h>
#include <ctype.h>

char* reverse_str(char* str) {
    int len = strlen(str);
    for (int i = 0; i < len/2; i++) {
        char temp = str[i];
        str[i] = str[len-i-1];
        str[len-i-1] = temp;
    }
    return str;
}

void rot13(char* text) {
    for (int i = 0; text[i]; i++) {
        if (isalpha(text[i])) {
            if (tolower(text[i]) <= 'm') text[i] += 13;
            else text[i] -= 13;
        }
    }
}

void log_action(const char* action, const char* filename) {
    FILE* log = fopen("/var/log/it24.log", "a");
    if (log) {
        time_t now = time(NULL);
        char timestr[20];
        strftime(timestr, 20, "%Y-%m-%d %H:%M:%S", localtime(&now));
        fprintf(log, "[%s] [%s] %s\n", timestr, action, filename);
        fclose(log);
    }
}

static int antink_getattr(const char* path, struct stat* st) {
    memset(st, 0, sizeof(struct stat));
    if (strcmp(path, "/") == 0) {
        st->st_mode = S_IFDIR | 0755;
        st->st_nlink = 2;
    } else {
        st->st_mode = S_IFREG | 0644;
        st->st_nlink = 1;
        st->st_size = 1024;
    }
    return 0;
}

static int antink_readdir(const char* path, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi) {
    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);

    DIR* dir = opendir("/it24_host");
    if (!dir) return -ENOENT;

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strstr(entry->d_name, "nafis") || strstr(entry->d_name, "kimcun")) {
            char reversed[256];
            strcpy(reversed, entry->d_name);
            filler(buf, reverse_str(reversed), NULL, 0);
            log_action("ALERT", entry->d_name);
        } else {
            filler(buf, entry->d_name, NULL, 0);
        }
    }
    closedir(dir);
    return 0;
}

static int antink_read(const char* path, char* buf, size_t size, off_t offset, struct fuse_file_info* fi) {
    char full_path[1024];
    snprintf(full_path, sizeof(full_path), "/it24_host/%s", path);

    FILE* file = fopen(full_path, "r");
    if (!file) return -ENOENT;

    char content[4096];
    size_t len = fread(content, 1, sizeof(content), file);
    fclose(file);

    if (!strstr(path, "nafis") && !strstr(path, "kimcun")) {
        rot13(content);
        log_action("ENCRYPTED", path);
    } else {
        log_action("ACCESSED", path);
    }

    memcpy(buf, content, len);
    return len;
}

static struct fuse_operations antink_ops = {
    .getattr = antink_getattr,
    .readdir = antink_readdir,
    .read = antink_read,
};

int main(int argc, char* argv[]) {
    return fuse_main(argc, argv, &antink_ops, NULL);
}

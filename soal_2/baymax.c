#define FUSE_USE_VERSION 30
#include <fuse3/fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <unistd.h>
#include <stdarg.h> 

#define NAMA_FILE_UTUH "Baymax.jpeg"
#define DIREKTORI_RELICS "/home/deefen/sisopmodul4/soalno2/relics"   
#define FILE_LOG "/home/deefen/sisopmodul4/soalno2/activity.log"
#define UKURAN_PECAHAN 1024
#define MAKS_PATH 512

void catat_log(const char *format, ...) {
    FILE *log = fopen(FILE_LOG, "a");
    if (!log) return;

    time_t t = time(NULL);
    struct tm *tm = localtime(&t);

    fprintf(log, "[%04d-%02d-%02d %02d:%02d:%02d] ",
            tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
            tm->tm_hour, tm->tm_min, tm->tm_sec);

    va_list args;
    va_start(args, format);
    vfprintf(log, format, args);
    va_end(args);

    fprintf(log, "\n");
    fclose(log);
}

static int fs_getattr(const char *jalur, struct stat *stbuf, struct fuse_file_info *fi) {
    (void) fi;
    memset(stbuf, 0, sizeof(struct stat));

    if (strcmp(jalur, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;
    } 
    
    if (strcmp(jalur + 1, NAMA_FILE_UTUH) == 0) {
        stbuf->st_mode = S_IFREG | 0444;
        stbuf->st_nlink = 1;
        stbuf->st_size = 0;
        int file_count = 0;

        for (int i = 0; i < 14; i++) {
            char path[MAKS_PATH];
            snprintf(path, sizeof(path), "%s/%s.%03d", DIREKTORI_RELICS, NAMA_FILE_UTUH, i);
            FILE *f = fopen(path, "rb");
            if (f) {
                fseek(f, 0, SEEK_END);
                stbuf->st_size += ftell(f);
                fclose(f);
                file_count++;
            }
        }
        
        if (file_count == 0) {
            catat_log("ERROR: Tidak ada pecahan file ditemukan untuk %s", NAMA_FILE_UTUH);
            return -ENOENT;
        }
        return 0;
    }
    
    struct dirent *entry;
    DIR *dir = opendir(DIREKTORI_RELICS);
    if (dir) {
        char nama_file[256];
        strncpy(nama_file, jalur + 1, sizeof(nama_file) - 1);
        nama_file[sizeof(nama_file) - 1] = '\0';
        
        char pattern[MAKS_PATH];
        snprintf(pattern, sizeof(pattern), "%s/%s.000", DIREKTORI_RELICS, nama_file);
        
        if (access(pattern, F_OK) == 0) {
            stbuf->st_mode = S_IFREG | 0644;
            stbuf->st_nlink = 1;
            stbuf->st_size = 0;
            
            for (int i = 0; i < 1000; i++) { 
                char path[MAKS_PATH];
                snprintf(path, sizeof(path), "%s/%s.%03d", DIREKTORI_RELICS, nama_file, i);
                
                if (access(path, F_OK) != 0) break;
                
                struct stat st;
                if (stat(path, &st) == 0) {
                    stbuf->st_size += st.st_size;
                }
            }
            
            closedir(dir);
            return 0;
        }
        closedir(dir);
    }
    
    char path_tmp[MAKS_PATH];
    snprintf(path_tmp, sizeof(path_tmp), "/tmp/%s", jalur + 1);
    struct stat st;
    if (stat(path_tmp, &st) == 0) {
        stbuf->st_mode = S_IFREG | 0644;
        stbuf->st_nlink = 1;
        stbuf->st_size = st.st_size;
        return 0;
    }
    
    return -ENOENT;
}

static int fs_readdir(const char *jalur, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi, enum fuse_readdir_flags flags) {
    (void) offset;
    (void) fi;
    (void) flags;

    if (strcmp(jalur, "/") != 0) return -ENOENT;

    filler(buf, ".", NULL, 0, 0);
    filler(buf, "..", NULL, 0, 0);
    
    filler(buf, NAMA_FILE_UTUH, NULL, 0, 0);
    
    DIR *dir = opendir(DIREKTORI_RELICS);
    if (dir) {
        struct dirent *entry;
        char nama_file_sebelumnya[256] = {0};
        
        while ((entry = readdir(dir)) != NULL) {
            char *nama = entry->d_name;
            char *titik = strrchr(nama, '.');
            
            if (!titik || strcmp(nama, ".") == 0 || strcmp(nama, "..") == 0 ||
                strncmp(nama, NAMA_FILE_UTUH, strlen(NAMA_FILE_UTUH)) == 0) {
                continue;
            }
            
            char nama_dasar[256];
            int panjang = titik - nama;
            if (panjang >= sizeof(nama_dasar)) panjang = sizeof(nama_dasar) - 1;
            strncpy(nama_dasar, nama, panjang);
            nama_dasar[panjang] = '\0';
            
            if (strcmp(nama_dasar, nama_file_sebelumnya) != 0) {
                filler(buf, nama_dasar, NULL, 0, 0);
                strcpy(nama_file_sebelumnya, nama_dasar);
            }
        }
        closedir(dir);
    }

    return 0;
}

static int fs_open(const char *jalur, struct fuse_file_info *fi) {
    if (strcmp(jalur + 1, NAMA_FILE_UTUH) != 0) return -ENOENT;

    catat_log("READ: %s", NAMA_FILE_UTUH);
    return 0;
}

static int fs_read(const char *jalur, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    (void) fi;
    char nama_file[201];
    strncpy(nama_file, jalur + 1, 200);
    nama_file[200] = '\0'; 
  
    size_t total_dibaca = 0, posisi = 0;
    int max_pecahan = 0;
    
    while (1) {
        char path[MAKS_PATH];
        snprintf(path, sizeof(path), "%s/%s.%03d", DIREKTORI_RELICS, nama_file, max_pecahan);
        if (access(path, R_OK) != 0) break;
        max_pecahan++;
    }
    
    if (max_pecahan == 0) {
        catat_log("READ ERROR: Tidak ada pecahan untuk file %s", nama_file);
        return -ENOENT;
    }
    
    catat_log("READ: %s (dari %d pecahan)", nama_file, max_pecahan);
    
    for (int i = 0; i < max_pecahan && total_dibaca < size; i++) {
        char path[MAKS_PATH];
        snprintf(path, sizeof(path), "%s/%s.%03d", DIREKTORI_RELICS, nama_file, i);

        FILE *f = fopen(path, "rb");
        if (!f) {
            catat_log("WARN: Tidak dapat membuka pecahan %s", path);
            continue; 
        }

        fseek(f, 0, SEEK_END);
        size_t ukuran = ftell(f);
        rewind(f);

        if (posisi + ukuran <= offset) {
            posisi += ukuran;
            fclose(f);
            continue;
        }

        size_t mulai = offset > posisi ? offset - posisi : 0;
        fseek(f, mulai, SEEK_SET);
        size_t sisa = ukuran - mulai;
        if (total_dibaca + sisa > size) sisa = size - total_dibaca;

        size_t dibaca = fread(buf + total_dibaca, 1, sisa, f);
        total_dibaca += dibaca;
        posisi += ukuran;

        fclose(f);
    }

    return total_dibaca > 0 ? total_dibaca : -ENOENT;
}

static int fs_create(const char *jalur, mode_t mode, struct fuse_file_info *fi) {
    char nama_file[201];
    strncpy(nama_file, jalur + 1, 200);
    nama_file[200] = '\0';

    if (strlen(nama_file) > 200) return -ENAMETOOLONG;

    char path_tmp[MAKS_PATH];
    snprintf(path_tmp, sizeof(path_tmp), "/tmp/%s", nama_file);
    
    unlink(path_tmp);
    
    FILE *tmp = fopen(path_tmp, "w");
    if (!tmp) {
        catat_log("ERROR: Gagal membuat file sementara %s", path_tmp);
        return -EACCES;
    }

    fclose(tmp);
    catat_log("CREATE: %s", nama_file);
    return 0;
}

static int fs_write(const char *jalur, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    char nama_file[201];
    strncpy(nama_file, jalur + 1, 200);
    nama_file[200] = '\0';

    char path_tmp[MAKS_PATH];
    snprintf(path_tmp, sizeof(path_tmp), "/tmp/%s", nama_file);

    FILE *tmp;
    if (offset > 0) {
        tmp = fopen(path_tmp, "r+");
        if (tmp) {
            fseek(tmp, offset, SEEK_SET);
        } else {
            catat_log("ERROR: Tidak dapat membuka file %s untuk menulis dengan offset", path_tmp);
            return -EACCES;
        }
    } else {
        tmp = fopen(path_tmp, "w");
        if (!tmp) {
            catat_log("ERROR: Tidak dapat membuka file %s untuk menulis", path_tmp);
            return -EACCES;
        }
    }

    size_t written = fwrite(buf, 1, size, tmp);
    fclose(tmp);
    
    catat_log("WRITE: %s (%zu bytes pada offset %lld)", nama_file, written, (long long)offset);
    return written;
}

static int fs_release(const char *jalur, struct fuse_file_info *fi) {
    char nama_file[201];
    strncpy(nama_file, jalur + 1, 200);
    nama_file[200] = '\0'; 

    char path_tmp[MAKS_PATH];
    snprintf(path_tmp, sizeof(path_tmp), "/tmp/%s", nama_file);

    if (strcmp(nama_file, NAMA_FILE_UTUH) == 0) {
        catat_log("RELEASE: %s (file read-only)", nama_file);
        return 0;
    }

    FILE *tmp = fopen(path_tmp, "r");
    if (!tmp) {
        catat_log("RELEASE: %s (file tidak ditemukan)", nama_file);
        return 0;
    }

    if (access(DIREKTORI_RELICS, F_OK) != 0) {
        if (mkdir(DIREKTORI_RELICS, 0755) != 0) {
            catat_log("ERROR: Gagal membuat direktori %s", DIREKTORI_RELICS);
            fclose(tmp);
            return -EACCES;
        }
    }

    for (int i = 0; i < 1000; i++) {
        char path_pecahan[MAKS_PATH];
        snprintf(path_pecahan, sizeof(path_pecahan), "%s/%s.%03d", DIREKTORI_RELICS, nama_file, i);
        if (access(path_pecahan, F_OK) == 0) {
            unlink(path_pecahan);
        } else {
            break;
        }
    }

    char buf[UKURAN_PECAHAN];
    int idx = 0;
    size_t jumlah;

    while ((jumlah = fread(buf, 1, UKURAN_PECAHAN, tmp)) > 0) {
        char path_pecahan[MAKS_PATH];
        snprintf(path_pecahan, sizeof(path_pecahan), "%s/%s.%03d", DIREKTORI_RELICS, nama_file, idx++);
        FILE *fpecahan = fopen(path_pecahan, "wb");
        if (!fpecahan) {
            catat_log("ERROR: Gagal menulis pecahan %s", path_pecahan);
            break;
        }

        fwrite(buf, 1, jumlah, fpecahan);
        fclose(fpecahan);
    }

    fclose(tmp);
    unlink(path_tmp);

    catat_log("RELEASE: %s -> %s.000 - %s.%03d", nama_file, nama_file, nama_file, idx - 1);
    return 0;
}

static int fs_unlink(const char *jalur) {
    char nama_file[201];
    strncpy(nama_file, jalur + 1, 200);
    nama_file[200] = '\0'; 

    int sukses = 0;
    for (int i = 0; i < 1000; i++) {
        char path_pecahan[MAKS_PATH];
        snprintf(path_pecahan, sizeof(path_pecahan), "%s/%s.%03d", DIREKTORI_RELICS, nama_file, i);

        if (access(path_pecahan, F_OK) == 0) {
            if (remove(path_pecahan) == 0) {
                sukses++;
            } else {
                catat_log("ERROR: Gagal menghapus %s", path_pecahan);
                return -errno;
            }
        } else {
            break;
        }
    }

    if (sukses > 0) {
        catat_log("DELETE: %s.000 - %s.%03d", nama_file, nama_file, sukses - 1);
        return 0;
    }

    return -ENOENT;
}

static struct fuse_operations operasi = {
    .getattr = fs_getattr,
    .readdir = fs_readdir,
    .open = fs_open,
    .read = fs_read,
    .create = fs_create,
    .write = fs_write,
    .release = fs_release,
    .unlink = fs_unlink,
};

int main(int argc, char *argv[]) {
    if (access(DIREKTORI_RELICS, F_OK) != 0) {
        if (mkdir(DIREKTORI_RELICS, 0755) != 0) {
            fprintf(stderr, "ERROR: Gagal membuat direktori %s: %s\n", 
                    DIREKTORI_RELICS, strerror(errno));
            return 1;
        }
    }
    
    fprintf(stderr, "Memeriksa keberadaan file pecahan di %s...\n", DIREKTORI_RELICS);
    int file_count = 0;
    for (int i = 0; i < 14; i++) {
        char path[MAKS_PATH];
        snprintf(path, sizeof(path), "%s/%s.%03d", DIREKTORI_RELICS, NAMA_FILE_UTUH, i);
        if (access(path, R_OK) == 0) {
            file_count++;
            struct stat st;
            if (stat(path, &st) == 0) {
                fprintf(stderr, "  Ditemukan pecahan %s (%ld bytes)\n", path, (long)st.st_size);
            } else {
                fprintf(stderr, "  Ditemukan pecahan %s\n", path);
            }
        } else {
            fprintf(stderr, "  Pecahan %s tidak ditemukan\n", path);
        }
    }
    fprintf(stderr, "Total %d dari 14 pecahan ditemukan.\n", file_count);

    umask(0);
    return fuse_main(argc, argv, &operasi, NULL);
}

#define FUSE_USE_VERSION 28
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#include <stdlib.h>
#include <openssl/aes.h>
#include <openssl/rand.h>
#include <openssl/evp.h>
#include <zlib.h>

static const char *direktori_sumber_chiho = "/home/deefen/sisopmodul4/soalno4/chiho"; 

static const unsigned char kunci_aes[32] = "maimai_heaven_chiho_key_12345678";

void rot13(char *teks) {
    while (*teks) {
        if ((*teks >= 'a' && *teks <= 'z') || (*teks >= 'A' && *teks <= 'Z')) {
            if ((*teks >= 'a' && *teks <= 'm') || (*teks >= 'A' && *teks <= 'M')) {
                *teks += 13;
            } else {
                *teks -= 13;
            }
        }
        teks++;
    }
}

void geser_karakter_metro(char *data, size_t ukuran) {
    for (size_t i = 0; i < ukuran; i++) {
        data[i] = (data[i] + (i % 256)) % 256;
    }
}

void balikan_geser_karakter_metro(char *data, size_t ukuran) {
    for (size_t i = 0; i < ukuran; i++) {
        data[i] = (data[i] - (i % 256) + 256) % 256;
    }
}

int enkripsi_aes(const unsigned char *plaintext, int panjang_plaintext,
                const unsigned char *kunci, unsigned char *iv,
                unsigned char *ciphertext) {
    EVP_CIPHER_CTX *konteks_sandi;
    int panjang_hasil;
    int panjang_ciphertext;

    if (!(konteks_sandi = EVP_CIPHER_CTX_new())) {
        return -1;
    }
    if (1 != EVP_EncryptInit_ex(konteks_sandi, EVP_aes_256_cbc(), NULL, kunci, iv)) {
        EVP_CIPHER_CTX_free(konteks_sandi);
        return -1;
    }
    if (1 != EVP_EncryptUpdate(konteks_sandi, ciphertext, &panjang_hasil, plaintext, panjang_plaintext)) {
        EVP_CIPHER_CTX_free(konteks_sandi);
        return -1;
    }
    panjang_ciphertext = panjang_hasil;
    if (1 != EVP_EncryptFinal_ex(konteks_sandi, ciphertext + panjang_hasil, &panjang_hasil)) {
        EVP_CIPHER_CTX_free(konteks_sandi);
        return -1;
    }
    panjang_ciphertext += panjang_hasil;
    EVP_CIPHER_CTX_free(konteks_sandi);
    return panjang_ciphertext;
}

int dekripsi_aes(const unsigned char *ciphertext, int panjang_ciphertext,
                const unsigned char *kunci, unsigned char *iv,
                unsigned char *plaintext) {
    EVP_CIPHER_CTX *konteks_sandi;
    int panjang_hasil;
    int panjang_plaintext;

    if (!(konteks_sandi = EVP_CIPHER_CTX_new())) {
        return -1;
    }
    if (1 != EVP_DecryptInit_ex(konteks_sandi, EVP_aes_256_cbc(), NULL, kunci, iv)) {
        EVP_CIPHER_CTX_free(konteks_sandi);
        return -1;
    }
    if (1 != EVP_DecryptUpdate(konteks_sandi, plaintext, &panjang_hasil, ciphertext, panjang_ciphertext)) {
        EVP_CIPHER_CTX_free(konteks_sandi);
        return -1;
    }
    panjang_plaintext = panjang_hasil;
    if (1 != EVP_DecryptFinal_ex(konteks_sandi, plaintext + panjang_hasil, &panjang_hasil)) {
        EVP_CIPHER_CTX_free(konteks_sandi);
        return -1;
    }
    panjang_plaintext += panjang_hasil;
    EVP_CIPHER_CTX_free(konteks_sandi);
    return panjang_plaintext;
}

int kompres_data_zlib(const unsigned char *sumber, size_t panjang_sumber,
                 unsigned char *tujuan, size_t *panjang_tujuan) {
    z_stream stream_zlib;
    stream_zlib.zalloc = Z_NULL; stream_zlib.zfree = Z_NULL; stream_zlib.opaque = Z_NULL;
    if (deflateInit2(&stream_zlib, Z_DEFAULT_COMPRESSION, Z_DEFLATED, MAX_WBITS + 16, 8, Z_DEFAULT_STRATEGY) != Z_OK) {
        return -1;
    }
    stream_zlib.avail_in = panjang_sumber; stream_zlib.next_in = (unsigned char *)sumber;
    stream_zlib.avail_out = *panjang_tujuan; stream_zlib.next_out = tujuan;
    deflate(&stream_zlib, Z_FINISH);
    *panjang_tujuan = stream_zlib.total_out;
    deflateEnd(&stream_zlib);
    return 0;
}

int dekompres_data_zlib(const unsigned char *sumber, size_t panjang_sumber,
                   unsigned char *tujuan, size_t *panjang_tujuan) {
    z_stream stream_zlib;
    stream_zlib.zalloc = Z_NULL; stream_zlib.zfree = Z_NULL; stream_zlib.opaque = Z_NULL;
    stream_zlib.avail_in = 0; stream_zlib.next_in = Z_NULL;
    if (inflateInit2(&stream_zlib, MAX_WBITS + 16) != Z_OK) {
        return -1;
    }
    stream_zlib.avail_in = panjang_sumber; stream_zlib.next_in = (unsigned char *)sumber;
    stream_zlib.avail_out = *panjang_tujuan; stream_zlib.next_out = tujuan;
    inflate(&stream_zlib, Z_FINISH);
    *panjang_tujuan = stream_zlib.total_out;
    inflateEnd(&stream_zlib);
    return 0;
}


char* dapatkan_path_asli_dengan_ekstensi(const char *path_fuse) {
    char salinan_path_fuse[1024];
    strcpy(salinan_path_fuse, path_fuse);

    char *penanda_slash_pertama = strchr(salinan_path_fuse + 1, '/');
    char nama_area[256] = {0};
    char nama_file_di_fuse[1024] = {0}; 

    if (strcmp(path_fuse, "/") == 0) { 
        char *path_hasil = malloc(strlen(direktori_sumber_chiho) + 2);
        sprintf(path_hasil, "%s/", direktori_sumber_chiho);
        return path_hasil;
    }

    if (penanda_slash_pertama) { 
        strncpy(nama_area, salinan_path_fuse + 1, penanda_slash_pertama - (salinan_path_fuse + 1));
        strcpy(nama_file_di_fuse, penanda_slash_pertama + 1);
    } else { 
        strcpy(nama_area, salinan_path_fuse + 1);
        nama_file_di_fuse[0] = '\0'; 
    }

    char buffer_path_asli[2048];

    if (strcmp(nama_area, "7sref") == 0 && strlen(nama_file_di_fuse) > 0) {
        char *penanda_underscore = strchr(nama_file_di_fuse, '_');
        if (penanda_underscore) {
            char area_target[256];
            char nama_file_target_di_fuse[1024];
            strncpy(area_target, nama_file_di_fuse, penanda_underscore - nama_file_di_fuse);
            area_target[penanda_underscore - nama_file_di_fuse] = '\0';
            strcpy(nama_file_target_di_fuse, penanda_underscore + 1);

            char path_fuse_rekonstruksi[1280];
            sprintf(path_fuse_rekonstruksi, "/%s/%s", area_target, nama_file_target_di_fuse);
            return dapatkan_path_asli_dengan_ekstensi(path_fuse_rekonstruksi); 
        } else { 
            sprintf(buffer_path_asli, "%s/%s/%s", direktori_sumber_chiho, nama_area, nama_file_di_fuse);
        }
    } else {
        const char *ekstensi_file = "";
        if (strcmp(nama_area, "starter") == 0) { ekstensi_file = ".mai"; }
        else if (strcmp(nama_area, "metro") == 0) { ekstensi_file = ".ccc"; }
        else if (strcmp(nama_area, "dragon") == 0) { ekstensi_file = ".rot"; }
        else if (strcmp(nama_area, "blackrose") == 0) { ekstensi_file = ".bin"; }
        else if (strcmp(nama_area, "heaven") == 0) { ekstensi_file = ".enc"; }
        else if (strcmp(nama_area, "youth") == 0) { ekstensi_file = ".gz"; }

        if (strlen(nama_file_di_fuse) > 0) { 
            sprintf(buffer_path_asli, "%s/%s/%s%s", direktori_sumber_chiho, nama_area, nama_file_di_fuse, ekstensi_file);
        } else { 
            sprintf(buffer_path_asli, "%s/%s", direktori_sumber_chiho, nama_area);
        }
    }
    
    return strdup(buffer_path_asli);
}


static int maimai_getattr(const char *path, struct stat *stbuf) {
    int hasil_op;
    char *path_asli_chiho;

    memset(stbuf, 0, sizeof(struct stat)); 

    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2; 
        return 0;
    }

    char salinan_path[1024];
    strcpy(salinan_path, path);
    char *penanda_slash = strchr(salinan_path + 1, '/');

    if (!penanda_slash) { 
        const char *daftar_area_valid[] = {"starter", "metro", "dragon", "blackrose", "heaven", "youth", "7sref"};
        int adalah_root_area = 0;
        for (size_t i = 0; i < sizeof(daftar_area_valid)/sizeof(daftar_area_valid[0]); ++i) {
            if (strcmp(path + 1, daftar_area_valid[i]) == 0) {
                adalah_root_area = 1;
                break;
            }
        }
        if (adalah_root_area) {
            path_asli_chiho = dapatkan_path_asli_dengan_ekstensi(path); 
            hasil_op = lstat(path_asli_chiho, stbuf);
            
            stbuf->st_mode = S_IFDIR | 0755;
            stbuf->st_nlink = 2; 
            free(path_asli_chiho);
            return 0; 
        }
    }
    
    path_asli_chiho = dapatkan_path_asli_dengan_ekstensi(path);
    hasil_op = lstat(path_asli_chiho, stbuf);
    free(path_asli_chiho);

    if (hasil_op == -1) {
        return -errno;
    }
    return 0;
}

static int maimai_readdir(const char *path, void *buf, fuse_fill_dir_t pengisi_direktori,
                          off_t offset, struct fuse_file_info *fi) {
    (void) offset; 
    (void) fi;

    pengisi_direktori(buf, ".", NULL, 0);
    pengisi_direktori(buf, "..", NULL, 0);

    if (strcmp(path, "/") == 0) {
        pengisi_direktori(buf, "starter", NULL, 0);
        pengisi_direktori(buf, "metro", NULL, 0);
        pengisi_direktori(buf, "dragon", NULL, 0);
        pengisi_direktori(buf, "blackrose", NULL, 0);
        pengisi_direktori(buf, "heaven", NULL, 0);
        pengisi_direktori(buf, "youth", NULL, 0);
        pengisi_direktori(buf, "7sref", NULL, 0);
        return 0;
    }

    if (strcmp(path, "/7sref") == 0) {
        const char *area_sumber_untuk_7sref[] = {"starter", "metro", "dragon", "blackrose", "heaven", "youth"};
        for (int i = 0; i < 6; i++) {
            char path_fuse_area_sumber[1024];
            sprintf(path_fuse_area_sumber, "/%s", area_sumber_untuk_7sref[i]);
            
            char *path_asli_area_sumber = dapatkan_path_asli_dengan_ekstensi(path_fuse_area_sumber); 

            DIR *dp_sumber = opendir(path_asli_area_sumber);
            if (dp_sumber) {
                struct dirent *entri_direktori_sumber;
                while ((entri_direktori_sumber = readdir(dp_sumber)) != NULL) {
                    if (entri_direktori_sumber->d_type == DT_REG) { 
                        char nama_file_asli_di_chiho[1024];
                        strcpy(nama_file_asli_di_chiho, entri_direktori_sumber->d_name);
                        
                        char nama_file_dasar_untuk_fuse[1024] = {0};
                        
                        char *penanda_titik = strrchr(nama_file_asli_di_chiho, '.');
                        int ekstensi_cocok_dihapus = 0;
                        if (penanda_titik) {
                            if (strcmp(area_sumber_untuk_7sref[i], "starter") == 0 && strcmp(penanda_titik, ".mai") == 0) { ekstensi_cocok_dihapus = 1; }
                            else if (strcmp(area_sumber_untuk_7sref[i], "metro") == 0 && strcmp(penanda_titik, ".ccc") == 0) { ekstensi_cocok_dihapus = 1; }
                            else if (strcmp(area_sumber_untuk_7sref[i], "dragon") == 0 && strcmp(penanda_titik, ".rot") == 0) { ekstensi_cocok_dihapus = 1; }
                            else if (strcmp(area_sumber_untuk_7sref[i], "blackrose") == 0 && strcmp(penanda_titik, ".bin") == 0) { ekstensi_cocok_dihapus = 1; }
                            else if (strcmp(area_sumber_untuk_7sref[i], "heaven") == 0 && strcmp(penanda_titik, ".enc") == 0) { ekstensi_cocok_dihapus = 1; }
                            else if (strcmp(area_sumber_untuk_7sref[i], "youth") == 0 && strcmp(penanda_titik, ".gz") == 0) { ekstensi_cocok_dihapus = 1; }

                            if (ekstensi_cocok_dihapus) {
                                strncpy(nama_file_dasar_untuk_fuse, nama_file_asli_di_chiho, penanda_titik - nama_file_asli_di_chiho);
                            } else { 
                                strcpy(nama_file_dasar_untuk_fuse, nama_file_asli_di_chiho);
                            }
                        } else { 
                             strcpy(nama_file_dasar_untuk_fuse, nama_file_asli_di_chiho);
                        }

                        if (strlen(nama_file_dasar_untuk_fuse) > 0) {
                            char nama_file_referensi_7sref[2048];
                            sprintf(nama_file_referensi_7sref, "%s_%s", area_sumber_untuk_7sref[i], nama_file_dasar_untuk_fuse);
                            pengisi_direktori(buf, nama_file_referensi_7sref, NULL, 0);
                        }
                    }
                }
                closedir(dp_sumber);
            }
            free(path_asli_area_sumber);
        }
        return 0;
    }

    char *path_asli_direktori_chiho = dapatkan_path_asli_dengan_ekstensi(path); 
    DIR *dp = opendir(path_asli_direktori_chiho);
    if (!dp) {
        free(path_asli_direktori_chiho);
        return -errno;
    }

    struct dirent *entri_direktori;
    char nama_area_saat_ini[256] = {0};
    
    if (path[0] == '/') {
        const char *awal_nama_area = path + 1;
        const char *akhir_nama_area = strchr(awal_nama_area, '/');
        if (akhir_nama_area) {
            strncpy(nama_area_saat_ini, awal_nama_area, akhir_nama_area - awal_nama_area);
        } else { 
            strcpy(nama_area_saat_ini, awal_nama_area);
        }
    }


    while ((entri_direktori = readdir(dp)) != NULL) {
        if (strcmp(entri_direktori->d_name, ".") == 0 || strcmp(entri_direktori->d_name, "..") == 0) {
            continue;
        }

        char nama_file_untuk_ditampilkan[1024];
        strcpy(nama_file_untuk_ditampilkan, entri_direktori->d_name);

        char *penanda_titik_ekstensi = strrchr(nama_file_untuk_ditampilkan, '.');
        if (penanda_titik_ekstensi) {
            int hapus_ekstensi = 0;
            if (strcmp(nama_area_saat_ini, "starter") == 0 && strcmp(penanda_titik_ekstensi, ".mai") == 0) { hapus_ekstensi = 1; }
            else if (strcmp(nama_area_saat_ini, "metro") == 0 && strcmp(penanda_titik_ekstensi, ".ccc") == 0) { hapus_ekstensi = 1; }
            else if (strcmp(nama_area_saat_ini, "dragon") == 0 && strcmp(penanda_titik_ekstensi, ".rot") == 0) { hapus_ekstensi = 1; }
            else if (strcmp(nama_area_saat_ini, "blackrose") == 0 && strcmp(penanda_titik_ekstensi, ".bin") == 0) { hapus_ekstensi = 1; }
            else if (strcmp(nama_area_saat_ini, "heaven") == 0 && strcmp(penanda_titik_ekstensi, ".enc") == 0) { hapus_ekstensi = 1; }
            else if (strcmp(nama_area_saat_ini, "youth") == 0 && strcmp(penanda_titik_ekstensi, ".gz") == 0) { hapus_ekstensi = 1; }
            
            if (hapus_ekstensi) {
                *penanda_titik_ekstensi = '\0'; 
            }
        }
        pengisi_direktori(buf, nama_file_untuk_ditampilkan, NULL, 0);
    }
    closedir(dp);
    free(path_asli_direktori_chiho);
    return 0;
}

static int maimai_read(const char *path_fuse, char *buffer_tujuan, size_t ukuran_baca, off_t offset_baca,
                       struct fuse_file_info *fi) {
    (void) fi;
    char *path_asli_chiho = dapatkan_path_asli_dengan_ekstensi(path_fuse);
    int fd = open(path_asli_chiho, O_RDONLY);
    if (fd == -1) {
        free(path_asli_chiho);
        return -errno;
    }

    char nama_area_efektif[256] = {0};
  
    if (strncmp(path_fuse, "/7sref/", 7) == 0) {
        char temp_path_7sref[1024];
        strcpy(temp_path_7sref, path_fuse + 7); 
        char *penanda_underscore = strchr(temp_path_7sref, '_');
        if (penanda_underscore) {
            strncpy(nama_area_efektif, temp_path_7sref, penanda_underscore - temp_path_7sref);
        } else {  }
    } else if (path_fuse[0] == '/') {
        const char *awal_nama_area = path_fuse + 1;
        const char *akhir_nama_area = strchr(awal_nama_area, '/');
        if (akhir_nama_area) {
            strncpy(nama_area_efektif, awal_nama_area, akhir_nama_area - awal_nama_area);
        } else { }
    }


    struct stat info_file;
    fstat(fd, &info_file);
    char *buffer_data_mentah = malloc(info_file.st_size);
    if (!buffer_data_mentah) { 
        close(fd); 
        free(path_asli_chiho); 
        return -ENOMEM; 
    }

    int hasil_baca_pread = pread(fd, buffer_data_mentah, info_file.st_size, 0);
    close(fd);
    if (hasil_baca_pread == -1) { 
        free(buffer_data_mentah); 
        free(path_asli_chiho); 
        return -errno; 
    }

    size_t ukuran_data_setelah_proses = hasil_baca_pread;
    char *data_setelah_proses = NULL; 

    if (strcmp(nama_area_efektif, "metro") == 0) {
        data_setelah_proses = malloc(ukuran_data_setelah_proses); 
        memcpy(data_setelah_proses, buffer_data_mentah, ukuran_data_setelah_proses);
        balikan_geser_karakter_metro(data_setelah_proses, ukuran_data_setelah_proses);
    } else if (strcmp(nama_area_efektif, "dragon") == 0) {
        data_setelah_proses = malloc(ukuran_data_setelah_proses + 1); 
        memcpy(data_setelah_proses, buffer_data_mentah, ukuran_data_setelah_proses);
        data_setelah_proses[ukuran_data_setelah_proses] = '\0'; 
        rot13(data_setelah_proses);
    } else if (strcmp(nama_area_efektif, "heaven") == 0) {
        if (ukuran_data_setelah_proses < 16) { free(buffer_data_mentah); free(path_asli_chiho); return 0; } 
        unsigned char *iv_enkripsi = (unsigned char *)buffer_data_mentah;
        unsigned char *pointer_data_terenkripsi = (unsigned char *)(buffer_data_mentah + 16);
        size_t ukuran_data_terenkripsi = ukuran_data_setelah_proses - 16;
        data_setelah_proses = malloc(ukuran_data_terenkripsi + AES_BLOCK_SIZE); 
        int panjang_hasil_dekripsi = dekripsi_aes(pointer_data_terenkripsi, ukuran_data_terenkripsi, kunci_aes, iv_enkripsi, (unsigned char*)data_setelah_proses);
        if (panjang_hasil_dekripsi < 0) { free(data_setelah_proses); free(buffer_data_mentah); free(path_asli_chiho); return -EIO; }
        ukuran_data_setelah_proses = panjang_hasil_dekripsi;
    } else if (strcmp(nama_area_efektif, "youth") == 0) {
        size_t ukuran_buffer_dekompresi = info_file.st_size * 10; 
        if (ukuran_buffer_dekompresi < 1024) { ukuran_buffer_dekompresi = 1024; } 
        data_setelah_proses = malloc(ukuran_buffer_dekompresi);
        size_t ukuran_aktual_setelah_dekompresi = ukuran_buffer_dekompresi;
        if (dekompres_data_zlib((unsigned char*)buffer_data_mentah, ukuran_data_setelah_proses, (unsigned char*)data_setelah_proses, &ukuran_aktual_setelah_dekompresi) != 0) {
            free(data_setelah_proses); free(buffer_data_mentah); free(path_asli_chiho); return -EIO;
        }
        ukuran_data_setelah_proses = ukuran_aktual_setelah_dekompresi;
    } else { 
        data_setelah_proses = malloc(ukuran_data_setelah_proses);
        memcpy(data_setelah_proses, buffer_data_mentah, ukuran_data_setelah_proses);
    }
    
    free(buffer_data_mentah);

    if (offset_baca < ukuran_data_setelah_proses) {
        size_t sisa_data = ukuran_data_setelah_proses - offset_baca;
        if (ukuran_baca > sisa_data) {
            ukuran_baca = sisa_data;
        }
        memcpy(buffer_tujuan, data_setelah_proses + offset_baca, ukuran_baca);
    } else {
        ukuran_baca = 0;
    }

    free(data_setelah_proses);
    free(path_asli_chiho);
    return ukuran_baca;
}

static int maimai_write(const char *path_fuse, const char *buffer_sumber, size_t ukuran_tulis,
                        off_t offset_tulis, struct fuse_file_info *fi) {
    (void) fi;
    char nama_area_efektif[256] = {0};

    if (strncmp(path_fuse, "/7sref/", 7) == 0) {
        char temp_path_7sref[1024];
        strcpy(temp_path_7sref, path_fuse + 7);
        char *penanda_underscore = strchr(temp_path_7sref, '_');
        if (penanda_underscore) {
            strncpy(nama_area_efektif, temp_path_7sref, penanda_underscore - temp_path_7sref);
        } else { return -EINVAL;
    } else if (path_fuse[0] == '/') {
        const char *awal_nama_area = path_fuse + 1;
        const char *akhir_nama_area = strchr(awal_nama_area, '/');
        if (akhir_nama_area) {
            strncpy(nama_area_efektif, awal_nama_area, akhir_nama_area - awal_nama_area);
        } else { return -EINVAL; 
    } else { return -EINVAL;


    char *data_untuk_ditulis_ke_chiho = NULL;
    size_t ukuran_data_untuk_ditulis_ke_chiho = ukuran_tulis;

    if (strcmp(nama_area_efektif, "metro") == 0) {
        data_untuk_ditulis_ke_chiho = malloc(ukuran_tulis); 
        memcpy(data_untuk_ditulis_ke_chiho, buffer_sumber, ukuran_tulis);
        geser_karakter_metro(data_untuk_ditulis_ke_chiho, ukuran_tulis);
    } else if (strcmp(nama_area_efektif, "dragon") == 0) {
        data_untuk_ditulis_ke_chiho = malloc(ukuran_tulis + 1); 
        memcpy(data_untuk_ditulis_ke_chiho, buffer_sumber, ukuran_tulis);
        data_untuk_ditulis_ke_chiho[ukuran_tulis] = '\0'; 
        rot13(data_untuk_ditulis_ke_chiho);
    } else if (strcmp(nama_area_efektif, "heaven") == 0) {
        unsigned char iv_enkripsi[16];
        if (RAND_bytes(iv_enkripsi, sizeof(iv_enkripsi)) != 1) { return -EIO; }
        data_untuk_ditulis_ke_chiho = malloc(16 + ukuran_tulis + AES_BLOCK_SIZE); 
        memcpy(data_untuk_ditulis_ke_chiho, iv_enkripsi, 16); 
        int panjang_hasil_enkripsi = enkripsi_aes((unsigned char*)buffer_sumber, ukuran_tulis, kunci_aes, iv_enkripsi, (unsigned char*)(data_untuk_ditulis_ke_chiho + 16));
        if (panjang_hasil_enkripsi < 0) { free(data_untuk_ditulis_ke_chiho); return -EIO; }
        ukuran_data_untuk_ditulis_ke_chiho = 16 + panjang_hasil_enkripsi;
    } else if (strcmp(nama_area_efektif, "youth") == 0) {
        
        size_t ukuran_buffer_kompresi = ukuran_tulis + (ukuran_tulis / 10) + 128; 
        data_untuk_ditulis_ke_chiho = malloc(ukuran_buffer_kompresi);
        size_t ukuran_aktual_setelah_kompresi = ukuran_buffer_kompresi;
        if (kompres_data_zlib((unsigned char*)buffer_sumber, ukuran_tulis, (unsigned char*)data_untuk_ditulis_ke_chiho, &ukuran_aktual_setelah_kompresi) != 0) {
            free(data_untuk_ditulis_ke_chiho); return -EIO;
        }
        ukuran_data_untuk_ditulis_ke_chiho = ukuran_aktual_setelah_kompresi;
    } else { 
        data_untuk_ditulis_ke_chiho = malloc(ukuran_tulis);
        memcpy(data_untuk_ditulis_ke_chiho, buffer_sumber, ukuran_tulis);
    }

    char *path_asli_chiho = dapatkan_path_asli_dengan_ekstensi(path_fuse);
            
    int flags_open = O_WRONLY | O_CREAT | (offset_tulis > 0 ? 0 : O_TRUNC);
    int fd = open(path_asli_chiho, flags_open , 0644); 
    if (fd == -1) {
        free(data_untuk_ditulis_ke_chiho);
        free(path_asli_chiho);
        return -errno;
    }

    int hasil_tulis_pwrite = pwrite(fd, data_untuk_ditulis_ke_chiho, ukuran_data_untuk_ditulis_ke_chiho, offset_tulis);
    close(fd);
    free(data_untuk_ditulis_ke_chiho);
    free(path_asli_chiho);

    if (hasil_tulis_pwrite == -1) {
        return -errno;
    }
    return hasil_tulis_pwrite;
}

static int maimai_create(const char *path_fuse, mode_t mode, struct fuse_file_info *fi) {
    char *path_asli_chiho = dapatkan_path_asli_dengan_ekstensi(path_fuse);
    int fd = open(path_asli_chiho, O_WRONLY | O_CREAT | O_EXCL | O_TRUNC, mode);
    free(path_asli_chiho);

    if (fd == -1) {
        return -errno;
    }
    
    fi->fh = fd; 
    return 0;
}

static int maimai_unlink(const char *path_fuse) {
    char *path_asli_chiho = dapatkan_path_asli_dengan_ekstensi(path_fuse);
    int hasil_op = unlink(path_asli_chiho);
    free(path_asli_chiho);
    if (hasil_op == -1) {
        return -errno;
    }
    return 0;
}

static int maimai_mkdir(const char *path_fuse, mode_t mode) {
    char *path_asli_chiho = dapatkan_path_asli_dengan_ekstensi(path_fuse); 
    int hasil_op = mkdir(path_asli_chiho, mode); 
    free(path_asli_chiho);
    if (hasil_op == -1) {
        return -errno;
    }
    return 0;
}

static int maimai_release(const char *path_fuse, struct fuse_file_info *fi) {
    (void)path_fuse; 
    if (fi->fh != 0) { 
        close(fi->fh);
        fi->fh = 0; 
    }
    return 0;
}

static int maimai_open(const char *path_fuse, struct fuse_file_info *fi) {
    
    char *path_asli_chiho = dapatkan_path_asli_dengan_ekstensi(path_fuse);
    int fd = open(path_asli_chiho, fi->flags); 
    free(path_asli_chiho);
    if (fd == -1) {
        return -errno;
    }
    fi->fh = fd; 
    return 0;
}


static struct fuse_operations maimai_oper = {
    .getattr    = maimai_getattr,
    .readdir    = maimai_readdir,
    .read       = maimai_read,
    .write      = maimai_write,
    .create     = maimai_create,
    .unlink     = maimai_unlink,
    .mkdir      = maimai_mkdir,
    .open       = maimai_open,      
    .release    = maimai_release,   
};

int main(int argc, char *argv[]) {
    OpenSSL_add_all_algorithms(); 
    const char *areas[] = {"starter", "metro", "dragon", "blackrose", "heaven", "youth", "7sref"};
    for (size_t i = 0; i < sizeof(areas) / sizeof(areas[0]); ++i) {
        char path_area_chiho[1024];
        sprintf(path_area_chiho, "%s/%s", direktori_sumber_chiho, areas[i]);
        mkdir(path_area_chiho, 0755); 
    }

    return fuse_main(argc, argv, &maimai_oper, NULL);
}

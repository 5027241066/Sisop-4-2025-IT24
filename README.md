# Laporan Resmi Praktikum Sisop Modul 3


## Anggota Kelompok

| No | Nama                   | NRP         |
|----|------------------------|-------------|
| 1  | Aditya Reza Daffansyah | 5027241034  |
| 2  | Ahmad Yafi Ar Rizq     | 5027241066  |
| 3  | Zahra Khaalishah       | 5027241070  |



## Daftar Isi
### Soal 1
- [a. ]()
- [b. ]()
- [c. ]()
- [d. ]()

  
### Soal 2
- [a. Membuat Sistem FUSE untuk Merepresentasikan Gambar](#a-membuat-sistem-fuse-untuk-merepresentasikan-gambar)
- [b. Menggabungkan Pecahan Relics menjadi Gambar Utuh](#b-menggabungkan-pecahan-relics-menjadi-gambar-utuh)
- [c. Membuat File Baru dan Memecah File menjadi Beberapa Bagian](#c-membuat-file-baru-dan-memecah-file-menjadi-beberapa-bagian)
- [d. Menghapus File akan Menghapus Pecahannya di Relics](#d-menghapus-file-akan-menghapus-pecahannya-di-relics)
- [e. Pencacatan Log Aktivitas](#e-pencatatan-log-aktivitas)
- [f. Tambahan Penjelasan Main Function](f-tambahan-penjelasan-main-function)
  

 
### Soal 3
- [a. ](#a-)
- [b.](#b-)
- [c.](#c-)
- [d.](#d-)
- [e. ](#e-)

 
### Soal 4
- [a. Starter Area - Starter Chiho](#a-starter-area-starter--chiho)
- [b. World's End Area - Metropolis Chiho](#b-world-s-end-area--metropolis-chiho)
- [c. World Tree Area - Dragon Chiho](#c-world-tree-area--dragon-chiho)
- [d. Black Rose Area - Black Rose Chiho](#d-black-rose-area--black-rose-chiho)
- [e. Tenkai Area - Heaven Chiho](#e-tenkai-area--heaven-chiho)
- [f. Youth Area - Skystreet Chiho](#f-youth-area--skystreet-chiho)
- [g. Prism Area - 7sRef Chiho](#g-prism-area--7sref-chiho)



# Soal 1

# Soal 2
## a. Membuat Sistem FUSE untuk Merepresentasikan Gambar

```
#define NAMA_FILE_UTUH "Baymax.jpeg"

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
}

static int fs_readdir(const char *jalur, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi, enum fuse_readdir_flags flags) {
    (void) offset;
    (void) fi;
    (void) flags;

    if (strcmp(jalur, "/") != 0) return -ENOENT;

    filler(buf, ".", NULL, 0, 0);
    filler(buf, "..", NULL, 0, 0);
    
    filler(buf, NAMA_FILE_UTUH, NULL, 0, 0);

}
```

- `#define NAMA_FILE_UTUH "Baymax.jpeg"` mendefinisikan konstanta yang menyimpan nama file virtual agar mencegah terjadinya kesalahan penulisan dan pembacaan pada code.
- `fs_getattr` dipanggil oleh FUSE ketika perlu untuk mendapat atribut dari suatu file.
    - `if (strcmp(jalur + 1, NAMA_FILE_UTUH) == 0)` digunakan untuk memeriksa apakah jalur yg diminta sesuai. Jika sesuai maka akan mengeksekusi kode dalam blok `if`.
    - `stbuf->st_mode = S_IFREG | 0444` akan menandakan bahwa ini adalah file reguler. `0444` memberikan izin baca untuk semua pengguna.
    - `for` akan melakukan loop untuk menghitung ukuran file "Baymax.jpeg" dengan menjumlahkan semua ukuran pecahannya.
    - Jika tidak ada pecahan file, akan return sebagai `ENOENT` (no such file or directory).
- `fs_readdir` digunakan ketika membutuhkan daftar isi pada sebuah direktori.
    - `filler(bf, NAMA_FILE_UTUH, NULL, 0, 0);` akan menambahkan entri ke daftar direktori, memastikan "Baymax.jpeg" akan selalu muncul pada direktori mount, meskipun dalam bentuk pecahan-pecahan file.

 
## b. Menggabungkan Pecahan Relics menjadi Gambar Utuh

```
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
```

- Fungsi `fs_open` digunakan untuk membuka sebuah file.
     - `if(strcmp(jalur +1, NAMA_FILE_UTUH) != 0) return -ENOENT` memastikan open hanya diizinkan untuk file "Baymax.jpeg". Mengembalikan error jika input tidak sesuai.
     - `catat_log(READ : %s, NAMA_FILE_UTUH);` akan mencatat log aktivitas untuk pembukaan suatu file.
- Fungsi `fs_read` digunakan untuk membaca data suatu file, fungsi ini merupakan fungsi esensial dalam hal rekonstruksi file "Baymax.jpeg".
     - Kode ini akan menghitung jumlah pecahan file.
     - Loop utama akan membaca file pecahan berurutan.
     - `offset` dan `size` akan menangani pembacaaan untuk menghitung bagian mana dari setiap pecahan yang perlu dibaca berdasarkan posisi awal dan jumlah byte yang diminta.
     - Data yang telah dibaca akan disalin dlam buffer `buf` oleh FUSE.
- Operasi "tampil" dan "salin" diimplementasikan melalui fungsi `fs_read`. ketika terdapat command seperti `cat` atau `cp`, maka FUSE akan memanggil `fs_read` untuk mendapatkan data file.


## c. Membuat File Baru dan Memecah File menjadi Beberapa Bagian

```
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

    // Jika file adalah Baymax.jpeg, abaikan saja
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
```

- `fs_create` digunakan untuk membuat  sebuah file baru. 
  - Membuat file sementara di `/tmp/` dengan nama yang sama dengan file yang dibuat di     direktori mount. Tujuannya adalah untuk menampung data yang ditulis sebelum dipecah.
  - `catat_log("CREATE: %s", nama_file);` Mencatat pembuatan file ke log.
- `fs_write` digunakan ketika data ditulis ke file yang sudah dibuka.
  - Menulis data ke file sementara di `/tmp/`. Jika offset lebih besar dari 0, maka akan membuka file dalam mode r+ agar data yang ditulis tidak menimpa data yang sudah ada.
  - `catat_log("WRITE: %s (%zu bytes pada offset %lld)", nama_file, written, (long long)offset);`: Mencatat operasi penulisan ke log.
- `fs_release` digunakan ketika file ditutup. Logika pemecahan file diterapkan pada fungsi ini
  - Membaca data dari file sementara di `/tmp/`.
  - Memecah data yang dibaca menjadi potongan-potongan berukuran `UKURAN_PECAHAN` (1KB) dan menulis setiap potongan ke file terpisah di direktori "relics". Penamaan file pecahan adalah "`[namafile].000`", "`[namafile].001`", dst.
  - Menghapus file sementara di `/tmp/` setelah selesai.
  - `catat_log("RELEASE: %s -> %s.000 - %s.%03d", nama_file, nama_file, nama_file, idx - 1);` akan mencatat operasi pelepasan (yang mencakup pemecahan dan penyimpanan) ke log.

## d. Menghapus File akan Menghapus Pecahannya di Relics

```
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
```

- fs_unlink digunakan untuk menghapus sebuah file.
  -Kode ini menghapus semua pecahan file yang sesuai di direktori "relics". Loop for digunakan untuk mengiterasi hingga 1000, mencoba menghapus setiap kemungkinan pecahan file ([namafile].000, [namafile].001, dst.).
  - `access(path_pecahan, F_OK) == 0` akan memeriksa apakah file pecahan ada sebelum mencoba menghapusnya.
  - `remove(path_pecahan) == 0` akan menghapus file pecahan. Jika penghapusan gagal, pesan error dicatat.
  - `catat_log("DELETE: %s.000 - %s.%03d", nama_file, nama_file, sukses - 1);` digunakan dalam pencatatan aktivitas penghapusan ke log, termasuk rentang pecahan file yang dihapus.

## e. Pencatatan Log Aktivitas

```
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
```

- `catat_log`digunakan untuk menyederhanakan proses pencatatan log.
  - `FILE *log = fopen(FILE_LOG, "a");` akan membuka file log ("activity.log") dalam mode append. Ini memastikan bahwa setiap pesan log baru ditambahkan ke akhir file tanpa menimpa konten yang sudah ada. Jika file tidak ada, file akan dibuat.
  - `time_t t = time(NULL); struct tm *tm = localtime(&t);` akan mendapatkan timestamp saat ini dan mengonversinya ke dalam format waktu lokal.
  - `fprintf(log, "[%04d-%02d-%02d %02d:%02d:%02d] ", ...);` digunakan dalam penulisan timestamp ke file log dalam format `[YYYY-MM-DD HH:MM:SS]`.
  - `va_list args; va_start(args, format); vfprintf(log, format, args); va_end(args);` berguna untuk menangani variable arguments. `catat_log` dapat dipanggil dengan berbagai format pesan (seperti `printf`). `va_list, va_start, vfprintf, dan va_end` adalah bagian dari standar C untuk bekerja dengan fungsi yang menerima sejumlah argumen yang tidak diketahui.
  - `fprintf(log, "\n"); fclose(log);` akan menambahkan newline ke pesan log dan menutup file.

## f. Tambahan Penjelasan Main Function

```
int main(int argc, char *argv[]) {
    /* Pastikan direktori relics ada */
    if (access(DIREKTORI_RELICS, F_OK) != 0) {
        if (mkdir(DIREKTORI_RELICS, 0755) != 0) {
            fprintf(stderr, "ERROR: Gagal membuat direktori %s: %s\n", 
                    DIREKTORI_RELICS, strerror(errno));
            return 1;
        }
    }
    
    /* Periksa dan log informasi tentang file pecahan yang ada */
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
```

- Inisialisasi Direktori:
  - Kode ini memastikan bahwa direktori "relics" ada sebelum `FUSE filesystem` dimulai. Jika direktori tidak ada, kode akan mencoba membuatnya. Ini penting karena sistem file virtual bergantung pada direktori ini untuk menyimpan pecahan file.
- Pemeriksaan File Pecahan:
  - Kode ini memeriksa keberadaan pecahan file "Baymax.jpeg" saat program dimulai. Ini berguna untuk:
    - Memverifikasi bahwa pecahan file yang diharapkan ada.
    - Memberikan informasi debugging jika ada masalah dengan file pecahan.
    - Menampilkan ukuran setiap pecahan file.
- `fuse_main`
  - `umask(0);` digunakan untuk mengatur file creation mask ke 0, yang berarti file akan dibuat dengan izin yang ditentukan dalam fungsi create.
  - `return fuse_main(argc, argv, &operasi, NULL);` akan memulai loop utama FUSE. `fuse_main` adalah fungsi yang disediakan oleh pustaka FUSE yang mengambil alih eksekusi program dan memproses operasi filesystem (seperti `read`, `write`, `getattr`, dll.) yang diminta oleh sistem operasi. operasi adalah struct yang berisi pointer ke fungsi-fungsi yang kita definisikan ( `fs_getattr`, `fs_read`, dll.).


# Soal 3

# Soal 4
## a. Starter Area - Starter Chiho
## b. World's End Area - Metropolis Chiho
## c. World Tree Area - Dragon Chiho
## d. Black Rose Area - Black Rose Chiho
## e. Tenkai Area - Heaven Chiho
## f. Youth Area - Skystreet Chiho
## g. Prism Area - 7sRef Chiho


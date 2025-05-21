# Laporan Resmi Praktikum Sisop Modul 3


## Anggota Kelompok

| No | Nama                   | NRP         |
|----|------------------------|-------------|
| 1  | Aditya Reza Daffansyah | 5027241034  |
| 2  | Ahmad Yafi Ar Rizq     | 5027241066  |
| 3  | Zahra Khaalishah       | 5027241070  |



## Daftar Isi
### Soal 1
- [a. Download Unzip](#a-download-unzip)
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
```
#define BASE_DIR "anomali"
#define IMAGE_DIR "anomali/image"
#define LOG_FILE "anomali/conversion.log"
#define ZIP_URL "https://drive.usercontent.google.com/u/0/uc?id=1hi_GDdP51Kn2JJMw02WmCOxuc3qrXzh5&export=download"
#define ZIP_NAME "anomali.zip"
```
Code berikut berfungsi untuk mendefinisikan directory file txt setelah extract zip, directory file image yang akan digunakan untuk convert, lokasi log file, link download ZIP dan nama dari file yang akan di download.

## a. Download Unzip
```
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
```
Fungsi download menggunakan `curl`. Saat proses berjalan, akan ditampilkan pesan "Downloading zip file...". Setelah file berhasil didownload, kode akan mengekstrak file tersebut dan menampilkan pesan "Files extracted successfully to anomali/".

Code `%s` merujuk pada `BASE_DIR`, yaitu folder anomali. Jika proses ekstrak gagal, maka akan muncul pesan "Unzip failed". Apabila ekstraksi berhasil, file ZIP akan dihapus secara otomatis menggunakan `remove(ZIP_NAME)`.

## b. Convert dari hexadecimal menjadi image
```
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

    char output_path[512];
    snprintf(output_path, sizeof(output_path), "%s/%s", IMAGE_DIR, img_filename);

    FILE *img = fopen(output_path, "wb");
    if (img) {
        fwrite(img_data, 1, len, img);
        fclose(img);
        printf("Created image: %s\n", output_path);
    }

    free(hex_data);
    free(clean_hex);
    free(img_data);
    pthread_mutex_unlock(&convert_mutex);
}
```
```
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
```
Fungsi converthex digunakan untuk mengubah isi file `.txt` yang berisi hexadecimal menjadi file gambar `.png`. Pertama-tama code membuka file `.txt` dari direktori anomali dan membaca seluruh isinya ke dalam memori. Setelah isi file dibaca, karakter yang bukan hexadecimal akan dibuang. Data yang sudah dibersihkan kemudian dicek menggunakan fungsi `checkhex` untuk memastikan  hanya karakter hexadecimal yang didalamnya. Ketika code berjalan akan muncul message "Processing file: (file path & name)". Namun jika gagal maka akan muncul message "Failed to open file". 

Selanjutnya, data hexadecimal di convert menjadi data biner (byte array) menggunakan `sscanf`. Hasil convert dimasukkan ke file gambar `.png` di dalam direktori `anomali/image`. Fungsi `converthex` dilakukan pada fungsi `xmp_open`. Apabila file yang dibuka memiliki ekstensi `.txt`, maka secara otomatis isi file akan dikonversi menjadi gambar menggunakan `converthex`. Ketika file berhasil di convert maka akan muncul message "Created image: (file name)"

## c. File Name
```
char file_timestamp[32];
    strftime(file_timestamp, sizeof(file_timestamp), "%Y-%m-%d_%H:%M:%S", tm_info);
```
```
char img_filename[256];
    snprintf(img_filename, sizeof(img_filename), "%.*s_image_%s.png", 
         (int)(strlen(filename) - 4), filename, file_timestamp);
char output_path[512];
snprintf(output_path, sizeof(output_path), "%s/%s", IMAGE_DIR, img_filename);
```
File name disini menggunakan format [nama file]_image_[YYYY-mm-dd]_[HH:MM:SS]. Untuk tanggal dan jam diambil current time saat proses konversi.

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

```
static int get_real_path_chiho(char *full_path, const char *path) {
    // Fungsi utilitas untuk mendapatkan path asli di direktori sumber
    // Misal: /fuse_dir/starter/file.txt -> /home/deefen/sisopmodul4/soalno4/chiho/starter/file.txt
    sprintf(full_path, "%s%s", direktori_sumber_chiho, path);
    return 0;
}

// ... (dalam fungsi maimai_getattr, maimai_readdir, maimai_read, maimai_write, dll.)

static int maimai_getattr(const char *path, struct stat *stbuf) {
    char path_asli_chiho[1024];
    get_real_path_chiho(path_asli_chiho, path); // Mendapatkan path asli
    // ...
    // Di sini tidak ada penanganan khusus untuk "starter" karena tidak ada enkripsi/transformasi.
    // stat() akan dipanggil langsung pada path_asli_chiho.
    // ...
}

static int maimai_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                           off_t offset, struct fuse_file_info *fi) {
    char path_asli_chiho[1024];
    get_real_path_chiho(path_asli_chiho, path); // Mendapatkan path asli
    // ...
    // Di sini tidak ada penanganan khusus untuk "starter".
    // readdir() akan langsung membaca isi direktori asli.
    // ...
}

static int maimai_read(const char *path, char *buf, size_t size, off_t offset,
                        struct fuse_file_info *fi) {
    // ...
    if (strncmp(path_relatif + 1, "starter", 7) == 0) { // Cek jika path dimulai dengan "/starter"
        // Tidak ada enkripsi/transformasi. Langsung baca dari file asli.
        ret = pread(fd, buf, size, offset);
    }
    // ...
}

static int maimai_write(const char *path, const char *buf, size_t size,
                         off_t offset, struct fuse_file_info *fi) {
    // ...
    if (strncmp(path_relatif + 1, "starter", 7) == 0) { // Cek jika path dimulai dengan "/starter"
        // Tidak ada enkripsi/transformasi. Langsung tulis ke file asli.
        ret = pwrite(fd, buf, size, offset);
    }
    // ...
}

static int maimai_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    char path_asli_chiho[1024];
    get_real_path_chiho(path_asli_chiho, path); // Mendapatkan path asli
    // ...
    // Jika di direktori starter, tidak ada perubahan nama atau enkripsi.
    // File akan dibuat langsung dengan nama dan mode yang diberikan.
    // ...
}

static int maimai_unlink(const char *path) {
    char path_asli_chiho[1024];
    get_real_path_chiho(path_asli_chiho, path); // Mendapatkan path asli
    // ...
    // Jika di direktori starter, langsung hapus file.
    // ...
}

static int maimai_mkdir(const char *path, mode_t mode) {
    char path_asli_chiho[1024];
    get_real_path_chiho(path_asli_chiho, path); // Mendapatkan path asli
    // ...
    // Jika di direktori starter, langsung buat direktori.
    // ...
}

// ... (dalam main)
const char *areas[] = {"starter", "metro", "dragon", "blackrose", "heaven", "youth", "7sref"};
for (size_t i = 0; i < sizeof(areas) / sizeof(areas[0]); ++i) {
    char area_path[1024];
    sprintf(area_path, "%s/%s", direktori_sumber_chiho, areas[i]);
    mkdir(area_path, 0755); // Memastikan direktori starter ada
}
```

## b. World's End Area - Metropolis Chiho

```
// Fungsi utilitas untuk Metro Shift
void metro_shift(char *teks) {
    while (*teks) {
        if (*teks == 'A' || *teks == 'a') {
            *teks = 'I';
        } else if (*teks == 'I' || *teks == 'i') {
            *teks = 'O';
        } else if (*teks == 'O' || *teks == 'o') {
            *teks = 'A';
        }
        teks++;
    }
}

// ...

static int maimai_getattr(const char *path, struct stat *stbuf) {
    // ...
    char path_asli_chiho[1024];
    char nama_file_metro_shifted[1024];
    char *nama_file_start = strrchr(path_relatif, '/');
    if (nama_file_start) {
        nama_file_start++; // Lewati '/'
    } else {
        nama_file_start = path_relatif; // Jika tidak ada '/', berarti path adalah nama file itu sendiri
    }

    if (strncmp(path_relatif + 1, "metro/", 6) == 0 && nama_file_start != NULL && *nama_file_start != '\0') {
        strcpy(nama_file_metro_shifted, nama_file_start);
        metro_shift(nama_file_metro_shifted); // Shift nama file
        sprintf(path_asli_chiho, "%s/metro/%s", direktori_sumber_chiho, nama_file_metro_shifted);
    } else {
        // Untuk path lain, gunakan path asli
        get_real_path_chiho(path_asli_chiho, path);
    }
    // ...
}

static int maimai_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                           off_t offset, struct fuse_file_info *fi) {
    // ...
    if (strcmp(path_relatif, "/metro") == 0) { // Jika masuk ke direktori metro
        // Iterasi melalui direktori asli
        while ((dp = readdir(dp_dir)) != NULL) {
            char nama_asli_temp[1024];
            strcpy(nama_asli_temp, dp->d_name);
            metro_shift(nama_asli_temp); // Shift kembali nama file yang ditemukan
            // Hanya tampilkan nama file yang "normal" ke pengguna
            if (filler(buf, nama_asli_temp, &st, 0)) {
                closedir(dp_dir);
                return 0;
            }
        }
    } else {
        // Untuk direktori lain, gunakan perilaku default
        // ... (kode yang memanggil readdir di direktori asli)
    }
    // ...
}

static int maimai_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    // ...
    char path_asli_chiho[1024];
    char nama_file_metro_shifted[1024];
    char *nama_file_start = strrchr(path, '/');
    if (nama_file_start) {
        nama_file_start++;
    } else {
        nama_file_start = (char*)path; // Should not happen for file creation inside a dir
    }

    if (strncmp(path + 1, "metro/", 6) == 0) { // Jika file dibuat di dalam direktori metro
        strcpy(nama_file_metro_shifted, nama_file_start);
        metro_shift(nama_file_metro_shifted); // Shift nama file yang akan dibuat
        sprintf(path_asli_chiho, "%s/metro/%s", direktori_sumber_chiho, nama_file_metro_shifted);
    } else {
        get_real_path_chiho(path_asli_chiho, path);
    }
    // ...
}

static int maimai_unlink(const char *path) {
    // ...
    char path_asli_chiho[1024];
    char nama_file_metro_shifted[1024];
    char *nama_file_start = strrchr(path, '/');
    if (nama_file_start) {
        nama_file_start++;
    } else {
        nama_file_start = (char*)path;
    }

    if (strncmp(path + 1, "metro/", 6) == 0) { // Jika file dihapus di direktori metro
        strcpy(nama_file_metro_shifted, nama_file_start);
        metro_shift(nama_file_metro_shifted); // Shift nama file yang akan dihapus
        sprintf(path_asli_chiho, "%s/metro/%s", direktori_sumber_chiho, nama_file_metro_shifted);
    } else {
        get_real_path_chiho(path_asli_chiho, path);
    }
    // ...
}
```

## c. World Tree Area - Dragon Chiho

```
// Fungsi utilitas untuk ROT13
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

// ...

static int maimai_read(const char *path, char *buf, size_t size, off_t offset,
                        struct fuse_file_info *fi) {
    // ...
    if (strncmp(path_relatif + 1, "dragon", 6) == 0) { // Cek jika path dimulai dengan "/dragon"
        char *buffer_baca = malloc(size); // Alokasi buffer untuk membaca data asli
        if (buffer_baca == NULL) return -ENOMEM;

        ret = pread(fd, buffer_baca, size, offset); // Baca data asli
        if (ret > 0) {
            rot13(buffer_baca); // Lakukan ROT13 pada data yang dibaca
            memcpy(buf, buffer_baca, ret); // Salin ke buffer output FUSE
        }
        free(buffer_baca);
    }
    // ...
}

static int maimai_write(const char *path, const char *buf, size_t size,
                         off_t offset, struct fuse_file_info *fi) {
    // ...
    if (strncmp(path_relatif + 1, "dragon", 6) == 0) { // Cek jika path dimulai dengan "/dragon"
        char *buffer_tulis = strdup(buf); // Duplikasi buffer untuk ROT13
        if (buffer_tulis == NULL) return -ENOMEM;

        rot13(buffer_tulis); // Lakukan ROT13 pada data yang akan ditulis
        ret = pwrite(fd, buffer_tulis, size, offset); // Tulis data yang sudah di-ROT13
        free(buffer_tulis);
    }
    // ...
}
```

## d. Black Rose Area - Black Rose Chiho

```
// ... (dalam fungsi maimai_getattr, maimai_readdir, maimai_read, maimai_write, dll.)

static int maimai_read(const char *path, char *buf, size_t size, off_t offset,
                        struct fuse_file_info *fi) {
    // ...
    if (strncmp(path_relatif + 1, "blackrose", 9) == 0) { // Cek jika path dimulai dengan "/blackrose"
        // Tidak ada enkripsi/transformasi. Langsung baca dari file asli.
        ret = pread(fd, buf, size, offset);
    }
    // ...
}

static int maimai_write(const char *path, const char *buf, size_t size,
                         off_t offset, struct fuse_file_info *fi) {
    // ...
    if (strncmp(path_relatif + 1, "blackrose", 9) == 0) { // Cek jika path dimulai dengan "/blackrose"
        // Tidak ada enkripsi/transformasi. Langsung tulis ke file asli.
        ret = pwrite(fd, buf, size, offset);
    }
    // ...
}

// Catatan: Karena tidak ada perlakuan khusus, operasi untuk Black Rose Chiho
// sebagian besar akan masuk ke blok `else` pada pengecekan `strncmp` atau
// tidak memiliki pengecekan khusus sama sekali, yang berarti mereka menggunakan
// perilaku default yaitu langsung mengakses file asli.
```

## e. Tenkai Area - Heaven Chiho

```
// Kunci AES (global)
static const unsigned char kunci_aes[32] = "maimai_heaven_chiho_key_12345678";

// Fungsi enkripsi AES
int aes_encrypt(const unsigned char *plaintext, int plaintext_len,
                const unsigned char *key, const unsigned char *iv,
                unsigned char *ciphertext) {
    EVP_CIPHER_CTX *ctx;
    int len;
    int ciphertext_len;

    ctx = EVP_CIPHER_CTX_new();
    EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv);
    EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len);
    ciphertext_len = len;
    EVP_EncryptFinal_ex(ctx, ciphertext + len, &len);
    ciphertext_len += len;

    EVP_CIPHER_CTX_free(ctx);
    return ciphertext_len;
}

// Fungsi dekripsi AES
int aes_decrypt(const unsigned char *ciphertext, int ciphertext_len,
                const unsigned char *key, const unsigned char *iv,
                unsigned char *plaintext) {
    EVP_CIPHER_CTX *ctx;
    int len;
    int plaintext_len;

    ctx = EVP_CIPHER_CTX_new();
    EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv);
    EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len);
    plaintext_len = len;
    EVP_DecryptFinal_ex(ctx, plaintext + len, &len);
    plaintext_len += len;

    EVP_CIPHER_CTX_free(ctx);
    return plaintext_len;
}

// ...

static int maimai_read(const char *path, char *buf, size_t size, off_t offset,
                        struct fuse_file_info *fi) {
    // ...
    if (strncmp(path_relatif + 1, "heaven", 6) == 0) { // Cek jika path dimulai dengan "/heaven"
        // Baca seluruh isi file yang terenkripsi terlebih dahulu
        long file_size;
        fstat(fd, &st);
        file_size = st.st_size;

        if (file_size < AES_BLOCK_SIZE) { // Jika ukuran file terlalu kecil untuk IV
            // Handle error, mungkin file kosong atau rusak
            return -EIO;
        }

        unsigned char iv_from_file[AES_BLOCK_SIZE];
        pread(fd, iv_from_file, AES_BLOCK_SIZE, 0); // Baca IV dari awal file

        long encrypted_data_size = file_size - AES_BLOCK_SIZE;
        unsigned char *encrypted_data = malloc(encrypted_data_size);
        if (encrypted_data == NULL) return -ENOMEM;

        pread(fd, encrypted_data, encrypted_data_size, AES_BLOCK_SIZE); // Baca data terenkripsi setelah IV

        unsigned char *decrypted_data = malloc(encrypted_data_size + AES_BLOCK_SIZE); // Ukuran lebih besar untuk padding
        if (decrypted_data == NULL) {
            free(encrypted_data);
            return -ENOMEM;
        }

        int decrypted_len = aes_decrypt(encrypted_data, encrypted_data_size,
                                         kunci_aes, iv_from_file, decrypted_data);
        if (decrypted_len < 0) {
            fprintf(stderr, "Gagal mendekripsi file AES: %s\n", path);
            free(encrypted_data);
            free(decrypted_data);
            return -EIO;
        }

        // Salin bagian yang diminta oleh FUSE
        if (offset < decrypted_len) {
            size_t bytes_to_copy = size;
            if (offset + size > decrypted_len) {
                bytes_to_copy = decrypted_len - offset;
            }
            memcpy(buf, decrypted_data + offset, bytes_to_copy);
            ret = bytes_to_copy;
        } else {
            ret = 0; // Offset di luar data terdekripsi
        }

        free(encrypted_data);
        free(decrypted_data);
    }
    // ...
}

static int maimai_write(const char *path, const char *buf, size_t size,
                         off_t offset, struct fuse_file_info *fi) {
    // ...
    if (strncmp(path_relatif + 1, "heaven", 6) == 0) { // Cek jika path dimulai dengan "/heaven"
        // Untuk write ke file terenkripsi, kita perlu:
        // 1. Baca seluruh isi file, dekripsi.
        // 2. Modifikasi bagian yang diminta (buf, size, offset).
        // 3. Enkripsi ulang seluruh konten, tulis kembali.

        // Jika file baru atau offset 0 dan belum ada data, buat IV baru
        unsigned char iv_baru[AES_BLOCK_SIZE];
        if (offset == 0) {
            RAND_bytes(iv_baru, AES_BLOCK_SIZE); // Hasilkan IV acak
        } else {
            // Jika bukan offset 0, baca IV yang sudah ada
            if (pread(fd, iv_baru, AES_BLOCK_SIZE, 0) != AES_BLOCK_SIZE) {
                 fprintf(stderr, "Gagal membaca IV untuk penulisan AES: %s\n", path);
                 return -EIO;
            }
        }

        long file_size;
        fstat(fd, &st);
        file_size = st.st_size;

        unsigned char *full_decrypted_data = NULL;
        long full_decrypted_len = 0;

        if (file_size > AES_BLOCK_SIZE) { // Jika ada data terenkripsi sebelumnya
            unsigned char iv_from_file[AES_BLOCK_SIZE];
            pread(fd, iv_from_file, AES_BLOCK_SIZE, 0); // Baca IV yang ada

            long existing_encrypted_size = file_size - AES_BLOCK_SIZE;
            unsigned char *existing_encrypted_data = malloc(existing_encrypted_size);
            if (existing_encrypted_data == NULL) return -ENOMEM;
            pread(fd, existing_encrypted_data, existing_encrypted_size, AES_BLOCK_SIZE);

            full_decrypted_data = malloc(existing_encrypted_size + AES_BLOCK_SIZE);
            if (full_decrypted_data == NULL) {
                free(existing_encrypted_data);
                return -ENOMEM;
            }
            full_decrypted_len = aes_decrypt(existing_encrypted_data, existing_encrypted_size,
                                              kunci_aes, iv_from_file, full_decrypted_data);
            free(existing_encrypted_data);
        } else {
            // File kosong atau hanya berisi IV, anggap konten awal kosong
            full_decrypted_data = NULL; // Akan dialokasikan ulang nanti jika perlu
            full_decrypted_len = 0;
        }

        // Tentukan ukuran buffer baru setelah modifikasi
        long new_data_len = offset + size;
        if (full_decrypted_len > new_data_len) {
            new_data_len = full_decrypted_len;
        }

        unsigned char *temp_data_buffer = malloc(new_data_len);
        if (temp_data_buffer == NULL) {
            if (full_decrypted_data) free(full_decrypted_data);
            return -ENOMEM;
        }

        // Salin data lama (sebelum offset)
        if (offset > 0 && full_decrypted_data && full_decrypted_len > 0) {
            memcpy(temp_data_buffer, full_decrypted_data, (offset < full_decrypted_len ? offset : full_decrypted_len));
        }

        // Salin data baru
        memcpy(temp_data_buffer + offset, buf, size);

        // Salin data lama (setelah offset + size)
        if (offset + size < full_decrypted_len) {
            memcpy(temp_data_buffer + offset + size, full_decrypted_data + offset + size, full_decrypted_len - (offset + size));
        }

        if (full_decrypted_data) free(full_decrypted_data);

        // Enkripsi ulang seluruh data
        unsigned char *encrypted_output = malloc(new_data_len + AES_BLOCK_SIZE); // Cukup besar untuk padding
        if (encrypted_output == NULL) {
            free(temp_data_buffer);
            return -ENOMEM;
        }
        int encrypted_len = aes_encrypt(temp_data_buffer, new_data_len,
                                        kunci_aes, iv_baru, encrypted_output);
        free(temp_data_buffer);

        if (encrypted_len < 0) {
            fprintf(stderr, "Gagal mengenkripsi file AES: %s\n", path);
            free(encrypted_output);
            return -EIO;
        }

        // Tulis IV baru dan data terenkripsi ke file
        pwrite(fd, iv_baru, AES_BLOCK_SIZE, 0);
        ret = pwrite(fd, encrypted_output, encrypted_len, AES_BLOCK_SIZE); // Mulai setelah IV

        // Potong file jika ukuran baru lebih kecil
        ftruncate(fd, AES_BLOCK_SIZE + encrypted_len);

        free(encrypted_output);
    }
    // ...
}
```

## f. Youth Area - Skystreet Chiho

```
// Fungsi kompresi gzip
int gzip_compress(const char *in_buf, size_t in_len, char **out_buf, size_t *out_len) {
    z_stream strm;
    int ret;
    const int CHUNK = 16384; // Ukuran chunk untuk zlib
    char temp_out[CHUNK];
    int total_out = 0;

    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    ret = deflateInit2(&strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 15 + 16, 8, Z_DEFAULT_STRATEGY); // 15+16 untuk gzip header
    if (ret != Z_OK) return ret;

    strm.avail_in = in_len;
    strm.next_in = (Bytef*)in_buf;

    *out_buf = NULL;
    *out_len = 0;

    do {
        strm.avail_out = CHUNK;
        strm.next_out = (Bytef*)temp_out;
        ret = deflate(&strm, Z_FINISH); // Gunakan Z_FINISH untuk menyelesaikan kompresi
        if (ret == Z_STREAM_ERROR) {
            deflateEnd(&strm);
            return ret;
        }
        int have = CHUNK - strm.avail_out;
        *out_buf = realloc(*out_buf, *out_len + have);
        if (*out_buf == NULL) {
            deflateEnd(&strm);
            return Z_MEM_ERROR;
        }
        memcpy(*out_buf + *out_len, temp_out, have);
        *out_len += have;
        total_out += have;
    } while (strm.avail_out == 0);

    deflateEnd(&strm);
    return Z_OK;
}

// Fungsi dekompresi gzip
int gzip_decompress(const char *in_buf, size_t in_len, char **out_buf, size_t *out_len) {
    z_stream strm;
    int ret;
    const int CHUNK = 16384;
    char temp_out[CHUNK];
    int total_out = 0;

    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    ret = inflateInit2(&strm, 15 + 32); // 15+32 untuk mendeteksi zlib atau gzip header
    if (ret != Z_OK) return ret;

    strm.avail_in = in_len;
    strm.next_in = (Bytef*)in_buf;

    *out_buf = NULL;
    *out_len = 0;

    do {
        strm.avail_out = CHUNK;
        strm.next_out = (Bytef*)temp_out;
        ret = inflate(&strm, Z_NO_FLUSH);
        if (ret == Z_STREAM_ERROR) {
            inflateEnd(&strm);
            return ret;
        }
        int have = CHUNK - strm.avail_out;
        *out_buf = realloc(*out_buf, *out_len + have);
        if (*out_buf == NULL) {
            inflateEnd(&strm);
            return Z_MEM_ERROR;
        }
        memcpy(*out_buf + *out_len, temp_out, have);
        *out_len += have;
        total_out += have;
    } while (strm.avail_out == 0);

    inflateEnd(&strm);
    return Z_OK;
}

// ...

static int maimai_read(const char *path, char *buf, size_t size, off_t offset,
                        struct fuse_file_info *fi) {
    // ...
    if (strncmp(path_relatif + 1, "youth", 5) == 0) { // Cek jika path dimulai dengan "/youth"
        // Baca seluruh isi file terkompresi
        long file_size;
        fstat(fd, &st);
        file_size = st.st_size;

        char *compressed_data = malloc(file_size);
        if (compressed_data == NULL) return -ENOMEM;

        pread(fd, compressed_data, file_size, 0);

        char *decompressed_data = NULL;
        size_t decompressed_len = 0;
        int z_ret = gzip_decompress(compressed_data, file_size, &decompressed_data, &decompressed_len);
        free(compressed_data);

        if (z_ret != Z_OK) {
            fprintf(stderr, "Gagal mendekripsi file GZIP: %s\n", path);
            if (decompressed_data) free(decompressed_data);
            return -EIO;
        }

        // Salin bagian yang diminta oleh FUSE
        if (offset < decompressed_len) {
            size_t bytes_to_copy = size;
            if (offset + size > decompressed_len) {
                bytes_to_copy = decompressed_len - offset;
            }
            memcpy(buf, decompressed_data + offset, bytes_to_copy);
            ret = bytes_to_copy;
        } else {
            ret = 0; // Offset di luar data terdekripsi
        }
        free(decompressed_data);
    }
    // ...
}

static int maimai_write(const char *path, const char *buf, size_t size,
                         off_t offset, struct fuse_file_info *fi) {
    // ...
    if (strncmp(path_relatif + 1, "youth", 5) == 0) { // Cek jika path dimulai dengan "/youth"
        // Untuk write ke file terkompresi, kita perlu:
        // 1. Baca seluruh isi file, dekompresi.
        // 2. Modifikasi bagian yang diminta (buf, size, offset).
        // 3. Kompresi ulang seluruh konten, tulis kembali.

        long file_size;
        fstat(fd, &st);
        file_size = st.st_size;

        char *full_decompressed_data = NULL;
        size_t full_decompressed_len = 0;

        if (file_size > 0) {
            char *existing_compressed_data = malloc(file_size);
            if (existing_compressed_data == NULL) return -ENOMEM;
            pread(fd, existing_compressed_data, file_size, 0);

            int z_ret = gzip_decompress(existing_compressed_data, file_size, &full_decompressed_data, &full_decompressed_len);
            free(existing_compressed_data);
            if (z_ret != Z_OK) {
                fprintf(stderr, "Gagal dekompresi saat write ke GZIP: %s\n", path);
                return -EIO;
            }
        }

        // Tentukan ukuran buffer baru setelah modifikasi
        size_t new_data_len = offset + size;
        if (full_decompressed_len > new_data_len) {
            new_data_len = full_decompressed_len;
        }

        char *temp_data_buffer = malloc(new_data_len);
        if (temp_data_buffer == NULL) {
            if (full_decompressed_data) free(full_decompressed_data);
            return -ENOMEM;
        }

        // Salin data lama (sebelum offset)
        if (offset > 0 && full_decompressed_data && full_decompressed_len > 0) {
            memcpy(temp_data_buffer, full_decompressed_data, (offset < full_decompressed_len ? offset : full_decompressed_len));
        }

        // Salin data baru
        memcpy(temp_data_buffer + offset, buf, size);

        // Salin data lama (setelah offset + size)
        if (offset + size < full_decompressed_len) {
            memcpy(temp_data_buffer + offset + size, full_decompressed_data + offset + size, full_decompressed_len - (offset + size));
        }

        if (full_decompressed_data) free(full_decompressed_data);

        // Kompresi ulang seluruh data
        char *compressed_output = NULL;
        size_t compressed_len = 0;
        int z_ret = gzip_compress(temp_data_buffer, new_data_len, &compressed_output, &compressed_len);
        free(temp_data_buffer);

        if (z_ret != Z_OK) {
            fprintf(stderr, "Gagal kompresi saat write ke GZIP: %s\n", path);
            if (compressed_output) free(compressed_output);
            return -EIO;
        }

        // Tulis data terkompresi ke file
        ret = pwrite(fd, compressed_output, compressed_len, 0);
        ftruncate(fd, compressed_len); // Potong file ke ukuran baru

        free(compressed_output);
    }
    // ...
}
```

## g. Prism Area - 7sRef Chiho

```
static int get_real_path_chiho(char *full_path, const char *path) {
    // Fungsi utilitas untuk mendapatkan path asli di direktori sumber
    // Fungsi ini dimodifikasi untuk menangani 7sref.
    // Jika path adalah /7sref/[area]_[nama_file], ubah menjadi /[area]/[nama_file]
    // sebelum mem-prefix dengan direktori_sumber_chiho.

    // Cek jika path dimulai dengan "/7sref/"
    if (strncmp(path, "/7sref/", 7) == 0) {
        const char *filename_in_7sref = path + 7; // Ambil bagian setelah "/7sref/"
        char *underscore_pos = strchr(filename_in_7sref, '_');

        if (underscore_pos) {
            int area_name_len = underscore_pos - filename_in_7sref;
            char area_name[256];
            strncpy(area_name, filename_in_7sref, area_name_len);
            area_name[area_name_len] = '\0';

            const char *original_filename = underscore_pos + 1;

            // Bangun path virtual yang sesuai dengan area lain
            // Misalnya, "/starter/guide.txt"
            char virtual_path_from_7sref[1024];
            sprintf(virtual_path_from_7sref, "/%s/%s", area_name, original_filename);

            // Kemudian panggil get_real_path_chiho lagi dengan virtual_path_from_7sref
            // Ini akan memanggil dirinya sendiri secara rekursif tapi untuk path yang sudah "diterjemahkan"
            // (kecuali jika ada penanganan khusus lebih lanjut di area lain, yang sudah diimplementasikan)
            sprintf(full_path, "%s%s", direktori_sumber_chiho, virtual_path_from_7sref);
            return 0;
        }
    }
    // Untuk path selain 7sref atau yang tidak sesuai format 7sref, gunakan perilaku default
    sprintf(full_path, "%s%s", direktori_sumber_chiho, path);
    return 0;
}

static int maimai_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                           off_t offset, struct fuse_file_info *fi) {
    // ...
    if (strcmp(path_relatif, "/7sref") == 0) {
        const char *areas[] = {"starter", "metro", "dragon", "blackrose", "heaven", "youth"};
        for (size_t i = 0; i < sizeof(areas) / sizeof(areas[0]); ++i) {
            char area_dir_path[1024];
            sprintf(area_dir_path, "%s/%s", direktori_sumber_chiho, areas[i]);

            DIR *dp_area = opendir(area_dir_path);
            if (dp_area == NULL) continue;

            struct dirent *dp_file;
            while ((dp_file = readdir(dp_area)) != NULL) {
                if (strcmp(dp_file->d_name, ".") == 0 || strcmp(dp_file->d_name, "..") == 0) continue;

                char filename_in_7sref[1024];
                // Bentuk nama file di 7sref: [area]_[nama_file_asli]
                sprintf(filename_in_7sref, "%s_%s", areas[i], dp_file->d_name);

                struct stat st;
                memset(&st, 0, sizeof(st));
                st.st_ino = dp_file->d_ino;
                st.st_mode = dp_file->d_type << 12; // Dapatkan tipe file

                if (filler(buf, filename_in_7sref, &st, 0)) {
                    closedir(dp_area);
                    return 0;
                }
            }
            closedir(dp_area);
        }
    } else {
        // ... (kode readdir untuk direktori lain)
    }
    // ...
}

// Catatan: Fungsi open, read, write, create, unlink, mkdir, dll.
// secara otomatis akan mendapatkan path asli yang benar melalui modifikasi
// `get_real_path_chiho` ketika mereka dipanggil dengan path yang berasal dari `/7sref/`.
// Jadi, tidak perlu ada blok `if (strncmp(path_relatif + 1, "7sref", ...)` tambahan di fungsi-fungsi tersebut
// kecuali untuk `readdir` yang perlu menampilkan daftar file yang "diterjemahkan".
```



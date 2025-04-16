# Table of Contents
- Soal 1
	- a. Downloading the Clues
	- b. Filtering the Files
	- c. Combine the File Content
 	- d. Decode the file
- Soal 2
	- a. Download dan Unzip 
 	- b. x
  	- c. x
  	- d. x
  	- e. x
  	- f. x
- Soal 3
	- a. x
	- b. x
	- c. x
	- d. x
 	- e. x
  - g. x
  - h. x 
- Soal 4
	- a. x
 	- b. x
  	- c. x
  	- d. x
  	- e. x
  	- f. x

# Soal 1
## a. Downloading the Clues
Pada soal ini diperintahkan untuk mendownload, unzip, dan kemudian menghapus Clues.zip

### Download
Untuk mendownload soal, saya menggunakan curl 
```
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
```
Disini `-L` berarti mengikuti link yang terdapat dibawahnya, kemudian `-o Clues.zip` berfungsi untuk menyimpan file yang di download bernama Clues.zip

Kemudian ```execve("/usr/bin/curl", curl_argv, NULL);``` digunakan untuk menjalankan program curl. ```perror("Gagal menjalankan curl");``` tidak akan jalan apabila code berhasil mendownload file dari link drive. 
`waitpid(pid1, &status, 0);` berfungsi untuk menunggu code tersebut selesai tereksekusi, setelah itu dilanjut pada code berikutnya.

### Unzip
```
    char *unzip_argv[] = {
            "/usr/bin/unzip",
            "-o",
            "Clues.zip",
            NULL
        };
        execve("/usr/bin/unzip", unzip_argv, NULL);
        perror("Gagal menjalankan unzip");
        exit(1);
```
Untuk soal ini saya menggunakan code `unzip` yang terletak di `usr/bin/unzip` pada linux. Hasil extrak akan disimpan pada folder dengan file Clues.zip. `execve("/usr/bin/unzip", unzip_argv, NULL);` execve ini digunakan untuk menjalankan code tersebut. `perror("Gagal menjalankan unzip");` akan berjalan ketika file gagal di ekstrak.

### Remove Clues.zip
```
    char *rm_argv[] = {
            "/usr/bin/rm",
            "-f",
            "Clues.zip",
            NULL
        };
        execve("/usr/bin/rm", rm_argv, NULL);
        perror("Gagal remove");
        exit(1);
```
Saya menggunakan perintah `rm` yang terletak di `/usr/bin/rm` untuk menghapus file Clues.zip setelah file tersebut berhasil diekstrak.

Argumen yang saya gunakan adalah `-f` (force) yang berfungsi untuk menghapus file secara paksa, kemudian `"Clues.zip"` adalah nama file yang ingin dihapus. `execve("/usr/bin/rm", rm_argv, NULL);` digunakan untuk menjalankan perintah rm.

## b. Filtering the Files
Pada soal ini diperintahkan untuk memindahkan file yang memiliki nama 1 huruf dan 1 angka kedalam folder Filtered.

```
int syarat(char *name) {
    int len = strlen(name);
    if (len != 5) return 0;
    if (strcmp(&name[1], ".txt") != 0) return 0;
    char c = name[0];
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) return 1;
    return 0;
}
```
Pada function ini saya buat untuk memeriksa nama file apakah memenuhi syarat yaitu 1 huruf atau 1 angka kemudian setelahnya format file .txt.

Selanjutnya pada function filter ```void filter () {```

Code `mkdir("Filtered",0777);` berfungsi untuk membuat folder baru bernama "Filtered" dan `0777` berarti folder tersebut dapar di read, write, dan execute. Code `char *folders[] = {"Clues/ClueA", "Clues/ClueB", "Clues/ClueC", "Clues/ClueD"};` berfungsi untuk mengatur folder mana yang ingin diperiksa. Code `for (int i = 0; i < 4; i++) { DIR *dir = opendir(folders[i]); if (dir == NULL) continue;` berfungsi untuk melakukan looping ke setiap folder yang akan diperiksa, jika tidak bisa dibuka atau `NULL` maka akan lanjut ke folder berikutnya. 

```
struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_name[0] != '.')
```
Code ini berfungsi untuk membaca semua isi pada folder, kemudian diperiksa `d_name[0] != '.'` untuk memeriksa juga file tersembunyi.

```
if (syarat(entry->d_name)) {
	char from[100], to[100];
        sprintf(from, "%s/%s", folders[i], entry->d_name);
        sprintf(to, "Filtered/%s", entry->d_name);
        rename(from, to);
```
Code ini membuat file yang memennuhi syarat fipindah kedalam folder "Filtered" dengan menggunakan argumen `rename()`

```
else {
	char path[100];
        sprintf(path, "%s/%s", folders[i], entry->d_name);
        remove(path);
} closedir(dir);
```
Code ini membuat file yang tidak memenuhi syarat akan dihapus menggunakan `remove(path)`. Selanjutnya `closedir(dir)` berfungsi untuk menutup folder setelah semua berhasil dijalankan.

```
if (argc == 3 && strcmp(argv[1], "-m") == 0) {
        if (strcmp(argv[2], "Filter") == 0) {
            filter();
```
Code berikut digunakan untuk menjalankan program filter dengan `-m Filter`.

## c. Combine the File Content
Pada soal ini diperintahkan untuk menggabungkan isi file dengan urutan huruf lalu angka kemudian huruf dan seterusnya. Setelah itu file pada folder sebelumnya dihapus.

```
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
```
Code berikut berfungsi untuk membuka folder bernama "Filtered" kemudian menyimpan nama file dalam folder tersebut menggunakan `angka[]` dan `huruf_arr[]`.  Code `nomer` dan `huruf` berfungsi untuk memeriksa apakah file tersebut angka atau huruf.

```
int txt(char *name) {
    return strlen(name) == 5 && strcmp(&name[1], ".txt") == 0;
}
int nomer(char c) {
    return c >= '0' && c <= '9';
}
int huruf(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}
```
Code tersebut adalah code lengkap untuk memeriksa apakah file tersebut angka atau huruf dan apakah format file .txt.

```
    sort(angka, a);
    sort(huruf_arr, h);
```
Code teresbut berfungsi untuk menenentukan untuk sort angka dilambangkan dengan a, dan huruf_arr dengan h.

```
    FILE *gabung = fopen("Combined.txt", "w");
    int i = 0, j = 0;
    while (i < a || j < h) {
```
Membuka file Combined.txt untuk ditulis. Loop dilakukan sampai semua file angka (a) dan huruf (h) selesai diproses.

```
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
```
Membuka file angka dan huruf kemudian isi dari file tersebut di masukkan kedalam file Combined.txt setelah itu hapus file yang telah dipindahkan isinya dan dilanjutkan ke file berikutnya. 

```
if (argc == 3 && strcmp(argv[1], "-m") == 0) {
    ...
    else if (strcmp(argv[2], "Combine") == 0) {
        combine();
    }
}

```
Code berikut digunakan untuk menjalankan program filter dengan `-m Combine`.

## d. Decode the file
```
char rot13(char c) {
    if (c >= 'a' && c <= 'z') return ((c - 'a' + 13) % 26) + 'a';
    if (c >= 'A' && c <= 'Z') return ((c - 'A' + 13) % 26) + 'A';
    return c;
}
```
Code berikut berfungsi untuk mengubah isi dari Combined.txt dengan rot13 menjadi Password untuk masuk kedalam Lokasi dengan cara mengubah isi dari Combined.txt untuk setiap karakter ditambah 13 kemudian modulus 26 kemudian ditambah huruf awalnya.

```
void decode() {
    FILE *input = fopen("Combined.txt", "r");
    FILE *output = fopen("Decoded.txt", "w");
```
Code ini berfungsi untuk membuka file Combined.txt sebagai input, kemudian sebagai output dimasukkan dalam file Decoded.txt

```
if (input && output) {
        char ch;
        while ((ch = fgetc(input)) != EOF) {
            fputc(rot13(ch), output);
```
Code ini akan dijalankan setelah input dan output dibuka. Code akan membaca tiap karakter dari isi file Combined.txt kemudian mengubahnya dengan function rot13 dan dituliskan hasilnya kedalam Decoded.txt.

```
fclose(input);
fclose(output);
```
Menutup file input dan output.

```
if (argc == 3 && strcmp(argv[1], "-m") == 0) {
    ...
    else if (strcmp(argv[2], "Decode") == 0) {
        combine();
    }
}
```
Code berikut digunakan untuk menjalankan program filter dengan `-m Decode`.

## e. Password Check
Setelah mendapatkan hasil dari Decoded.txt, selanjutnya output dimasukkan kedalam Lokasi Password Check dan hasilnya sebagai berikut.
![image](https://github.com/user-attachments/assets/64b30716-0f6d-41e1-9595-bfb2d40b7522)


## Error Handling

## Revisi
### Download
Dengan menggunakan code if-else untuk memeriksa directory, file tidak akan terdownload kembali setiap dijalankan
```
DIR *cek = opendir("Clues");
    if (cek != NULL) {
        closedir(cek);
    } else {
```

### Unavailable Command
Dengan menggunakan if-else, ketika dijalankan command selain Filter, Combine, dan Decode maka akan muncul message "Command tidak tersedia"
```
else {
            printf("Command tidak tersedia\n");
        }
    } else if (argc > 1) {
        printf("Command tidak tersedia\n");
    }
```

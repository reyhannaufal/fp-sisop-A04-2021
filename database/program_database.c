#define _GNU_SOURCE
#include <stdio.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <signal.h>
#include <stdlib.h>
#include <dirent.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <pthread.h>
#include <ctype.h>
#include <stdbool.h>
#include <regex.h>
#include <time.h>
#include <wait.h>
#include <fcntl.h>
#include <limits.h>
#include <syslog.h>
#include <errno.h>
#include <assert.h>

#define PORT 8080
#define buffSize 256

//DML
char *PATH;
char *namaDB = "db_percobaan";
char TBLARR[100][100];
char OUTCLIENT[100][100];
int OCLENGTH = 0;

typedef struct user
{
	char auth[buffSize], input[buffSize];
	int sock, isRoot, isLogged;
} user;
user *client[10];

//DML Fungsi
char *fpath_tbl(char *nama_tbl);
int ftxt_toArr(char *path);
char *str_withoutq(char *inpt);
char *tokens_toStr(char **arg, int src, int dest, char *delim);
char **str_split(char *a_str, const char a_delim);
char *substr(const char *src, int m, int n);
int is_schar(char x, char *schar);
char *remove_schar(char *str, char *schar);
int find_position(int argc, char **argv, char *str);
int find_index(char *arg, char ichar);
int fline_where(int argc, char **argv, int whereC, int *kolom);
void addStringtoTxt(char *str, char *path, char *type, int tipe_call);
int cek_tipe(char *inpt);
void delete_tbl(int argc, char **argv, int line);
void insert_tbl(int argc, char **argv, char *parameter);
void select_tbl(int argc, char **argv);
void run_command(char *comm, int indexClient);

void *inRoutine(void *arg)
{
	int i = *(int *)arg - 1;
	char buffer[buffSize];
	while (1)
	{
		recv(client[i]->sock, buffer, buffSize, 0);
		strcpy(client[i]->input, buffer);
	}
}

void createuser(int indexClient, int argc, char **argv)
{
	if (client[indexClient]->isLogged != 1)
	{
		send(client[indexClient]->sock, "Not Root User\n", 256, 0);
		return;
	}
	FILE *fp = fopen("akun.txt", "a+");
	//send(client[indexClient]->sock,argv[1],256,0);
	//send(client[indexClient]->sock,"Not Root User\n",256,0);
	if ((strcmp(argv[1], "USER") == 0) && (strcmp(argv[3], "IDENTIFIED") == 0))
	{
		fprintf(fp, "%s.%s\n", argv[2], argv[5]);
	}
	fclose(fp);
}

void login(char auth[], int clientIndex)
{

	if ((strcmp(auth, "ISROOTCLIENT") == 0) && (client[clientIndex]->isRoot != -1))
	{
		//send(client[clientIndex]->sock,"ping4\n",256,0);
		client[clientIndex]->isLogged = 1;
		client[clientIndex]->isRoot = 1;
		return;
	}
	else
	{
		client[clientIndex]->isRoot = -1;
	}
	ssize_t read;
	FILE *fp;
	fp = fopen("akun.txt", "r");
	size_t len;
	char *string;
	sprintf(auth, "%s\n", auth);

	while ((read = getline(&string, &len, fp)) != -1)
	{
		//send(client[clientIndex]->sock,auth,256,0);
		if (strcmp(auth, string) == 0)
		{
			client[clientIndex]->isLogged = 1;
			strcpy(client[clientIndex]->auth, auth);
			break;
		}
	}
	fclose(fp);
}

void *outRoutine(void *arg)
{
	int i = *(int *)arg - 1;
	char buffer[buffSize];
	char buffer_name[buffSize];
	char path[buffSize];
	while (1)
	{
		//DDL DML below
		if (strlen(client[i]->input) != 0)
		{

			if (client[i]->isLogged == 1)
			{

				run_command(client[i]->input, i);

				if (OCLENGTH != 0)
				{
					for (int j = 0; j < OCLENGTH; j++)
					{
						send(client[i]->sock, OUTCLIENT[j], buffSize, 0);
						strcpy(OUTCLIENT[j], "\0");
					}
					send(client[i]->sock, "\n", buffSize, 0);
				}
				OCLENGTH = 0;
			}
			else
			{
				//send(client[i]->sock,client[i]->input,256,0);
				login(client[i]->input, i);
				if (client[i]->isLogged == 1)
				{
					send(client[i]->sock, "Logged in\n", 256, 0);
				}
				else
				{
					send(client[i]->sock, "wrong username or password\n", 256, 0);
				}
			}
			memset(client[i]->input, 0, buffSize);
		}
	}
}

// void killer(int pid);

int main(int argc, char *argv[])
{

	char s[PATH_MAX];
	getcwd(s, sizeof(s));
	PATH = (char *)malloc(PATH_MAX);

	FILE *fp = fopen("akun.txt", "a");
	fclose(fp);
	PATH = strdup(s);
	char *tok = strrchr(PATH, '/');
	*tok = 0;
	// printf("PAth: %s\n", PATH);

	int indexClient = 0;

	pthread_t clientThread[10][2];

	pid_t pid, sid;
	pid = fork();

	if (pid < 0)
	{
		exit(EXIT_FAILURE);
	}
	if (pid > 0)
	{
		exit(EXIT_SUCCESS);
	}
	umask(0);
	sid = setsid();
	if (sid < 0)
	{
		exit(EXIT_FAILURE);
	}
	if ((chdir(s)) < 0)
	{
		exit(EXIT_FAILURE);
	}

	//	close(STDIN_FILENO);
	//	close(STDOUT_FILENO);
	//	close(STDERR_FILENO);

	// killer((int)getpid());
	while (1)
	{
		int opt = 1;
		int server_fd, valread;
		struct sockaddr_in address;
		int addrlen = sizeof(address);

		if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
		{
			perror("socket failed");
			exit(EXIT_FAILURE);
		}
		if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
		{
			perror("setsockopt");
			exit(EXIT_FAILURE);
		}
		address.sin_family = AF_INET;
		address.sin_addr.s_addr = INADDR_ANY;
		address.sin_port = htons(PORT);

		if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
		{
			perror("bind failed");
			exit(EXIT_FAILURE);
		}

		if (listen(server_fd, 10) < 0)
		{
			perror("listen");
			exit(EXIT_FAILURE);
		}
		for (indexClient = 0; indexClient < 10; indexClient++)
		{
			client[indexClient] = (user *)malloc(sizeof(user));
			client[indexClient]->isLogged = 0;
			client[indexClient]->isRoot = 0;
			if ((client[indexClient]->sock = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
			{
				perror("accept");
				exit(EXIT_FAILURE);
			}
			pthread_create(&clientThread[indexClient][0], NULL, inRoutine, (void *)&indexClient);
			pthread_create(&clientThread[indexClient][1], NULL, outRoutine, (void *)&indexClient);
		}
		for (indexClient = 0; indexClient < 10; indexClient++)
		{

			pthread_join(clientThread[indexClient][0], NULL);
			pthread_join(clientThread[indexClient][1], NULL);
		}
	}
	//return 0;
}

// mkae killer for easy termination
// void killer(int pid){
// 	FILE *tex;
// 	char* script;
// 	tex = fopen("Killer.sh", "w");
// 	asprintf(&script,"\#!/bin/bash\nkill -9 %d\nrm -- \"$0\"",pid);
// 	fputs(script,tex);
// 	fclose(tex);
// }

int log_db(char *logging)
{
	/* get time */
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
	char output[1000];
	/* get system unsername */
	char username[32];
	cuserid(username);

	sprintf(output, "%d-%02d-%02d %02d:%02d:%02d:%s:%s\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, username, logging);
	// printf("%s", output);
	FILE *fp;
	fp = fopen("sql.log", "a");
	fputs(output, fp);
	fclose(fp);
	return 1;
}

char *fpath_tbl(char *nama_tbl)
{
	char *tbl_str = malloc(buffSize);
	strcpy(tbl_str, PATH);
	strcat(tbl_str, "/");
	strcat(tbl_str, namaDB);
	strcat(tbl_str, "/");
	strcat(tbl_str, nama_tbl);
	strcat(tbl_str, ".txt");

	return tbl_str;
}

int ftxt_toArr(char *path)
{
	char fname[20];
	FILE *fptr = NULL;
	int i = 0;
	int tot = 0;

	// for (int i=0; i<100; i++){
	// 	strcpy(TBLARR[i], "\0");
	// }

	fptr = fopen(path, "r");
	if (fptr == NULL)
	{
		return -1;
	}

	while (fgets(TBLARR[i], 100, fptr))
	{
		TBLARR[i][strlen(TBLARR[i]) - 1] = '\0';
		i++;
	}

	fclose(fptr);

	return i;
}

char *str_withoutq(char *inpt)
{
	char *out = malloc(buffSize);
	int c = 0;
	for (int i = 0; i < strlen(inpt); i++)
	{
		if ((inpt[i] >= 'a' && inpt[i] <= 'z') || (inpt[i] >= 'A' && inpt[i] <= 'Z') ||
			(inpt[i] >= '0' && inpt[i] <= '9') || (inpt[i] == ' ' && i != 0))
		{
			out[c] = inpt[i];
			c++;
		}
	}
	out[c] = '\0';
	return out;
}

char *tokens_toStr(char **arg, int src, int dest, char *delim)
{
	char *output = malloc(buffSize);
	for (int i = src; i < dest; i++)
	{
		if (i == src)
		{
			strcpy(output, arg[i]);
		}
		else
		{
			if (i != dest)
			{
				strcat(output, delim);
			}
			strcat(output, arg[i]);
		}
	}
	return output;
}

char **str_split(char *a_str, const char a_delim)
{
	char **result = 0;
	size_t count = 0;
	char *tmp = a_str;
	char *last_comma = 0;
	char delim[2];
	delim[0] = a_delim;
	delim[1] = 0;

	while (*tmp)
	{
		if (a_delim == *tmp)
		{
			count++;
			last_comma = tmp;
		}
		tmp++;
	}

	count += last_comma < (a_str + strlen(a_str) - 1);
	count++;

	result = malloc(sizeof(char *) * count);

	if (result)
	{
		size_t idx = 0;
		char *token = strtok(a_str, delim);

		while (token)
		{
			assert(idx < count);
			*(result + idx++) = strdup(token);
			token = strtok(0, delim);
		}
		assert(idx == count - 1);
		*(result + idx) = 0;
	}

	return result;
}

char *substr(const char *src, int m, int n)
{
	int len = n - m;
	char *dest = (char *)malloc(sizeof(char) * (len + 1));
	strncpy(dest, (src + m), len);
	return dest;
}

int is_schar(char x, char *schar)
{
	for (int i = 0; i < strlen(schar); i++)
	{
		if (x == schar[i])
		{
			return 1;
		}
	}
	return 0;
}

char *remove_schar(char *str, char *schar)
{

	char *strStripped = malloc(buffSize);
	int c = 0;
	for (int i = 0; i < strlen(str); i++)
	{
		if (is_schar(str[i], schar) == 0)
		{
			strStripped[c] = str[i];
			c++;
		}
	}
	strStripped[c] = '\0';

	return strStripped;
}

int find_position(int argc, char **argv, char *str)
{
	for (int i = 0; i < argc; i++)
	{
		if (strcmp(argv[i], str) == 0)
		{
			return i;
			break;
		}
	}
	return -1;
}

int find_index(char *arg, char ichar)
{
	for (int i = 0; i < strlen(arg); i++)
	{
		if (arg[i] == ichar)
		{
			return i;
		}
	}
	return -1;
}

int fline_where(int argc, char **argv, int whereC, int *kolom)
{
	if (whereC == -1)
	{
		return -1;
	}
	else
	{
		int tbl_int = whereC - 1;
		int cek_b = ftxt_toArr(fpath_tbl(argv[tbl_int]));
		if (cek_b == -1)
		{
			return -2; //table tidak ditemukan
		}
		char param_where[100]; //menyatukan tokens jadi string lagi dgn delim
		strcpy(param_where, tokens_toStr(argv, whereC + 1, argc, " "));

		char **token_where;
		token_where = str_split(param_where, '=');

		int kol_n = -3; //mencari kolom pada
		for (int i = 0; i < cek_b; i++)
		{
			char **tblline;
			tblline = str_split(TBLARR[i], ',');
			if (i == 0)
			{ //nyari kolom pada urutan ke berapa
				for (int j = 0; *(tblline + j); j++)
				{
					if (strcmp(*(tblline + j), *(token_where + 0)) == 0)
					{
						kol_n = j;
						*kolom = j;
						printf("kolom asli: %d\n", kol_n);
						break;
					}
				}
			}
			else if (kol_n == -3)
			{
				return -3; //tidak ditemukan nama kolom
			}
			else
			{ // nyari line
				if (strcmp(str_withoutq(*(token_where + 1)), *(tblline + kol_n)) == 0)
				{
					// printf("namap: %s, namadb: %s\n", str_withoutq(*(token_where + 1)), *(tblline + kol_n));
					return i;
					break;
				}
			}
		}
		return -4; //tidak ditemukan data
	}
	return -1;
}

void addStringtoTxt(char *str, char *path, char *type, int tipe_call)
{
	FILE *fptr = NULL;

	fptr = fopen(path, type);
	if (fptr == NULL)
	{
		// printf("Data Gagal Dimasukan!\n");
		strcpy(OUTCLIENT[0], "Data Gagal Dimasukan!\n");
		OCLENGTH = 1;
		return;
	}

	fputs(str, fptr);
	fclose(fptr);
	if (tipe_call == 1)
	{ //insert
		// printf("Data Berhasil Dimasukan!\n");
		strcpy(OUTCLIENT[0], "Data Berhasil Dimasukan!\n");
		OCLENGTH = 1;
	}
}

int cek_tipe(char *inpt)
{
	for (int i = 0; i < strlen(inpt); i++)
	{
		if (inpt[i] < '0' || inpt[i] > '9')
		{
			return 2;
		}
	}
	return 1;
}

void delete_tbl(int argc, char **argv, int line)
{
	if (line == -2)
	{
		// printf("Tabel Tidak Ditemukan!\n");
		strcpy(OUTCLIENT[0], "Tabel Tidak Ditemukan!\n");
		OCLENGTH = 1;
		return;
	}
	else if (line == -3)
	{
		// printf("Tidak Ditemukan Nama Kolom!\n");
		strcpy(OUTCLIENT[0], "Tidak Ditemukan Nama Kolom!\n");
		OCLENGTH = 1;
		return;
	}
	else if (line == -4)
	{
		// printf("Tidak Ditemukan Data!\n");
		strcpy(OUTCLIENT[0], "Tidak Ditemukan Data!\n");
		OCLENGTH = 1;
		return;
	}

	int from_int = find_position(argc, argv, "FROM");
	int tbl_int = from_int + 1;
	int cek_b = ftxt_toArr(fpath_tbl(argv[tbl_int]));

	if (line == -1)
	{ //2 karena delete
		addStringtoTxt(strcat(TBLARR[0], "\n"), fpath_tbl(argv[tbl_int]), "w", 2);
		addStringtoTxt(strcat(TBLARR[1], "\n"), fpath_tbl(argv[tbl_int]), "a+", 2);
		// printf("Data Berhasil Dihapus!\n");
		strcpy(OUTCLIENT[0], "Data Berhasil Dihapus!\n");
		OCLENGTH = 1;
	}
	else
	{
		for (int i = 0; i < cek_b; i++)
		{
			if (i != line)
			{ //hapus line
				if (i == 0)
				{
					addStringtoTxt(strcat(TBLARR[i], "\n"), fpath_tbl(argv[tbl_int]), "w", 2);
				}
				else
				{
					addStringtoTxt(strcat(TBLARR[i], "\n"), fpath_tbl(argv[tbl_int]), "a+", 2);
				}
			}
		}
		// printf("Data Berhasil Dihapus!\n");
		strcpy(OUTCLIENT[0], "Data Berhasil Dihapus!\n");
		OCLENGTH = 1;
	}
}

void insert_tbl(int argc, char **argv, char *parameter)
{
	int into_int = find_position(argc, argv, "INTO");
	int tbl_int = into_int + 1;
	int data_int = tbl_int + 1;

	// printf("%s\n", parameter);

	int cek_b = ftxt_toArr(fpath_tbl(argv[tbl_int]));
	int cek_k = 0, param_c = 0; //panjang kolom dari database (cek_k),
								//panjang kolom parameter command(param_c)

	if (cek_b == -1)
	{
		// printf("Tabel Tidak Ditemukan!\n");
		strcpy(OUTCLIENT[0], "Tabel Tidak Ditemukan!\n");
		OCLENGTH = 1;
		return;
	}
	else
	{
		// isi di dalam database tipenya
		char **tokens;
		tokens = str_split(TBLARR[1], ',');
		for (int i = 0; *(tokens + i); i++)
		{
			cek_k++;
		}
		//parameter dari command inseret
		char **params;
		params = str_split(parameter, ',');
		for (int i = 0; *(params + i); i++)
		{
			param_c++;
		}

		if (param_c != cek_k)
		{
			// printf("Panjang Argumen dengan Kolom Tabel Tidak Sama!\n");
			strcpy(OUTCLIENT[0], "Panjang Argumen dengan Kolom Tabel Tidak Sama!\n");
			OCLENGTH = 1;
			return;
		}
		char schar[4] = {'(', ')', ' '};
		char saveStr[100];
		for (int i = 0; i < param_c; i++)
		{
			// printf("string: %s tipe: %d\n", remove_schar(argv[i], schar),
			//		cek_tipe(remove_schar(argv[i], schar)));

			if ((strcmp(*(tokens + i), "int") == 0) &&
				(cek_tipe(remove_schar(*(params + i), schar)) == 1))
			{
				if (i == 0)
				{
					strcpy(saveStr, remove_schar(*(params + i), schar));
				}
				else
				{
					strcat(saveStr, remove_schar(*(params + i), schar));
				}
				if (i < param_c - 1)
					strcat(saveStr, ",");
			}
			else if ((strcmp(*(tokens + i), "string") == 0) &&
					 (cek_tipe(remove_schar(*(params + i), schar)) == 2))
			{
				if (i == 0)
				{
					strcpy(saveStr, str_withoutq(*(params + i)));
				}
				else
				{
					strcat(saveStr, str_withoutq(*(params + i)));
				}
				if (i < param_c - 1)
					strcat(saveStr, ",");
			}

			else
			{
				// printf("Gagal! Tipe data ke-%d: %s!\n", i+1, *(tokens + i));
				char outc[100];
				sprintf(outc, "Gagal! Tipe data ke-%d: %s!\n", i + 1, *(tokens + i));

				strcpy(OUTCLIENT[0], outc);
				OCLENGTH = 1;
				return;
			}
		}
		strcat(saveStr, "\n");
		// printf("save: %s\n", saveStr);
		addStringtoTxt(saveStr, fpath_tbl(argv[tbl_int]), "a+", 1);
	}
}

int findMax_strTbl(char *namaTbl)
{
	int out = 0;
	int cek = ftxt_toArr(fpath_tbl(namaTbl));
	if (cek == -1)
	{
		return -1;
	}
	for (int i = 0; i < cek; i++)
	{
		char **tokens;
		tokens = str_split(TBLARR[i], ',');
		if (tokens)
		{
			for (int j = 0; *(tokens + j); j++)
			{
				if (strlen(*(tokens + j)) > out)
				{
					out = strlen(*(tokens + j));
				}
			}
		}
	}
	return out;
}

void select_tbl(int argc, char **argv)
{
	int from_int = find_position(argc, argv, "FROM");
	int tbl_int = from_int + 1;
	int *kol_int = malloc(buffSize);

	if (from_int == 1)
	{
		// printf("Format Perintah Salah!\n");
		// printf("SELECT [namakolom1, namakolom2...] FROM [namatabel]\n");
		strcpy(OUTCLIENT[0], "Format Perintah Salah!\n");
		strcpy(OUTCLIENT[1], "SELECT [namakolom1, namakolom2...] FROM [namatabel]\n");
		OCLENGTH = 2;
		return;
	}

	int lenTab = (findMax_strTbl(argv[tbl_int]) + 1);
	// printf("lentab: %d\n", lenTab);
	// int lenTab = 5;

	int cek = ftxt_toArr(fpath_tbl(argv[tbl_int]));
	if (cek == -1)
	{
		// printf("Tabel Tidak Ditemukan!\n");
		strcpy(OUTCLIENT[0], "Tabel Tidak Ditemukan!\n");
		OCLENGTH = 1;
		return;
	}
	else
	{

		char **tokens;
		tokens = str_split(TBLARR[0], ',');

		int kol_c = 0;
		if (tokens)
		{
			// nama kolom (sebelum from (fromint - 1)) dicek apakah bintang?
			if (strcmp(argv[from_int - 1], "*") == 0)
			{
				for (int i = 0; *(tokens + i); ++i)
				{
					kol_int[kol_c] = i;
					kol_c++;
				}
			}
			else
			{ //mulai dari command(argv[j]) nested loop database(tokens + i)
				for (int j = 1; j < from_int; j++)
				{ //kol_c nya ngikut j urutannya,
					for (int i = 0; *(tokens + i); ++i)
					{ // dan isi dari tokens + i
						if (strcmp(*(tokens + i), argv[j]) == 0)
						{
							kol_int[kol_c] = i;
							kol_c++;
						}
						// printf("argv: %s\n", argv[j]);
						// printf("db: %s\n", *(tokens + i));
					}
				}
			}
			if (kol_c == 0)
			{
				strcpy(OUTCLIENT[0], "Kolom Tidak DItemukan!\n");
				OCLENGTH = 1;
				return;
			}

			char outc[buffSize];
			//print nama kolom
			for (int x = 0; x < kol_c; x++)
			{
				// printf("%s", *(tokens + kol_int[x]));
				char ctab[buffSize];
				sprintf(ctab, "%*s", lenTab, *(tokens + kol_int[x]));
				// sprintf(ctab, "%s", *(tokens + kol_int[x]));
				if (x == 0)
				{
					strcpy(outc, ctab);
					// printf("outc: %s\n", outc);
				}
				else
				{
					strcat(outc, ctab);
				}
				if (x < kol_c - 1)
				{
					// printf("\t");
					strcat(outc, " ");
				}
			}
			// printf("\n");
			strcat(outc, "\n");
			strcpy(OUTCLIENT[0], outc);
			OCLENGTH = 1;
		}
		// for(int x=0; x<kol_c; x++){
		// 	printf("urutan jal: %d, str: %s, urutan didb: %d\n",
		// 	x, *(tokens + kol_int[x]), kol_int[x]);
		// }

		//print isi database, panjang sepanjang cek (dari ftxt_arr)
		for (int x = 2; x < cek; x++)
		{
			char **isiDb;
			isiDb = str_split(TBLARR[x], ',');

			char outc[buffSize];
			if (isiDb)
			{
				for (int i = 0; i < kol_c; i++)
				{ //print urutan sesuai kol_c dari argv(command)
					// printf("%s", *(isiDb + kol_int[i]));
					char ctab[buffSize];
					sprintf(ctab, "%*s", lenTab, *(isiDb + kol_int[i]));
					// sprintf(ctab, "%s", *(isiDb + kol_int[i]));

					if (i == 0)
					{
						strcpy(outc, ctab);
					}
					else
					{
						strcat(outc, ctab);
					}
					if (i < kol_c - 1)
					{
						// printf("\t");
						strcat(outc, " ");
					}
				}
				// printf("\n");
				strcat(outc, "\n");
				strcpy(OUTCLIENT[x - 1], outc);
				OCLENGTH += 1;
			}
		}

		// for (int i=2; i<cek; i++) {
		// 	printf("%s\n", TBLARR[i]);
		// }
	}
}

void run_command(char *comm, int indexClient)
{

	char schar[3] = {',', '.', ';'};
	int i, whereC = -1;
	char database_name[256];

	log_db(comm);

	char *param = malloc(buffSize);
	strcpy(param, comm);

	char **tokens;
	tokens = str_split(comm, ' ');

	if (tokens)
	{
		for (i = 0; *(tokens + i); i++)
		{
			// free(*(tokens + i));
			//buang spesialchar(schar)
			strcpy(*(tokens + i), remove_schar(*(tokens + i), schar));
			if (strcmp(*(tokens + i), "WHERE") == 0)
			{
				whereC = i;
			}
		}
		// free(tokens);
	}

	// i sebagai panjang dari command yang telah ditokens

	if (strcmp(*(tokens + 0), "INSERT") == 0)
	{
		insert_tbl(i, tokens, substr(param, find_index(param, '('), find_index(param, ')') + 1));
	}
	else if (strcmp(*(tokens + 0), "UPDATE") == 0)
	{
		int kolom = -1;
		int baris = fline_where(i, tokens, whereC, &kolom);
		printf("kolom: %d\n", kolom);
	}
	else if (strcmp(*(tokens + 0), "DELETE") == 0)
	{
		int kolom;
		delete_tbl(i, tokens, fline_where(i, tokens, whereC, &kolom));
	}
	else if (strcmp(*(tokens + 0), "SELECT") == 0)
	{
		select_tbl(i, tokens);
	}
	else if (strcmp(*(tokens + 0), "CREATE") == 0)
	{
		if (strcmp(*(tokens + 1), "USER") == 0)
		{
			createuser(indexClient, i, tokens);
		}
		else if (strcmp(*(tokens + 1), "DATABASE") == 0)
		{
			/*
				TODO:
				DATABASE INTEGRATION FOR EVERY COMMAND
			*/
			FILE *fp;
			char message[256];
			sprintf(database_name, "%s", *(tokens + 2));
			mkdir(database_name, 0777);
			sprintf(message, "Database %s telah dibuat!", database_name);
			strcpy(OUTCLIENT[0], message);
			fclose(fp);
		}
		else if (strcmp(*(tokens + 1), "TABLE") == 0)
		{
			/*
				TODO:
				- ADD ATRRIBUT
				- ADD DATA TYPE
			*/
			FILE *fp;
			char table_name[256];
			char table_path[256];
			sprintf(table_name, "%s", *(tokens + 2));
			sprintf(table_path, "%s/%s", database_name, table_name);
			fopen(table_path, "w");
			fputs("asdsa", table_path);
			fclose(fp);
		}
	}
}

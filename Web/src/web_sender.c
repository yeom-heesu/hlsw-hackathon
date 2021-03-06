#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include "meta.h"

#define BUFSIZE 1024
#define SENDINGUNIT 1000

META meta_data;

void argument_check(int argc){
		if(argc != 7){
			printf("usage : ./web_sender ip port meta_requst_number meta_id meta_subject_id meta_assignment_id\n");
			exit(EXIT_FAILURE);
		}
}

void set_meta(char * argv[]){
	meta_data.request_number = atoi(argv[3]);
	strcpy(meta_data.id, argv[4]);
	meta_data.subject_id = atoi(argv[5]);
	meta_data.assignment_id = atoi(argv[6]);
}

#ifdef DEBUG
void check_meta(){
	printf("meta_data.request_number : %d\n", meta_data.request_number);
	printf("meta_data.id : %s\n", meta_data.id);
	printf("meta_data.subject_id : %d\n", meta_data.subject_id);
	printf("meta_data.assginment_id : %d\n", meta_data.assignment_id);
}
#endif
 
int open_file(){
	int fd;
	char file_name[BUFSIZE];

#ifdef DEBUG
	strcat(file_name, meta_data.id);
	strcat(file_name, ".c");
#endif

#ifndef DEBUG
	getcwd(file_name, BUFSIZE);
	strcat(file_name, "/files/./");
	strcat(file_name, meta_data.id);
	strcat(file_name, ".c");
#endif

	fd = open(file_name, O_RDONLY);
	if(fd == -1){
		printf("file open error!\n");
		exit(EXIT_FAILURE);
	}
	return fd;
}

void socket_create(int *server_sockfd){
	if((*server_sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		printf("server socket create failed\n");
		exit(EXIT_FAILURE);
	}
}

void binding(struct sockaddr_in *server_addr,int server_sockfd, const char *ip, const char *port){
	server_addr->sin_family = AF_INET;
	server_addr->sin_addr.s_addr = inet_addr(ip);
	server_addr->sin_port = htons(atoi(port));

	if(connect(server_sockfd, (struct sockaddr*)server_addr, sizeof(struct sockaddr_in)) == -1) {
		printf("connect(bind) error!\n");
		exit(EXIT_FAILURE);
	}
}

#ifdef DEBUG
void check_buf(char *buf){
	for(int i=0; i<30; i++)
		printf("%c\n", buf[i]);
}
#endif

void send_meta(int server_sockfd){
	char buf[BUFSIZE];

	memset(buf, 0x00, BUFSIZE);
	memcpy(buf, &meta_data, sizeof(META));
	write(server_sockfd, buf, SENDINGUNIT);
#ifdef DEBUG
	check_buf(buf);
#endif
}


void read_and_send(int file_fd, int server_sockfd){
	int read_size;
	char buf[BUFSIZE];

	while(1){
		memset(buf, 0x00, BUFSIZE);
		read_size = read(file_fd, buf, SENDINGUNIT);

		if(read_size == 0){
#ifdef DEBUG
			printf("%d\n", read_size);
			printf("finished file!\n");
#endif
			break;
		}

		if(write(server_sockfd, buf, SENDINGUNIT) <= 0) {
			printf("write error : ");
			exit(EXIT_FAILURE);
		}
	}

	memset(buf, 0xff, 4);
	write(server_sockfd, buf, SENDINGUNIT);
}

#ifdef DEBUG
void receive_ack(int server_sockfd){
	char buf[BUFSIZE];

	if(read(server_sockfd, buf, SENDINGUNIT) <= 0) {
		printf("read error: ");
		exit(EXIT_FAILURE);
	}
	printf("[server]\n%s", buf);
}
#endif

void print_rs(){
	int fd;
	char buf[BUFSIZE];

	getcwd(buf, BUFSIZE);
	strcat(buf, "/files/./");
	strcat(buf, meta_data.id);
	strcat(buf, ".rs");

	while( (fd = open(buf, O_RDONLY)) == -1)
		usleep(100000);

	while(1){
		memset(buf, 0x00, BUFSIZE);
		if (( fd = read(fd, buf, BUFSIZE)) <= 0)
			break;

		printf("%s", buf);
	}
}

void rm_file(){
	char command[BUFSIZE];
	char buf[BUFSIZE];

	getcwd(buf, BUFSIZE);
	strcat(buf, "/files/");
	strcat(buf, meta_data.id);
	strcat(buf, ".rs");
	
	strcpy(command, "rm -r ");
	strcat(command, buf);
	strcat(command, " > /dev/null 2>&1");

	system(command);
}


int main(int argc, char *argv[]) {
	struct sockaddr_in server_addr;
	int server_sockfd;
	int fd;


#ifdef DEBUG
	for(int i = 0; i < argc; i++){
		printf("%s\n", argv[i]);
	}
#endif

	argument_check(argc);
	set_meta(argv);
	rm_file();

#ifdef DEBUG
	check_meta();
#endif
	fd = open_file();

	socket_create(&server_sockfd);
	binding(&server_addr, server_sockfd, argv[1], argv[2]);
	
	send_meta(server_sockfd);
	read_and_send(fd, server_sockfd);
#ifdef DEBUG
	check_meta();
#endif

#ifdef DEBUG
	receive_ack(server_sockfd);
#endif
	close(server_sockfd);
	close(fd);


	print_rs();

	return 0;
}


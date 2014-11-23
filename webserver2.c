#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define BUF_SIZE 1024
#define MIN_BUF 100

void* requestHandler(void* arg);
void sendData(FILE* fp, char* contentType, char* fileName);
const char* getContentType(const char* file);
void sendError(FILE* fp);
void errorHandling(char* message);

int main(int argc, char *argv[]){
    int servSock, clntSock;
    struct sockaddr_in servAddr, clnt_adr;
    int lengthClientAddress;
    char buf[BUF_SIZE];
    pthread_t t_id;
    if(argc != 2){
        printf("usage: %s <port>\n", argv[0]);
        exit(1);
    }
    
    servSock = socket(PF_INET, SOCK_STREAM, 0);
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family=AF_INET;
    servAddr.sin_addr.s_addr=htonl(INADDR_ANY);
    servAddr.sin_port=htons(atoi(argv[1]));
    
    if(bind(servSock, (struct sockaddr*)&servAddr, sizeof(servAddr)) == -1)
        errorHandling("bind error");
    
    if(listen(servSock, 12)==-1)
        errorHandling("listen error");
    
    while(1){
        lengthClientAddress = sizeof(clnt_adr);
        clntSock = accept(servSock, (struct sockaddr*)&clnt_adr, &lengthClientAddress);
        if (clntSock == -1)
            continue;
        printf("connect request! :%s:%d\n", inet_ntoa(clnt_adr.sin_addr), ntohs(clnt_adr.sin_port));
        pthread_create(&t_id, NULL, requestHandler, &clntSock);
        pthread_detach(t_id);
    }
    close(servSock);
    return 0;
}

void* requestHandler(void* arg){
    int clntSock = *((int*)arg);
    char requestLine[MIN_BUF];
    FILE* clientWrite;
    FILE* clientRead;
    int i = 0;
    int length;
    
    char method[10];
    char contentType[15];
    char fileName[30];
    
    clientRead = fdopen(clntSock, "r");
    clientWrite = fdopen(dup(clntSock), "w");
    fgets(requestLine, MIN_BUF, clientRead);
    
    if(strstr(requestLine, "HTTP/") == NULL){
        sendError(clientWrite);
        fclose(clientRead);
        fclose(clientWrite);
        return NULL;
    }
    
    strcpy(method, strtok(requestLine, " /"));
    strcpy(fileName, strtok(NULL, " /"));
    length = sizeof(fileName)/sizeof(fileName[0]);
    /*
    for(i = 0; i< length; i++){
        printf("%c", fileName[i]);
    }
    printf("\n");
     */

    //주소 뒤가 비었을 때 index.html로 가도록 처리
    if(!strcmp(fileName, "HTTP")){
        strcpy(fileName, "index.html");
    }
    


    strcpy(contentType, getContentType(fileName));
    //메소드가 get이 아닐 경우 에러 처리
    if(strcmp(method, "GET") != 0){
        sendError(clientWrite);
        fclose(clientRead);
        fclose(clientWrite);
        return NULL;
    }
    
    fclose(clientRead);
    sendData(clientWrite, contentType, fileName);
    
    return NULL;
}

/*
    브라우저로 보내는 정보를 프로토콜 형식에 맞춰서 만들기
 */
void sendData(FILE* fp, char* contentType, char* fileName){
    char protocol[] = "HTTP/1.0 200 OK\r\n";
    char server[] = "Server: Linux Web server \r\n";
    char contentLength[] = "content length: 2048\r\n";
    char contentTypeLine[MIN_BUF];
    char buf[BUF_SIZE];
    FILE* sendFile;
    size_t readLength;
    
    sprintf(contentTypeLine, "contentType: %s\r\n\r\n", contentType);
    sendFile = fopen(fileName, "r");
    if(sendFile == NULL){
        sendError(fp);
        fclose(fp);
        return;
    }
    
    fputs(protocol, fp);
    fputs(server, fp);
    fputs(contentLength, fp);
    fputs(contentTypeLine, fp);
    
    while((readLength = fread(buf, 1, BUF_SIZE, sendFile)) > 0){
        fwrite(buf, readLength, 1, fp);
        fflush(fp);
    }
    fflush(fp);
    fclose(fp);
}

const char* getContentType(const char* file){
    char extension[MIN_BUF];
    char fileName[MIN_BUF];
    strcpy(fileName, file);
    strtok(fileName, ".");
    strcpy(extension, strtok(NULL, "."));

    if(!strcmp(extension, "html") || !strcmp(extension, "htm"))
        return "text/html";
    else if(!strcmp(extension, "jpg"))
        return "image/jpeg";
    else if(!strcmp(extension, "png"))
        return "image/png";
    else
        return "text/plain";
}

void sendError(FILE* fp){
    char protocol[] = "HTTP/1.0 404 Not Found\r\n";
    char server[] = "Server: linux Web Server \r\n";
    char contentLength[] = "Content length:2048\r\n";
    char contentType[] = "Content type: text/html\r\n\r\n";
    char content[] = "<html><head><title> network </title></head>"
    "<body><font-size = +5>error occur! check the content type & file name"
    "</font></bodly></html>";
    
    fputs(protocol, fp);
    fputs(server, fp);
    fputs(contentLength, fp);
    fputs(contentType, fp);
    fputs(content, fp);
    fflush(fp);
}

void errorHandling(char* message){
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
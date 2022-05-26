//****************************************************************************
//                         REDES Y SISTEMAS DISTRIBUIDOS
//                      
//                     2º de grado de Ingeniería Informática
//                       
//              This class processes an FTP transaction.
// 
//****************************************************************************


  
#include <cstring>
#include <cstdarg>
#include <cstdio>
#include <cerrno>
#include <netdb.h>
 
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <locale.h>
#include <langinfo.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/stat.h> 
#include <iostream>
#include <dirent.h>

#include "common.h"

#include "ClientConnection.h"
#include "FTPServer.h"




ClientConnection::ClientConnection(int s) {
    int sock = (int)(s);
  
    char buffer[MAX_BUFF];

    control_socket = s;
    // Check the Linux man pages to know what fdopen does.
    fd = fdopen(s, "a+");
    if (fd == NULL){
	std::cout << "Connection closed" << std::endl;

	fclose(fd);
	close(control_socket);
	ok = false;
	return ;
    }
    
    ok = true;
    data_socket = -1;
    parar = false;
   
  
  
};


ClientConnection::~ClientConnection() {
 	fclose(fd);
	close(control_socket); 
  
}


int connect_TCP( uint32_t address,  uint16_t  port) { /// 
     // Implement your code to define a socket here
  struct sockaddr_in sin;
  int s;

  memset(&sin, 0, sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_port = htons(port);
  sin.sin_addr.s_addr = htonl(address);
     
  s = socket(AF_INET, SOCK_STREAM, 0);

  if(s < 0)
    errexit("No se puede crear el socket: %s\n", strerror(errno));

  if(connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
    errexit("No se puede conectar con : %s\n", strerror(errno));
 
  return s;
}






void ClientConnection::stop() {
    close(data_socket);
    close(control_socket);
    parar = true;
  
}





    
#define COMMAND(cmd) strcmp(command, cmd)==0

// This method processes the requests.
// Here you should implement the actions related to the FTP commands.
// See the example for the USER command.
// If you think that you have to add other commands feel free to do so. You 
// are allowed to add auxiliary methods if necessary.

void ClientConnection::WaitForRequests() {
    if (!ok) {
	 return;
    }
    
    fprintf(fd, "220 Service ready\n");
  
    while(!parar) {

      fscanf(fd, "%s", command);
      if (COMMAND("USER")) {
	    fscanf(fd, "%s", arg);
	    fprintf(fd, "331 User name ok, need password\n");
      }
      else if (COMMAND("PWD")) {
	   
      }
      else if (COMMAND("PASS")) {
        fscanf(fd, "%s", arg);
        if(strcmp(arg,"1234") == 0){
            fprintf(fd, "230 User logged in\n");
        }
        else{
            fprintf(fd, "530 Not logged in.\n");
            parar = true;
        }
	   
      }
      else if (COMMAND("PORT")) {
	  // To be implemented by student
      int a0, a1, a2, a3, p0, p1;
        fscanf(fd, "%d, %d, %d, %d,%d, %d", &a0, &a1, &a2, &a3, &p0, &p1);
        int32_t address = a0 << 24 | a1 << 16 | a2 << 8 | a3; /// PARA CONFIGURAR LA IP, LOS DESPLAZOS ES PARA POSIICONAR 127.0.0.1 == 24, 16, 8, 0
        int16_t port = p0 << 8 | p1; /// PARA CONFIGURAR LOS PUERTOS , LOS DESPLAZOS ES PARA POSIICONAR 56, 17
        data_socket = connect_TCP(address, port);
        if (data_socket < 0) {
          fprintf(fd, "425 Can't open data connection.\n");
        } else {
          fprintf(fd, "200 OK\n");   
        }
      }
      else if (COMMAND("PASV")) {
	  // To be implemented by students
        sockaddr_in socket;
        socklen_t lenght=sizeof(socket);
        int s = define_socket_TCP(0);
        getsockname(s, (sockaddr*)&socket, &lenght);
        uint16_t port = socket.sin_port;
        int p1, p2;
        p1 = port >> 8;
        p2 = port & 0xFF;
        fprintf(fd, "227 Entering passive mode (127,0,0,1,%d,%d)\n", p2, p1);
        fflush(fd);
        data_socket = accept(s, (sockaddr*)&socket, &lenght);
      }
      else if (COMMAND("STOR") ) {
	    // To be implemented by students
        //char buffer[1024];
        //int maxbuffer = 32;
        fscanf(fd, "%s", arg);  
        FILE* file = fopen(arg, "wb");
        if (!file) {
          fprintf(fd, "450 Requested file action not taken.\n");
          close(data_socket);
        }
        else {
          fprintf(fd, "150 File status okay; about to open data connection.\n");
          fflush(fd);
          char buffer[MAX_BUFF];
          size_t recive;
          int count(0);
          do {
            recive = recv(data_socket, buffer, MAX_BUFF, 0);
            fwrite(buffer, 1, recive, file);
          } while (recive != 0);
          fprintf(fd, "226 Closing data connection.\n");
           fflush(fd);
          fclose(file);
          close(data_socket);
        }
      }
      else if (COMMAND("RETR")) {
	   // To be implemented by students
        fscanf(fd, "%s", arg);
        printf("(RETR):%s\n", arg);
        FILE* file = fopen(arg, "rb");
        if (!file) {
          fprintf(fd, "550 Requested action not taken.\n");
          fflush(fd);
        }
        else {
          fprintf(fd,"150 File status okay; about to open data connection.\n");
          fflush(fd);
          char buff[MAX_BUFF];
          int aux;

          do {
            aux = fread(buff, 1,MAX_BUFF, file);
            send(data_socket, buff, aux, 0);
          } while (aux == MAX_BUFF);
          
          fprintf(fd, "226 Closing data connection.\n");
          fflush(fd);
          fclose(file);
          close(data_socket);
        }
          
      }else if (COMMAND("LIST")) {
	   // To be implemented by students	
        DIR *dir = opendir(".");
        struct dirent *directorio;

        if(dir > 0) {
          fprintf(fd,"125 Data connection already open; transfer starting.\n");
          while ((directorio = readdir(dir))) {
            std::string buffer = directorio->d_name;
            buffer += "\x0D\x0A";
            send(data_socket, buffer.c_str(), buffer.size(), 0);  ///MIRAR ESTO POR QUE NO VA  
          }
          close(data_socket);
          closedir(dir);
          fprintf(fd,"250 Requested file action okay, completed\n");
        } 
        //fprintf(fd, "200  OK\n");
      }
      else if (COMMAND("SYST")) {
           fprintf(fd, "215 UNIX Type: L8.\n");   
      }

      else if (COMMAND("TYPE")) {
	  fscanf(fd, "%s", arg);
	  fprintf(fd, "200 OK\n");   
      }
     
      else if (COMMAND("QUIT")) {
        fprintf(fd, "221 Service closing control connection. Logged out if appropriate.\n");
        close(data_socket);	
        parar=true;
        break;
      }
  
      else  {
	    fprintf(fd, "502 Command not implemented.\n"); fflush(fd);
	    printf("Comando : %s %s\n", command, arg);
	    printf("Error interno del servidor\n");
	
      }
      
    }
    
    fclose(fd);
   
    
    return;
  
};

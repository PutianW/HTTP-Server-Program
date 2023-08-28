
const char * usage =
"                                                               \n"
"myhttpd:                                                       \n"
"                                                               \n"
"HTTP program that allows clients to communicate with server    \n"
"using sockets. (Concurrency allowed)                           \n"
"                                                               \n"
"                                                               \n"
"Username: wpt                                                  \n"
"Password: 12345                                                \n"
"                                                               \n"
"                                                               \n"
"To use it:                                                     \n"
"1) In one window type:                                         \n"
"                                                               \n"
"      myhttpd [-f|-t|-p] [<port>]                              \n"
"                                                               \n"
"   -f: process mode                                            \n"
"   -t: thread mode                                             \n"
"   -p: pool of threads                         .               \n"
"   1024 < port < 65536.                                        \n"
"                                                               \n"
"2) Else, you can type:                                         \n"
"                                                               \n"
"      myhttpd [<port>]                                         \n"
"                                                               \n"
"   Where 1024 < port < 65536.                                  \n"
"   And it opens in default iterative mode.                     \n"
"                                                               \n"
"3) Else, you can type:                                         \n"
"                                                               \n"
"      myhttpd                                                  \n"
"                                                               \n"
"   Where default port is set to be \"9104\".                   \n"
"   And it opens in default iterative mode.                     \n"
"                                                               \n"
"                                                               \n"
"                                                               \n"
"In another window:                                             \n"
"1) You can type:                                               \n"
"                                                               \n"
"   telnet <host> <port>                                        \n"
"                                                               \n"
"2) Else, you can use Chrome/Firefox, and type:                 \n"
"                                                               \n"
"   data.cs.purdue.edu: <port>                                  \n"
"                                                               \n"
"Where <host> is the name of the machine where myhttpd          \n"
"is running. And <port> is the port number you used             \n"
"when you run myhttpd.                                          \n"
"                                                               \n"
"                                                               \n";


#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <fcntl.h>
#include <cstdio>
#include <iostream>
#include <signal.h>
#include <sys/wait.h>
#include <pthread.h>
#include <cstdlib>


int QueueLength = 5;
pthread_mutex_t mutex;
int masterSocket;

// username: wpt,   password: 12345
char * correctPW = (char *) "d3B0OjEyMzQ1";

// document path
char * httpDocPath = (char *) "/http-root-dir/htdocs";

// helper function for sending request and response
void processRequest( int socket );
void sendHeader(int fd, char * contentType);
void sendPlainText(int fd, char * content);
void sendHTML(int fd, int file);
void sendGIF(int fd, int file);
void sendSVG(int fd, int file);
void sendLogin(int fd);
void sendErr(int fd, char * errMsg);
void * loopthread(int masterSocket);
bool endsWith(char* str, char* suffix);
void processRequestThread (int fd);
void newThread(int fd);

void disp_zombie(int sig) {
  if (sig == SIGCHLD) {
	  while (waitpid(-1, NULL, WNOHANG) > 0) {
    }
  }
}

void disp_ctrl_c(int sig) {
  // close the Master Socket first, then exit
  if (sig == SIGINT) {
    printf("\nMaster Socket Closed!\n\n\n");
	  close(masterSocket);
    exit(0);
  }
}


int main( int argc, char ** argv ) {

  // set ctrl-C handler to close the Master Socket first, then exit
  struct sigaction sa_ctrl_c;
  sa_ctrl_c.sa_handler = disp_ctrl_c;
  sigemptyset(&sa_ctrl_c.sa_mask);
  sa_ctrl_c.sa_flags = SA_RESTART;

  // setting action handler for zombie process
  struct sigaction sa;
  sa.sa_handler = disp_zombie;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;

  // handling zombie process whenever a child exits
  if (sigaction(SIGCHLD, &sa, NULL)) {
      perror("sigaction");
      exit(2);
  }

  // handling ctrl-c
  if(sigaction(SIGINT, &sa_ctrl_c, NULL)){
    perror("sigaction");
    exit(2);
  }

  int port;
  char flag;

  // Print usage if not enough arguments
  if ( argc != 3 && argc != 2 && argc != 1) {
    fprintf( stderr, "%s", usage );
    exit( -1 );
  }

  if (argc == 1) {
    // basic server mode with port = 9014
    port = 9104;
    flag = 'n';

  } else if (argc == 2) {

    // determine if the second argument is numeric or not
    char * secondArg = (char *) argv[1];
    char c;
    for (int i = 0; i < strlen(secondArg); i++) {
      c = secondArg[i];
      if (! (c >= 48 && c <= 57) ) {
        fprintf( stderr, "%s", usage );
        exit( -1 );
      }
    }

    port = atoi( argv[1] );
    flag = 'n';

  } else if (argc == 3) {

    // get the flag from the second argument, and check the format
    char * secondArg = (char *) argv[1];
    if (strlen(secondArg) != 2 || secondArg[0] != '-' || (secondArg[1] != 'f' && secondArg[1] != 't' && secondArg[1] != 'p') ) {
      fprintf( stderr, "%s", usage );
      exit( -1 );
    }

    // get the flag value
    flag = secondArg[1];
    
    // determine if the third argument is numeric or not
    char * thirdArg = (char *) argv[2];
    char c;
    for (int i = 0; i < strlen(thirdArg); i++) {
      c = thirdArg[i];
      if (! (c >= 48 && c <= 57) ) {
        fprintf( stderr, "%s", usage );
        exit( -1 );
      }
    }

    // Get the port from the third argument
    port = atoi( argv[2] );

  }


  printf("flag is: %c\n", flag);
  printf("port is: %d\n", port);

  
  // Set the IP address and port for this server
  struct sockaddr_in serverIPAddress; 
  memset( &serverIPAddress, 0, sizeof(serverIPAddress) );
  serverIPAddress.sin_family = AF_INET;
  serverIPAddress.sin_addr.s_addr = INADDR_ANY;
  serverIPAddress.sin_port = htons((u_short) port);
  
  // Allocate a socket
  masterSocket =  socket(PF_INET, SOCK_STREAM, 0);
  if ( masterSocket < 0) {
    perror("socket");
    exit( -1 );
  }

  // Set socket options to reuse port. Otherwise we will
  // have to wait about 2 minutes before reusing the sae port number
  int optval = 1; 
  int err = setsockopt(masterSocket, SOL_SOCKET, SO_REUSEADDR, 
		       (char *) &optval, sizeof( int ) );
   
  // Bind the socket to the IP address and port
  int error = bind( masterSocket,
		    (struct sockaddr *)&serverIPAddress,
		    sizeof(serverIPAddress) );
  if ( error ) {
    perror("bind");
    exit( -1 );
  }
  
  // Put socket in listening mode and set the 
  // size of the queue of unprocessed connections
  error = listen( masterSocket, QueueLength);
  if ( error ) {
    perror("listen");
    exit( -1 );
  }

  if (flag == 'p') {
    // Pool of threads
    pthread_t threads[ QueueLength ];

    // initialize mutex lock
    pthread_mutex_init(&mutex, NULL);

    // create multiple threads of "accept loops"
    for(int i = 0; i < QueueLength; i++) {
      pthread_create(&threads[i], NULL, (void * (*) (void *)) loopthread, (void *) masterSocket);
    }
    pthread_join(threads[0], NULL);

  } else {

    while ( 1 ) {

      struct sockaddr_in clientIPAddress;
      int alen = sizeof( clientIPAddress );
      int slaveSocket = accept( masterSocket, (struct sockaddr *) &clientIPAddress, (socklen_t*) &alen);

      if ( slaveSocket < 0 ) {
        perror( "accept" );
        exit( -1 );
      }

      if (flag == 'f') {
        // Create a new process for each request

        int ret = fork();
        if (ret == 0) {
          // child process
          // Process request.
          processRequest( slaveSocket );
          exit(0);

        } else if (ret < 0) {
          perror("fork");
          exit(1);
        }

        // parent process
        // close the socket fd in the parent process
        close( slaveSocket );

      } else if (flag == 't') {

        newThread(slaveSocket);

      } else if (flag == 'n') {
        // basic server mode
        // process request
        processRequest(slaveSocket);
      }
    }
  }

  return 0;
  
}


void newThread(int fd) {
  // Create a new thread for each request
  pthread_t t1;
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  // run the thread
  pthread_create(&t1, &attr, (void * (*) (void *)) processRequestThread, (void *) fd);
}


void processRequestThread(int fd) {
  processRequest(fd);
}


void * loopthread(int masterSocket) {
  while ( 1 ) {

    // lock the accept()
    pthread_mutex_lock(&mutex);
    struct sockaddr_in clientIPAddress;
    int alen = sizeof( clientIPAddress );
    int slaveSocket = accept( masterSocket, (struct sockaddr *) &clientIPAddress, (socklen_t*) &alen);
    pthread_mutex_unlock(&mutex);

    if ( slaveSocket < 0 ) {
      perror( "accept" );
      exit( -1 );
    }

    // process request
    processRequest(slaveSocket);

  }
}



void processRequest( int fd ) {

  // initialize variables for parsing
  const int MaxRequest = 1024;
  int length = 0;
  int n;
  char currStr[ MaxRequest + 1 ];
  char docPath[ MaxRequest + 1 ];
  bool isGet = false;
  bool isAuthor = false;
  bool isBasic = false;
  bool hasPW = false;
  bool hasDocPath = false;
  bool isPWvalid = false;
  int countRound = 0;
  char tryPassword[1024];

  // reset the reading buffers to all 0's
  memset(currStr, 0, sizeof(currStr));
  memset(docPath, 0, sizeof(docPath));
  memset(tryPassword, 0, sizeof(tryPassword));

  // Currently character read
  unsigned char newChar;

  // Last character read
  unsigned char lastChar = 0;

  // read the "GET" protocal line
  while ((n = read ( fd, &newChar, sizeof(newChar) ) )  > 0 ) {

    if (newChar == ' ') {
      // read <sp>
      if (!isGet) {
        // read the <sp> after "GET", then clear current string
        isGet = true;
        memset(currStr, 0, sizeof(currStr));
        length = 0;
      } else {
        // read the <sp> after "document path", then get the docPath
        currStr[ length ] = 0;
        strcpy(docPath, currStr);
        memset(currStr, 0, sizeof(currStr));
        length = 0;
      }
    } else if (lastChar == '\r' && newChar == '\n') {
      // read <crlf>, then stop reading the first protocal line that has "GET"
      break;
    } else {
      // read one normal character
      currStr[ length ] = newChar;
      length++;
      lastChar = newChar;
    }
  }

  // print the "GET" line in the server terminal
  printf("\n\n\n\"GET\" protocal line read, the docPath is: %s\n", docPath);

  // read the remaining part of the request
  while ((n = read( fd, &newChar, sizeof(newChar) ) )  > 0 ) {

    // read the ' ' trying to find the 'Authorization' and 'Basic'
    if (newChar == ' ') {
      if (!strncmp("Authorization:", currStr, 14)) {
        isAuthor = true;
      } else if (isAuthor && !strncmp("Basic", currStr, 5)) {
        isBasic = true;
      }
      // clear currStr
      memset(currStr, 0, sizeof(currStr));
      length = 0;

    } else if (lastChar == '\r' && newChar == '\n') {

      // read <crlf> tring to find the encoded password
      if (isAuthor && isBasic && !hasPW) {
        currStr[ length ] = 0;
        strcpy(tryPassword, currStr);
        memset(currStr, 0, sizeof(currStr));
        length = 0;
        hasPW = true;
      }

      // detect consecutive <crlf>, if true, then break the loop
      if (countRound == 1) {
        break;
      } else {
        // reset the "count consecutive <crlf>" variables
        memset(currStr, 0, sizeof(currStr));
        length = 0;
        countRound = 0;
      }

    } else {

      // read one normal character
      currStr[ length ] = newChar;
      length++;
      lastChar = newChar;

      // seting "count rounds" variable in order to detect consecutive <crlf>
      countRound++;
    }
  }

  // compare the input password and the correct password
  if (!strncmp(tryPassword, correctPW, 12)){
    isPWvalid = true;
  } else {
    isPWvalid = false;
  }


  // printf("The complete http request is: \n\n%s\n", test);
  printf("Input Password is:   %s\n", tryPassword);


  // initialize the variables for path extension
  char cwd[1024];
  char filePath[1024];
  memset(cwd, 0, sizeof(cwd));
  memset(filePath, 0, sizeof(filePath));

  // recognize the type of request, send back the corresponding response to http client
  if (!isAuthor || !isBasic || !isPWvalid) {
    // no PW, or incorrect PW
    sendLogin(fd);
    printf("Invalid login!!!!!!!\n\n\n\n\n");

  } else {
    // get the current directory
    getcwd(cwd, sizeof(cwd));
    getcwd(filePath, sizeof(filePath));
    strcat(cwd, (char *) "/http-root-dir");
    strcat(filePath, httpDocPath);

    // add the docPath to the filePath
    if ( (!strncmp("/", docPath, 1) && strlen(docPath) == 1) || (!strncmp("/index.html", docPath, 11)) ) {
      char * index = (char *) "/index.html";
      strcat(filePath, index);

    } else if (!strncmp("/favicon.ico", docPath, 12)) {
      // the small icon for the web page header
      char * favicon = (char *) "/pok1.gif";
      strcat(filePath, favicon);

    } else {
      strcat(filePath, docPath);

    }

    // get the realPath of the request
    char realPath[1024];
    if (realpath(filePath, realPath) == NULL) {
      // if the real path doesn't exists, send err message
      printf("ERROR! Could not open this specified URL!!!!!!!!!!\n\n\n\n\n\n");
      sendErr(fd, (char *) "ERROR! Could not open the specified URL!\n");

    } else if (strlen(realPath) < strlen(cwd)) {
      // if the real path is above the scope, send err message
      printf("Realpath is: %s\n", realPath);
      printf("cwd is: %s\n\n\n", cwd);
      printf("ERROR! This request is asking for file that is above the selected directoty!!!!!!!!!!!\n\n\n\n\n\n");
      sendErr(fd, (char *) "ERROR! This request is asking for file that is above the selected directoty!\n");

    } else {
      // if the real path fits
      printf("Realpath is: %s\n", realPath);
      printf("cwd is: %s\n\n\n", cwd);

      // open the file
      int file = open(filePath, O_RDONLY, 0777);
      
      if (file == -1) {
        // if failed to open the file, send err message
        printf("ERROR! Could not open this specified URL!!!!!!!!!!\n\n\n\n\n\n");
        sendErr(fd, (char *) "ERROR! Could not open the specified URL!\n");

      } else {
        // determine the type of content
        if (endsWith(filePath, (char *) ".html")) {
          sendHTML(fd, file);

        } else if (endsWith(filePath, (char *) ".png") || endsWith(filePath, (char *) ".gif")) {
          sendGIF(fd, file);

        } else if (endsWith(filePath, (char *) ".svg")) {
          sendSVG(fd, file);

        } else {
          // send err message if the content type doesn't exist
          printf("ERROR! Could not open this specified URL!!!!!!!!!!\n\n\n\n\n\n");
          sendErr(fd, (char *) "ERROR! Could not open the specified URL!\n");

        }
      }
    }
  }

  // reset the reading buffers to all 0's
  memset(currStr, 0, sizeof(currStr));
  memset(docPath, 0, sizeof(docPath));
  memset(tryPassword, 0, sizeof(tryPassword));
  memset(cwd, 0, sizeof(cwd));
  memset(filePath, 0, sizeof(filePath));

  // close the client/slave socket
  close(fd);
}



void sendLogin(int fd) {
  // write authorization prompt as response
  const char * content = "HTTP/1.1 401 Unauthorized\r\nWWW-Authenticate: Basic realm=\"myhttpd-cs252\"";
  write(fd, content, strlen(content));
}


void sendHeader(int fd, char * contentType) {
  // write the first half of the response protocal
  const char * firstPart = "HTTP/1.1 200 Document follows\r\nServer: CS 252 lab5\r\nContent-type: ";
  write(fd, firstPart, strlen(firstPart));

  // write the content type
  write(fd, (const char *) contentType, strlen(contentType));

  // write the rest of the response protocal
  const char * secondPart = "\r\n\r\n";
  write(fd, secondPart, strlen(secondPart));
}


void sendPlainText(int fd, char * content) {
  sendHeader(fd, (char *) "text/plain");
  write(fd, (const char *) content, strlen(content));
}


void sendHTML(int fd, int file) {
  // read the HTML file as binary file, and then write
  int n;
  sendHeader(fd, (char *) "text/html");

  char buf[1024];
  while ( (n = read(file, buf, 1024)) > 0 ) {
    write(fd, (const char *) buf, n);
  }

  close(file);
}


void sendGIF(int fd, int file) {
  // read the GIF file as binary file, and then write
  int n;
  sendHeader(fd, (char *) "image/gif");

  char buf[1024];
  while ( (n = read(file, buf, 1024)) > 0 ) {
    write(fd, (const char *) buf, n);
  }

  close(file);
}


void sendSVG(int fd, int file) {
  // read the SVG file as binary file, and then write
  int n;
  sendHeader(fd, (char *) "image/svg+xml");

  char buf[1024];
  while ( (n = read(file, buf, 1024)) > 0 ) {
    write(fd, (const char *) buf, n);
  }

  close(file);
}


void sendErr(int fd, char * errMsg) {
  const char * errHeader = "HTTP/1.1 404 File Not Found\r\nServer: CS 252 lab5\r\nContent-type: text/html\r\n\r\n";
  write(fd, errHeader, strlen(errHeader));
  write(fd, errMsg, strlen(errMsg));
}


bool endsWith(char* str, char* suffix) {
  // determine if the string ends with a specific pattern
  if (!str || !suffix) {
    return false;
  }
  
  int str_len = strlen(str);
  int suffix_len = strlen(suffix);
  
  if (suffix_len > str_len) {
    return false;
  }
  
  return strcmp(str + str_len - suffix_len, suffix) == 0;
}
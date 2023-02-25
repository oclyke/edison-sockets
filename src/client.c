/**
 * @file client.c
 * @author oclyke
 * @brief client best practices
 *
 * This code implements a client to send a message to the
 * server and read the response.
 */

#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

static int show_usage(char* progname);

int main(int argc, char* argv[]) {
  // set some initial values
  int ret = 0;
  char* hostname = "localhost";
  int port_number = -1;
  char* message = "hello world";

  // parse arguments
  // - the supplied arguments always begins with the name of the program
  // argv[0] = program name
  // argv[1 ... n] = a string for each space-separated argument on the command
  // line
  char* progname = argv[0];

  // since we are requiring one positional argument the number of arguments
  // must be *at least* 2 (the required argument )
  if (argc < 2) {
    fprintf(stderr, "ERROR: not enough arguments supplied\n");
    show_usage(progname);
    return 1;
  }

  // parse all arguments after the program name
  for (int idx = 1; idx < argc; idx++) {
    char* arg = argv[idx];
    if (strcmp(arg, "--hostname") == 0) {
      idx++;
      hostname = argv[idx];
    } else if (strcmp(arg, "--message") == 0) {
      idx++;
      message = argv[idx];
    } else {
      port_number = atoi(arg);
    }
  }

  // construct a socket to be used in connection mode
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    fprintf(stderr, "ERROR creating socket\n");
    return 1;
  }

  // get server information
  struct hostent* server = gethostbyname(hostname);
  if (server == NULL) {
    fprintf(stderr, "ERROR, no such host\n");
    return 1;
  }

  // get server address information
  struct sockaddr_in serv_addr;
  bzero((char*)&serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  bcopy(
      (char*)server->h_addr, (char*)&serv_addr.sin_addr.s_addr,
      server->h_length);
  serv_addr.sin_port = htons(port_number);

  // connect the socket to the server
  printf("connecting to server at %s:%d\n", hostname, port_number);
  ret = connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
  if (ret < 0) {
    fprintf(stderr, "ERROR connecting to server\n");
    return 1;
  }

  // send the message to the server
  printf("sending message: \"%s\"\n", message);
  int message_len = strlen(message);
  int chars_sent = send(sockfd, &message, message_len, 0);
  if (chars_sent < 0) {
    fprintf(stderr, "ERROR sending message\n");
    return 1;
  }
  if (chars_sent != message_len) {
    fprintf(
        stderr,
        "ERROR: expected to send %d characters but actually sent %d "
        "characters\n",
        message_len, chars_sent);
    return 1;
  }

  // read the response from the server
  printf("receiving response: \"");
  const size_t rx_buffer_len = message_len;
  char rx_buffer[rx_buffer_len + 1];
  int total_received = 0;
  while (total_received < message_len) {
    // determine how many characters left to get back the whole message
    int chars_remaining = message_len - total_received;

    // determine how many characters to receive so as not to overflow the rx
    // buffer
    int chars_request = chars_remaining;
    if (chars_request > rx_buffer_len) {
      chars_request = rx_buffer_len;
    }

    // receive a chunk from the server
    int chars_received = recv(sockfd, &rx_buffer, chars_request, 0);
    if (chars_received < 0) {
      fprintf(stderr, "ERROR receiving message\n");
      return 1;
    }
    if (chars_received != chars_request) {
      fprintf(
          stderr, "ERROR: expected to read %d chars but actually read %d\n",
          chars_request, chars_received);
      return 1;
    }

    // increment the number of total characters received
    total_received += chars_received;

    // ensure null-termination
    // (this uses a secret extra entry in the rx buffer that is not accounted
    // for in rx_buffer_len)
    rx_buffer[rx_buffer_len] = 0;

    // show the portion of received characters:
    printf("%s");
  }
  printf("\"\n");

  return 0;
}

static int show_usage(char* progname) {
  int ret = 0;

  printf(
      "Usage: %s [options] <listening port number>\n"
      "Options:\n"
      "--hostname <hostname>: the hostname to use, defualts to \"localhost\"\n"
      "--message <message>: the message to send to the server\n",
      progname);

out:
  return ret;
}

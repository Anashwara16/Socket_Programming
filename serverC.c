
#include "common.h"

typedef struct CredentialT
{
    char username[MAX_USERNAME_LEN];
    char password[MAX_PASSWORD_LEN];
} Credential;

Credential credential[MAX_FILE_LINES] = {0};
int totalLines = 0;

// read and parse credentials file. 
void readFile()
{
    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t bytes;

    fp = fopen(CRED_FILE, "r");
    if (fp == NULL)
    {
        printf("serverC : Couldn't open file : %s", CRED_FILE);
        exit(EXIT_FAILURE);
    }
    int i = 0;
    while ((bytes = getline(&line, &len, fp)) != -1)
    {

        strncpy(credential[i].username, line, MAX_USERNAME_LEN);
        // Reference for usage of strcspn - 
        // https://stackoverflow.com/questions/2693776/removing-trailing-newline-character-from-fgets-input
        size_t delimiterPos = strcspn(line, COMMA);
        credential[i].username[delimiterPos] = 0;
        strncpy(credential[i].password, line + delimiterPos + 1, MAX_PASSWORD_LEN);
        credential[i].password[strcspn(credential[i].password, "\n\r")] = 0;
        debug("username : %s , password : %s ", credential[i].username, credential[i].password);
        i++;
    }
    totalLines = i;
    if (line)
        free(line);

    fclose(fp);
}

// validate the user credentials 
void validateCredentials(char *username, char *password, char *result)
{
    int i = 0;
    int usernameMatch = 0;
    if (!strcmp("", username))
    {
        strcpy(result, MSG_TYPE_NOAUTH_USER);
        return;

    } 
    if(!strcmp("", password))
    {
        strcpy(result, MSG_TYPE_NOAUTH_PASS);
        return;
    }

    for (i = 0; i < MAX_FILE_LINES; i++)
    {
        // debug(" map : %s : %s ", credential[i].username, credential[i].password);
        if (!strcmp(credential[i].username, username) &&
            !strcmp(credential[i].password, password))
        {
            strcpy(result, MSG_TYPE_AUTH);
            return;
        }
        if (!strcmp(credential[i].username, username))
        {
            usernameMatch++;
        }
    }
    strcpy(result, usernameMatch ? MSG_TYPE_NOAUTH_PASS : MSG_TYPE_NOAUTH_USER);
    return;
}

// verify credentials 
void verifyCredentials(int sockfd, char *buf)
{
    char username[MAX_USERNAME_LEN + 1] = {0};
    char password[MAX_PASSWORD_LEN + 1] = {0};
    strcpy(username, buf);
    size_t delimiterPos = strcspn(buf, DELIMITER);
    username[delimiterPos] = 0;
    strcpy(password, buf + delimiterPos + 1);

    // Beej’s guide to network programming, 6.3
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;
    char serverMPort[10];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // set to AF_INET to use IPv4
    hints.ai_socktype = SOCK_DGRAM;

    sprintf(serverMPort, "%d", SERVERM_UDP_PORT);

    /* convert a text string representation of an IP address to an addrinfo structure 
    that contains a sockaddr structure for the IP address and other information. */
    if ((rv = getaddrinfo(localhost, serverMPort, &hints, &servinfo)) != 0)
    {
        perror("serverC: getaddrinfo");
        return;
    }

    // loop through all the results and make a socket
    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        if (p != NULL)
        {
            break;
        }
    }

    // check if the socket was bound successfully or not
    if (p == NULL)
    {
        printf("serverC: failed to create socket\n");
        return;
    }
    char msg[MAX_CREDENTIAL_LEN] = {0};
    validateCredentials(username, password, msg);
    if ((numbytes = sendto(sockfd, msg, strlen(msg), 0,
                           p->ai_addr, p->ai_addrlen)) == -1)
    {
        perror("serverC: sendto");
        close(sockfd);
        exit(1);
    }
    debug("Sent %s : %d bytes", msg, numbytes);
    printf(SERVERC_CRED_SENT);
}

// receive credentials from socket
void receiveCredentials()
{
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;
    struct sockaddr_storage their_addr;
    char buf[MAX_CREDENTIAL_LEN];
    socklen_t addr_len;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // set to AF_INET to use IPv4
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE; // use my IP
    char serverCPort[10];
    sprintf(serverCPort, "%d", SERVERC_PORT);

    /* convert a text string representation of an IP address to an addrinfo structure 
    that contains a sockaddr structure for the IP address and other information. */
    if ((rv = getaddrinfo(localhost, serverCPort, &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return;
    }

    // loop through all the results and bind to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        // make a socket using the information gleaned from getaddrinfo():
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1)
        {
            perror("serverC: socket");
            continue;
        }

        // bind the socket to the port we passed in to getaddrinfo():
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(sockfd);
            perror("serverC: Failed to bind");
            continue;
        }

        break;
    }

    // check if the socket was bound successfully or not
    if (p == NULL)
    {
        printf("serverC: failed to bind socket\n");
        return;
    }

    freeaddrinfo(servinfo); // all done with this structure
    struct sockaddr_in sin;
    socklen_t len = sizeof(sin);
    int portNo = 0;
    
    //Retrieve the locally-bound name of the specified socket and store it in the sockaddr structure
    if (getsockname(sockfd, (struct sockaddr *)&sin, &len) == -1)
    {
        perror("client: getsockname");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    else
    {
        portNo = ntohs(sin.sin_port);
        debug("port number %d\n", portNo);
        if (portNo != SERVERC_PORT)
        {
            assert(0);
        }
    }
    printf(SERVERC_BOOT_MSG, portNo);

    addr_len = sizeof their_addr;
    while (1)
    {   
        // receive data from socket
        if ((numbytes = recvfrom(sockfd, buf, MAX_CREDENTIAL_LEN - 1, 0,
                                 (struct sockaddr *)&their_addr, &addr_len)) == -1)
        {
            perror("serverC: recvfrom");
            close(sockfd);
            exit(1);
        }
        buf[numbytes] = '\0';
        debug("serverC: message received : \"%s\"", buf);
        printf(SERVERC_CRED_RECV);
        char messageType[MSG_TYPE_LEN] = {0};
        strncpy(messageType, buf, MSG_TYPE_LEN);
        size_t delimiterPos = strcspn(buf, DELIMITER);
        messageType[delimiterPos] = 0;
        if (!strcmp(messageType, MSG_TYPE_CRED))
        {
            verifyCredentials(sockfd, buf + delimiterPos + 1);
        }
        else
        {
            debug("serverC: invalid message received : \"%s\"", messageType);
            continue;
        }
    }
    close(sockfd);
}

int main()
{
    readFile();
    receiveCredentials();
    return 0;
}
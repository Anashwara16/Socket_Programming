
#include "common.h"

int tcpSockFd = 0;
int udpSockFd = 0;

/*
Caesar cipher - a monoalphabetic substitution cipher, where each character/digit is replaced by another 
character/digit; in this case by an offset of 4.
*/ 
void caesarCipher(char *inputString, char *outputString)
{
    int i = 0;
    for (i = 0; i < strlen(inputString); i++)
    {
        if (inputString[i] >= '0' && inputString[i] <= '5')
        {
            outputString[i] = inputString[i] + 4;
        }
        else if (inputString[i] >= '6' && inputString[i] <= '9')
        {
            outputString[i] = '0' + (inputString[i] - '6');
        }
        else if (inputString[i] >= 'a' && inputString[i] <= 'v')
        {
            outputString[i] = inputString[i] + 4;
        }
        else if (inputString[i] >= 'v' && inputString[i] <= 'z')
        {
            outputString[i] = 'a' + (inputString[i] - 'w');
        }
        else if (inputString[i] >= 'A' && inputString[i] <= 'V')
        {
            outputString[i] = inputString[i] + 4;
        }
        else if (inputString[i] >= 'V' && inputString[i] <= 'Z')
        {
            outputString[i] = 'A' + (inputString[i] - 'W');
        }
        else
        {
            outputString[i] = inputString[i];
        }
    }
}

// Associate socket with a port 
void bindTCPSocket()
{
    // Beej’s guide to network programming, 5.6
    struct addrinfo hints, *servinfo, *p;
    int yes = 1;

    // first, load up address structs with getaddrinfo():
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;       // AF_INET, AF_INET6, or AF_UNSPEC
    hints.ai_socktype = SOCK_STREAM; // SOCK_STREAM or SOCK_DGRAM
    hints.ai_flags = AI_PASSIVE;     // fill in my IP for me

    char serverPort[10];
    sprintf(serverPort, "%d", SERVERM_TCP_PORT);

    /* convert a text string representation of an IP address to an addrinfo structure 
    that contains a sockaddr structure for the IP address and other information. */
    int ret = getaddrinfo(localhost, serverPort, &hints, &servinfo);
    if (ret != 0)
    {
        perror("getaddrinfo 4");
        exit(EXIT_FAILURE);
    }

    // loop through all the results and bind to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        // make a socket using the information gleaned from getaddrinfo():
        tcpSockFd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
        if (tcpSockFd == -1)
        {
            perror("serverM: TCP socket");
            continue;
        }
        // set the current value for a socket option associated with a socket of any type, in any state
        if (setsockopt(tcpSockFd, SOL_SOCKET, SO_REUSEADDR, &yes,
                       sizeof(int)) == -1)
        {
            perror("serverM: TCP setsockopt");
            close(tcpSockFd);
            exit(EXIT_FAILURE);
        }

        // bind the socket to the port we passed in to getaddrinfo():
        if (bind(tcpSockFd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(tcpSockFd);
            perror("serverM: Failed to bind");
            continue;
        }
        break;
    }
    // check if the socket was bound successfully or not
    if (p == NULL)
    {
        debug("serverM: Failed to bind to TCP socket\n");
        close(tcpSockFd);
        exit(EXIT_FAILURE);
    }

    /* mark the socket as a passive socket, which will be used to accept incoming
       connection requests using accept */
    if (listen(tcpSockFd, BACKLOG) == -1)
    {
        perror("serverM: TCP listen");
        close(tcpSockFd);
        exit(EXIT_FAILURE);
    }
}

// Associate socket with a port 
void bindUDPSocket()
{
    // Beej’s guide to network programming, 6.3
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char serverMPort[10];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;      // set to AF_INET to use IPv4
    hints.ai_socktype = SOCK_DGRAM; // UDP stream sockets
    hints.ai_flags = AI_PASSIVE;    // fill in my IP for me

    sprintf(serverMPort, "%d", SERVERM_UDP_PORT);

    /* convert a text string representation of an IP address to an addrinfo structure 
    that contains a sockaddr structure for the IP address and other information. */
    if ((rv = getaddrinfo(localhost, serverMPort, &hints, &servinfo)) != 0)
    {
        char errorStr[50] = {0};
        sprintf(errorStr, "serverM 5: UDP getaddrinfo %d", rv);
        perror(errorStr);
        exit(EXIT_FAILURE);
        return;
    }

    // loop through all the results and make a socket
    for (p = servinfo; p != NULL; p = p->ai_next)
    {   
        // make a socket using the information gleaned from getaddrinfo():
        if ((udpSockFd = socket(p->ai_family, p->ai_socktype,
                                p->ai_protocol)) == -1)
        {
            perror("serverM: UDP socket");
            continue;
        }

        // bind the socket to the port we passed in to getaddrinfo():
        if (bind(udpSockFd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(udpSockFd);
            perror("serverM: Failed to bind");
            continue;
        }
        break;
    }
    
    // check if the socket was bound successfully or not
    if (p == NULL)
    {
        perror("serverM: failed to create UDP socket\n");
        close(udpSockFd);
        exit(EXIT_FAILURE);
        return;
    }
}

// Get sockaddr, IPv4 or IPv6:
void *getInAddr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

// send 
void sendToCS(char *csRequest, char *csResponse)
{
    // Beej’s guide to network programming, 6.3

    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;

    char serverCSPort[10] = {0};
    sprintf(serverCSPort, "%d", SERVERCS_PORT);

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;      // set to AF_INET to use IPv4
    hints.ai_socktype = SOCK_DGRAM; // UDP stream sockets
    hints.ai_flags = AI_PASSIVE;    // fill in my IP for me

    /* convert a text string representation of an IP address to an addrinfo structure 
    that contains a sockaddr structure for the IP address and other information. */
    if ((rv = getaddrinfo(localhost, serverCSPort, &hints, &servinfo)) != 0)
    {
        perror("serverM: getaddrinfo 1");
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
    if (p == NULL)
    {
        perror("serverM: failed to find sender socket\n");
        return;
    }
    if ((numbytes = sendto(udpSockFd, csRequest, strlen(csRequest), 0,
                           p->ai_addr, p->ai_addrlen)) == -1)
    {
        perror("serverM: sendto");
        close(udpSockFd);
        exit(EXIT_FAILURE);
    }
    debug("Sent to ServerCS %s : %d bytes", csRequest, numbytes);
    printf(SERVERM_SENT_DEPT_REQ, "CS");
    struct sockaddr_storage their_addr;
    socklen_t addr_len;
    if ((numbytes = recvfrom(udpSockFd, csResponse, MAX_COURSE_RES_LEN, 0,
                             (struct sockaddr *)&their_addr, &addr_len)) == -1)
    {
        perror("recvfrom");
        close(udpSockFd);
        exit(EXIT_FAILURE);
    }
    csResponse[numbytes] = '\0';
    debug("serverM: message received : \"%s\"", csResponse);
    printf(SERVERM_RECV_DEPT_RES, "CS", SERVERM_UDP_PORT);
    freeaddrinfo(servinfo); // all done with this structure
}

void sendToEE(char *eeRequest, char *eeResponse)
{
    // Beej’s guide to network programming, 6.3

    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;

    char serverEEPort[10] = {0};
    sprintf(serverEEPort, "%d", SERVEREE_PORT);

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;      // set to AF_INET to use IPv4
    hints.ai_socktype = SOCK_DGRAM; // UDP stream sockets
    hints.ai_flags = AI_PASSIVE;    // fill in my IP for me
    
    /* convert a text string representation of an IP address to an addrinfo structure 
    that contains a sockaddr structure for the IP address and other information. */
    if ((rv = getaddrinfo(localhost, serverEEPort, &hints, &servinfo)) != 0)
    {
        perror("serverM: getaddrinfo 2");
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
    if (p == NULL)
    {
        perror("serverM: failed to find sender socket\n");
        return;
    }
    if ((numbytes = sendto(udpSockFd, eeRequest, strlen(eeRequest), 0,
                           p->ai_addr, p->ai_addrlen)) == -1)
    {
        perror("serverM: sendto");
        close(udpSockFd);
        exit(EXIT_FAILURE);
    }
    debug("Sent to ServerEE %s : %d bytes", eeRequest, numbytes);
    printf(SERVERM_SENT_DEPT_REQ, "EE");
    struct sockaddr_storage their_addr;
    socklen_t addr_len;
    if ((numbytes = recvfrom(udpSockFd, eeResponse, MAX_COURSE_RES_LEN, 0,
                             (struct sockaddr *)&their_addr, &addr_len)) == -1)
    {
        perror("recvfrom");
        close(udpSockFd);
        exit(EXIT_FAILURE);
    }
    eeResponse[numbytes] = '\0';
    debug("serverM: message received : \"%s\"", eeResponse);
    printf(SERVERM_RECV_DEPT_RES, "EE", SERVERM_UDP_PORT);
    freeaddrinfo(servinfo); // all done with this structure
}

void sendCredentials(char *authRequest, char *authResponse)
{
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;
    char username[MAX_USERNAME_LEN + 1] = {0};
    char password[MAX_PASSWORD_LEN + 1] = {0};
    char encryptedUsername[MAX_USERNAME_LEN + 1] = {0};
    char encryptedPassword[MAX_PASSWORD_LEN + 1] = {0};
    strcpy(username, authRequest);

    // Reference for usage of strcspn - 
    // https://stackoverflow.com/questions/2693776/removing-trailing-newline-character-from-fgets-input
    size_t delimiterPos = strcspn(authRequest, DELIMITER);
    username[delimiterPos] = 0;
    strcpy(password, authRequest + delimiterPos + 1);
    debug("serverM: username '%s'", username);
    debug("serverM: password '%s'", password);
    caesarCipher(username, encryptedUsername);
    caesarCipher(password, encryptedPassword);
    debug("serverM: encrypted username '%s'", encryptedUsername);
    debug("serverM: encrypted password '%s'", encryptedPassword);
    char msg[MAX_CREDENTIAL_LEN] = {0};
    strcat(msg, MSG_TYPE_CRED);
    strcat(msg, DELIMITER);
    strcat(msg, encryptedUsername);
    strcat(msg, DELIMITER);
    strcat(msg, encryptedPassword);

    char serverCPort[10];
    sprintf(serverCPort, "%d", SERVERC_PORT);

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;      // don't care IPv4 or IPv6
    hints.ai_socktype = SOCK_DGRAM; // UDP stream sockets
    hints.ai_flags = AI_PASSIVE;    // fill in my IP for me

    /* convert a text string representation of an IP address to an addrinfo structure 
    that contains a sockaddr structure for the IP address and other information. */
    if ((rv = getaddrinfo(localhost, serverCPort, &hints, &servinfo)) != 0)
    {
        printf("%s\n", serverCPort);
        printf("serverM: sendCredentials getaddrinfo %d %s \n", rv, gai_strerror(rv));
        exit(EXIT_FAILURE);
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
    if (p == NULL)
    {
        perror("serverM: failed to find sender socket\n");
        return;
    }
    if ((numbytes = sendto(udpSockFd, msg, strlen(msg), 0,
                           p->ai_addr, p->ai_addrlen)) == -1)
    {
        perror("serverM: sendto serverC");
        close(udpSockFd);
        exit(EXIT_FAILURE);
    }
    debug("Sent to ServerC %s : %d bytes", msg, numbytes);
    printf(SERVERM_SENT_AUTH_REQ);
    struct sockaddr_storage their_addr;
    socklen_t addr_len;
    if ((numbytes = recvfrom(udpSockFd, authResponse, MAX_CREDENTIAL_LEN, 0,
                             (struct sockaddr *)&their_addr, &addr_len)) == -1)
    {
        perror("recvfrom");
        close(udpSockFd);
        exit(EXIT_FAILURE);
    }
    authResponse[numbytes] = '\0';
    debug("serverM: message received : \"%s\"", authResponse);
    printf(SERVERM_RECV_AUTH_RES, SERVERM_UDP_PORT);
    freeaddrinfo(servinfo); // all done with this structure
}

// Communicate with the client
void communicateWithClient()
{
    // Beej’s guide to network programming, 5.6, 6.1, 6.2
    int numBytes;
    struct sockaddr_storage their_addr;
    socklen_t addr_size;
    char clientMsg[MAX_COURSE_LEN] = {0};
    char authResponse[MAX_CREDENTIAL_LEN] = {0};
    char messageType[MSG_TYPE_LEN] = {0};
    char username[MAX_USERNAME_LEN] = {0};
    char courseCode[MAX_COURSE_LEN] = {0};
    char category[MAX_CATEGORY_LEN] = {0};
    char curCourseCode[MAX_COURSE_LEN] = {0};
    char eeRequest[MAX_COURSE_LEN] = {0};
    char csRequest[MAX_COURSE_LEN] = {0};
    char eeResponse[MAX_COURSE_LEN] = {0};
    char csResponse[MAX_COURSE_LEN] = {0};
    char eeMultRequest[MAX_COURSE_LEN] = {0};
    char csMultRequest[MAX_COURSE_LEN] = {0};
    char eeMultResponse[MAX_COURSE_RES_LEN] = {0};
    char csMultResponse[MAX_COURSE_RES_LEN] = {0};
    char eecsMultResponse[MAX_COURSE_RES_LEN] = {0};
    size_t delimiterPos = 0;
    size_t resDelimiterPos = 0;
    while (1)
    {
        addr_size = sizeof their_addr;

        debug("Waiting at accept");

        /* extract the first connection request on the queue of pending connections 
        for the listening socket, create a new socket descriptor with the same properties 
        as socket and returns a new file descriptor referring to that socket */
        int newfd = accept(tcpSockFd, (struct sockaddr *)&their_addr, &addr_size);
        if (newfd == -1)
        {
            perror("serverM: accept");
            return;
        }
        char s[INET6_ADDRSTRLEN];
        
        // convert a numeric network address (their_addr) into a text string in the buffer s.
        inet_ntop(their_addr.ss_family, getInAddr((struct sockaddr *)&their_addr), s, sizeof s);
        debug("serverM: got connection from %s", s);
        while (1)
        {
            memzero(clientMsg, MAX_COURSE_LEN);
            memzero(authResponse, MAX_CREDENTIAL_LEN);
            memzero(messageType, MSG_TYPE_LEN);
            memzero(username, MAX_USERNAME_LEN);
            memzero(courseCode, MAX_COURSE_LEN);
            memzero(category, MAX_CATEGORY_LEN);
            memzero(eeRequest, MAX_COURSE_LEN);
            memzero(csRequest, MAX_COURSE_LEN);
            memzero(eeResponse, MAX_COURSE_LEN);
            memzero(csResponse, MAX_COURSE_LEN);
            memzero(eeMultRequest, MAX_COURSE_LEN);
            memzero(csMultRequest, MAX_COURSE_LEN);
            memzero(eeMultResponse, MAX_COURSE_RES_LEN);
            memzero(csMultResponse, MAX_COURSE_RES_LEN);
            memzero(eecsMultResponse, MAX_COURSE_RES_LEN);
            delimiterPos = 0;
            resDelimiterPos = 0;

            if ((numBytes = recv(newfd, (void *)clientMsg, MAX_CREDENTIAL_LEN - 1, 0)) == -1)
            {
                perror("serverM: recv");
                continue;
            }
            if (numBytes == 0)
            {
                // Client has closed connection, break out from loop and wait for next client
                break;
            }
            clientMsg[numBytes] = '\0';
            debug("serverM: received '%s'", clientMsg);

            strncpy(messageType, clientMsg, MSG_TYPE_LEN);
            delimiterPos = strcspn(clientMsg, DELIMITER);
            resDelimiterPos += delimiterPos + 1;
            messageType[delimiterPos] = 0;
            debug("delimiterPos : %d , resDelimiterPos : %d ", delimiterPos, resDelimiterPos);
            debug("messageType : %s ", messageType);
            strncpy(username, clientMsg + resDelimiterPos, MAX_USERNAME_LEN);
            delimiterPos = strcspn(clientMsg + resDelimiterPos, DELIMITER);
            resDelimiterPos += delimiterPos + 1;
            int courseDelimterPos = resDelimiterPos;
            username[delimiterPos] = 0;
            debug("Username : %s ", username);
            if (!strcmp(messageType, MSG_TYPE_CRED))
            {
                printf(SERVERM_RECEIVED_CREDENIALS, username, SERVERM_TCP_PORT);
                sendCredentials(clientMsg + resDelimiterPos - delimiterPos - 1, authResponse);
                int bytesSent = send(newfd, authResponse, strlen(authResponse), 0);
                debug("Sent to client %s : %d bytes", authResponse, bytesSent);
                printf(SERVERM_SENT_AUTH_RES);
            }
            if (!strcmp(messageType, MSG_TYPE_COURSE_REQ))
            {
                strncpy(courseCode, clientMsg + resDelimiterPos, MAX_COURSE_LEN);
                delimiterPos = strcspn(clientMsg + resDelimiterPos, DELIMITER);
                courseCode[delimiterPos] = 0;
                resDelimiterPos += delimiterPos + 1;
                debug("CourseCode : %s ", courseCode);
                strncpy(category, clientMsg + resDelimiterPos, MAX_CATEGORY_LEN);
                delimiterPos = strcspn(clientMsg + resDelimiterPos, DELIMITER);
                category[delimiterPos] = 0;
                debug("Category : %s ", category);
                printf(SERVERM_RECV_COURSE_REQ, username, courseCode, category, SERVERM_TCP_PORT);
                if (strstr(courseCode, EE_DEPT))
                {
                    strcat(eeRequest, MSG_TYPE_COURSE_REQ);
                    strcat(eeRequest, DELIMITER);
                    strcat(eeRequest, clientMsg + courseDelimterPos);
                    sendToEE(eeRequest, eeResponse);
                    int bytesSent = send(newfd, eeResponse, strlen(eeResponse), 0);
                    debug("Sent to client %s : %d bytes", eeResponse, bytesSent);

                    printf(SERVERM_SENT_COURSE_RES);
                }
                else if (strstr(courseCode, CS_DEPT))
                {
                    strcat(csRequest, MSG_TYPE_COURSE_REQ);
                    strcat(csRequest, DELIMITER);
                    strcat(csRequest, clientMsg + courseDelimterPos);
                    sendToCS(csRequest, csResponse);
                    int bytesSent = send(newfd, csResponse, strlen(csResponse), 0);
                    debug("Sent to client %s : %d bytes", csResponse, bytesSent);
                    printf(SERVERM_SENT_COURSE_RES);
                }
                else
                {
                    strcat(eeResponse, MSG_TYPE_COURSE_RES);
                    strcat(eeResponse, DELIMITER);
                    strcat(eeResponse, courseCode);
                    strcat(eeResponse, DELIMITER);
                    strcat(eeResponse, DELIMITER);
                    strcat(eeResponse, COURSE_NOT_FOUND);
                    strcat(eeResponse, DELIMITER);
                    int bytesSent = send(newfd, eeResponse, strlen(eeResponse), 0);
                    debug("Sent to client %s : %d bytes", eeResponse, bytesSent);
                }
            }
            if (!strcmp(messageType, MSG_TYPE_MULT_COURSE_REQ))
            {
                strncpy(courseCode, clientMsg + resDelimiterPos, MAX_COURSE_LEN);
                delimiterPos = strcspn(clientMsg + resDelimiterPos, DELIMITER);
                resDelimiterPos += delimiterPos + 1;
                courseCode[delimiterPos] = 0;
                debug("CourseCodes : %s ", courseCode);
                resDelimiterPos = 0;
                strcat(eeMultRequest, MSG_TYPE_MULT_COURSE_REQ);
                strcat(eeMultRequest, DELIMITER);
                strcat(csMultRequest, MSG_TYPE_MULT_COURSE_REQ);
                strcat(csMultRequest, DELIMITER);
                strcat(eecsMultResponse, MSG_TYPE_MULT_COURSE_RES);
                strcat(eecsMultResponse, DELIMITER);
                while (strlen(courseCode) >= resDelimiterPos)
                {
                    memzero(curCourseCode, MAX_COURSE_LEN);
                    delimiterPos = strcspn(courseCode + resDelimiterPos, " ");
                    strncpy(curCourseCode, courseCode + resDelimiterPos, delimiterPos);
                    curCourseCode[delimiterPos] = '\0';
                    if (delimiterPos == 0)
                    {
                        break;
                    }
                    if (strstr(curCourseCode, EE_DEPT))
                    {
                        strcat(eeMultRequest, curCourseCode);
                        strcat(eeMultRequest, " ");
                        resDelimiterPos += delimiterPos + 1;
                    }
                    else if (strstr(curCourseCode, CS_DEPT))
                    {
                        strcat(csMultRequest, curCourseCode);
                        strcat(csMultRequest, " ");
                        resDelimiterPos += delimiterPos + 1;
                    }
                    else
                    {
                        strcat(eecsMultResponse, MSG_TYPE_MULT_COURSE_RES);
                        strcat(eecsMultResponse, DELIMITER);
                        strcat(eecsMultResponse, curCourseCode);
                        strcat(eecsMultResponse, ":");
                        strcat(eecsMultResponse, DELIMITER);
                        strcat(eecsMultResponse, COURSE_NOT_FOUND);
                        strcat(eecsMultResponse, DELIMITER);
                        strcat(eecsMultResponse, "\n");
                        resDelimiterPos += delimiterPos + 1;
                    }
                }
                strcat(eeMultRequest, DELIMITER);
                strcat(csMultRequest, DELIMITER);
                debug("EE Request : %s", eeMultRequest);
                debug("CS Request : %s", csMultRequest);
                if (strlen(eeMultRequest) > strlen(MSG_TYPE_MULT_COURSE_REQ) + 2)
                {
                    sendToEE(eeMultRequest, eeMultResponse);
                    strcat(eecsMultResponse, eeMultResponse);
                }
                if (strlen(csMultRequest) > strlen(MSG_TYPE_MULT_COURSE_REQ) + 2)
                {
                    debug("csMultRequest %d", strlen(csMultRequest));
                    debug("MSG_TYPE_MULT_COURSE_REQ %d", strlen(MSG_TYPE_MULT_COURSE_REQ) + 2);
                    sendToCS(csMultRequest, csMultResponse);
                    strcat(eecsMultResponse, csMultResponse);
                }
                int bytesSent = send(newfd, eecsMultResponse, strlen(eecsMultResponse), 0);
                debug("Sent to client %s : %d bytes", eecsMultResponse, bytesSent);
            }
        }
    }
}

int main()
{
    bindTCPSocket();
    bindUDPSocket();
    printf("%s\n", SERVERM_BOOT_MSG);
    communicateWithClient();
    close(udpSockFd); 
    close(tcpSockFd);
    return 0;
}
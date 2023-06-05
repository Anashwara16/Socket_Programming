

#include "common.h"

char username[MAX_USERNAME_LEN + 1];
char password[MAX_PASSWORD_LEN + 1];
char course[MAX_COURSE_LEN + 1];
char category[MAX_CATEGORY_LEN + 1];
int sockfd = 0;
int portNo = 0;

void getCredentials()
{
    memzero(username, MAX_USERNAME_LEN);
    memzero(password, MAX_PASSWORD_LEN);
    printf("%s ", CLIENT_ENTER_USERNAME);
    fgets(username, MAX_USERNAME_LEN, stdin);
    printf("%s ", CLIENT_ENTER_PASSWORD);
    fgets(password, MAX_PASSWORD_LEN, stdin);

    // Reference for usage of strcspn - 
    // https://stackoverflow.com/questions/2693776/removing-trailing-newline-character-from-fgets-input
    username[strcspn(username, "\n")] = 0;
    password[strcspn(password, "\n")] = 0;
}

// connect to the main server
void connectToServerM()
{
    // Beej’s guide to network programming, 5.4
    struct addrinfo hints, *res;

    // first, load up address structs with getaddrinfo():
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;       // AF_INET, AF_INET6, or AF_UNSPEC
    hints.ai_socktype = SOCK_STREAM; // SOCK_STREAM or SOCK_DGRAM

    char serverPort[10];
    sprintf(serverPort, "%d", SERVERM_TCP_PORT);

    /* convert a text string representation of an IP address to an addrinfo structure 
    that contains a sockaddr structure for the IP address and other information. */
    int ret = getaddrinfo(localhost, serverPort, &hints, &res);

    if (ret != 0)
    {
        perror("client: getaddrinfo");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // make a socket using the information gleaned from getaddrinfo():
    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd == -1)
    {
        perror("client: socket");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // connect!
    ret = connect(sockfd, res->ai_addr, res->ai_addrlen);
    if (ret == -1)
    {
        perror("connect");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in sin;
    socklen_t len = sizeof(sin);

    //Retrieve the locally-bound name of the specified socket and store it in the sockaddr structure
    if (getsockname(sockfd, (struct sockaddr *)&sin, &len) == -1)
    {
        perror("getsockname");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    else
    {
        portNo = ntohs(sin.sin_port);
        debug("port number %d\n", portNo);
    }
}

void getCourseDetails(char *courseType)
{
    char rawCourse[MAX_COURSE_LEN] = {0};
    char rawCategory[MAX_CATEGORY_LEN] = {0};
    char courseCode1[MAX_COURSE_LEN] = {0};
    char courseCode2[MAX_COURSE_LEN] = {0};
    int courseCode1Pos = 0;
    int courseCode2Pos = 0;

    memzero(rawCourse, MAX_COURSE_LEN);
    memzero(rawCategory, MAX_CATEGORY_LEN);
    memzero(course, MAX_COURSE_LEN);
    memzero(category, MAX_CATEGORY_LEN);

    printf("%s ", CLIENT_ENTER_COURSE);
    fgets(rawCourse, MAX_COURSE_LEN, stdin);
    // Remove trailing \n
    rawCourse[strcspn(rawCourse, "\n")] = 0;
    removeSpace(rawCourse, course);
    debug("course : %s", course);
    strcpy(courseCode1, course);
    courseCode1Pos = strcspn(course, " \n");
    courseCode1[courseCode1Pos] = 0;
    strcpy(courseCode2, course + courseCode1Pos + 1);
    courseCode2Pos = strcspn(course + courseCode1Pos + 1, " \n");
    courseCode2[courseCode2Pos] = 0;
    debug("course1 : %s, course2 : %s ", courseCode1, courseCode2);
    if (courseCode2Pos != 0)
    {
        strcpy(courseType, MSG_TYPE_MULT_COURSE_REQ);
        return;
    }
    strcpy(courseType, MSG_TYPE_COURSE_REQ);
    printf("%s ", CLIENT_ENTER_CATEGORY);
    fgets(rawCategory, MAX_CATEGORY_LEN, stdin);
    // Remove trailing \n
    rawCategory[strcspn(rawCategory, "\n")] = 0;
    removeSpace(rawCategory, category);
}

void parseMultiResponse(char *request, char *response, char *result)
{
    debug("%s", request);
    debug("%s", response);
    int reqDelimiterPos = 0;
    int resDelimiterPos = 0;
    int lineDelimiterPos = 0;
    int delimiterPos = 0;
    int courseFound = FALSE;
    char reqCourseCode[MAX_COURSE_LEN] = {0};
    char resCourseCode[MAX_COURSE_LEN] = {0};
    char resLine[MAX_COURSE_LEN] = {0};
    char courseNotFoundStr[MAX_COURSE_LEN] = {0};

    char courseSetStr[MAX_COURSE_LEN] = {0};

    while (strlen(request) >= reqDelimiterPos)
    {
        memzero(reqCourseCode, MAX_COURSE_LEN);
        delimiterPos = strcspn(request + reqDelimiterPos, " ");
        strncpy(reqCourseCode, request + reqDelimiterPos, delimiterPos);
        reqCourseCode[delimiterPos] = '\0';

        reqDelimiterPos += delimiterPos + 1;
        
        if (delimiterPos == 0)
        {
            break;
        }

        if (strstr(courseSetStr, reqCourseCode))
        {
            continue;
        }

        strcat(courseSetStr, reqCourseCode);

        lineDelimiterPos = 0;
        courseFound = FALSE;
        while (strlen(response) >= lineDelimiterPos)
        {
            resDelimiterPos = 0;
            memzero(resCourseCode, MAX_COURSE_LEN);
            memzero(resLine, MAX_COURSE_LEN);
            delimiterPos = strcspn(response + lineDelimiterPos, "\n");
            strncpy(resLine, response + lineDelimiterPos, delimiterPos);
            resLine[delimiterPos] = '\0';
            lineDelimiterPos += delimiterPos + 1;
            delimiterPos = strcspn(resLine + resDelimiterPos, ":");
            strncpy(resCourseCode, resLine + resDelimiterPos, delimiterPos);
            resCourseCode[delimiterPos] = '\0';
            resDelimiterPos += delimiterPos + 1;
            if (delimiterPos == 0)
            {
                break;
            }

            if (!strcmp(reqCourseCode, resCourseCode))
            {
                courseFound = TRUE;
                if (strstr(resLine, COURSE_NOT_FOUND))
                {
                    memzero(courseNotFoundStr, MAX_COURSE_LEN);
                    sprintf(courseNotFoundStr, CLIENT_RECV_COURSE_NOTFOUND, reqCourseCode);
                    strcat(result, courseNotFoundStr);
                    continue;
                }
                else
                {
                    strcat(result, resLine);
                    strcat(result, "\n");
                }
                break;
            }
        }
        if (courseFound == FALSE)
        {
            memzero(courseNotFoundStr, MAX_COURSE_LEN);
            sprintf(courseNotFoundStr, CLIENT_RECV_COURSE_NOTFOUND, reqCourseCode);
            strcat(result, courseNotFoundStr);
            continue;
        }
    }
    category[strcspn(result, "\n")] = 0;
}

// get all the course details as per the course request
void sendCourseDetails()
{
    char courseReq[MAX_COURSE_LEN] = {0};
    char courseRes[MAX_COURSE_RES_LEN] = {0};
    char messageType[MSG_TYPE_LEN] = {0};
    char courseCodeRes[MAX_COURSE_LEN] = {0};
    char categoryRes[MAX_COURSE_LEN] = {0};
    char result[MAX_COURSE_RES_LEN] = {0};
    char courseType[MSG_TYPE_LEN] = {0};

    int numBytes;
    while (1)
    {

        size_t resDelimiterPos = 0;
        memzero(courseReq, MAX_COURSE_LEN);
        memzero(courseRes, MAX_COURSE_RES_LEN);
        memzero(messageType, MSG_TYPE_LEN);
        memzero(courseType, MSG_TYPE_LEN);
        getCourseDetails(courseType);

        course[strcspn(course, "\n")] = 0;
        category[strcspn(category, "\n")] = 0;
        strcat(courseReq, courseType);
        strcat(courseReq, DELIMITER);
        strcat(courseReq, username);
        strcat(courseReq, DELIMITER);
        strcat(courseReq, course);
        strcat(courseReq, DELIMITER);
        strcat(courseReq, category);
        debug(courseReq);
        if (send(sockfd, courseReq, strlen(courseReq), 0) == -1)
        {
            perror("send");
        }
        debug("Sent %s", courseReq);
        if (!strcmp(courseType, MSG_TYPE_COURSE_REQ))
        {
            printf(CLIENT_SEND_COURSE_REQ, username);
        }
        else
        {
            printf(CLIENT_SEND_MULT_COURSE, username);
        }
        if ((numBytes = recv(sockfd, courseRes, MAX_COURSE_RES_LEN, 0)) == -1)
        {
            perror("recv");
        }
        debug("Received %s", courseRes);
        printf(CLIENT_RECV_COURSE_RES, portNo);
        strncpy(messageType, courseRes, MSG_TYPE_LEN);
        size_t delimiterPos = strcspn(courseRes, DELIMITER);
        resDelimiterPos += delimiterPos + 1;
        messageType[delimiterPos] = 0;
        debug("msgType : %s", messageType);
        if (!strcmp(messageType, MSG_TYPE_COURSE_RES))
        {
            memzero(courseCodeRes, MAX_COURSE_LEN);
            memzero(categoryRes, MAX_COURSE_LEN);
            memzero(result, MAX_COURSE_RES_LEN);
            strncpy(courseCodeRes, courseRes + resDelimiterPos, MAX_COURSE_LEN);
            delimiterPos = strcspn(courseRes + resDelimiterPos, DELIMITER);
            resDelimiterPos += delimiterPos + 1;
            courseCodeRes[delimiterPos] = 0;
            debug("CourseCodeRes : %s ", courseCodeRes);
            strncpy(categoryRes, courseRes + resDelimiterPos, MAX_CREDITS_LEN);
            delimiterPos = strcspn(courseRes + resDelimiterPos, DELIMITER);
            resDelimiterPos += delimiterPos + 1;
            categoryRes[delimiterPos] = 0;
            debug("Category : %s ", categoryRes);
            strncpy(result, courseRes + resDelimiterPos, MAX_COURSENAME_LEN);
            delimiterPos = strcspn(courseRes + resDelimiterPos, DELIMITER);
            result[delimiterPos] = 0;
            debug("Result : %s ", result);
            if (strstr(result, COURSE_NOT_FOUND))
            {
                printf(CLIENT_RECV_COURSE_NOTFOUND, courseCodeRes);
            }
            else
            {
                printf(CLIENT_RECV_COURSE_INFO, categoryRes, courseCodeRes, result);
            }
        }
        if (!strcmp(messageType, MSG_TYPE_MULT_COURSE_RES))
        {
            memzero(result, MAX_COURSE_RES_LEN);
            parseMultiResponse(course, courseRes + resDelimiterPos, result);
            size_t lastChar = strlen(result);
            result[lastChar-1] = 0;
            printf(CLIENT_RECV_MULT_COURSE, result);

        }
        printf("%s", CLIENT_START_NEW_REQ);
    }
}

int authenticate()
{
    // Beej’s guide to network programming, 5.7
    char authRequest[MAX_CREDENTIAL_LEN] = {0};
    char authResponse[MAX_CREDENTIAL_LEN] = {0};
    char authType[MAX_CREDENTIAL_LEN] = {0};
    char messageType[MSG_TYPE_LEN] = {0};
    int retryChance = 3;
    int numBytes;

    while (retryChance > 0)
    {
        getCredentials();
        memzero(authRequest, MAX_CREDENTIAL_LEN);
        memzero(authResponse, MAX_CREDENTIAL_LEN);
        memzero(authType, MAX_CREDENTIAL_LEN);
        memzero(messageType, MSG_TYPE_LEN);
        strcat(authRequest, MSG_TYPE_CRED);
        strcat(authRequest, DELIMITER);
        strcat(authRequest, username);
        strcat(authRequest, DELIMITER);
        strcat(authRequest, password);
        
        // Beej’s guide to network programming, 6.1

        if (send(sockfd, authRequest, strlen(authRequest), 0) == -1)
        {
            perror("send");
        }
        printf(CLIENT_AUTH_REQUEST, username);
        debug("Sent %s", authRequest);
        if ((numBytes = recv(sockfd, authResponse, MAX_CREDENTIAL_LEN, 0)) == -1)
        {
            perror("recv");
        }
        authResponse[numBytes] = '\0';
        debug("Received %s", authResponse);
        strncpy(messageType, authResponse, MSG_TYPE_LEN);
        size_t delimiterPos = strcspn(authResponse, DELIMITER);
        messageType[delimiterPos] = 0;
        debug("%s", messageType);

        if (!strcmp(messageType, MSG_TYPE_CRED))
        {
            strncpy(authType, authResponse + delimiterPos + 1, MSG_TYPE_LEN);
            debug("%s", authType);
            if (strstr(MSG_TYPE_AUTH, authType))
            {
                printf(CLIENT_AUTH_SUCCESS, username, portNo);
                return TRUE;
            }

            if (strstr(MSG_TYPE_NOAUTH_USER, authType))
            {
                retryChance--;
                printf(CLIENT_AUTH_FAILED_USER, username, portNo, retryChance);
                continue;
            }
            if (strstr(MSG_TYPE_NOAUTH_PASS, authType))
            {
                retryChance--;
                printf(CLIENT_AUTH_FAILED_PASS, username, portNo, retryChance);
                continue;
            }
        }
    }
    printf(CLIENT_AUTH_FAILED);
    return FALSE;
}

void communicateWithServerM()
{
    int isAuthenticated = authenticate();
    if (!isAuthenticated)
    {   
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    sendCourseDetails();
}

int main()
{
    connectToServerM();
    printf("%s\n", CLIENT_BOOT_MSG);
    communicateWithServerM();
    close(sockfd);
    return 0;
}

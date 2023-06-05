
#include "common.h"

typedef struct CourseInfoT
{
    char courseCode[MAX_COURSECODE_LEN];
    char credits[MAX_CREDITS_LEN];
    char professor[MAX_PROF_LEN];
    char days[MAX_DAYS_LEN];
    char courseName[MAX_COURSENAME_LEN];
} CourseInfo;

CourseInfo courseInfo[MAX_FILE_LINES] = {0};
int totalLines = 0;
int sockfd = 0;

// Associate socket with a port
void bindUDPSocket()
{
    // Beej’s guide to network programming, 6.3
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char serverEEPort[10];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // set to AF_INET to use IPv4
    hints.ai_socktype = SOCK_DGRAM;

    sprintf(serverEEPort, "%d", SERVEREE_PORT);

    /* convert a text string representation of an IP address to an addrinfo structure 
    that contains a sockaddr structure for the IP address and other information. */
    if ((rv = getaddrinfo(localhost, serverEEPort, &hints, &servinfo)) != 0)
    {
        perror("serverEE: UDP getaddrinfo");
        return;
    }

    // loop through all the results and make a socket
    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        // make a socket using the information gleaned from getaddrinfo():
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1)
        {
            perror("serverEE: UDP socket");
            continue;
        }

        // bind the socket to the port we passed in to getaddrinfo():
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(sockfd);
            perror("serverEE: Failed to bind");
            continue;
        }

        break;
    }

    // check if the socket was bound successfully or not
    if (p == NULL)
    {
        printf("serverEE: failed to create UDP socket\n");
        return;
    }
    printf(DEPT_BOOT_MSG, "EE", SERVEREE_PORT);
}

// Read and parse the course information file. 
void readFile()
{
    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t bytes;

    fp = fopen(EE_FILE, "r");
    if (fp == NULL)
    {
        printf("serverEE : Couldn't open file : %s", EE_FILE);
        exit(EXIT_FAILURE);
    }
    int i = 0;
    while ((bytes = getline(&line, &len, fp)) != -1)
    {
        size_t delimiterPos = 0;
        size_t lineDelimiterPos = 0;

        // Parse course code
        strncpy(courseInfo[i].courseCode, line, MAX_COURSECODE_LEN);

        // Reference for usage of strcspn - 
        // https://stackoverflow.com/questions/2693776/removing-trailing-newline-character-from-fgets-input
        delimiterPos = strcspn(line, COMMA);
        lineDelimiterPos += delimiterPos + 1;
        courseInfo[i].courseCode[delimiterPos] = 0;

        // Parse course credits
        strncpy(courseInfo[i].credits, line + lineDelimiterPos, MAX_CREDITS_LEN);
        delimiterPos = strcspn(line + lineDelimiterPos, COMMA);
        lineDelimiterPos += delimiterPos + 1;
        courseInfo[i].credits[delimiterPos] = 0;

        // Parse professor
        strncpy(courseInfo[i].professor, line + lineDelimiterPos, MAX_PROF_LEN);
        delimiterPos = strcspn(line + lineDelimiterPos, COMMA);
        lineDelimiterPos += delimiterPos + 1;
        courseInfo[i].professor[delimiterPos] = 0;

        // Parse days
        strncpy(courseInfo[i].days, line + lineDelimiterPos, MAX_DAYS_LEN);
        delimiterPos = strcspn(line + lineDelimiterPos, COMMA);
        lineDelimiterPos += delimiterPos + 1;
        courseInfo[i].days[delimiterPos] = 0;

        // Parse course name
        strncpy(courseInfo[i].courseName, line + lineDelimiterPos, MAX_COURSENAME_LEN);
        delimiterPos = strcspn(line + lineDelimiterPos, "\r\n");
        lineDelimiterPos += delimiterPos + 1;
        courseInfo[i].courseName[delimiterPos] = 0;
        debug("%s , %s, %s , %s, %s ", courseInfo[i].courseCode, courseInfo[i].credits, courseInfo[i].professor, courseInfo[i].days, courseInfo[i].courseName);

        i++;
    }
    totalLines = i;
    if (line)
        free(line);

    fclose(fp);
}

// Check if the input list of courses are present in the provided course information list
void getMultCourseResponse(char *courseCodes, char *courseRes)
{
    int resDelimiterPos = 0;
    int delimiterPos = 0;
    int courseFound = FALSE;
    char curCourseCode[MAX_COURSE_LEN] = {0};
    char courseInfoRes[MAX_COURSE_RES_LEN] = {0};
    while (strlen(courseCodes) >= resDelimiterPos)
    {
        courseFound = FALSE;
        memzero(curCourseCode, MAX_COURSE_LEN);
        memzero(courseInfoRes, MAX_COURSE_RES_LEN);
        delimiterPos = strcspn(courseCodes + resDelimiterPos, " |");
        strncpy(curCourseCode, courseCodes + resDelimiterPos, delimiterPos);
        curCourseCode[delimiterPos] = '\0';
        resDelimiterPos += delimiterPos + 1;
        debug("delimiterPos %d", delimiterPos);
        debug("resdelimiterPos %d", resDelimiterPos);
        if (delimiterPos == 0)
        {
            break;
        }

        for (int i = 0; i < totalLines; i++)
        {
            if (!strcmp(courseInfo[i].courseCode, curCourseCode))
            {
                strcat(courseInfoRes, courseInfo[i].courseCode);
                strcat(courseInfoRes, ": ");
                strcat(courseInfoRes, courseInfo[i].credits);
                strcat(courseInfoRes, ", ");
                strcat(courseInfoRes, courseInfo[i].professor);
                strcat(courseInfoRes, ", ");
                strcat(courseInfoRes, courseInfo[i].days);
                strcat(courseInfoRes, ", ");
                strcat(courseInfoRes, courseInfo[i].courseName);
                strcat(courseInfoRes, "\n");
                strcat(courseRes, courseInfoRes);
                courseFound = TRUE;
                printf(DEPT_MULT_COURSE_FOUND, curCourseCode, courseInfoRes);
            }
        }
        if (courseFound == FALSE)
        {
            strcat(courseRes, curCourseCode);
            strcat(courseRes, ":");
            strcat(courseRes, DELIMITER);
            strcat(courseRes, COURSE_NOT_FOUND);
            strcat(courseRes, DELIMITER);
            strcat(courseRes, "\n");
            printf(DEPT_COURSE_NOT_FOUND, curCourseCode);
        }
    }
    debug("Courses : %s", courseCodes);
    debug("CourseResponse : \n %s", courseRes);
}

// Check if course is present in the provided course information list
void getCourseResponse(char *courseReq, char *courseRes)
{
    char courseCode[MAX_COURSE_LEN + 1];
    char category[MAX_CATEGORY_LEN + 1];
    strcpy(courseCode, courseReq);
    size_t delimiterPos = strcspn(courseReq, DELIMITER);
    courseCode[delimiterPos] = 0;
    strcpy(category, courseReq + delimiterPos + 1);
    int i = 0;
    char result[MAX_COURSENAME_LEN] = {0};
    int courseFound = FALSE;
    for (i = 0; i < totalLines; i++)
    {

        if (!strcmp(courseInfo[i].courseCode, courseCode))
        {
            debug(" in : %s : %s ", courseInfo[i].courseCode, courseCode);
            debug(" cmp : %s : %s ", courseInfo[i].professor, category);
            if (!strcmp(category, "Credit"))
            {
                strcpy(result, courseInfo[i].credits);
                courseFound = TRUE;
            }
            else if (!strcmp(category, "Professor"))
            {
                strcpy(result, courseInfo[i].professor);
                courseFound = TRUE;
            }
            else if (!strcmp(category, "Days"))
            {
                strcpy(result, courseInfo[i].days);
                courseFound = TRUE;
            }
            else if (!strcmp(category, "CourseName"))
            {
                strcpy(result, courseInfo[i].courseName);
                courseFound = TRUE;
            }
            break;
        }
    }
    printf(DEPT_COURSE_RECV, "EE", category, courseCode);
    if (courseFound != TRUE)
    {
        strcpy(result, COURSE_NOT_FOUND);
        printf(DEPT_COURSE_NOT_FOUND, courseCode);
    }
    else
    {
        printf(DEPT_COURSE_FOUND, category, courseCode, result);
    }

    strcat(courseRes, MSG_TYPE_COURSE_RES);
    strcat(courseRes, DELIMITER);
    strcat(courseRes, courseCode);
    strcat(courseRes, DELIMITER);
    strcat(courseRes, category);
    strcat(courseRes, DELIMITER);
    strcat(courseRes, result);
    strcat(courseRes, DELIMITER);
    debug(courseRes);

    return;
}

// send course details as per the course request (single or multi-course)
void sendCourseDetails(char *courseReq, char *msgType)
{
    char courseRes[MAX_COURSE_RES_LEN] = {0};
    int numBytes;

    // Beej’s guide to network programming, 6.3
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char serverMPort[10];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // set to AF_INET to use IPv4
    hints.ai_socktype = SOCK_DGRAM;

    sprintf(serverMPort, "%d", SERVERM_UDP_PORT);
    
    /* convert a text string representation of an IP address to an addrinfo structure 
    that contains a sockaddr structure for the IP address and other information. */
    if ((rv = getaddrinfo(localhost, serverMPort, &hints, &servinfo)) != 0)
    {
        perror("serverEE: getaddrinfo");
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
        printf("serverEE: failed to create socket\n");
        return;
    }
    // take necessary action based on the course request
    if (!strcmp(msgType, MSG_TYPE_COURSE_REQ))
    {
        getCourseResponse(courseReq, courseRes);
    }
    else if (!strcmp(msgType, MSG_TYPE_MULT_COURSE_REQ))
    {
        getMultCourseResponse(courseReq, courseRes);
    }
    else
    {
        debug("serverEE: invalid message received : \"%s\"", msgType);
        return;
    }
    if ((numBytes = sendto(sockfd, courseRes, strlen(courseRes), 0,
                           p->ai_addr, p->ai_addrlen)) == -1)
    {
        perror("serverEE: sendto");
        close(sockfd);
        exit(1);
    }
    debug("Sent : %s : %d bytes", courseRes, numBytes);
    printf(DEPT_COURSE_SENT, "EE");
}

// receive course details from the socket  
void receiveCourseDetails()
{
    int numbytes;
    struct sockaddr_storage their_addr;
    char courseReq[MAX_COURSE_LEN];
    socklen_t addr_len;
    size_t resDelimiterPos = 0;

    addr_len = sizeof their_addr;
    while (1)
    {
        memzero(courseReq, MAX_COURSE_LEN);
        // receive data from socket
        if ((numbytes = recvfrom(sockfd, courseReq, MAX_COURSE_REQ_LEN - 1, 0,
                                 (struct sockaddr *)&their_addr, &addr_len)) == -1)
        {
            perror("serverEE: recvfrom");
            close(sockfd);
            exit(1);
        }
        resDelimiterPos = 0;
        courseReq[numbytes] = '\0';
        debug("serverEE: message received : \"%s\"", courseReq);
        char msgType[MSG_TYPE_LEN] = {0};
        strncpy(msgType, courseReq, MSG_TYPE_LEN);
        size_t delimiterPos = strcspn(courseReq, DELIMITER);
        resDelimiterPos += delimiterPos + 1;
        msgType[delimiterPos] = 0;
        debug("msgType : %s ", msgType);
        // send course details to process details and respond as per the course request
        sendCourseDetails(courseReq + resDelimiterPos, msgType);
    }
    close(sockfd);
}

int main()
{
    readFile();
    bindUDPSocket();
    receiveCourseDetails();
    return 0;
}
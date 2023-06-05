

#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <errno.h> 
#include <string.h> 
#include <netdb.h> 
#include <sys/types.h> 
#include <netinet/in.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <sys/wait.h>

#include <stdarg.h>
#include <ctype.h>
#include <assert.h>

#define TRUE 1
#define FALSE 0
#define DEBUG 1

/*
    debug("%s", authType);
    debug("%s", MSG_TYPE_NOAUTH_USER);
    for (int i = 0; i < strlen(MSG_TYPE_NOAUTH_USER); i++)
    {
        if (authType[i] != MSG_TYPE_NOAUTH_USER[i])
        {
            debug("%d : %c , %c ", i, authType[i], MSG_TYPE_NOAUTH_USER[i]);
        }
    }
*/

#define localhost "127.0.0.1"

#define MSG_TYPE_LEN 30
#define MSG_TYPE_CRED "CREDENTIAL"
#define MSG_TYPE_AUTH "CREDENTIAL|AUTHENTICATED"
#define MSG_TYPE_NOAUTH_USER "CREDENTIAL|FAIL_NO_USER"
#define MSG_TYPE_NOAUTH_PASS "CREDENTIAL|FAIL_PASS_NO_MATCH"
#define MSG_TYPE_COURSE_REQ "COURSE_REQUEST"
#define MSG_TYPE_COURSE_RES "COURSE_RESPONSE"
#define MSG_TYPE_MULT_COURSE_REQ "MULT_COURSE_REQUEST"
#define MSG_TYPE_MULT_COURSE_RES "MULT_COURSE_RESPONSE"
#define COURSE_NOT_FOUND "Course Not Found"

#define MIN_USERNAME_LEN 5
#define MIN_PASSWORD_LEN 5
#define MAX_USERNAME_LEN 50
#define MAX_PASSWORD_LEN 50
#define MAX_CREDENTIAL_LEN 150
#define MAX_COURSE_LEN 100
#define MAX_CATEGORY_LEN 15
#define MAX_COURSECODE_LEN 10
#define MAX_CREDITS_LEN 10
#define MAX_PROF_LEN 100
#define MAX_DAYS_LEN 20
#define MAX_COURSENAME_LEN 100
#define MAX_COURSE_REQ_LEN 100
#define MAX_COURSE_RES_LEN 80000

#define EE_DEPT "EE"
#define CS_DEPT "CS"

#define DELIMITER "|"

#define USC_ID 639
#define SERVERC_PORT 21000 + USC_ID
#define SERVERCS_PORT 22000 + USC_ID
#define SERVEREE_PORT 23000 + USC_ID
#define SERVERM_UDP_PORT 24000 + USC_ID
#define SERVERM_TCP_PORT 25000 + USC_ID

#define CRED_FILE "cred.txt"
#define CS_FILE "cs.txt"
#define EE_FILE "ee.txt"
#define MAX_FILE_LINES 550
#define MAX_CHAR_LINE 100
#define COMMA ","

// Beej’s guide to network programming, 5.6
#define BACKLOG 10 // how many pending connections queue will hold

/* serverEE and serverCS specific messages */
#define DEPT_BOOT_MSG "The Server%s is up and running using UDP on port %d.\n"
#define DEPT_COURSE_RECV "The Server%s received a request from the Main Server about the %s of %s.\n"
#define DEPT_COURSE_FOUND "The course information has been found: The %s of %s is %s.\n"
#define DEPT_MULT_COURSE_FOUND "The course information has been found: The course information of %s is %s.\n"
#define DEPT_COURSE_NOT_FOUND "Didn’t find the course: %s.\n"
#define DEPT_COURSE_SENT "The Server%s finished sending the response to the Main Server.\n"

/* serverC specific messages */
#define SERVERC_BOOT_MSG "The ServerC is up and running using UDP on port %d.\n"
#define SERVERC_CRED_RECV "The ServerC received an authentication request from the Main Server.\n"
#define SERVERC_CRED_SENT "The ServerC finished sending the response to the Main Server.\n"

/* serverM specific messages */
#define SERVERM_BOOT_MSG "The main server is up and running."
#define SERVERM_RECEIVED_CREDENIALS "The main server received the authentication for %s using TCP over port %d.\n"
#define SERVERM_SENT_AUTH_REQ "The main server sent an authentication request to serverC.\n"
#define SERVERM_RECV_AUTH_RES "The main server received the result of the authentication request from ServerC using UDP over port %d.\n"
#define SERVERM_SENT_AUTH_RES "The main server sent the authentication result to the client.\n"
#define SERVERM_RECV_COURSE_REQ "The main server received from %s to query course %s about %s using TCP over port %d.\n"
#define SERVERM_SENT_DEPT_REQ "The main server sent a request to server %s.\n"
#define SERVERM_RECV_DEPT_RES "The main server received the response from server%s using UDP over port %d.\n"
#define SERVERM_SENT_COURSE_RES "The main server sent the query information to the client.\n"

/* client specific messages */
#define CLIENT_BOOT_MSG "The client is up and running."
#define CLIENT_ENTER_USERNAME "Please enter the username:"
#define CLIENT_ENTER_PASSWORD "Please enter the password:"
#define CLIENT_AUTH_REQUEST "%s sent an authentication request to the main server.\n"
#define CLIENT_AUTH_SUCCESS "%s received the result of authentication using TCP over port %d. " \
                            "Authentication is successful\n"
#define CLIENT_AUTH_FAILED_USER "%s received the result of authentication using TCP over port %d. " \
                                "Authentication failed : Username Does not exist\nAttempts remaining : %d\n"
#define CLIENT_AUTH_FAILED_PASS "%s received the result of authentication using TCP over port %d. " \
                                "Authentication failed : Password does not match\nAttempts remaining : %d\n"
#define CLIENT_AUTH_FAILED "Authentication Failed for 3 attempts. Client will shut down.\n"
#define CLIENT_ENTER_COURSE "Please enter the course code to query:"
#define CLIENT_ENTER_CATEGORY "Please enter the category (Credit / Professor / Days / CourseName):"
#define CLIENT_SEND_COURSE_REQ "%s sent a request to the main server.\n"
#define CLIENT_RECV_COURSE_RES "The client received the response from the Main server using TCP over port %d.\n"
#define CLIENT_RECV_COURSE_INFO "The %s of %s is %s.\n"
#define CLIENT_START_NEW_REQ "\n\n-----Start a new request-----\n"
#define CLIENT_RECV_COURSE_NOTFOUND "Didn’t find the course: %s.\n"
#define CLIENT_SEND_MULT_COURSE "%s sent a request with multiple CourseCode to the main server.\n"
#define CLIENT_RECV_MULT_COURSE "CourseCode: Credits, Professor, Days, Course Name\n%s\n"

void removeSpace(char *inputStr, char *outputStr)
{

    char *startStr = inputStr;
    char *endStr = inputStr + strlen(inputStr) - 1;
    // Remove leading space
    while (isspace((unsigned char)*startStr))
    {
        startStr++;
    }
    // Remove trailing space
    while (isspace((unsigned char)*endStr))
    {
        endStr--;
        if (startStr == endStr)
        {
            outputStr[0] = 0;
            return;
        }
    }
    endStr++;
    // Replace multiple space by single space
    while (startStr != endStr)
    {
        while (*startStr == ' ' && *(startStr + 1) == ' ')
        {
            startStr++;
        }
        *outputStr++ = *startStr++;
    }
    *outputStr = '\0';
}

void debug(const char *format, ...)
{
    if (DEBUG)
    {
        char debugStr[1000] = {0};
        va_list args;
        va_start(args, format);
        strcpy(debugStr, "\tDEBUG : ");
        strcat(debugStr, format);
        strcat(debugStr, "\n");
        vprintf(debugStr, args);
        va_end(args);
    }
}

void memzero(char *string, size_t size)
{
    memset(string, 0, size);
}
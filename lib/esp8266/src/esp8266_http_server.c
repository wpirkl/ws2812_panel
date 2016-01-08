#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "FreeRTOS.h"
#include "esp8266.h"
#include "esp8266_http_server.h"

#include "web_content.h"
#include "web_content_handler.h"

/*!
    \todo Check if we loaded the whole header
    \todo Check if we loaded the whole content
*/



/* typedefs */

/*! server object */
typedef struct  {

    /*! Parser index */
    size_t      mParseIndex;

    /*! URL */
    uint8_t   * mURL;

    /*! Length of the URL */
    size_t      mURLLen;

    /*! Protocol */
    uint8_t   * mProtocol;

    /*! Length of protocol */
    size_t      mProtocolLen;

    /*! Size of data in buffer */
    size_t      mBufferSize;

    /*! buffer */
    uint8_t     mBuffer[1024];

} ts_http_server;

/*! enumerates a http request command */
typedef enum {
    ECMD_GET,
    ECMD_HEAD,
    ECMD_POST,
    ECMD_PUT,
    ECMD_DELETE,
    ECMD_TRACE,
    ECMD_OPTIONS,
    ECMD_CONNECT,
    ECMD_PATCH,
} te_esp8266_web_command;

/*! defines a http request command */
typedef struct {

    /*! The lenth of the command string */
    size_t                    mCommandLength;

    /*! The command string */
    const char              * mCommandName;

    /*! The corresponding command enum */
    te_esp8266_web_command    mCommandType;
} ts_esp8266_web_command;

/*! enumerates a http reply */
typedef enum {
    HTML_REPLY_OK,
    HTML_REPLY_NOT_FOUND,
    HTML_REPLY_INTERNAL_SERVER_ERROR,
} te_html_reply;

/*! defines a http reply */
typedef struct {

    /*! enum */
    te_html_reply   mEnum;

    /*! string */
    const char    * mString;

    /*! length of the string */
    size_t          mStringLen;

    /*! numeric */
    uint32_t        mNumeric;

} ts_html_reply;

/* forwards declaration */

static void esp8266_http_server_handler_task(ts_esp8266_socket * inSocket);
static void esp8266_http_parse(ts_esp8266_socket * inSocket, ts_http_server * inHttpServer);

static void esp8266_http_handle_get(ts_esp8266_socket * inSocket, ts_http_server * inHttpServer);


bool esp8266_http_server_start(void) {

    return esp8266_cmd_cipserver(80, esp8266_http_server_handler_task, configMAX_PRIORITIES - 3);
}


/*! Server Handler Task
*/
static void esp8266_http_server_handler_task(ts_esp8266_socket * inSocket) {

    /* local variables */
    ts_http_server lHttpServer;

    memset(&lHttpServer, 0, sizeof(lHttpServer));

    for(;;) {

        /* read data */
        if(esp8266_receive(inSocket, lHttpServer.mBuffer, sizeof(lHttpServer.mBuffer), &lHttpServer.mBufferSize)) {

            /* check if we receive something */
            if(lHttpServer.mBufferSize > 0) {

                esp8266_http_parse(inSocket, &lHttpServer);
            }
        } else {
            /* server error */
            break;
        }
    }
}


static const ts_esp8266_web_command sWebCommands[] = {
    { 3, "GET",     ECMD_GET },
    { 4, "POST",    ECMD_POST },
    { 4, "HEAD",    ECMD_HEAD },
    { 3, "PUT",     ECMD_PUT },
    { 6, "DELETE",  ECMD_DELETE },
    { 5, "TRACE",   ECMD_TRACE },
    { 7, "OPTIONS", ECMD_OPTIONS },
    { 7, "CONNECT", ECMD_CONNECT },
    { 5, "PATCH",   ECMD_PATCH },
};

static const ts_html_reply sReturnCodes[] = {
    { HTML_REPLY_OK,                    "OK",                    2,  200 },
    { HTML_REPLY_NOT_FOUND,             "Not Found",             9,  404 },
    { HTML_REPLY_INTERNAL_SERVER_ERROR, "Internal Server Error", 21, 500 },
};


/*! Skip all characters until next non-whitespace

    \param[in]  inIndex     The index of the first character to check

    \return the first index of a non-whitespace character
*/
static size_t esp8266_http_parse_skip_spaces(ts_http_server * inHttpServer, size_t inIndex) {

    /* skip spaces */
    while(isspace(inHttpServer->mBuffer[inIndex]) && inIndex < inHttpServer->mBufferSize) {
        inIndex++;
    }

    return inIndex;
}


/*! Skip all characters until next whitespace

    \param[in]  inIndex     The index of the first character to check

    \return the first index of a whitespace character
*/
static size_t esp8266_http_parse_skip_characters(ts_http_server * inHttpServer, size_t inIndex) {

    /* skip non white spaces */
    while(isspace(inHttpServer->mBuffer[inIndex]) && inIndex < inHttpServer->mBufferSize) {
        inIndex++;
    }

    return inIndex;
}


/*! Go to the next line

    Checks for carriage-return line-feed

    \param[in]  inIndex     The index of the first character to check

    \return the first character of the new line
*/
static size_t esp8266_http_parse_next_line(ts_http_server * inHttpServer, size_t inIndex) {

    bool lRSeen = false;
    bool lNSeen = false;

    while(inIndex < inHttpServer->mBufferSize) {

        if(!lRSeen) {
            if(inHttpServer->mBuffer[inIndex] == '\r') {
                lRSeen = true;
            }
        } else {
            if(!lNSeen) {
                if(inHttpServer->mBuffer[inIndex] == '\n') {
                    lNSeen = true;
                } else {
                    lRSeen = false;
                }
            } else {

                return inIndex;
            }
        }

        inIndex++;
    }

    return inIndex;
}


/*! Get Header Field

    \param[in]  inHeaderFieldName       The name of the header field
    \param[in]  inHeaderFieldNameLen    The length of the header field
    \param[out] outHeaderFieldValue     Returns the header field value
    \param[out] outHeaderFieldValueLen  Returns the length of the header field value

    \retval true    Header Field was found and decoded
    \retval false   Header Field was not found
*/
static bool esp8266_http_get_header_field(ts_http_server * inHttpServer, uint8_t * inHeaderFieldName, size_t inHeaderFieldNameLen, uint8_t ** outHeaderFieldValue, size_t * outHeaderFieldValueLen) {

    /* get the index after the first line */
    size_t lIndex;
    size_t lNextLine;

    for(lIndex = inHttpServer->mParseIndex; lIndex < inHttpServer->mBufferSize;) {
        /* not enough information left */
        if(lIndex + inHeaderFieldNameLen + 3 > inHttpServer->mBufferSize) {  /* <key>: <value>\r\n */
            break;
        }

        if(memcmp(inHeaderFieldName, &inHttpServer->mBuffer[lIndex], inHeaderFieldNameLen) == 0 &&
           inHttpServer->mBuffer[lIndex + inHeaderFieldNameLen] == ':') {

            /* decode value */
            lIndex += inHeaderFieldNameLen + 1;

            /* skip leading spaces */
            lIndex = esp8266_http_parse_skip_spaces(inHttpServer, lIndex);

            *outHeaderFieldValue = &inHttpServer->mBuffer[lIndex];
            *outHeaderFieldValueLen = esp8266_http_parse_next_line(inHttpServer, lIndex) - 2; /* remove \r\n */

            return true;
        } else {

            lNextLine = esp8266_http_parse_next_line(inHttpServer, lIndex);
            if(lNextLine == lIndex + 2) {    /* empty line means end of header */
                break;
            }
            lIndex = lNextLine;
        }
    }

    return false;
}


/*! Parse URL
*/
static void esp8266_http_parse_url(ts_http_server * inHttpServer) {

    if(inHttpServer->mURLLen == 1 && inHttpServer->mURL[0] == '/') {            /* point to index.html by default */

        inHttpServer->mURL = (uint8_t*)"index.html";
        inHttpServer->mURLLen = 10;

    } else if(inHttpServer->mURLLen > 1 && inHttpServer->mURL[0] == '/') {      /* remove leading spaces */

        inHttpServer->mURL = &inHttpServer->mURL[1];
        inHttpServer->mURLLen--;
    }
}


/*! Parse http header

*/
static void esp8266_http_parse(ts_esp8266_socket * inSocket, ts_http_server * inHttpServer) {

    size_t lCount;

    te_esp8266_web_command lWebCommand = ECMD_GET;

    /*
        we should have received something like

        GET /index.html?key1=value1&key2=value2&...&keyn=valuen HTTP/1.1\r\n
        Host: www.google.com\r\n
        \r\n
        \r\n

        or:

        POST /index.html HTTP/1.1\r\n
        Host: www.google.com\r\n
        Content-Type: application/x-www-form-urlencoded\r\n
        Content-Length: ??\r\n
        \r\n
        key1=value1&key2=value2&...&keyn=valuen\r\n
    */

    inHttpServer->mParseIndex = 0;

    /* find out which command was received */
    for(lCount = 0; lCount < sizeof(sWebCommands) / sizeof(ts_esp8266_web_command); lCount++) {

        size_t lCommandLen = sWebCommands[lCount].mCommandLength;
        if(inHttpServer->mBufferSize >= lCommandLen && memcmp(sWebCommands[lCount].mCommandName, &inHttpServer->mBuffer[inHttpServer->mParseIndex], lCommandLen) == 0) {

            lWebCommand = sWebCommands[lCount].mCommandType;
            inHttpServer->mParseIndex = lCommandLen;
            break;
        }
    }

    /* skip spaces */
    inHttpServer->mParseIndex = esp8266_http_parse_skip_spaces(inHttpServer, inHttpServer->mParseIndex);

    /* get url */
    inHttpServer->mURL = &inHttpServer->mBuffer[inHttpServer->mParseIndex];
    inHttpServer->mURLLen = esp8266_http_parse_skip_characters(inHttpServer, inHttpServer->mParseIndex) - inHttpServer->mParseIndex;
    inHttpServer->mParseIndex += inHttpServer->mURLLen;

    /* skip spaces */
    inHttpServer->mParseIndex = esp8266_http_parse_skip_spaces(inHttpServer, inHttpServer->mParseIndex);

    /* get protocol version */
    inHttpServer->mProtocol = &inHttpServer->mBuffer[inHttpServer->mParseIndex];
    inHttpServer->mProtocolLen = esp8266_http_parse_skip_characters(inHttpServer, inHttpServer->mParseIndex) - inHttpServer->mParseIndex;
    inHttpServer->mParseIndex += inHttpServer->mProtocolLen;

    /* skip newline */
    inHttpServer->mParseIndex = esp8266_http_parse_next_line(inHttpServer, inHttpServer->mParseIndex);

    /* mParseIndex now points to the first header field */

    esp8266_http_parse_url(inHttpServer);

    /* process the command */
    switch(lWebCommand) {
        case ECMD_GET:
            /* parse url, there could be url encoded data */
            esp8266_http_handle_get(inSocket, inHttpServer);
            break;
        case ECMD_HEAD:
        case ECMD_POST:
        case ECMD_PUT:
        case ECMD_DELETE:
        case ECMD_TRACE:
        case ECMD_OPTIONS:
        case ECMD_CONNECT:
        case ECMD_PATCH:
        default:
            break;
    }
}


/*! Handle GET

*/
static void esp8266_http_handle_get(ts_esp8266_socket * inSocket, ts_http_server * inHttpServer) {

    const ts_web_content_file * lContent = NULL;

    /* get the content */
    if(web_content_find_file(inHttpServer->mURL, inHttpServer->mURLLen, &lContent)) {
        
    } else {
        /* get the 404 page */
        web_content_find_file((uint8_t*)"404.html", 8, &lContent);
    }

    
}




/* eof */

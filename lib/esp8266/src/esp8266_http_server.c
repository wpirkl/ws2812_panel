#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <ctype.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "portable.h"
#include "esp8266.h"
#include "esp8266_http_server.h"

#include "web_content.h"
#include "web_content_handler.h"

/*!
    \todo Check if we loaded the whole header
    \todo Check if we loaded the whole content
*/

#define dbg_on(x...)      printf(x)
#define dbg_off(x...)

#define dbg dbg_off

#define dbg_err_on(x...)      printf(x)
#define dbg_err_off(x...)

#define dbg_err dbg_err_on

#define HTTP_HEADER_SIZE    (1024)

#define HTTP_BODY_SIZE      (4096)

/* typedefs */

/*! server object */
typedef struct  {

    /*! Socket object to send / receive on */
    ts_esp8266_socket   * mSocket;

    /*! Parser index */
    size_t      mParseIndex;

    /*! URL */
    uint8_t   * mURL;

    /*! Length of the URL */
    size_t      mURLLen;

    /*! Url Encoded Data */
    uint8_t   * mURLEncodedData;

    /*! Length of the URL encoded data */
    size_t      mURLEncodedDataLen;

    /*! Protocol */
    uint8_t   * mProtocol;

    /*! Length of protocol */
    size_t      mProtocolLen;

    /*! Size of data in buffer */
    size_t      mHeaderSize;

    /*! Size of body */
    size_t      mBodySize;

    /*! buffer */
    uint8_t   * mHeader;

    /*! Body */
    uint8_t   * mBody;

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
    HTTP_REPLY_OK,
    HTTP_REPLY_NOT_FOUND,
    HTTP_REPLY_INTERNAL_SERVER_ERROR,
    HTTP_REPLY_NO_CONTENT,
//    HTTP_REPLY_NOT_IMPLEMENTED,
} te_html_reply;

/*! defines a http reply */
typedef struct {

    /*! enum */
    te_html_reply   mEnum;

    /*! string */
    const char    * mString;

    /*! numeric */
    int             mNumeric;

} ts_html_reply;

/* forwards declaration */

static void esp8266_http_server_handler_task(ts_esp8266_socket * inSocket);
static bool esp8266_http_parse(ts_http_server * inHttpServer);

static bool esp8266_http_handle_get(ts_http_server * inHttpServer);
static bool esp8266_http_handle_post(ts_http_server * inHttpServer);


bool esp8266_http_server_start(void) {

    return esp8266_cmd_cipserver(80, esp8266_http_server_handler_task, configMAX_PRIORITIES - 4, configMINIMAL_STACK_SIZE * 6);
}


/*! Server Handler Task
*/
static void esp8266_http_server_handler_task(ts_esp8266_socket * inSocket) {

    /* local variables */
    ts_http_server lHttpServer;

    dbg("Server started on socket %p\r\n", inSocket);

    /* initialize without mHeader */
    memset(&lHttpServer, 0, sizeof(lHttpServer));

    lHttpServer.mSocket = inSocket;

    lHttpServer.mHeader = malloc(HTTP_HEADER_SIZE);
    lHttpServer.mBody   = malloc(HTTP_BODY_SIZE);
    if(lHttpServer.mHeader && lHttpServer.mBody) {
        for(;;) {

            /* read data */
            if(esp8266_receive(lHttpServer.mSocket, lHttpServer.mHeader, 1, &lHttpServer.mHeaderSize)) {

                /* check if we receive something */
                if(lHttpServer.mHeaderSize > 0) {

                    dbg("Received: %d\r\n", lHttpServer.mHeaderSize);

                    /* parse failed, kill connection */
                    if(!esp8266_http_parse(&lHttpServer)) {
                        dbg("Parsing failed!\r\n");
                        break;
                    }
                }
            } else {
                /* server error */
                dbg("Receive on socket %p failed\r\n", inSocket);
                break;
            }
        }
    } else {
        dbg_err("%s(%d): Malloc failed!\r\n", __FILE__, __LINE__);
    }

    free(lHttpServer.mBody);
    free(lHttpServer.mHeader);

    dbg("Closing socket %p\r\n", inSocket);
}


/*! HTTP Commands */
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


/*! HTTP Replies */
static const ts_html_reply sReturnCodes[] = {
    { HTTP_REPLY_OK,                    "OK",                    200 },
    { HTTP_REPLY_NOT_FOUND,             "Not Found",             404 },
    { HTTP_REPLY_INTERNAL_SERVER_ERROR, "Internal Server Error", 500 },
    { HTTP_REPLY_NO_CONTENT,            "No Content",            204 },
//    { HTTP_REPLY_NOT_IMPLEMENTED,       "Not Implemented",       501 },
};


/*! Skip all characters until next non-whitespace

    \param[in]  inIndex     The index of the first character to check

    \return the first index of a non-whitespace character
*/
static size_t esp8266_http_parse_skip_spaces(ts_http_server * inHttpServer, size_t inIndex) {

    /* skip spaces */
    while(isspace(inHttpServer->mHeader[inIndex]) && inIndex < inHttpServer->mHeaderSize) {
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
    while(!isspace(inHttpServer->mHeader[inIndex]) && inIndex < inHttpServer->mHeaderSize) {
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

    while(inIndex < inHttpServer->mHeaderSize) {

        if(!lRSeen) {
            if(inHttpServer->mHeader[inIndex] == '\r') {
                lRSeen = true;
            }
        } else {
            if(!lNSeen) {
                if(inHttpServer->mHeader[inIndex] == '\n') {
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

    for(lIndex = inHttpServer->mParseIndex; lIndex < inHttpServer->mHeaderSize;) {
        /* not enough information left */
        if(lIndex + inHeaderFieldNameLen + 3 > inHttpServer->mHeaderSize) {  /* <key>: <value>\r\n */
            break;
        }

        if(memcmp(inHeaderFieldName, &inHttpServer->mHeader[lIndex], inHeaderFieldNameLen) == 0 &&
           inHttpServer->mHeader[lIndex + inHeaderFieldNameLen] == ':') {

            /* decode value */
            lIndex += inHeaderFieldNameLen + 1;

            /* skip leading spaces */
            lIndex = esp8266_http_parse_skip_spaces(inHttpServer, lIndex);

            *outHeaderFieldValue = &inHttpServer->mHeader[lIndex];
            *outHeaderFieldValueLen = esp8266_http_parse_next_line(inHttpServer, lIndex) - lIndex - 2; /* remove \r\n */

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

    } else if(inHttpServer->mURLLen > 1 && inHttpServer->mURL[0] == '/') {      /* remove leading slash */

        inHttpServer->mURL = &inHttpServer->mURL[1];
        inHttpServer->mURLLen--;
    }
}

#undef dbg
#define dbg dbg_off

/*! Send Reply

    \param[in]  inHttpServer    HTTP context
    \param[in]  inContent       HTTP content to send
    \param[in]  inReply         The reply to send
*/
static bool esp8266_http_send_reply(ts_http_server * inHttpServer, const ts_web_content_file * inContent, te_html_reply inReply) {

    bool lChunkedTransfer = false;
    bool lRetVal = true;
    size_t lOffset = 0;

    inHttpServer->mHeaderSize = 0;
    inHttpServer->mHeaderSize += snprintf((char*)&inHttpServer->mHeader[inHttpServer->mHeaderSize], HTTP_HEADER_SIZE - inHttpServer->mHeaderSize,
                                          "HTTP/1.%d %d %s\r\n", (inReply != HTTP_REPLY_OK)? 0 : 1, sReturnCodes[inReply].mNumeric, sReturnCodes[inReply].mString);

    if(inContent != NULL) {

        dbg("Preparing output\r\n");

        /* prepare data */
        lChunkedTransfer = web_content_prepare_output(inContent, (char*)inHttpServer->mBody, HTTP_BODY_SIZE, &inHttpServer->mBodySize, &lOffset);

        dbg("Body length: %d\r\n", inHttpServer->mBodySize);

        inHttpServer->mHeaderSize += snprintf((char*)&inHttpServer->mHeader[inHttpServer->mHeaderSize], HTTP_HEADER_SIZE - inHttpServer->mHeaderSize,
                                              "Content-Type: %s\r\n", web_content_get_type(inContent));

        if(web_content_is_cachable(inContent)) {

            inHttpServer->mHeaderSize += snprintf((char*)&inHttpServer->mHeader[inHttpServer->mHeaderSize], HTTP_HEADER_SIZE - inHttpServer->mHeaderSize,
                                                  "cache-control: private, max-age=604800\r\n");
        } else {

            inHttpServer->mHeaderSize += snprintf((char*)&inHttpServer->mHeader[inHttpServer->mHeaderSize], HTTP_HEADER_SIZE - inHttpServer->mHeaderSize,
                                                  "Cache-control: no-cache\r\n");
        }

        if(lChunkedTransfer) {
            /* do chunked encoding */
            inHttpServer->mHeaderSize += snprintf((char*)&inHttpServer->mHeader[inHttpServer->mHeaderSize], HTTP_HEADER_SIZE - inHttpServer->mHeaderSize,
                                                "Transfer-Encoding: chunked\r\n");
        } else {
            /* send it at once */
            inHttpServer->mHeaderSize += snprintf((char*)&inHttpServer->mHeader[inHttpServer->mHeaderSize], HTTP_HEADER_SIZE - inHttpServer->mHeaderSize,
                                                "Content-Length: %d\r\n", inHttpServer->mBodySize);
        }
    }

    /* end of header */
    inHttpServer->mHeaderSize += snprintf((char*)&inHttpServer->mHeader[inHttpServer->mHeaderSize], HTTP_HEADER_SIZE - inHttpServer->mHeaderSize,
                                           "\r\n");

    /* just for debug */
//    inHttpServer->mHeader[inHttpServer->mHeaderSize] = '\0';
    dbg("Header length: %d\r\n", inHttpServer->mHeaderSize);
//    dbg("Sending header:\r\n\"%s\"\r\n", inHttpServer->mHeader);

    /* send header */
    if(!esp8266_cmd_cipsend_tcp(inHttpServer->mSocket, inHttpServer->mHeader, inHttpServer->mHeaderSize)) {
        dbg_err("%s(%d): Sending header failed! %p\r\n", __FILE__, __LINE__, inHttpServer->mSocket);
        return false;
    }

    /* send data */
    if(inContent != NULL) {

        /* just for debug */
//        inHttpServer->mBody[inHttpServer->mBodySize] = '\0';
//        dbg("Body length: %d\r\n", inHttpServer->mBodySize);
//        dbg("Sending Body:\r\n\"%s\"\r\n", inHttpServer->mBody);

        for(;;) {
            size_t lSent = 0;
            size_t lChunk = 0;

            bool lStatus = true;

            if(lChunkedTransfer) {

                /* add chunk size info */
                inHttpServer->mHeaderSize = snprintf((char*)inHttpServer->mHeader, HTTP_HEADER_SIZE, "%x\r\n", inHttpServer->mBodySize);

                /* send chunk header */
                if(!esp8266_cmd_cipsend_tcp(inHttpServer->mSocket, inHttpServer->mHeader, inHttpServer->mHeaderSize)) {
                    dbg_err("%s(%d): Sending chunk header failed! %p\r\n", __FILE__, __LINE__, inHttpServer->mSocket);
                    return false;
                }
            }

            for(lSent = 0; lSent < inHttpServer->mBodySize && lStatus; lSent += lChunk) {

                if(inHttpServer->mBodySize - lSent > 2048) {
                    lChunk = 2048;
                } else {
                    lChunk = inHttpServer->mBodySize - lSent;
                }

                lStatus = esp8266_cmd_cipsend_tcp(inHttpServer->mSocket, &inHttpServer->mBody[lSent], lChunk);
                if(!lStatus) {

                    dbg_err("%s(%d): Sending data packet failed! %p\r\n", __FILE__, __LINE__, inHttpServer->mSocket);
                    return lStatus;
                }
            }

            if(!lRetVal && lChunkedTransfer) {

                /* last chunk */

                /* send a chunk with 0 length */
                inHttpServer->mHeaderSize = snprintf((char*)inHttpServer->mHeader, HTTP_HEADER_SIZE, "\r\n0\r\n\r\n");

                /* send final chunk header */
                if(!esp8266_cmd_cipsend_tcp(inHttpServer->mSocket, inHttpServer->mHeader, inHttpServer->mHeaderSize)) {
                    dbg_err("%s(%d): Sending final chunk header failed! %p\r\n", __FILE__, __LINE__, inHttpServer->mSocket);
                    return false;
                }

                /* we're done */
                break;
            }

            if(lChunkedTransfer) {

                /* send end of body \r\n */
                inHttpServer->mHeaderSize = snprintf((char*)inHttpServer->mHeader, HTTP_HEADER_SIZE, "\r\n");

                /* send chunk header */
                if(!esp8266_cmd_cipsend_tcp(inHttpServer->mSocket, inHttpServer->mHeader, inHttpServer->mHeaderSize)) {
                    dbg_err("%s(%d): Sending end of body failed! %p\r\n", __FILE__, __LINE__, inHttpServer->mSocket);
                    return false;
                }

                /* fill next chunk */
                lRetVal = web_content_prepare_output(inContent, (char*)inHttpServer->mBody, HTTP_BODY_SIZE, &inHttpServer->mBodySize, &lOffset);
            } else {

                /* nt chunked, so we're done */
                break;
            }
        }
    }

    dbg("returning: %s\r\n", (inReply == HTTP_REPLY_OK)? "true" : "false");

    return (inReply == HTTP_REPLY_OK);
}


/*! Read http body
    \param[in]  inHttpServer    HTTP context
*/
static bool esp8266_http_read_body(ts_http_server * inHttpServer) {

    const char lHeaderFieldContentLengthName[] = "Content-Length";
    uint8_t *  lHeaderFieldContentLengthValue;
    size_t     lHeaderFieldContentLengthValueLen;

    size_t     lBodyLength;

    char lChar;

    bool lRetVal;
    size_t lRcvCount = 0;
    size_t lRetryCount;

    if(esp8266_http_get_header_field(inHttpServer, (uint8_t*)lHeaderFieldContentLengthName, sizeof(lHeaderFieldContentLengthName)-1, &lHeaderFieldContentLengthValue, &lHeaderFieldContentLengthValueLen)) {

        /* get the length of the body */
        lChar = lHeaderFieldContentLengthValue[lHeaderFieldContentLengthValueLen];
        lHeaderFieldContentLengthValue[lHeaderFieldContentLengthValueLen] = '\0';

        lBodyLength = strtoul((char*)lHeaderFieldContentLengthValue, NULL, 0);

        lHeaderFieldContentLengthValue[lHeaderFieldContentLengthValueLen] = lChar;

        dbg("Body length is: %d\r\n", lBodyLength);
        inHttpServer->mBodySize = 0;

        /* make sure we don't read more than the buffer can digest! */

        for(lRetryCount = 0; inHttpServer->mBodySize < lBodyLength && lRetryCount < 4; inHttpServer->mBodySize += lRcvCount, lRetryCount++) {

            lRcvCount = 0;
            lRetVal = esp8266_receive(inHttpServer->mSocket, &inHttpServer->mBody[inHttpServer->mBodySize], lBodyLength - inHttpServer->mBodySize, &lRcvCount);
            if(!lRetVal) {
                dbg_err("%s(%d): Receive failed\r\n", __FILE__, __LINE__);
                return lRetVal;
            }
        }

        if(inHttpServer->mBodySize < lBodyLength) {
            dbg_err("%s(%d): did not read enough data! Expected: %d, read: %d\r\n", __FILE__, __LINE__, lBodyLength, inHttpServer->mBodySize);
        }

        return true;

    } else {
        dbg_err("%s(%d): Missing header field \"%s\"\r\n", __FILE__, __LINE__, lHeaderFieldContentLengthName);
    }

    return false;
}


/*! Parse http header

    \param[in]  inHttpServer    HTTP context
*/
static bool esp8266_http_parse(ts_http_server * inHttpServer) {

    size_t lCount;
    size_t lRcvCount;
    size_t lState = 0;
    size_t lRetryCount = 0;
    bool lRetVal;
    bool lContinue = true;

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

    /* read until end of header */
    for(;lContinue;) {
        lRetVal = esp8266_receive(inHttpServer->mSocket, &inHttpServer->mHeader[inHttpServer->mHeaderSize], 1, &lRcvCount);
        if(!lRetVal) {
            dbg_err("%s(%d): receive on socket %p failed\r\n", __FILE__, __LINE__, inHttpServer->mSocket);
            return lRetVal;
        }

        if(lRcvCount == 0) {
            lRetryCount++;
            if(lRetryCount >= 4) {
                dbg("too many retries\r\n");
                return false;
            }
            continue;
        }

        /*
            1: \r
            2: \r\n
            3: \r\n\r
            4: \r\n\r\n
        */
        switch(lState) {
            default:
            case 0:
            case 2:
                if(inHttpServer->mHeader[inHttpServer->mHeaderSize] == '\r') {
                    lState++;
                } else {
                    lState = 0;
                }
                break;
            case 1:
            case 3:
                if(inHttpServer->mHeader[inHttpServer->mHeaderSize] == '\n') {
                    lState++;
                    if(lState == 4) {
                        lContinue = false;
                    }
                } else {
                    lState = 0;
                }
                break;
        }

        inHttpServer->mHeaderSize += lRcvCount;
    }

    dbg("Header Length is: %d\r\n", inHttpServer->mHeaderSize);

    inHttpServer->mParseIndex = 0;

    /* find out which command was received */
    for(lCount = 0; lCount < sizeof(sWebCommands) / sizeof(ts_esp8266_web_command); lCount++) {

        size_t lCommandLen = sWebCommands[lCount].mCommandLength;
        if(inHttpServer->mHeaderSize >= lCommandLen && memcmp(sWebCommands[lCount].mCommandName, &inHttpServer->mHeader[inHttpServer->mParseIndex], lCommandLen) == 0) {

            lWebCommand = sWebCommands[lCount].mCommandType;
            inHttpServer->mParseIndex = lCommandLen;
            dbg("Command: %s\r\n", sWebCommands[lCount].mCommandName);
            break;
        }
    }

    /* skip spaces */
    inHttpServer->mParseIndex = esp8266_http_parse_skip_spaces(inHttpServer, inHttpServer->mParseIndex);

    /* get url */
    inHttpServer->mURL = &inHttpServer->mHeader[inHttpServer->mParseIndex];
    inHttpServer->mURLLen = esp8266_http_parse_skip_characters(inHttpServer, inHttpServer->mParseIndex) - inHttpServer->mParseIndex;
    inHttpServer->mParseIndex += inHttpServer->mURLLen;

    /* skip spaces */
    inHttpServer->mParseIndex = esp8266_http_parse_skip_spaces(inHttpServer, inHttpServer->mParseIndex);

    /* get protocol version */
    inHttpServer->mProtocol = &inHttpServer->mHeader[inHttpServer->mParseIndex];
    inHttpServer->mProtocolLen = esp8266_http_parse_skip_characters(inHttpServer, inHttpServer->mParseIndex) - inHttpServer->mParseIndex;
    inHttpServer->mParseIndex += inHttpServer->mProtocolLen;

    /* skip newline */
    inHttpServer->mParseIndex = esp8266_http_parse_next_line(inHttpServer, inHttpServer->mParseIndex);

    /* mParseIndex now points to the first header field */

    /* just for debug */
    inHttpServer->mURL[inHttpServer->mURLLen] = '\0';
    dbg("URL: \"%s\"\r\n", inHttpServer->mURL);

    /* process the command */
    switch(lWebCommand) {
        case ECMD_GET:
            /* parse url, there could be url encoded data */
            return esp8266_http_handle_get(inHttpServer);
        case ECMD_POST:
            return esp8266_http_handle_post(inHttpServer);
        case ECMD_HEAD:
        case ECMD_PUT:
        case ECMD_DELETE:
        case ECMD_TRACE:
        case ECMD_OPTIONS:
        case ECMD_CONNECT:
        case ECMD_PATCH:
        default:
            {
                const ts_web_content_file * lContent = NULL;

                web_content_find_file((uint8_t*)"500.html", 8, &lContent);

                return esp8266_http_send_reply(inHttpServer, NULL, HTTP_REPLY_INTERNAL_SERVER_ERROR);
            }
    }
}


/*! HTTP GET - Get URL encoded data

    \param[in]  inHttpServer            HTTP context

    \retval true    URL encoded data found
    \retval false   No data found
*/
static bool esp8266_http_get_get_url_encoded_data(ts_http_server * inHttpServer) {

    size_t lCount;
    bool lRetVal = false;

    for(lCount = 0; lCount < inHttpServer->mURLLen; lCount++) {

        if(inHttpServer->mURL[lCount] == '?') {

            /* all characters after ? are url encoded */

            /* check if there's space left in the encoded data */
            if(lCount + 1 < inHttpServer->mURLLen) {

                inHttpServer->mURLEncodedData = &inHttpServer->mURL[lCount+1];
                inHttpServer->mURLEncodedDataLen = inHttpServer->mURLLen - (lCount + 1);
                lRetVal = true;
            } else {

                inHttpServer->mURLEncodedData = NULL;
                inHttpServer->mURLEncodedDataLen = 0;
                lRetVal = false;
            }

            /* so remove the data from the url */
            inHttpServer->mURLLen = lCount;

            return lRetVal;
        }
    }

    inHttpServer->mURLEncodedData = NULL;
    inHttpServer->mURLEncodedDataLen = 0;

    return false;
}


/*! Handle GET

    Handles HTTP Get

    \param[in]  inHttpServer            HTTP context
*/
static bool esp8266_http_handle_get(ts_http_server * inHttpServer) {

    const ts_web_content_file * lContent = NULL;
    te_html_reply lReply = HTTP_REPLY_INTERNAL_SERVER_ERROR;

    bool lUrlEncodedData = false;

    lUrlEncodedData = esp8266_http_get_get_url_encoded_data(inHttpServer);
    if(lUrlEncodedData) {

        /* just for debug */
        inHttpServer->mURLEncodedData[inHttpServer->mURLEncodedDataLen] = '\0';
        dbg("URL Encoded data: \"%s\"\r\n", inHttpServer->mURLEncodedData);
    }

    /* parse url */
    esp8266_http_parse_url(inHttpServer);

    if(lUrlEncodedData) {

        /* notify parsing start */
        web_content_notify_parsing_start();

        /* process url encoded data */
        web_content_parse_url_encoded_data((char*)inHttpServer->mURLEncodedData, inHttpServer->mURLEncodedDataLen);

        /* notify parsing done */
        web_content_notify_parsing_done();
    }

    /* just for debug */
    inHttpServer->mURL[inHttpServer->mURLLen] = '\0';
    dbg("URL after parse: \"%s\"\r\n", inHttpServer->mURL);

    /* get the content */
    if(web_content_find_file(inHttpServer->mURL, inHttpServer->mURLLen, &lContent)) {

        dbg("found file \"%s\"\r\n", lContent->mFileName);
        lReply = HTTP_REPLY_OK;

    } else {

        dbg("Did not find file\r\n");
        lReply = HTTP_REPLY_NOT_FOUND;

        /* get the 404 page */
        web_content_find_file((uint8_t*)"404.html", 8, &lContent);
    }

    return esp8266_http_send_reply(inHttpServer, lContent, lReply);
}


/*! HTTP POST - Get URL encoded data

    \param[in]  inHttpServer            HTTP context

    \retval true    URL encoded data found
    \retval false   No data found
*/
static bool esp8266_http_post_get_url_encoded_data(ts_http_server * inHttpServer) {

    const char lHeaderFieldContentTypeName[] = "Content-Type";
    const char lHeaderFieldContentTypeURLEncoded[] = "application/x-www-form-urlencoded";
    uint8_t *  lHeaderFieldContentTypeValue;
    size_t     lHeaderFieldContentTypeValueLen;

//    "application/x-www-form-urlencoded"

    if(esp8266_http_get_header_field(inHttpServer, (uint8_t*)lHeaderFieldContentTypeName, sizeof(lHeaderFieldContentTypeName)-1, &lHeaderFieldContentTypeValue, &lHeaderFieldContentTypeValueLen)) {
        /* header field is present */

        /* just for debug */
        uint8_t lChar;

        lChar = lHeaderFieldContentTypeValue[lHeaderFieldContentTypeValueLen];
        lHeaderFieldContentTypeValue[lHeaderFieldContentTypeValueLen] = '\0';
        dbg("Header Field Value is: \"%s\"\r\n", lHeaderFieldContentTypeValue);
        lHeaderFieldContentTypeValue[lHeaderFieldContentTypeValueLen] = lChar;

        if(sizeof(lHeaderFieldContentTypeURLEncoded)-1 == lHeaderFieldContentTypeValueLen && memcmp(lHeaderFieldContentTypeURLEncoded, lHeaderFieldContentTypeValue, lHeaderFieldContentTypeValueLen) == 0) {

            /* just for debug */
            inHttpServer->mBody[inHttpServer->mBodySize] = '\0';
            dbg("Post Data Length: %d\r\n", inHttpServer->mBodySize);
            dbg("Post Data:\r\n\"%s\"\r\n", inHttpServer->mBody);

            inHttpServer->mURLEncodedData    = inHttpServer->mBody;
            inHttpServer->mURLEncodedDataLen = inHttpServer->mBodySize;
            return true;
        } else {
            dbg("Did not match\r\n");
        }
    } else {
        dbg("Header field is not present!\r\n");
    }

    return false;
}


/*! Handle Post

    \param[in]  inHttpServer            HTTP context
*/
static bool esp8266_http_handle_post(ts_http_server * inHttpServer) {

    te_html_reply lReply = HTTP_REPLY_NO_CONTENT;

    /* parse url */
    esp8266_http_parse_url(inHttpServer);

    /* read body */
    if(esp8266_http_read_body(inHttpServer)) {

        if(esp8266_http_post_get_url_encoded_data(inHttpServer)) {

            /* notify parsing start */
            web_content_notify_parsing_start();

            /* process url encoded data */
            web_content_parse_url_encoded_data((char*)inHttpServer->mURLEncodedData, inHttpServer->mURLEncodedDataLen);

            /* notify parsing done */
            web_content_notify_parsing_done();
        } else {
            dbg("%s(%d): Some other content type\r\n", __FILE__, __LINE__);
        }

    } else {
        dbg("%s(%d): no body present\r\n", __FILE__, __LINE__);
    }

#if 0
    /* just for debug */
    inHttpServer->mURL[inHttpServer->mURLLen] = '\0';
    dbg("URL after parse: \"%s\"\r\n", inHttpServer->mURL);

    /* get the content */
    if(web_content_find_file(inHttpServer->mURL, inHttpServer->mURLLen, &lContent)) {

        lReply = HTTP_REPLY_OK;

    } else {
        lReply = HTTP_REPLY_NOT_FOUND;

        /* get the 404 page */
        web_content_find_file((uint8_t*)"404.html", 8, &lContent);
    }
#endif

    return esp8266_http_send_reply(inHttpServer, NULL, lReply);
}



/* eof */

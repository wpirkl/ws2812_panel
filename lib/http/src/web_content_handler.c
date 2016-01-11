
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

#include "web_content.h"
#include "web_content_handler.h"

#define dbg_on(x...)      printf(x)
#define dbg_off(x...)

#define dbg dbg_off

/*! Defines the web content type */
typedef struct {
    /*! Extention of the file */
    const char  * mExtension;

    /*! MIME type definition */
    const char  * mType;
} ts_web_content_type;

/*! Defines parser states */
typedef enum {
    /*! Parser is waiting for < */
    WEB_PARSER_IDLE,
    /*! Parser found < */
    WEB_PARSER_OPEN_BRACKET_FOUND,
    /*! Parser found <! */
    WEB_PARSER_OPEN_BRACKET_ECLAMATION_FOUND,
    /*! Parser found <!- */
    WEB_PARSER_OPEN_BRACKET_ECLAMATION_DASH_FOUND,
    /*! Parser found <!-- */
    WEB_PARSER_OPEN_BRAKCET_ECLAMATION_DASH_DASH_FOUND,
    /*! Parser found <!-- xyz */
    WEB_PARSER_TOKEN_SEARCH_END,
    /*! Parser found <!-- token */
    WEB_PARSER_CLOSE_SEARCH,
    /*! Parser found <!-- token - */
    WEB_PARSER_CLOSE_DASH_FOUND,
    /*! Parser found <!-- token -- */
    WEB_PARSER_CLOSE_DASH_DASH_FOUND,
    /*! Parser found <!-- token --> and is able to process data */
    WEB_PARSER_TOKEN_PROCESS,
} te_web_parser_state;

/*! Stores the web content handlers */
static const ts_web_content_handlers * sWebContentHandlers = NULL;


void web_content_set_handlers(ts_web_content_handlers * const inWebContentHandlers) {

    sWebContentHandlers = inWebContentHandlers;
}


bool web_content_find_file(uint8_t * inFilename, size_t inFilenameLen, const ts_web_content_file ** outWebContent) {

    size_t lCount;

    for(lCount = 0; lCount < gWebContent.mFileCount; lCount++) {

        if(gWebContent.mFiles[lCount].mFilenameLen == inFilenameLen && memcmp(inFilename, gWebContent.mFiles[lCount].mFileName, inFilenameLen) == 0) {

            *outWebContent = &gWebContent.mFiles[lCount];
            return true;
        }
    }

    return false;
}


const ts_web_content_type sWebContentType[] = {
    { "html", "text/html" },
    { "css",  "text/css" },
    { "js",   "text/javascript" },
    { "png",  "image/png" },
    { "jpg",  "image/jpeg" },
    { "gif",  "image/gif" },
    { "txt",  "text/plain" },
//    { "mp3",  "audio/mpeg3" },
//    { "wav",  "audio/wav" },
//    { "flac", "audio/ogg" },
//    { "pdf",  "application/pdf" },
    { "ttf",  "application/x-font-ttf" },
    { "ttc",  "application/x-font-ttf" }
};


const char * web_content_get_type(const ts_web_content_file * inWebContent) {

    size_t lCount;
    size_t lIndex = 0;
    size_t lLength = 0;

    for(lCount = inWebContent->mFilenameLen; lCount > 0; lCount--) {
        if(inWebContent->mFileName[lCount-1] == '.') {
            lIndex = lCount;
            lLength = inWebContent->mFilenameLen - lIndex;
            break;
        }
    }

    if(lLength > 0) {   /* in case we didn't find the point, return text/html */

        for(lCount = 0; lCount < sizeof(sWebContentType) / sizeof(ts_web_content_type); lCount++) {

            if(strlen(sWebContentType[lCount].mExtension) == lLength && memcmp(&inWebContent->mFileName[lIndex], sWebContentType[lCount].mExtension, lLength) == 0) {
                return sWebContentType[lCount].mType;
            }
        }
    }
    return sWebContentType[0].mType;
}


bool web_content_get_token_value(const char * const inToken, size_t inTokenLength, char * outBuffer, size_t inBufferSize, size_t * outBufferLen) {

    size_t lCount;

    /* for debug */
    char lBuffer[32];

    memcpy(lBuffer, inToken, inTokenLength);
    lBuffer[inTokenLength] = '\0';
    dbg("Got token: \"%s\"\r\n", lBuffer);

    if(sWebContentHandlers) {

        for(lCount = 0; lCount < sWebContentHandlers->mHandlerCount; lCount++) {

            size_t lCurrentTokenLen = strlen(sWebContentHandlers->mHandler[lCount].mToken);

            /* token found */
            if(lCurrentTokenLen == inTokenLength && memcmp(sWebContentHandlers->mHandler[lCount].mToken, inToken, inTokenLength) == 0) {

                dbg("Token match!\r\n");

                if(sWebContentHandlers->mHandler[lCount].mGet) {

                    dbg("Handler is not null\r\n");

                    return sWebContentHandlers->mHandler[lCount].mGet(outBuffer, inBufferSize, outBufferLen);
                } else {

                    dbg("Handler is not registered\r\n");

                    /* no handler registered */
                    break;
                }
            }
        }
    }

    return false;
}


bool web_gontent_set_token_value(const char * const inToken, size_t inTokenLength, const char * const inValue, size_t inValueLength) {

    size_t lCount;

    if(sWebContentHandlers) {

        for(lCount = 0; lCount < sWebContentHandlers->mHandlerCount; lCount++) {

            size_t lCurrentTokenLen = strlen(sWebContentHandlers->mHandler[lCount].mToken);

            /* token found */
            if(lCurrentTokenLen == inTokenLength && memcmp(sWebContentHandlers->mHandler[lCount].mToken, inToken, inTokenLength) == 0) {

                if(sWebContentHandlers->mHandler[lCount].mSet) {

                    return sWebContentHandlers->mHandler[lCount].mSet(inValue, inValueLength);
                } else {

                    /* no handler registered */
                    break;
                }
            }
        }
    }

    return false;
}


static size_t web_content_print_missing_output(const ts_web_content_file * inWebContent,
                                               char * outWebContent,
                                               size_t inOutWebContentSize,
                                               size_t inPrintFrom,
                                               size_t inPrintLen,
                                               size_t inOutPointer) {

    size_t lCount;
    size_t lOutPointer = inOutPointer;

    for(lCount = 0; lCount <= inPrintLen; lCount++) {

        if(lOutPointer < inOutWebContentSize) {
            outWebContent[lOutPointer++] = inWebContent->mFile[inPrintFrom + lCount];
        }
    }

    return lOutPointer;
}


bool web_content_prepare_output(const ts_web_content_file * inWebContent, char * outWebContent, size_t inOutWebContentSize, size_t * outWebContentLen) {

    size_t lCount;
    size_t lOutPointer = 0;

    size_t lCommentBegin = 0;

    size_t lTokenBegin = 0;
    size_t lTokenLength = 0;

    size_t lWrittenLength = 0;

    te_web_parser_state lState = WEB_PARSER_IDLE;

    dbg("Parsing\r\n");

    /* iterate over all characters */
    for(lCount = 0; lCount < inWebContent->mFileLength; lCount++) {

        switch(lState) {
            case WEB_PARSER_IDLE:
                if(inWebContent->mFile[lCount] == '<') {
                    lCommentBegin = lCount;
                    lState = WEB_PARSER_OPEN_BRACKET_FOUND;
                    dbg("<\r\n");
                } else {
                    if(lOutPointer < inOutWebContentSize) {
                        outWebContent[lOutPointer++] = inWebContent->mFile[lCount];
                    }
                }
                break;
            case WEB_PARSER_OPEN_BRACKET_FOUND:
                if(inWebContent->mFile[lCount] == '!') {
                    lState = WEB_PARSER_OPEN_BRACKET_ECLAMATION_FOUND;
                    dbg("<!\r\n");
                } else {
                    dbg("IDLE\r\n");
                    lState = WEB_PARSER_IDLE;
                    lOutPointer = web_content_print_missing_output(inWebContent, outWebContent, inOutWebContentSize, lCommentBegin, lCount - lCommentBegin, lOutPointer);
                }
                break;
            case WEB_PARSER_OPEN_BRACKET_ECLAMATION_FOUND:
                if(inWebContent->mFile[lCount] == '-') {
                    dbg("<!-\r\n");
                    lState = WEB_PARSER_OPEN_BRACKET_ECLAMATION_DASH_FOUND;
                } else {
                    dbg("IDLE\r\n");
                    lState = WEB_PARSER_IDLE;
                    lOutPointer = web_content_print_missing_output(inWebContent, outWebContent, inOutWebContentSize, lCommentBegin, lCount - lCommentBegin, lOutPointer);
                }
                break;
            case WEB_PARSER_OPEN_BRACKET_ECLAMATION_DASH_FOUND:
                if(inWebContent->mFile[lCount] == '-') {
                    dbg("<!--\r\n");
                    lState = WEB_PARSER_OPEN_BRAKCET_ECLAMATION_DASH_DASH_FOUND;
                } else {
                    dbg("IDLE\r\n");
                    lState = WEB_PARSER_IDLE;
                    lOutPointer = web_content_print_missing_output(inWebContent, outWebContent, inOutWebContentSize, lCommentBegin, lCount - lCommentBegin, lOutPointer);
                }
                break;
            case WEB_PARSER_OPEN_BRAKCET_ECLAMATION_DASH_DASH_FOUND:
                if(isalnum((int)inWebContent->mFile[lCount])) {
                    dbg("Token begin at: %d\r\n", lCount);
                    lTokenBegin = lCount;
                    lState = WEB_PARSER_TOKEN_SEARCH_END;
                }
                break;
            case WEB_PARSER_TOKEN_SEARCH_END:
                if(!isalnum((int)inWebContent->mFile[lCount])) {
                    lTokenLength = lCount - lTokenBegin;
                    dbg("Token length: %d\r\n", lTokenLength);
                    lState = WEB_PARSER_CLOSE_SEARCH;
                    /* fall through to check for - */
                } else {
                    break;
                }
                /* fall through is wanted */
            case WEB_PARSER_CLOSE_SEARCH:
                if(inWebContent->mFile[lCount] == '-') {
                    dbg("-\r\n");
                    lState = WEB_PARSER_CLOSE_DASH_FOUND;
                } else if(!isspace((int)inWebContent->mFile[lCount])) {
                    dbg("IDLE\r\n");
                    lState = WEB_PARSER_IDLE;
                    lOutPointer = web_content_print_missing_output(inWebContent, outWebContent, inOutWebContentSize, lCommentBegin, lCount - lCommentBegin, lOutPointer);
                }
                break;
            case WEB_PARSER_CLOSE_DASH_FOUND:
                if(inWebContent->mFile[lCount] == '-') {
                    dbg("--\r\n");
                    lState = WEB_PARSER_CLOSE_DASH_DASH_FOUND;
                } else {
                    dbg("IDLE\r\n");
                    lState = WEB_PARSER_IDLE;
                    lOutPointer = web_content_print_missing_output(inWebContent, outWebContent, inOutWebContentSize, lCommentBegin, lCount - lCommentBegin, lOutPointer);
                }
                break;
            case WEB_PARSER_CLOSE_DASH_DASH_FOUND:
                if(inWebContent->mFile[lCount] == '>') {
                    dbg("-->\r\n");
                    lState = WEB_PARSER_TOKEN_PROCESS;
                    /* process */
                } else {
                    dbg("IDLE\r\n");
                    lState = WEB_PARSER_IDLE;
                    lOutPointer = web_content_print_missing_output(inWebContent, outWebContent, inOutWebContentSize, lCommentBegin, lCount - lCommentBegin, lOutPointer);
                    break;
                }
                /* fall through is wanted */
            case WEB_PARSER_TOKEN_PROCESS:
                /* whole token has been found */

                if(web_content_get_token_value(&inWebContent->mFile[lTokenBegin], lTokenLength, &outWebContent[lOutPointer], inOutWebContentSize - lOutPointer, &lWrittenLength)) {

                    lOutPointer += lWrittenLength;
                } else {

                    dbg("Did not find token!\r\n");

                    /* print comment */
                    lOutPointer = web_content_print_missing_output(inWebContent, outWebContent, inOutWebContentSize, lCommentBegin, lCount - lCommentBegin, lOutPointer);
                }
                lState = WEB_PARSER_IDLE;
                break;
        }
    }

    if(lState != WEB_PARSER_IDLE) {
        lOutPointer = web_content_print_missing_output(inWebContent, outWebContent, inOutWebContentSize, lCommentBegin, lCount - lCommentBegin, lOutPointer);
    }

    *outWebContentLen = lOutPointer;

    return true;
}


void web_content_notify_parsing_done(void) {

    if(sWebContentHandlers) {
        if(sWebContentHandlers->mParsingDone) {
            sWebContentHandlers->mParsingDone();
        }
    }
}


bool web_content_parse_url_encoded_data(char * inURLEncodedData, size_t inURLEncodedDataLen) {

    return false;
}

/* eof */

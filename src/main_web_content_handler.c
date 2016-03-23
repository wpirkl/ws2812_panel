#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "web_content_handler.h"

#include "FreeRTOS.h"
#include "semphr.h"

typedef struct {

    /*! Mutex which handles data structure access */
    SemaphoreHandle_t mMutex;

} ts_web_content_data;


/*! Holds user data */
static ts_web_content_data sUserData;


/*! Initialize web content */
bool main_web_content_init(void) {



    return true;
}


/*! Called when parsing starts

*/
static void web_content_start_parse(void * inUserData) {

}


/*! Called when parsing is done

*/
static void web_content_done_parse(void * inUserData) {


}


/*! Get variable ver

    \param[in]  inUserData      User Data pointer
    \param[out] outBuffer       Buffer to be filled by this function
    \param[in]  inBufferSize    Maximum length of data to be put
    \param[out] outBufferLen    Returns the length of the written data

    \retval true    success
    \retval false   failed
*/
static bool web_content_variable_ver_get(void * inUserData, char * outBuffer, size_t inBufferSize, size_t * outBufferLen) {

    *outBufferLen = snprintf(outBuffer, inBufferSize, "1.0");

    return true;
}


/*! Set variable ver

    \param[in]  inUserData      User Data pointer
    \param[in]  inValue         Value to set
    \param[in]  inValueLength   Length of the value

    \retval true    success
    \retval false   failed
*/
static bool web_content_variable_ver_set(void * inUserData, const char * const inValue, size_t inValueLength) {

    char lBuffer[16];
    size_t lCopyLen = (inValueLength > sizeof(lBuffer)-1)? (sizeof(lBuffer) - 1) : inValueLength;

    memcpy(lBuffer, inValue, lCopyLen);
    lBuffer[lCopyLen] = '\0';

    printf("%s(%d): \"%s\"\r\n", __func__, __LINE__, lBuffer);

    return true;
}



const ts_web_content_handlers g_WebContentHandler = {

    .mHandlerCount = 1,
    .mParsingStart = web_content_start_parse,
    .mParsingDone  = web_content_done_parse,
    .mUserData = (void*)&sUserData,
    .mHandler = {
        {
            .mToken = "ver",
            .mGet = web_content_variable_ver_get,
            .mSet = web_content_variable_ver_set,
        },
    }
};

/* eof */

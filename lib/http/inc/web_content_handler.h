#ifndef WEB_CONTENT_HANDLER_H_
#define WEB_CONTENT_HANDLER_H_

#include <stdbool.h>
#include <stdint.h>

#include "web_content.h"



/*! Handler for one token */
typedef struct {

    /*! Token to handle */
    const char * const    mToken;

    /*! Getter */
    bool               (* mGet)(void * inUserData, char * outBuffer, size_t inBufferSize, size_t * outBufferLen);

    /*! Setter */
    bool               (* mSet)(void * inUserData, const char * const inValue, size_t inValueLength);

} ts_web_content_handler;


/*! Web parser structure */
typedef struct {

    /*! Number of web content handlers*/
    size_t                      mHandlerCount;

    /*! Parsing start handler */
    void                     (* mParsingStart)(void * inUserData);

    /*! Parsing done handler */
    void                     (* mParsingDone)(void * inUserData);

    /*! User data */
    void                      * mUserData;

    /*! Web content handlers */
    ts_web_content_handler      mHandler[];

} ts_web_content_handlers;


extern const ts_web_content_handlers g_WebContentHandler __attribute__((weak));


/*! Find a file in web content

    \param[in]  inFilename      The filename to search
    \param[in]  inFilenameLen   The length of the filename
    \param[out] outWebContent   Pointer to a web content structure
*/
bool web_content_find_file(uint8_t * inFilename, size_t inFilenameLen, const ts_web_content_file ** outWebContent);


/*! Get the content type
    
    \param[in]  inWebContent    Pointer to a web content structure

    \return a constant null terminated character sequence
*/
const char * web_content_get_type(const ts_web_content_file * inWebContent);


/*! Get Token

    \param[in]  inToken         Token to find and replace by it's value
    \param[out] outBuffer       The buffer to be filled by this function
    \param[in]  inBufferSize    Size of the buffer to be filled

    \return The value of the token, or an empty string if token is not found
*/
bool web_content_get_token_value(const char * const inToken, size_t inTokenLength, char * outBuffer, size_t inBufferSize, size_t * outBufferLen);


/*! Set Token

    No action will be performed if token is not found

    \param[in]  inToken     Token to find and set it's value
    \param[in]  inValue     Value to set
*/
bool web_gontent_set_token_value(const char * const inToken, size_t inTokenLength, const char * const inValue, size_t inValueLength);


/*! Prepare output

    This function replaces <!-- Token --!> by the token's value

    \param[in]      inWebContent            The raw content to parse
    \param[out]     outWebContent           Place to store outgoing data
    \param[in]      inOutWebContentSize     Maximum space to store outgoing data
    \param[in|out]  inOutOffset             Offset in web content

    \retval true    Still data left to prepare... use chunked transfer
    \retval false   No data left... can use regular transfer
*/
bool web_content_prepare_output(const ts_web_content_file * inWebContent, char * outWebContent, size_t inOutWebContentSize, size_t * outWebContentLen, size_t * inOutOffset);


/*! Notify parsing done

*/
void web_content_notify_parsing_done(void);

/*! Notify parsing start

*/
void web_content_notify_parsing_start(void);


/*! Process URL encoded data

    \param[in]  inURLEncodedData    URL encoded data to parse
    \param[in]  inURLEncodedDataLen Length of the URL encoded data
*/
bool web_content_parse_url_encoded_data(char * inURLEncodedData, size_t inURLEncodedDataLen);


/*! Check if content is cacheable

    \param[in]  inWebContent    Pointer to a web content structure

    \retval true    Cacheable
    \retval false   Not cacheable
*/
bool web_content_is_cachable(const ts_web_content_file * inWebContent);


/*! Check if a file is compressed

    \param[in]  inWebContent    Pointer to a web content structure

    \retval true    Compressed
    \retval false   Not compressed
*/
bool web_content_is_compressed(const ts_web_content_file * inWebContent);


#endif /* WEB_CONTENT_HANDLER_H_ */

/* eof */

#ifndef WEB_CONTENT_HANDLER_H_
#define WEB_CONTENT_HANDLER_H_

#include <stdbool.h>
#include <stdint.h>


/*! Find a file in web content

    \param[in]  inFilename      The filename to search
    \param[in]  inFilenameLen   The length of the filename
    \param[out] outWebContent   Pointer to a web content structure
*/
bool web_content_find_file(uint8_t * inFilename, size_t inFilenameLen, const ts_web_content_file ** outWebContent);



#endif /* WEB_CONTENT_HANDLER_H_ */

/* eof */

#ifndef WEB_CONTENT_H_
#define WEB_CONTENT_H_

#include <stddef.h>


typedef struct {

    /*! Length of the filename */
    const size_t            mFilenameLen;

    /*! Length of the file */
    const size_t            mFileLength;

    /*! The file name */
    const char * const      mFileName;

    /*! The file content */
    const char * const      mFile;

} ts_web_content_file;


typedef struct {

    /*! Number of files */
    size_t              mFileCount;

    /*! Files */
    ts_web_content_file mFiles[];

} ts_web_content;

extern const ts_web_content gWebContent;


#endif /* WEB_CONTENT_H_ */

/* eof */

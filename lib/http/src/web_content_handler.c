
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "web_content.h"
#include "web_content_handler.h"



bool web_content_find_file(uint8_t * inFilename, size_t inFilenameLen, const ts_web_content_file ** outWebContent) {

    size_t lCount;

    for(lCount = 0; lCount < gWebContent.mFileCount; lCount++) {

        size_t lFilenameLength = strlen(gWebContent.mFiles[lCount].mFileName);

        if(lFilenameLength == inFilenameLen && memcmp(inFilename, gWebContent.mFiles[lCount].mFileName, inFilenameLen) == 0) {

            *outWebContent = &gWebContent.mFiles[lCount];
            return true;
        }
    }

    return false;
}



/* eof */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>

void encode_x(FILE * inFile, uint8_t inCharacter) {

    fprintf(inFile, "\\x%02x", inCharacter);
}

int main(int argc, char * argv[]) {

    FILE * lOutputFile;
    int lCount;

    if(argc < 2) {
        printf("Output file missing!\n");
        return -1;
    }

    lOutputFile = fopen(argv[1], "w");

    if(!lOutputFile) {
        perror ("Could not open output file");
        return -1;
    }

    fprintf(lOutputFile, "#include \"web_content.h\"\n");
    fprintf(lOutputFile, "\n");
    fprintf(lOutputFile, "const ts_web_content gWebContent = {\n");
    fprintf(lOutputFile, "    .mFileCount = %d,\n", argc - 2);
    fprintf(lOutputFile, "    .mFiles = {\n");

    for(lCount = 2; lCount < argc; lCount++) {

        FILE    * lInputFile;
        size_t    lFileLength;
        size_t    lFileNameLength;
        uint8_t * lFileBuffer;

        size_t lRead;
        size_t lChunk;
        size_t lIndex = 0;
        size_t lFileIndex;
        uint8_t lBuffer[16];

        struct stat statbuf;

        printf("Processing file \"%s\"\n", argv[lCount]);

        if(stat(argv[lCount], &statbuf) == 0) {

            if(S_ISDIR(statbuf.st_mode)) {
                printf("Skipping directory\r\n");
                continue;
            }

        } else {
            printf("Skipping this file (fstat didn't work)\n");
            continue;
        }

        lInputFile = fopen(argv[lCount], "r");
        if(!lInputFile) {
            printf("Could not open input file: %s\n", argv[lCount]);
            continue;
        }


        lFileNameLength = strlen(argv[lCount]);

        fseek(lInputFile, 0, SEEK_END);
        lFileLength = ftell(lInputFile);
        fseek(lInputFile, 0, SEEK_SET);

        printf("File length is: %d\n", lFileLength);
        printf("File name length is: %d\n", lFileNameLength);

        fprintf(lOutputFile, "        { /* %s */\n", argv[lCount]);
        fprintf(lOutputFile, "            .mFilenameLen = %d,\n", lFileNameLength);
        fprintf(lOutputFile, "            .mFileLength = %d,\n",  lFileLength);
        fprintf(lOutputFile, "            .mFileName = \"");

        printf("Encoding filename\n");
        for(lIndex = 0; lIndex < lFileNameLength; lIndex++) {
            encode_x(lOutputFile, (uint8_t)argv[lCount][lIndex]);
        }
        fprintf(lOutputFile, "\",\n");

        fprintf(lOutputFile, "            .mFile = \n");
        for(lFileIndex = 0; lFileIndex < lFileLength; ) {

            if(lFileLength - lFileIndex > 16) {
                lChunk = 16;
            } else {
                lChunk = lFileLength - lFileIndex;
            }

            printf("Reading %d bytes from file\r\n", lChunk);
            lRead = fread(lBuffer, 1, lChunk, lInputFile);
            if(lRead != lChunk) {
                perror("Error reading from file");
                return -1;
            }

            fprintf(lOutputFile, "                \"");
            for(lIndex = 0; lIndex < lChunk; lIndex++) {

                encode_x(lOutputFile, lBuffer[lIndex]);
            }
            fprintf(lOutputFile, "\"\n");

            lFileIndex += lChunk;
        }

        if(lCount + 1 < argc) {
            fprintf(lOutputFile, "        },\n");
        } else {
            fprintf(lOutputFile, "        }\n");
        }

        fclose(lInputFile);
    }

    fprintf(lOutputFile, "    }\n");
    fprintf(lOutputFile, "};\n");
    fprintf(lOutputFile, "\n");
    fprintf(lOutputFile, "/* eof */\n");

    fclose(lOutputFile);

    return 0;
}



/* eof */

#ifndef DEPRESSORFILE_H
#define DEPRESSORFILE_H

#include <stdint.h>
#include <stdio.h>
#include <lzma.h>

// Structure to manage resources
typedef struct {
    FILE* inputFile;
    FILE* outputFile;
    char* extension;
    char* fullOutputPath;
    uint8_t* inputBuffer;
    uint8_t* outputBuffer;
    lzma_stream* strm;
} ResourceManager;

// Function declarations
void cleanupResources(ResourceManager* resources);
void decompressFileWithExtension(const char* inputFilePath, const char* outputFilePath, int numThreads);

#endif // DEPRESSORFILE_H

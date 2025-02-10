#include "depressorfile.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <lzma.h>

// Cleanup function
void cleanupResources(ResourceManager* resources) {
    if (resources->inputFile) fclose(resources->inputFile);
    if (resources->outputFile) fclose(resources->outputFile);
    if (resources->extension) free(resources->extension);
    if (resources->fullOutputPath) free(resources->fullOutputPath);
    if (resources->inputBuffer) free(resources->inputBuffer);
    if (resources->outputBuffer) free(resources->outputBuffer);
    if (resources->strm) {
        lzma_end(resources->strm);
        free(resources->strm);
    }
}

// Main decompression function
void decompressFileWithExtension(const char* inputFilePath, const char* outputFilePath, int numThreads) {
    ResourceManager resources = {0}; // Initialize all members to NULL or 0
    resources.strm = (lzma_stream*)malloc(sizeof(lzma_stream));
    if (!resources.strm) {
        fprintf(stderr, "Memory allocation failed for lzma_stream.\n");
        return;
    }
    *resources.strm = LZMA_STREAM_INIT;

    // Open the input file
    resources.inputFile = fopen(inputFilePath, "rb");
    if (!resources.inputFile) {
        fprintf(stderr, "Failed to open input file: %s\n", inputFilePath);
        cleanupResources(&resources);
        return;
    }

    // Read the extension length and extension
    uint8_t extLength;
    if (fread(&extLength, sizeof(extLength), 1, resources.inputFile) != 1) {
        fprintf(stderr, "Failed to read extension length.\n");
        cleanupResources(&resources);
        return;
    }

    resources.extension = (char*)malloc(extLength + 1);
    if (!resources.extension) {
        fprintf(stderr, "Memory allocation failed for extension.\n");
        cleanupResources(&resources);
        return;
    }

    if (fread(resources.extension, 1, extLength, resources.inputFile) != extLength) {
        fprintf(stderr, "Failed to read extension.\n");
        cleanupResources(&resources);
        return;
    }
    resources.extension[extLength] = '\0';

    // Set up the decompression with threading options
    lzma_mt mt_options = {
        .threads = static_cast<uint32_t>(numThreads),  // Explicit cast to uint32_t
        .block_size = 0,
        .preset = 6, // Default compression level
    };

    if (lzma_stream_decoder(resources.strm, UINT64_MAX, LZMA_CONCATENATED) != LZMA_OK) {
        fprintf(stderr, "lzma_stream_decoder failed!\n");
        cleanupResources(&resources);
        return;
    }

    // Allocate buffers
    size_t bufferSize = 1024;
    resources.inputBuffer = (uint8_t*)malloc(bufferSize);
    resources.outputBuffer = (uint8_t*)malloc(bufferSize);
    if (!resources.inputBuffer || !resources.outputBuffer) {
        fprintf(stderr, "Memory allocation failed for buffers.\n");
        cleanupResources(&resources);
        return;
    }

    resources.strm->next_out = resources.outputBuffer;
    resources.strm->avail_out = bufferSize;

    // Construct the output file path with extension
    size_t pathLength = strlen(outputFilePath) + strlen(resources.extension) + 2;
    resources.fullOutputPath = (char*)malloc(pathLength);
    if (!resources.fullOutputPath) {
        fprintf(stderr, "Memory allocation failed for output file path.\n");
        cleanupResources(&resources);
        return;
    }
    snprintf(resources.fullOutputPath, pathLength, "%s.%s", outputFilePath, resources.extension);

    // Open the output file
    resources.outputFile = fopen(resources.fullOutputPath, "wb");
    if (!resources.outputFile) {
        fprintf(stderr, "Failed to open output file: %s\n", resources.fullOutputPath);
        cleanupResources(&resources);
        return;
    }

    // Decompress the data
    while (!feof(resources.inputFile)) {
        size_t bytesRead = fread(resources.inputBuffer, 1, bufferSize, resources.inputFile);
        resources.strm->next_in = resources.inputBuffer;
        resources.strm->avail_in = bytesRead;

        while (resources.strm->avail_in > 0 || resources.strm->avail_out == 0) {
            lzma_ret ret = lzma_code(resources.strm, LZMA_RUN);
            if (ret == LZMA_STREAM_END) break;
            if (ret != LZMA_OK) {
                fprintf(stderr, "Decompression failed! Error code: %d\n", ret);
                cleanupResources(&resources);
                return;
            }
            if (resources.strm->avail_out == 0) {
                fwrite(resources.outputBuffer, 1, bufferSize, resources.outputFile);
                resources.strm->next_out = resources.outputBuffer;
                resources.strm->avail_out = bufferSize;
            }
        }
    }

    // Write remaining data
    if (resources.strm->avail_out < bufferSize) {
        fwrite(resources.outputBuffer, 1, bufferSize - resources.strm->avail_out, resources.outputFile);
    }

    printf("File decompressed with restored extension: %s\n", resources.fullOutputPath);

    // Clean up
    cleanupResources(&resources);
}

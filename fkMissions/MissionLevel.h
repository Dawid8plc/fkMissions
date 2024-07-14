typedef struct IUnknown IUnknown;

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include <Windows.h> // For Bitmap handling

#include "zlc/zlibcomplete.hpp"

using namespace zlibcomplete;
using namespace std;

const int gzipSignature = 559903;

class MissionLevel {
public:
    // Fields
    unsigned char terrainType1;
    unsigned char terrainType2;
    int terrainGenerationSeed;
    int objectGenerationSeed;
    int BaseLineNoise;
    int NoiseRandomization;
    int ObjectsCount;
    string terrainSeed;
    string objectSeed;
    string levelName;
    unsigned char edited;
    string levelStyle;
    string waterColor;
    unsigned short width;
    unsigned short height;
    vector<unsigned char> map;

    // Constructors
    MissionLevel() {}
    MissionLevel(const string& path) {
        Read(path);
    }

    MissionLevel(ifstream& stream) {
        Read(stream);
    }

    std::streamsize getFileSize(std::istream& file) {
        // Save the current position
        std::streampos currentPos = file.tellg();

        // Seek to the end to get the file size
        file.seekg(0, std::ios::end);
        std::streamsize fileSize = file.tellg();

        // Seek back to the original position
        file.seekg(currentPos);

        return fileSize;
    }

    // Methods
    void Read(const string& path) {
        ifstream stream(path, ios::binary);
        if (!stream.is_open()) {
            cerr << "Error opening file: " << path << endl;
            return;
        }

        Read(stream);

        stream.close();
    }

    void Read(istream& stream) {
        stream.seekg(0, ios::beg); // Ensure we start reading from the beginning of the file

        int gzipSig;
        stream.read(reinterpret_cast<char*>(&gzipSig), sizeof(gzipSig));

        if (gzipSig == gzipSignature) {
            stream.seekg(0, ios::beg);

            std::stringstream buffer;
            buffer << stream.rdbuf();

            int readBytes = 0;
            GZipDecompressor decompressor;

            string input(buffer.str(), readBytes);
            string output = decompressor.decompress(input);

            std::istringstream decompressedStream(output);
            Read(decompressedStream);
            return;
        }

        stream.seekg(0, ios::beg); // Reset to the beginning of the file

        // Read fields
        stream.read(reinterpret_cast<char*>(&terrainType1), sizeof(terrainType1));
        stream.read(reinterpret_cast<char*>(&terrainType2), sizeof(terrainType2));
        stream.read(reinterpret_cast<char*>(&terrainGenerationSeed), sizeof(terrainGenerationSeed));
        stream.read(reinterpret_cast<char*>(&objectGenerationSeed), sizeof(objectGenerationSeed));
        stream.read(reinterpret_cast<char*>(&BaseLineNoise), sizeof(BaseLineNoise));
        stream.read(reinterpret_cast<char*>(&NoiseRandomization), sizeof(NoiseRandomization));
        stream.read(reinterpret_cast<char*>(&ObjectsCount), sizeof(ObjectsCount));

        terrainSeed = ReadPascalString(stream);
        objectSeed = ReadPascalString(stream);
        levelName = ReadPascalString(stream);

        stream.read(reinterpret_cast<char*>(&edited), sizeof(edited));

        levelStyle = ReadPascalString(stream);
        waterColor = ReadPascalString(stream);

        if (edited == 0x00) {
            // Read map data
            stream.seekg(12, ios::cur); // Skip over remaining portion sizes
            
            long read = 0;

            while (stream.tellg() != -1 && stream.peek() != EOF) {
                unsigned char interpreter;
                stream.read(reinterpret_cast<char*>(&interpreter), sizeof(interpreter));
                read++;

                // If the byte is under 0x80, the number given will be the number of bytes interpreted.
                // These will be used to draw the map.
                if (interpreter < 0x80) {
                    for (int i = 0; i < interpreter; i++) {
                        unsigned char value;
                        stream.read(reinterpret_cast<char*>(&value), sizeof(value));
                        map.push_back(value);
                        read++;
                    }
                }
                // If the byte is over 0x80, the number given - 0x80 will be the number of times the next one byte is repeated.
                else if (interpreter > 0x80) {
                    unsigned char repeated;
                    stream.read(reinterpret_cast<char*>(&repeated), sizeof(repeated));
                    read++;

                    for (int i = 0; i < interpreter - 0x80; i++) {
                        map.push_back(repeated);
                    }
                }
            }
        }
        else {
            // Read width (2 bytes, little-endian)
            stream.read(reinterpret_cast<char*>(&width), sizeof(width));

            // Read height (2 bytes, little-endian)
            stream.read(reinterpret_cast<char*>(&height), sizeof(height));

            // Read the remaining data into the map vector
            while (!stream.eof()) {
                unsigned char byte;
                stream.read(reinterpret_cast<char*>(&byte), sizeof(byte));
                if (stream.gcount() > 0) { // Ensure a byte was read
                    map.push_back(byte);
                }
            }
        }
    }

    // Helper function to flip the bits in a byte
    BYTE ReverseBits(BYTE b)
    {
        BYTE result = 0;
        for (int i = 0; i < 8; i++)
        {
            result = (result << 1) | ((b >> i) & 1);
        }
        return result;
    }

    // Helper function to create a 1bpp indexed bitmap
    HBITMAP Create1bppBitmap(const BYTE* pixelData, int width, int height, bool flipBits = true)
    {
        // Create a 1bpp indexed bitmap
        // Allocate BITMAPINFO structure
        BITMAPINFO* bmi = (BITMAPINFO*) new BYTE[sizeof(BITMAPINFOHEADER) + 2 * sizeof(RGBQUAD)];
        memset(bmi, 0, sizeof(BITMAPINFOHEADER) + 2 * sizeof(RGBQUAD));
        bmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi->bmiHeader.biWidth = width;
        bmi->bmiHeader.biHeight = -height; // top-down bitmap
        bmi->bmiHeader.biPlanes = 1;
        bmi->bmiHeader.biBitCount = 1;
        bmi->bmiHeader.biCompression = BI_RGB;

        // Define color palette
        bmi->bmiColors[0].rgbBlue = 255;
        bmi->bmiColors[0].rgbGreen = 160;
        bmi->bmiColors[0].rgbRed = 0;
        bmi->bmiColors[0].rgbReserved = 0;

        bmi->bmiColors[1].rgbBlue = 0;
        bmi->bmiColors[1].rgbGreen = 60;
        bmi->bmiColors[1].rgbRed = 102;
        bmi->bmiColors[1].rgbReserved = 0;

        // Create bitmap handle
        HDC hdc = GetDC(NULL);
        void* bits;
        HBITMAP hBitmap = CreateDIBSection(hdc, bmi, DIB_RGB_COLORS, &bits, NULL, 0);
        ReleaseDC(NULL, hdc);
            
        // Copy pixel data and flip bits if needed
        if (flipBits)
        {
            std::vector<BYTE> flippedData(width * height);
            for (int i = 0; i < width * height; i++)
            {
                flippedData[i] = ReverseBits(pixelData[i]);
            }
            memcpy(bits, flippedData.data(), width * height / 8);
        }
        else
        {
            memcpy(bits, pixelData, width * height / 8);
        }

        return hBitmap;
    }

    // Function to convert vector<unsigned char> to BYTE array
    BYTE* VectorToByteArray(const std::vector<unsigned char>& vec)
    {
        BYTE* arr = new BYTE[vec.size()];
        std::copy(vec.begin(), vec.end(), arr);
        return arr;
    }

    // Main function to convert vector<unsigned char> to HBITMAP
    HBITMAP ToBitmap()
    {
        // Seeded case
        if (edited == 0x00)
        {
            return Create1bppBitmap(VectorToByteArray(map), 160, 58, false);
        }
        // Edited case
        else
        {
            // Create 1bpp bitmap with custom palette
            HBITMAP hBitmap = Create1bppBitmap(VectorToByteArray(map), 1920, 696, false);

            return hBitmap;
        }
    }


private:
    string ReadPascalString(istream& stream) {
        unsigned char size;
        stream.read(reinterpret_cast<char*>(&size), sizeof(size));

        string text;
        text.resize(size);
        stream.read(&text[0], size);

        return text;
    }
};
#include <iostream>
#include <fstream>
#include <vector>
#include <thread>
#include <mutex>
#include <string>
using namespace std;

mutex mtx; // For synchronized console output

// Simple RLE compression
string compressChunk(const string &data) {
    string result;
    int n = data.size();
    for (int i = 0; i < n; ) {
        char ch = data[i];
        int count = 1;
        while (i + count < n && data[i + count] == ch && count < 255) {
            count++;
        }
        result.push_back(ch);
        result.push_back((char)count);
        i += count;
    }
    return result;
}

// Simple RLE decompression
string decompressChunk(const string &data) {
    string result;
    int n = data.size();
    for (int i = 0; i < n; i += 2) {
        char ch = data[i];
        unsigned char count = (unsigned char)data[i + 1];
        result.append(count, ch);
    }
    return result;
}

// Multithreaded compression function
void compressFile(const string &inputFile, const string &outputFile, int numThreads) {
    ifstream in(inputFile, ios::binary);
    ofstream out(outputFile, ios::binary);
    if (!in || !out) {
        cout << "File error!" << endl;
        return;
    }

    const size_t chunkSize = 1024 * 1024; // 1 MB
    vector<string> chunks;
    string buffer(chunkSize, '\0');

    while (in.read(&buffer[0], chunkSize) || in.gcount() > 0) {
        chunks.push_back(buffer.substr(0, in.gcount()));
    }

    vector<string> compressedChunks(chunks.size());
    vector<thread> threads;
    size_t chunkPerThread = (chunks.size() + numThreads - 1) / numThreads;

    for (int t = 0; t < numThreads; t++) {
        threads.push_back(thread([t, &chunks, &compressedChunks, chunkPerThread]() {
            size_t start = t * chunkPerThread;
            size_t end = min(start + chunkPerThread, chunks.size());
            for (size_t i = start; i < end; i++) {
                compressedChunks[i] = compressChunk(chunks[i]);
            }
        }));
    }

    for (auto &th : threads) th.join();

    for (auto &chunk : compressedChunks) {
        size_t size = chunk.size();
        out.write(reinterpret_cast<char*>(&size), sizeof(size));
        out.write(chunk.data(), size);
    }

    cout << "Compression completed using " << numThreads << " threads.\n";
}

// Multithreaded decompression function
void decompressFile(const string &inputFile, const string &outputFile, int numThreads) {
    ifstream in(inputFile, ios::binary);
    ofstream out(outputFile, ios::binary);
    if (!in || !out) {
        cout << "File error!" << endl;
        return;
    }

    vector<string> compressedChunks;
    while (true) {
        size_t size;
        if (!in.read(reinterpret_cast<char*>(&size), sizeof(size))) break;
        string chunk(size, '\0');
        in.read(&chunk[0], size);
        compressedChunks.push_back(chunk);
    }

    vector<string> decompressedChunks(compressedChunks.size());
    vector<thread> threads;
    size_t chunkPerThread = (compressedChunks.size() + numThreads - 1) / numThreads;

    for (int t = 0; t < numThreads; t++) {
        threads.push_back(thread([t, &compressedChunks, &decompressedChunks, chunkPerThread]() {
            size_t start = t * chunkPerThread;
            size_t end = min(start + chunkPerThread, compressedChunks.size());
            for (size_t i = start; i < end; i++) {
                decompressedChunks[i] = decompressChunk(compressedChunks[i]);
            }
        }));
    }

    for (auto &th : threads) th.join();

    for (auto &chunk : decompressedChunks) {
        out.write(chunk.data(), chunk.size());
    }

    cout << "Decompression completed using " << numThreads << " threads.\n";
}

int main() {
    int choice;
    cout << "1. Compress File\n2. Decompress File\nEnter choice: ";
    cin >> choice;

    string inputFile, outputFile;
    cout << "Enter input file name: ";
    cin >> inputFile;
    cout << "Enter output file name: ";
    cin >> outputFile;

    int threads;
    cout << "Enter number of threads: ";
    cin >> threads;

    if (choice == 1) {
        compressFile(inputFile, outputFile, threads);
    } else if (choice == 2) {
        decompressFile(inputFile, outputFile, threads);
    } else {
        cout << "Invalid choice!\n";
    }

    return 0;
}

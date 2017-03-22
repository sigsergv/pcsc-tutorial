/*
 * Copyright (c) 2017, Sergey Stolyarov <sergei@regolit.com>
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the <organization> nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file dump-atr.cpp
 * @author Sergey Stolyarov <sergei@regolit.com>
 *
 * Scan card using provided file with keys and print scanned
 * data to standard output.
 */

#include <xpcsc.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <cstring>
#include <cstdlib>
#include <algorithm>

#include "debug.hpp"

typedef enum {KeyA, KeyB, KeyNone} KeyType;
typedef unsigned char Byte6[6];

struct AccessBits {
    unsigned char C1 : 1, C2 : 1, C3 :1;
    bool is_set;
    AccessBits() : is_set(false) {};
};

struct SectorKeys {
    Byte6 key_A;
    bool Key_A_set;
    Byte6 key_B;
    bool Key_B_set;
    KeyType key_use[4];
};
typedef std::map<unsigned int, SectorKeys> Keys;

struct CardContents {
    unsigned char blocks_data[64][16];
    unsigned char blocks_keys[64][6];

    KeyType blocks_key_types[64];
    AccessBits blocks_access_bits[64];
};


void help(const std::string & program)
{
    std::cout << "Usage:\n"
        "    " << program << " [-h] [-f KEYS]";
    std::cout << std::endl;
}

#define error(msg) do { std::cerr << msg << std::endl; } while (0);

std::string arg_keys(const xpcsc::Strings args)
{
    xpcsc::Strings::const_iterator p = std::find(args.begin(), args.end(), "-f");
    std::string filename;

    if (p == args.end()) {
        return filename;
    }

    p++;

    if (p == args.end()) {
        return filename;
    }

    filename = *p;

    return filename;
}

unsigned char char2val(char c, bool &ok)
{
    ok = true;

    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }

    ok = false;
    return 0;
}

bool parse_hex(const std::string & str, Byte6 data)
{
    bool ok;
    size_t pos = 0;

    for (std::string::const_iterator i=str.begin(); i!=str.end(); i++) {
        unsigned char c1 = char2val(*i, ok);
        if (!ok) {
            return false;
        }
        i++;
        if (i == str.end()) {
            return false;
        }
        unsigned char c2 = char2val(*i, ok);
        if (!ok) {
            return false;
        }
        data[pos] = c1*16 + c2;
        pos++;
    }

    return true;
}

bool read_keys(std::ifstream & file, Keys & keys)
{
    keys.clear();

    std::string line;
    char s_buf[40];
    while (std::getline(file, line)) {
        if (line.length() == 0) {
            continue;
        }
        if (line.at(0) == '#') {
            continue;
        }
        // transform to lower case
        std::transform(line.begin(), line.end(), line.begin(), ::tolower);
        // tokenize
        std::vector<std::string> tokens;
        const char delim[] = ":";

        strncpy(s_buf, line.c_str(), 35);

        for (char * i = strtok(&s_buf[0], delim); i != 0; i = strtok(0, delim)) {
            tokens.push_back(i);
        }

        if (tokens.size() != 4) {
            error("Invalid sector key line format:");
            error(line);
            return false;
        }

        // extract data from tokens

        // sector number
        char * endptr;
        long sector_num = strtol(tokens[0].c_str(), &endptr, 10);
        if (sector_num == 0 && strlen(endptr) != 0) {
            error("Invalid sector key line format [sector number invalid]:");
            error(line);
            return false;
        }

        if (sector_num < 0) {
            error("Invalid sector key line format [sector number must be positive]:");
            error(line);
            return false;
        }

        SectorKeys sks;

        // key A
        bool key_A_missing = false;
        if (tokens[1].length() != 12 && tokens[1].length() != 1) {
            error("Invalid sector key line format [KeyA length]:");
            error(line);
            return false;
        }

        if (tokens[1].compare("-") == 0) {
            key_A_missing = true;
        } else {
            if (!parse_hex(tokens[1], sks.key_A)) {
                error("Invalid sector key line format [KeyA format]:");
                error(line);
                return false;
            }
        }
        sks.Key_A_set = !key_A_missing;

        // key B
        bool key_B_missing = false;
        if (tokens[2].length() != 12 && tokens[2].length() != 1) {
            error("Invalid sector key line format [KeyB length]:");
            error(line);
            return false;
        }

        if (tokens[2].compare("-") == 0) {
            key_B_missing = true;
        } else {
            if (!parse_hex(tokens[2], sks.key_B)) {
                error("Invalid sector key line format [KeyB format]:");
                error(line);
                return false;
            }
        }
        sks.Key_B_set = !key_B_missing;

        // key use
        if (tokens[3].length() != 4) {
            error("Invalid sector key line format [key use]:");
            error(tokens[3]);
            return false;
        }
        // each character must be either 'a' or 'b'
        bool found_key_use_a = false;
        bool found_key_use_b = false;
        for (size_t i=0; i < 4; i++) {
            if (tokens[3][i] == 'a') {
                found_key_use_a = true;
                sks.key_use[i] = KeyA;
                continue;
            }
            if (tokens[3][i] == 'b') {
                found_key_use_b = true;
                sks.key_use[i] = KeyB;
                continue;
            }
            error("Invalid sector key line format [key use]: a and b required");
            return false;
        }

        if (key_A_missing && found_key_use_a) {
            error("Invalid sector key line format [key use]: you cannot use 'a' here");
            error(line);
            return false;
        }
        if (key_B_missing && found_key_use_b) {
            error("Invalid sector key line format [key use]: you cannot use 'b' here");
            error(line);
            return false;
        }

        // everything looks fine here,
        keys[sector_num] = sks;
        // PRINT_DEBUG(tokens.size());
    }
    return true;
}

int main(int argc, char **argv)
{
    if (argc == 1) {
        help(argv[0]);
        return 0;
    }

    // first parse command line arguments
    xpcsc::Strings args(argc);
    for (size_t i=0; i < argc; i++) {
        args.push_back(argv[i]);
    }

    if (std::find(args.begin(), args.end(), "-h") != args.end()) {
        help(args[0]);
        return 0;
    }

    std::string keys_file = arg_keys(args);

    if (keys_file.length() == 0) {
        error("-f argument is required!");
        return 1;
    }

    // open file "keys_file" and read keys from there
    std::ifstream file;
    file.open(keys_file);

    if (file.fail()) {
        error("Cannot open keys file!");
        return 1;
    }

    Keys keys;
    if (!read_keys(file, keys)) {
        return 1;
    }

    xpcsc::Connection c;

    try {
        c.init();
    } catch (xpcsc::PCSCError &e) {
        std::cerr << "Connection to PC/SC failed: " << e.what() << std::endl;
        return 1;
    }

    // get readers list
    std::vector<std::string> readers = c.readers();
    if (readers.size() == 0) {
        std::cerr << "[E] No connected readers" << std::endl;
        return 1;
    }

    // connect to reader
    try {
        std::string reader = *readers.begin();
        std::cout << "Found reader: " << reader << std::endl;
        c.wait_for_card(reader);
    } catch (xpcsc::PCSCError &e) {
        std::cerr << "Wait for card failed: " << e.what() << std::endl;
        return 1;
    }

    // fetch and parse ATR
    xpcsc::Bytes atr = c.atr();
    xpcsc::ATRParser p;
    p.load(atr);

    if (!p.checkFeature(xpcsc::ATR_FEATURE_PICC)
        || !(p.checkFeature(xpcsc::ATR_FEATURE_MIFARE_1K)
            || p.checkFeature(xpcsc::ATR_FEATURE_INFINEON_SLE_66R35) )
    ) {
        std::cerr << "Not compatible card!" << std::endl;
        return 1;
    }

    // Mifare 1K-compatible card, read 16 sectors/64 blocks
    // template for Load Keys command
    unsigned char cmd_load_keys[] = {xpcsc::CLA_PICC, xpcsc::INS_MIFARE_LOAD_KEYS, 0x00, 0x00, 
        0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    // template for General Auth command
    unsigned char cmd_general_auth[] = {xpcsc::CLA_PICC, xpcsc::INS_MIFARE_GENERAL_AUTH, 0x00, 0x00, 
        0x05, 0x01, 0x00, 0x00, 0x60, 0x00};

    // template for Read Binary command
    unsigned char cmd_read_binary[] = {xpcsc::CLA_PICC, xpcsc::INS_MIFARE_READ_BINARY, 0x00, 0x00, 0x10};

    xpcsc::Bytes command;
    xpcsc::Bytes response;

    CardContents card;

    // walk through all sectors and try to read data
    for (size_t sector = 0; sector < 16; sector++) {
        Keys::const_iterator ps = keys.find(sector);

        if (ps == keys.end()) {
            error("Undefined sector keys: " << sector);
            return 1;
        }

        const SectorKeys & sector_keys = ps->second;

        if (sector_keys.Key_A_set) {
            
        }
    }

    return 0;
}

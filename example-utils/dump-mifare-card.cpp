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

#define CHECK_BIT(value, b) (((value) >> (b))&1)

typedef enum {KeyA, KeyB, KeyNone} KeyType;
typedef unsigned char Byte6[6];

/*
 * This structure represents one sector keys:
 *   - Key A and Key B (presense and data)
 *   - which blocks are read by key A
 *   - which blocks are read by key B
 */
struct SectorKeys {
    Byte6 key_A;
    char key_A_str[13];
    Byte6 key_B;
    char key_B_str[13];

    size_t key_A_blocks_size;
    int key_A_blocks[4];

    size_t key_B_blocks_size;
    int key_B_blocks[4];
};

/*
 * This data structure maps sector numbers to SectorKeys structs
 */
typedef std::map<unsigned int, SectorKeys> Keys;


/*
 * This data structure represents one block: sector number, used key type, 16 bytes of data, access bits
 */
struct Block {
    size_t sector;
    KeyType key_type;
    unsigned char data[16];

    // access bits
    bool is_access_set;
    unsigned char C1 : 1, C2 : 1, C3 :1;
};


typedef std::vector<Block> CardContents;


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
            strncpy(sks.key_A_str, tokens[1].c_str(), 13);
        }

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
            strncpy(sks.key_B_str, tokens[2].c_str(), 13);
        }

        // key use
        if (tokens[3].length() != 4) {
            error("Invalid sector key line format [key use]:");
            error(tokens[3]);
            return false;
        }
        // each character must be either 'a' or 'b'
        bool found_key_use_a = false;
        bool found_key_use_b = false;

        sks.key_A_blocks_size = 0;
        sks.key_B_blocks_size = 0;
        for (size_t i=0; i < 4; i++) {
            // blocks
            if (tokens[3][i] == 'a') {
                found_key_use_a = true;
                sks.key_A_blocks[sks.key_A_blocks_size] = i;
                sks.key_A_blocks_size++;
                continue;
            }
            if (tokens[3][i] == 'b') {
                found_key_use_b = true;
                sks.key_B_blocks[sks.key_B_blocks_size] = i;
                sks.key_B_blocks_size++;
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

bool read_mifare_1k(xpcsc::Connection & c, const Keys & keys, CardContents & card)
{

    unsigned char cmd_load_keys[] = {xpcsc::CLA_PICC, xpcsc::INS_MIFARE_LOAD_KEYS, 0x00, 0x00, 
        0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    // template for General Auth command
    unsigned char cmd_general_auth[] = {xpcsc::CLA_PICC, xpcsc::INS_MIFARE_GENERAL_AUTH, 0x00, 0x00, 
        0x05, 0x01, 0x00, 0x00, 0x60, 0x00};

    // template for Read Binary command
    unsigned char cmd_read_binary[] = {xpcsc::CLA_PICC, xpcsc::INS_MIFARE_READ_BINARY, 0x00, 0x00, 0x10};

    // 16 sectors, 4 blocks each
    xpcsc::Bytes command;
    xpcsc::Bytes response;

    for (size_t sector = 0; sector < 16; sector++) {
        const size_t first_block = sector * 4;

        // initialize block with default data
        for (size_t i=0; i < 4; i++) {
            Block & b = card[first_block+i];
            b.sector = sector;
            b.key_type = KeyNone;
            b.is_access_set = false;
        }

        Keys::const_iterator ps = keys.find(sector);
        if (ps == keys.end()) {
            // no key, all 4 blocks are not possible to read
            continue;  // next sector
        }

        const SectorKeys & sector_keys = ps->second;

        // Keys A first
        if (sector_keys.key_A_blocks_size > 0) {
            // there are  Key A blocks so authenticate as Key A
            memcpy(cmd_load_keys+5, sector_keys.key_A, 6);
            command.assign(cmd_load_keys, 11);
            c.transmit(command, &response);
            if (c.response_status(response) != 0x9000) {
                error("Cannot load key A for sector" << sector );
                continue;  // try next sector
            }

            cmd_general_auth[7] = first_block;
            cmd_general_auth[8] = 0x60;
            command.assign(cmd_general_auth, 10);
            c.transmit(command, &response);
            if (c.response_status(response) != 0x9000) {
                error("Cannot use key A for sector " << sector << " auth.");
                continue;  // try next sector
            }

            // read "sector_keys.key_A_blocks_size" blocks
            for (size_t j = 0; j < sector_keys.key_A_blocks_size; j++) {
                size_t block = first_block + sector_keys.key_A_blocks[j];
                Block & b = card[block];

                cmd_read_binary[3] = block;
                command.assign(cmd_read_binary, 5);
                c.transmit(command, &response);
                if (c.response_status(response) != 0x9000) {
                    error("Failed to read block " << block << " using key A " << sector_keys.key_A_str);
                    continue;
                }
                memcpy(b.data, response.data(), 16);
                b.key_type = KeyA;
            }
        }

        if (sector_keys.key_B_blocks_size > 0) {
            // there are  Key A blocks so authenticate as Key A
            memcpy(cmd_load_keys+5, sector_keys.key_B, 6);
            command.assign(cmd_load_keys, 11);
            c.transmit(command, &response);
            if (c.response_status(response) != 0x9000) {
                error("Cannot load key B for sector" << sector );
                continue;  // try next sector
            }

            cmd_general_auth[7] = first_block;
            cmd_general_auth[8] = 0x61;
            command.assign(cmd_general_auth, 10);
            c.transmit(command, &response);
            if (c.response_status(response) != 0x9000) {
                error("Cannot use key B for sector " << sector << " auth.");
                continue;  // try next sector
            }

            // read "sector_keys.key_B_blocks_size" blocks
            for (size_t j = 0; j < sector_keys.key_B_blocks_size; j++) {
                size_t block = first_block + sector_keys.key_B_blocks[j];
                Block & b = card[block];

                cmd_read_binary[3] = block;
                command.assign(cmd_read_binary, 5);
                c.transmit(command, &response);
                if (c.response_status(response) != 0x9000) {
                    error("Failed to read block " << block << " using key B " << sector_keys.key_B_str);
                    continue;
                }
                memcpy(b.data, response.data(), 16);
                b.key_type = KeyB;
            }
        }
        
        // fill access bits if they are available
        Block & trailer = card[first_block+3];
        Block * pblock;
        if (trailer.key_type != KeyNone) {
            unsigned char b7 = trailer.data[7];
            unsigned char b8 = trailer.data[8];

            pblock = &(card[first_block+0]);
            pblock->is_access_set = true;
            pblock->C1 = CHECK_BIT(b7, 4);
            pblock->C2 = CHECK_BIT(b8, 0);
            pblock->C3 = CHECK_BIT(b7, 4);

            pblock = &(card[first_block+1]);
            pblock->is_access_set = true;
            pblock->C1 = CHECK_BIT(b7, 5);
            pblock->C2 = CHECK_BIT(b8, 1);
            pblock->C3 = CHECK_BIT(b7, 5);

            pblock = &(card[first_block+2]);
            pblock->is_access_set = true;
            pblock->C1 = CHECK_BIT(b7, 6);
            pblock->C2 = CHECK_BIT(b8, 2);
            pblock->C3 = CHECK_BIT(b7, 6);

            pblock = &(card[first_block+3]);
            pblock->is_access_set = true;
            pblock->C1 = CHECK_BIT(b7, 7);
            pblock->C2 = CHECK_BIT(b8, 3);
            pblock->C3 = CHECK_BIT(b7, 7);
        }
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
    for (int i=0; i < argc; i++) {
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

    // STAGE 1
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

    // STAGE 2
    // establish connection to reader and card

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


    // STAGE 3
    // check is card compatible
    if (!p.checkFeature(xpcsc::ATR_FEATURE_PICC)) {
        error("Contactless card required!");
        return 1;
    }

    if (!p.checkFeature(xpcsc::ATR_FEATURE_MIFARE_1K) && 
        !p.checkFeature(xpcsc::ATR_FEATURE_INFINEON_SLE_66R35)) 
    {
        error("Card type is not supported");
        return 1;
    }

    // STAGE 4
    // read card contents
    size_t total_blocks = 0;

    CardContents card(64);
    if (p.checkFeature(xpcsc::ATR_FEATURE_MIFARE_1K) ||
        p.checkFeature(xpcsc::ATR_FEATURE_INFINEON_SLE_66R35)) 
    {
        if (!read_mifare_1k(c, keys, card)) {
            error("Failed to read card contents");
            return 1;
        }
        total_blocks = 64;
    }

    char buf[32];
    for (size_t i=0; i<total_blocks; i++) {
        const Block & block_data = card[i];

        snprintf(buf, 31, "0x%02X", (unsigned int)block_data.sector);
        std::cout << buf << "  ";
        snprintf(buf, 31, "0x%02X", (unsigned int)i);
        std::cout << buf << "  ";
        if (block_data.key_type == KeyNone) {
            std::cout << "?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ??";
        } else {
            std::cout << xpcsc::format(xpcsc::Bytes(block_data.data, 16));
        }

        switch (block_data.key_type) {
        case KeyA:
            std::cout << "  Key: A " << keys[block_data.sector].key_A_str;
            break;
        case KeyB:
            std::cout << "  Key: B " << keys[block_data.sector].key_B_str;
            break;
        default:
            std::cout << "  Key: ?             ";
        }

        if (block_data.is_access_set) {
            std::cout << "  Access: " << int(block_data.C1) 
                << " " << int(block_data.C2) 
                << " " << int(block_data.C3);
        }

        std::cout << std::endl;
    }

    return 0;
}

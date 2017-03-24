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

#include <xpcsc.hpp>
#include <iostream>
#include <sstream>

#include <cstring>
#include <unistd.h>

#define CHECK_BIT(value, b) (((value) >> (b))&1)

typedef enum {Key_None, Key_A, Key_B} CardBlockKeyType;
struct AccessBits {
    xpcsc::Byte C1 : 1, C2 : 1, C3 :1;
    bool is_set;
    AccessBits() : is_set(false) {};
};
struct CardContents {
    xpcsc::Byte blocks_data[64][16];
    xpcsc::Byte blocks_keys[64][6];

    CardBlockKeyType blocks_key_types[64];
    AccessBits blocks_access_bits[64];
};

int main(int argc, char **argv)
{
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

    // fetch and print ATR
    xpcsc::Bytes atr = c.atr();
    std::cout << "ATR: " << xpcsc::format(atr) << std::endl;

    // parse ATR
    xpcsc::ATRParser p;
    p.load(atr);

    if (!p.checkFeature(xpcsc::ATR_FEATURE_PICC)) {
        std::cerr << "Contactless card required!" << std::endl;
        return 1;
    }

    if (!p.checkFeature(xpcsc::ATR_FEATURE_MIFARE_1K)) {
        std::cerr << "Mifare card required!" << std::endl;
        return 1;
    }

    // template for Load Keys command
    const xpcsc::Byte CMD_LOAD_KEYS[] = {xpcsc::CLA_PICC, xpcsc::INS_MIFARE_LOAD_KEYS, 0x00, 0x00, 
        0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    // template for General Auth command
    const xpcsc::Byte CMD_GENERAL_AUTH[] = {xpcsc::CLA_PICC, xpcsc::INS_MIFARE_GENERAL_AUTH, 0x00, 0x00, 
        0x05, 0x01, 0x00, 0x00, 0x60, 0x00};

    // template for Read Binary command
    const xpcsc::Byte CMD_READ_BINARY[] = {xpcsc::CLA_PICC, xpcsc::INS_MIFARE_READ_BINARY, 0x00, 0x00, 0x10};

    // standard keys
    const size_t keys_number = 3;
    xpcsc::Byte keys[keys_number][6] = {
        {0xff, 0xff, 0xff, 0xff, 0xff, 0xff},  // NXP factory default key
        {0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5},  // Infineon factory default key A
        {0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5}  // Infineon factory default key B
        // {0xd3, 0xf7, 0xd3, 0xf7, 0xd3, 0xf7},
        // {0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff},
        // {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
    };

    std::string str_keys[keys_number];

    for (size_t i=0; i<keys_number; i++) {
        str_keys[i] = xpcsc::format(xpcsc::Bytes(keys[i], 6));
    }

    xpcsc::Bytes command;
    xpcsc::Bytes response;

    CardContents card;

    std::stringstream s1;
    std::stringstream s2;

    // prepare display
    s1 << "Sectors: ";
    s2 << "Blocks:  ";
    char buf[10];

    for (xpcsc::Byte i = 0; i < 16; i++) {
        snprintf(buf, 6, "0x%01X   ", i);
        s1 << buf;
        s2 << "==== ";
    }

    std::cout << s1.str() << std::endl;
    std::cout << s2.str() << std::endl;

    // walk through all sectors and try to read data
    std::cout << "         ";
    for (size_t sector = 0; sector < 16; sector++) {
        xpcsc::Byte first_block = sector * 4;
        size_t k;

        bool key_found = false;

        for (k=0; k<keys_number; k++) {
            // try to authenticate on first sector

            // first load key into a terminal 
            command.assign(CMD_LOAD_KEYS, sizeof(CMD_LOAD_KEYS));
            command.replace(5, 6, keys[k], 6);
            c.transmit(command, &response);
            if (c.response_status(response) != 0x9000) {
                // next key
                continue;
            }

            // usleep(1700000);

            // authenticate as Key B
            command.assign(CMD_GENERAL_AUTH, sizeof(CMD_GENERAL_AUTH));
            command[7] = first_block;
            command[8] = 0x61;
            c.transmit(command, &response);
            if (c.response_status(response) == 0x9000) {
                // success, try to read data from all blocks (for this sector only!):
                for (size_t i=0; i<4; i++) {
                    xpcsc::Byte block = first_block+i;
                    command.assign(CMD_READ_BINARY, sizeof(CMD_READ_BINARY));
                    command[3] = block;
                    c.transmit(command, &response);
                    if (c.response_status(response) != 0x9000) {
                        // failed to read block
                        // std::cerr << "[Failed read] Key B " << block << ", key: " << str_keys[k] << std::endl;
                        return 1;
                    } else {
                        memcpy(card.blocks_keys[block], keys[k], 6);
                        card.blocks_key_types[block] = Key_B;
                        memcpy(card.blocks_data[block], response.data(), 16);
                        key_found = true;
                    }
                }
            }

            // usleep(1700000);

            // authenticate as Key A
            command.assign(CMD_GENERAL_AUTH, sizeof(CMD_GENERAL_AUTH));
            command[7] = first_block;
            command[8] = 0x60;
            c.transmit(command, &response);
            if (c.response_status(response) == 0x9000) {
                // success, try to read data from all blocks (for this sector only!):
                for (size_t i=0; i<4; i++) {
                    xpcsc::Byte block = first_block+i;
                    command.assign(CMD_READ_BINARY, sizeof(CMD_READ_BINARY));
                    command[3] = block;
                    c.transmit(command, &response);
                    if (c.response_status(response) != 0x9000) {
                        // failed to read block
                        // std::cerr << "[Failed read] Key A " << block << ", key: " << str_keys[k] << std::endl;
                    } else {
                        memcpy(card.blocks_keys[block], keys[k], 6);
                        card.blocks_key_types[block] = Key_A;
                        memcpy(card.blocks_data[block], response.data(), 16);
                        key_found = true;
                    }
                }
            }

            if (key_found) {
                break;
            }
        }

        if (key_found) {
            std::cout << "++++ " << std::flush;        
            // read access bits
            size_t block = sector * 4 + 3;
            const xpcsc::Byte *sector_trailer = card.blocks_data[block];

            xpcsc::Byte b7 = sector_trailer[7];
            xpcsc::Byte b8 = sector_trailer[8];

            card.blocks_access_bits[block-3].is_set = true;
            card.blocks_access_bits[block-3].C1 = CHECK_BIT(b7, 4);
            card.blocks_access_bits[block-3].C2 = CHECK_BIT(b8, 0);
            card.blocks_access_bits[block-3].C3 = CHECK_BIT(b8, 4);

            card.blocks_access_bits[block-2].is_set = true;
            card.blocks_access_bits[block-2].C1 = CHECK_BIT(b7, 5);
            card.blocks_access_bits[block-2].C2 = CHECK_BIT(b8, 1);
            card.blocks_access_bits[block-2].C3 = CHECK_BIT(b8, 5);

            card.blocks_access_bits[block-1].is_set = true;
            card.blocks_access_bits[block-1].C1 = CHECK_BIT(b7, 6);
            card.blocks_access_bits[block-1].C2 = CHECK_BIT(b8, 2);
            card.blocks_access_bits[block-1].C3 = CHECK_BIT(b8, 6);

            card.blocks_access_bits[block].is_set = true;
            card.blocks_access_bits[block].C1 = CHECK_BIT(b7, 7);
            card.blocks_access_bits[block].C2 = CHECK_BIT(b8, 3);
            card.blocks_access_bits[block].C3 = CHECK_BIT(b8, 7);

        } else {
            std::cout << "---- " << std::flush;        
            for (size_t i=0; i<4; i++) {
                size_t block = sector * 4 + i;
                card.blocks_key_types[block] = Key_None;
            }
        }
    }

    std::cout << std::endl;

    std::cout << "BLOCK|       DATA                                      | KEY                      | ACCESS BITS" << std::endl;
    std::cout << "     |                                                 |                          |  C1 C2 C3 " << std::endl;

    // print card contents
    xpcsc::Bytes key;
    for (size_t i=0; i<64; i++) {
        std::cout << "0x" << xpcsc::format((xpcsc::Byte)i) << " | ";

        xpcsc::Bytes row(card.blocks_data[i], 16);
        if (card.blocks_key_types[i] == Key_None) {
            std::cout << "?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ??";
            std::cout << "                            | ";
        } else {
            key.assign(card.blocks_keys[i], 6);

            // print row of data first
            std::cout << xpcsc::format(row) << " | Key ";
            // key next
            if (card.blocks_key_types[i] == Key_A) {
                std::cout << "A: ";
            } else if (card.blocks_key_types[i] == Key_B) {
                std::cout << "B: ";
            } else {
                std::cout << " : ";
            }
            std::cout << xpcsc::format(key) << " | ";
            
        }
        // render access bits
        if (card.blocks_access_bits[i].is_set) {
            std::cout << " " \
                << int(card.blocks_access_bits[i].C1) << "  " \
                << int(card.blocks_access_bits[i].C2) << "  " \
                << int(card.blocks_access_bits[i].C3);
        }
        std::cout << std::endl;
    }

    std::cout << std::endl;

    return 0;
}
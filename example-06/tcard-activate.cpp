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

#include <string.h>
#include <unistd.h>

#include "card.hpp"

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
    xpcsc::Reader reader;
    try {
        std::string reader_name = *readers.begin();
        std::cout << "Found reader: " << reader_name << std::endl;
        reader = c.wait_for_reader_card(reader_name, SCARD_PROTOCOL_T0);
    } catch (xpcsc::PCSCError &e) {
        std::cerr << "Wait for card failed: " << e.what() << std::endl;
        return 1;
    }

    // fetch and print ATR
    xpcsc::Bytes atr = c.atr(reader);

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

    xpcsc::Bytes command;
    xpcsc::Bytes response;

    // load DEFAULT_KEY_A
    command.assign(CMD_LOAD_KEYS, sizeof(CMD_LOAD_KEYS));
    command.replace(5, 6, DEFAULT_KEY_A, 6);
    c.transmit(reader, command, &response);
    if (c.response_status(response) != 0x9000) {
        std::cerr << "Failed to load key" << std::endl;
        return 1;
    }

    // authenticate to access block CARD_BLOCK using loaded key as Key A 
    command.assign(CMD_GENERAL_AUTH, sizeof(CMD_GENERAL_AUTH));
    command[7] = CARD_BLOCK;
    command[8] = 0x60;
    c.transmit(reader, command, &response);
    if (c.response_status(response) != 0x9000) {
        std::cerr << "Cannot authenticate using DEFAULT_KEY_A!" << std::endl;
        return 1;
    }

    // read block CARD_BLOCK and check it contains only zeroes
    command.assign(CMD_READ_BINARY, sizeof(CMD_READ_BINARY));
    command[3] = CARD_BLOCK;
    c.transmit(reader, command, &response);
    if (c.response_status(response) != 0x9000) {
        std::cerr << "Cannot read block!" << std::endl;
        return 1;
    }

    for (size_t i=0; i < 15; i++) {
        if (response.at(i) != 0) {
            std::cerr << "Block must be filled with zeroes!" << std::endl;
            return 1;
        }
    }

    // initial balance
    uint16_t balance = INITIAL_BALANCE;

    // create block with balance, change first 2 bytes
    xpcsc::Bytes balance_block(16, 0);
    balance_block.replace(0, 2, (xpcsc::Byte*)&balance, 2);

    // update block
    command.assign(CMD_UPDATE_BINARY, sizeof(CMD_UPDATE_BINARY));
    command[3] = CARD_BLOCK;
    command.replace(5, 16, balance_block.c_str(), 16);
    c.transmit(reader, command, &response);
    if (c.response_status(response) != 0x9000) {
        std::cerr << "Cannot update block!" << std::endl;
        return 1;
    }

    // read sector trailer
    command.assign(CMD_READ_BINARY, sizeof(CMD_READ_BINARY));
    command[3] = CARD_SECTOR_TRAILER;
    c.transmit(reader, command, &response);
    if (c.response_status(response) != 0x9000) {
        std::cerr << "Cannot read block!" << std::endl;
        return 1;
    }

    // update trailer with a new Key A
    xpcsc::Bytes trailer = response.substr(0, 16);
    trailer.replace(0, 6, ACTIVE_KEY_A, 6);
    trailer.replace(10, 6, ACTIVE_KEY_A, 6);

    // update sector trailer
    command.assign(CMD_UPDATE_BINARY, sizeof(CMD_UPDATE_BINARY));
    command[3] = CARD_SECTOR_TRAILER;
    command.replace(5, 16, trailer.c_str(), 16);
    c.transmit(reader, command, &response);
    if (c.response_status(response) != 0x9000) {
        std::cerr << "Cannot update block!" << std::endl;
        return 1;
    }

    std::cout << "Card activated!" << std::endl;
}

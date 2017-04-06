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
    unsigned char cmd_load_keys[] = {xpcsc::CLA_PICC, xpcsc::INS_MIFARE_LOAD_KEYS, 0x00, 0x00, 
        0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    // template for General Auth command
    unsigned char cmd_general_auth[] = {xpcsc::CLA_PICC, xpcsc::INS_MIFARE_GENERAL_AUTH, 0x00, 0x00, 
        0x05, 0x01, 0x00, 0x00, 0x60, 0x00};

    // template for Read Binary command
    unsigned char cmd_read_binary[] = {xpcsc::CLA_PICC, xpcsc::INS_MIFARE_READ_BINARY, 0x00, 0x00, 0x10};

    // template for Update Binary command
    unsigned char cmd_update_binary[] = {xpcsc::CLA_PICC, xpcsc::INS_MIFARE_UPDATE_BINARY, 0x00, 0x00, 0x10,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    xpcsc::Bytes command;
    xpcsc::Bytes response;

    // load DEFAULT_KEY_A
    memcpy(cmd_load_keys+5, DEFAULT_KEY_A, 6);
    command.assign(cmd_load_keys, sizeof(cmd_load_keys));
    c.transmit(command, &response);
    if (c.response_status(response) != 0x9000) {
        std::cerr << "Failed to load key" << std::endl;
        return 1;
    }

    // authenticate to access block CARD_BLOCK using loaded key as Key A 
    cmd_general_auth[7] = CARD_BLOCK;
    cmd_general_auth[8] = 0x61;
    command.assign(cmd_general_auth, sizeof(cmd_general_auth));
    c.transmit(command, &response);
    if (c.response_status(response) != 0x9000) {
        std::cerr << "Cannot authenticate using DEFAULT_KEY_A!" << std::endl;
        return -1;
    }

    // read block CARD_BLOCK and check it contains only 0s
    cmd_read_binary[3] = CARD_BLOCK;
    command.assign(cmd_read_binary, sizeof(cmd_read_binary));
    c.transmit(command, &response);
    if (c.response_status(response) != 0x9000) {
        std::cerr << "Cannot read block!" << std::endl;
        return -1;
    }

    for (size_t i=0; i < 15; i++) {
        if (response.at(i) != 0) {
            std::cerr << "Block must be filled with zeroes!" << std::endl;
            return -1;
        }
    }

    // initial balance
    unsigned short balance = INITIAL_BALANCE;

    // create block with balance, change first 2 bytes
    xpcsc::Bytes balance_block(16, 0);
    balance_block.replace(0, 2, (unsigned char *)&balance, 2);

    // update block
    cmd_update_binary[3] = CARD_BLOCK;
    memcpy(cmd_update_binary+5, balance_block.c_str(), 16);
    command.assign(cmd_update_binary, sizeof(cmd_update_binary));
    c.transmit(command, &response);
    if (c.response_status(response) != 0x9000) {
        std::cerr << "Cannot update block!" << std::endl;
        return -1;
    }

    // read sector trailer
    cmd_read_binary[3] = CARD_SECTOR_TRAILER;
    command.assign(cmd_read_binary, sizeof(cmd_read_binary));
    c.transmit(command, &response);
    if (c.response_status(response) != 0x9000) {
        std::cerr << "Cannot read block!" << std::endl;
        return -1;
    }

    // update trailer with a new Key A
    xpcsc::Bytes trailer = response.substr(0, 16);
    trailer.replace(0, 6, ACTIVE_KEY_A, 6);
    trailer.replace(10, 6, ACTIVE_KEY_A, 6);

    // update sector trailer
    cmd_update_binary[3] = CARD_SECTOR_TRAILER;
    memcpy(cmd_update_binary+5, trailer.c_str(), 16);
    command.assign(cmd_update_binary, sizeof(cmd_update_binary));
    c.transmit(command, &response);
    if (c.response_status(response) != 0x9000) {
        std::cerr << "Cannot update block!" << std::endl;
        return -1;
    }

    std::cout << "Success!" << std::endl;
}

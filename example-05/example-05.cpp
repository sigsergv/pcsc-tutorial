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

    if (!p.checkFeature(xpcsc::ATR_FEATURE_PICC) || !p.checkFeature(xpcsc::ATR_FEATURE_MIFARE_1K)) {
        std::cerr << "Not Mifare Classic 1K!" << std::endl;
        return 1;
    }

    unsigned char cmd_load_keys[] = {xpcsc::CLA_PICC, xpcsc::INS_MIFARE_LOAD_KEYS, 0x00, 0x00, 
        0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    unsigned char cmd_general_auth[] = {xpcsc::CLA_PICC, xpcsc::INS_MIFARE_GENERAL_AUTH, 0x00, 0x00, 
        0x05, 0x01, 0x00, 0x00, 0x60, 0x00};

    unsigned char cmd_read_binary[] = {xpcsc::CLA_PICC, xpcsc::INS_MIFARE_READ_BINARY, 0x00, 0x00, 0x10};

    const size_t keys_number = 4;
    unsigned char keys[keys_number][6] = {
        {0xff,0xff,0xff,0xff,0xff,0xff},
        {0xa0,0xa1,0xa2,0xa3,0xa4,0xa5},
        {0xd3,0xf7,0xd3,0xf7,0xd3,0xf7},
        {0xaa,0xbb,0xcc,0xdd,0xee,0xff}
    };

    xpcsc::Bytes command;
    xpcsc::Bytes response;

    // we have 16 sectors and 64 blocks ahead
    size_t block = -1;
    for (size_t i=0; i<0x10; i++) {
        std::cout << "---------------------------" << std::endl;
        std::cout << "Sector: " << i << std::endl;
        // 3 regular blocks
        for (size_t j=0; j<3; j++) {
            block++;
            size_t k;
            bool key_found = false;
            std::cout << " block: " << block << std::endl;

            for (k=0; k<keys_number; k++) {
                std::cout << "  trying key " << k << " ";

                memcpy(cmd_load_keys+5, keys[k], 6);
                command.assign(cmd_load_keys, 11);
                c.transmit(command, &response);
                if (c.response_status(response) != 0x9000) {
                    std::cout << " .. failed" << std::endl;
                    continue;
                }

                // ok, now try auth
                cmd_general_auth[7] = block;
                command.assign(cmd_general_auth, 10);
                c.transmit(command, &response);
                if (c.response_status(response) != 0x9000) {
                    std::cout << " .. failed" << std::endl;
                    continue;
                }

                std::cout << " .. success!" << std::endl;
                key_found = true;
                break;
            }

            if (key_found) {
                // we can try to read block using key
                cmd_read_binary[3] = block;
                command.assign(cmd_read_binary, 5);
                c.transmit(command, &response);
                if (c.response_status(response) != 0x9000) {
                    std::cout << "  failed to read block data" << std::endl;
                } else {
                    std::cout << "  " << xpcsc::format(response.substr(0, response.size()-2)) << std::endl;
                }
            }
        }

        // and one special with keys and access bits
        block++;
    }

    // std::cout << p.str() << std::endl;
    // const unsigned char cmd_1_buf[] = {
    //     xpcsc::CLA_PICC,
    //     xpcsc::INS_MIFARE_LOAD_KEYS,
    //     0x00, // P1
    //     0x00, // P2
    //     0x06, // Lc
    //     0xff,0xff,0xff,0xff,0xff,0xff
    // };
    // xpcsc::Bytes cmd_1(cmd_1_buf, sizeof(cmd_1_buf));
    // xpcsc::Bytes response;

    // c.transmit(cmd_1, &response);
    // std::cout << "Response: " << xpcsc::format(response) << std::endl;

    return 0;
}
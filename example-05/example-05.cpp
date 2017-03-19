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
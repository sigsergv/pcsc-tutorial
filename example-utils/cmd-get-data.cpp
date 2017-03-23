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
 * Scan card, Get Data command result,
 * see pcsc3_v2.01.09.pdf, section 3.2.2.1.3
 */

#include <xpcsc.hpp>
#include <iostream>
#include <sstream>

#include <string.h>

#define error(msg) do { std::cerr << msg << std::endl; } while (0);

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
        error("Contactless card required!");
        return 1;
    }

    xpcsc::Bytes command;
    xpcsc::Bytes response;

    unsigned char cmd[] = {xpcsc::CLA_PICC, xpcsc::INS_PICC_GET_DATA, 0x0, 0x0, 0x0};

    command.assign(cmd, 5);
    c.transmit(command, &response);
    if (c.response_status(response) != 0x9000) {
        error("Cannot read card data");
        return -2;
    }

    xpcsc::Bytes uid = c.response_data(response);
    std::cout << "UID: " << xpcsc::format(uid) << std::endl;

    cmd[2] = 0x1;

    command.assign(cmd, 5);
    c.transmit(command, &response);
    int sw = c.response_status(response);

    if (sw == 0x6a81) {
        error("ATS HB: Function not supported");
    } else if (c.response_status(response) != 0x9000) {
        error("Cannot read HB");
        std::cerr << xpcsc::format(response) << std::endl;
        return -2;
    } else {
        xpcsc::Bytes hb = c.response_data(response);
        std::cout << "ATS HB: " << xpcsc::format(hb) << std::endl;
    }


    return 0;
}
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
 * @file acr122u.cpp
 * @author Sergey Stolyarov <sergei@regolit.com>
 *
 * some low level commands experiment
 */

#include <xpcsc.hpp>
#include <iostream>
#include <sstream>

#include <string.h>

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
        reader = c.wait_for_reader_card(reader_name);
    } catch (xpcsc::PCSCError &e) {
        std::cerr << "Wait for card failed: " << e.what() << std::endl;
        return 1;
    }

    // // fetch and print ATR
    // xpcsc::Bytes atr = c.atr();
    // std::cout << "ATR: " << xpcsc::format(atr) << std::endl;

    // // parse ATR
    // xpcsc::ATRParser p;
    // p.load(atr);

    // send direct command to card
    unsigned char cmd_1[] = {0xff, 0xca, 0x01, 0x00, 0x00};
    
    xpcsc::Bytes command;
    xpcsc::Bytes response;

    std::cout << "Send cmd_1" << std::endl;

    command.assign(cmd_1, sizeof(cmd_1));
    c.transmit(reader, command, &response);
    std::cout << "Response: " << xpcsc::format(response) << std::endl;
}

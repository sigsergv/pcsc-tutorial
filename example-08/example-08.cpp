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
#include <iomanip>
#include <sstream>
#include <unordered_map>

#include <cstring>
#include <string>
#include <unistd.h>

#include "apps.hpp"

#define CHECK_BIT(value, b) (((value) >> (b))&1)

const xpcsc::Bytes TAG_EMV_FCI = {0x6F};

std::string format_fci(const xpcsc::BerTlv & tlv)
{
    std::stringstream ss;

    // find element with tag `6F`
    const auto & FCI_block = tlv.find_by_tag(TAG_EMV_FCI);

    if (!FCI_block) {
        throw std::invalid_argument("Not FCI template");
    }

    ss << "FCI template:\n";
    // iterate over top level elements
    const auto & items = FCI_block->get_children();
    for (auto i=items.begin(); i!=items.end(); i++) {
        ss << "  Tag: " << xpcsc::format((*i)->get_tag()) << "\n";
    }
    return ss.str();
}

xpcsc::Bytes select_app_apdu(const std::string & aid)
{
    std::stringstream ss;

    if ((aid.length() % 2) == 1) {
        throw std::runtime_error("bad");
    }
    ss << "00  A4  04 00 ";
    uint8_t size = static_cast<uint8_t>(aid.length() / 2);
    ss << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(size) << " ";
    ss << aid;
    return xpcsc::parse_apdu(ss.str());
}

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
    auto readers = c.readers();
    if (readers.size() == 0) {
        std::cerr << "[E] No connected readers" << std::endl;
        return 1;
    }

    // connect to reader
    xpcsc::Reader reader;

    try {
        auto reader_name = *readers.begin();
        std::cout << "Found reader: " << reader_name << std::endl;
        reader = c.wait_for_reader_card(reader_name);
    } catch (xpcsc::PCSCError &e) {
        std::cerr << "Wait for card failed: " << e.what() << std::endl;
        return 1;
    }

    xpcsc::Bytes command;
    xpcsc::Bytes response;
    uint16_t response_status;

    for (auto x=KNOWN_APPLICATIONS.begin(); x!=KNOWN_APPLICATIONS.end(); x++) {
        auto p = x->find('|');
        if (p == std::string::npos) {
            continue;
        }
        auto aid = x->substr(0, p);
        auto desc = x->substr(p+1);

        command.assign(select_app_apdu(aid));
        c.transmit(reader, command, &response);
        response_status = c.response_status(response);

        if (response_status != 0x9000) {
            continue;
        }

        std::cout << "Probe successful for AID: " << aid << " - " << desc << std::endl;
        auto tlv = xpcsc::BerTlv::parse(response.substr(0, response.size()-2));
        std::cout << "Parsed response:" << std::endl;
        try {
            std::cout << format_fci(*tlv) << std::endl;
        } catch (std::invalid_argument e) {
            // print response
            std::cout << "No FCI: " << xpcsc::format(response) << std::endl;
        }

    }

    return 0;
}
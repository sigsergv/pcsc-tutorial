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


const xpcsc::Bytes TAG_EMV_FCI = {0x6F};
const xpcsc::Bytes TAG_EMV_FCI_PD = {0xA5};
const xpcsc::Bytes TAG_EMV_FCI_SFI = {0x88};
const xpcsc::Bytes TAG_PSD_REC = {0x70};
const xpcsc::Bytes TAG_PSD_TAG = {0x61};
const xpcsc::Bytes TAG_PSD_AID_TAG = {0x4F};

std::vector<xpcsc::Bytes> read_apps_from_pse(xpcsc::Connection & c, xpcsc::Reader & reader)
{
    xpcsc::Bytes command;
    xpcsc::Bytes response;
    uint16_t response_status;

    std::vector<xpcsc::Bytes> apps;

    // SELECT Payment System Environment (PSE)
    //                                CLA INS P1 P2   Lc  "1PAY.SYS.DDF01"
    command.assign(xpcsc::parse_apdu("00  A4  04 00   0E  31 50 41 59 2E 53 59 53 2E 44 44 46 30 31"));
    c.transmit(reader, command, &response);
    response_status = c.response_status(response);

    if (response_status == 0x6A82) {
        // no PSE on the card
        return apps;
    }

    if (response_status != 0x9000) {
        std::cerr << "Failed to fetch PSE: " << c.response_status_str(response) << std::endl;
        return apps;
    }

    // parse response data
    auto tlv = xpcsc::BerTlv::parse(response.substr(0, response.size()-2));

    const auto & items = tlv->get_children();
    if (items.size() == 0) {
        // no data
        return apps;
    }

    const auto & fci = items[0];

    if (fci->get_tag().compare(TAG_EMV_FCI) != 0) {
        // returned data is not a proper FCI
        return apps;
    }

    // find proprietary data (PD) block
    const auto & PD_block = fci->find_by_tag(TAG_EMV_FCI_PD);
    if (!PD_block) {
        // proprietary data block not found
        return apps;
    }

    // find SFI block
    const auto & SFI_block = PD_block->find_by_tag(TAG_EMV_FCI_SFI);
    if (!SFI_block) {
        // SFI data block not found
        return apps;
    }

    xpcsc::Byte sfi = SFI_block->get_data()[0];
    xpcsc::Byte P2 = (sfi << 3) | 4;

    // read Payment System Directory
    command = {0x00, 0xB2, 0x00, P2, 0x00};

    for (xpcsc::Byte i=1; i<=10; i++) {
        command[2] = i;
        command[4] = 0;
        c.transmit(reader, command, &response);
        response_status = c.response_status(response);

        if (response_status == 0x6A83) {
            // no more records
            break;
        }

        if ((response_status >> 8) == 0x6C) {
            // repeat with proper Le
            command[4] = static_cast<xpcsc::Byte>(response_status & 0xFF);
            c.transmit(reader, command, &response);
            response_status = c.response_status(response);
        }

        if (response_status != 0x9000) {
            // something wrong
            std::cerr << "Failed to fetch Payment System Directory record: " << c.response_status_str(response) << std::endl;
            break;
        }

        auto atlv(xpcsc::BerTlv::parse(response.substr(0, response.size()-2)));

        const auto & PSD_REC_block = atlv->find_by_tag(TAG_PSD_REC);
        if (!PSD_REC_block) {
            // malformed PSD record
            return apps;
        }

        // find all AID records, they all have tag 61
        const auto & psd_items =  PSD_REC_block->get_children();
        for (auto j=psd_items.begin(); j!=psd_items.end(); j++) {
            const xpcsc::BerTlvRef & psd_block = *j;
            if (psd_block->get_tag().compare(TAG_PSD_TAG) != 0) {
                continue;
            }

            const auto & aid_block = psd_block->find_by_tag(TAG_PSD_AID_TAG);
            if (aid_block) {
                // AID found!
                apps.push_back(aid_block->get_data());
            }
        }

    }

    return apps;
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

    auto apps = read_apps_from_pse(c, reader);
    if (apps.size() == 0) {
        std::cout << ">> Cannot fetch applications from PSE, trying some predefined AID" << std::endl;
    }

    // take first AID
    auto aid = *(apps.begin());

    std::cout << "Found AID: " << xpcsc::format(aid) << std::endl;

    return 0;
}
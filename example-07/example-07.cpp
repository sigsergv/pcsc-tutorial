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

int main(int argc, char **argv)
{
    xpcsc::Connection c;

    // xpcsc::Bytes test = xpcsc::parse_apdu("6F 15 84 0E 31 50 41 59 2E 53 59 53 2E 44 44 46 30 31 A5 03 88 01 01");
    // std::cout << "TEST DATA: " << xpcsc::format(test) << std::endl;
    // xpcsc::BerTlvRef t(xpcsc::BerTlv::parse(test));
    // std::cout << "PRINT" << std::endl;
    // return 123;

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


    xpcsc::Bytes command;
    xpcsc::Bytes response;
    uint16_t response_status;

    command.assign(xpcsc::parse_apdu("00 A4 04 00   0E  31 50 41 59 2E 53 59 53 2E 44 44 46 30 31"));
    c.transmit(reader, command, &response);
    response_status = c.response_status(response);

    const xpcsc::Bytes TAG_EMV_FCI(xpcsc::parse_apdu("6F"));
    const xpcsc::Bytes TAG_EMV_FCI_PR(xpcsc::parse_apdu("A5"));
    const xpcsc::Bytes TAG_EMV_FCI_SFI(xpcsc::parse_apdu("88"));
    const xpcsc::Bytes TAG_EMV_FCI_LANG(xpcsc::parse_apdu("5F 2D"));
    const xpcsc::Bytes TAG_EMV_FCI_ISSUER(xpcsc::parse_apdu("9F 11"));
    const xpcsc::Bytes TAG_EMV_FCI_ISSUER_DD(xpcsc::parse_apdu("BF 0C"));


    if (response_status == 0x9000) {
        // parse response data
        xpcsc::BerTlvRef tlv(xpcsc::BerTlv::parse(response.substr(0, response.size()-2)));

        // std::cout << xpcsc::format(*tlv) << std::endl;

        const xpcsc::BerTlvList & items = tlv->get_children();
        if (items.size() > 0 && items[0]->get_tag().compare(TAG_EMV_FCI)==0) {
            // FCI
            const xpcsc::BerTlvRef & p = items[0];

            // find A5 block
            const xpcsc::BerTlvRef & A5_block = p->find_by_tag(TAG_EMV_FCI_PR);
            if (A5_block) {
                // fetch SFI
                const xpcsc::BerTlvRef & SFI_block = A5_block->find_by_tag(TAG_EMV_FCI_SFI);
                if (SFI_block) {
                    const xpcsc::Bytes & data = SFI_block->get_data();
                }

                // const xpcsc::BerTlvRef & LANG_block = A5_block->find_by_tag(TAG_EMV_FCI_LANG);
                // if (LANG_block) {
                //     xpcsc::Bytes data = LANG_block->get_data();
                //     std::stringstream ss;
                //     for (xpcsc::Bytes::const_iterator i=data.begin(); i!=data.end(); i++) {
                //         ss << static_cast<char>(*i);
                //     }
                //     std::cout << "Lang pref: " << ss.str() << std::endl;
                // }

                // const xpcsc::BerTlvRef & ISSUER_block = A5_block->find_by_tag(TAG_EMV_FCI_ISSUER);
                // if (ISSUER_block) {
                //     const xpcsc::Bytes & data = ISSUER_block->get_data();
                //     std::cout << "Issuer: " << xpcsc::format(data) << std::endl;
                // }
            }
        }
        // std::cout << "RESPONSE STATUS: " << c.response_status_str(response) << std::endl;
        // std::cout << "RESPONSE: " << xpcsc::format(response) << std::endl;
    }

    return 0;
}
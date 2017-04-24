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
#include <unordered_map>

#include <cstring>
#include <unistd.h>

#define CHECK_BIT(value, b) (((value) >> (b))&1)

const xpcsc::Bytes TAG_EMV_FCI = {0x6F};
const xpcsc::Bytes TAG_EMV_FCI_PD = {0xA5};
const xpcsc::Bytes TAG_EMV_FCI_SFI = {0x88};
const xpcsc::Bytes TAG_PSD_REC = {0x70};
const xpcsc::Bytes TAG_PSD_TAG = {0x61};
const xpcsc::Bytes TAG_PSD_AID_TAG = {0x4F};
const xpcsc::Bytes TAG_EMV_FCI_APP_LABEL = {0x50};
const xpcsc::Bytes TAG_EMV_FCI_APP_PDOL = {0x9F, 0x38};

const xpcsc::Bytes TAG_PRIM = {0x80};
const xpcsc::Bytes TAG_STRUCT = {0x77};

const xpcsc::Bytes TAG_EMV_AIP = {0x82};
const xpcsc::Bytes TAG_EMV_AFL = {0x94};


const xpcsc::Bytes AID_VISA_ELECTRON = {0xA0, 0x00, 0x00, 0x00, 0x03, 0x20, 0x10};
const xpcsc::Bytes AID_VISA_CLASSIC = {0xA0, 0x00, 0x00, 0x00, 0x03, 0x10, 0x10};
const xpcsc::Bytes AID_MASTERCARD = {0xA0, 0x00, 0x00, 0x00, 0x04, 0x10, 0x10};
const xpcsc::Bytes AID_PRO100 = {0xA0, 0x00, 0x00, 0x04, 0x32, 0x00, 0x01};


uint32_t tag_to_long(const xpcsc::Bytes tag)
{
    size_t length = tag.size();
    if (length > 4) {
        throw std::out_of_range("too large tag");
    }

    uint32_t res = 0;
    for (auto i=tag.begin(); i!=tag.end(); i++) {
        res = (res << 8) | *i;
    }

    return res;
}

const char * tag_to_string(uint32_t tag)
{
    switch (tag) {
    case 0x56: return "Track 1 Data";
    case 0x57: return "Track 2 Equivalent Data";
    case 0x5A: return "Application Primary Account Number (PAN)";
    case 0x5F20: return "Cardholder Name";
    case 0x5F24: return "Application Expiration Date";
    case 0x5F25: return "Application Effective Date";
    case 0x5F28: return "Issuer Country Code";
    case 0x5F30: return "Service Code";
    case 0x5F34: return "Application Primary Account Number (PAN) Sequence Number";
    case 0x8C: return "Card Risk Management Data Object List 1 (CDOL1)";
    case 0x8D: return "Card Risk Management Data Object List 2 (CDOL2)";
    case 0x8E: return "Cardholder Verification Method (CVM) List";
    case 0x8F: return "Certification Authority Public Key Index";
    case 0x90: return "Issuer Public Key Certificate";
    case 0x92: return "Issuer Public Key Remainder";
    case 0x93: return "Signed Static Application Data";
    case 0x9F07: return "Application Usage Control";
    case 0x9F08: return "Application Version Number";
    case 0x9F0D: return "Issuer Action Code - Default";
    case 0x9F0E: return "Issuer Action Code - Denial";
    case 0x9F0F: return "Issuer Action Code - Online";
    case 0x9F1F: return "Track 1 Discretionary Data";
    case 0x9F32: return "Issuer Public Key Exponent";
    case 0x9F42: return "Application Currency Code";
    case 0x9F44: return "Application Currency Exponent";
    case 0x9F46: return "ICC Public Key Certificate";
    case 0x9F47: return "ICC Public Key Exponent";
    case 0x9F48: return "ICC Public Key Remainder";
    case 0x9F49: return "Dynamic Data Authentication Data Object List (DDOL)";
    case 0x9F4A: return "Static Data Authentication Tag List";
    case 0x9F62: return "PCVC3 (Track1)";
    case 0x9F63: return "PUNATC (Track1)";
    case 0x9F64: return "NATC (Track1)";
    case 0x9F65: return "PCVC3 (Track2)";
    case 0x9F66: return "Terminal Transaction Qualifiers (TTQ)";
    case 0x9F67: return "NATC (Track2)";
    case 0x9F68: return "Card Additional Processes";
    case 0x9F6B: return "Track 2 Data/Card CVM Limit";
    case 0x9F6C: return "Card Transaction Qualifiers (CTQ)";

    default: return "??? Unknown ???";
    }
}

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
    //         CLA   INS   P1    P2  Le
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

bool read_app(xpcsc::Connection & c, xpcsc::Reader & reader, const xpcsc::Bytes & aid)
{
    xpcsc::Bytes command;
    xpcsc::Bytes response;
    uint16_t response_status;

    // SELECT Payment System Environment (PSE)
    //                                CLA INS P1 P2 
    command.assign(xpcsc::parse_apdu("00  A4  04 00"));
    command.push_back(static_cast<xpcsc::Byte>(aid.size()));
    command.append(aid);

    c.transmit(reader, command, &response);
    response_status = c.response_status(response);

    if (response_status == 0x6A82) {
        // no application on the card
        return false;
    }

    if (response_status != 0x9000) {
        std::cerr << "Failed to fetch ADF: " << c.response_status_str(response) << std::endl;
        return false;
    }

    // parse response data
    auto tlv = xpcsc::BerTlv::parse(response.substr(0, response.size()-2));

    const auto & items = tlv->get_children();
    if (items.size() == 0) {
        // no data
        return false;
    }

    const auto & fci = items[0];

    if (fci->get_tag().compare(TAG_EMV_FCI) != 0) {
        // returned data is not a proper FCI
        return false;
    }

    // find proprietary data (PD) block
    const auto & PD_block = fci->find_by_tag(TAG_EMV_FCI_PD);
    if (!PD_block) {
        // proprietary data block not found
        return false;
    }

    // std::cout << "PD block: \n" << xpcsc::format(*PD_block) << std::endl;

    const auto & label_block = PD_block->find_by_tag(TAG_EMV_FCI_APP_LABEL);
    if (label_block) {
        // print application name in ASCII
        std::cout << "Application label: " << label_block->get_data().c_str() << std::endl;
    }

    // setup GET PROCESSING OPTIONS command
    // SELECT Payment System Environment (PSE)
    //                                CLA INS P1 P2 
    command.assign(xpcsc::parse_apdu("80  A8  00 00"));

    const auto & PDOL_block = PD_block->find_by_tag(TAG_EMV_FCI_APP_PDOL);
    if (PDOL_block) {
        const xpcsc::Bytes pdol = PDOL_block->get_data();
        xpcsc::Byte total_length = 0;

        for (auto i=pdol.begin(); i!=pdol.end(); i++) {
            if ((*i & 0x1F) == 0x1F) {
                // go deeper
                while (true) {
                    i++;
                    if ((*i & 0x80) == 0) {
                        break;
                    }
                }
            }
            // next byte is a length
            i++;
            total_length += *i;
        }

        xpcsc::Bytes data = {static_cast<xpcsc::Byte>(total_length+2), 0x83, total_length};
        xpcsc::Bytes nulls(total_length, 0);
        data.append(nulls);
        command.append(data);
    } else {
        // no PDOL
        xpcsc::Bytes data = {0x02, 0x83, 0x00};
        command.append(data);
    }

    // Append Le
    command.push_back(0x00);
    c.transmit(reader, command, &response);
    response_status = c.response_status(response);

    if (response_status != 0x9000) {
        std::cerr << "GET PROCESSING OPTIONS failed: " << c.response_status_str(response) << std::endl;
        return false;
    }

    const auto & po_tlv = xpcsc::BerTlv::parse(response.substr(0, response.size()-2));
    const auto & po_tlv_d = po_tlv->get_children().at(0);

    xpcsc::Bytes aip;
    xpcsc::Bytes afl;

    if (po_tlv_d->get_tag().compare(TAG_PRIM) == 0) {
        // primitive data, first 2 bytes AIP, the rest — AFL
        const auto & data = po_tlv_d->get_data();
        aip = data.substr(0, 2);
        afl = data.substr(2);
    } else if (po_tlv_d->get_tag().compare(TAG_STRUCT) == 0) {
        // structured/tlv data
        aip = po_tlv_d->find_by_tag(TAG_EMV_AIP)->get_data();
        afl = po_tlv_d->find_by_tag(TAG_EMV_AFL)->get_data();
    } else {
        std::cerr << "GET PROCESSING OPTIONS failed: unknown response format: " << xpcsc::format(response) << std::endl;
        return false;
    }

    // print card capabilities from "aip"
    xpcsc::Byte aip_1 = aip.at(0);
    std::cout << "Card capabilities:" << std::endl;
    std::cout << "  SDA supported: " << (CHECK_BIT(aip_1, 1) ? "true" : "false") << std::endl;
    std::cout << "  DDA supported: " << (CHECK_BIT(aip_1, 2) ? "true" : "false") << std::endl;
    std::cout << "  Cardholder verification supported: " << (CHECK_BIT(aip_1, 3) ? "true" : "false") << std::endl;
    std::cout << "  Terminal risk management is to be performed: " << (CHECK_BIT(aip_1, 4) ? "true" : "false") << std::endl;
    std::cout << "  Issuer authentication is supported: " << (CHECK_BIT(aip_1, 5) ? "true" : "false") << std::endl;
    std::cout << "  CDA supported: " << (CHECK_BIT(aip_1, 7) ? "true" : "false") << std::endl;

    std::unordered_map<uint32_t, xpcsc::Bytes> values;

    // process AFL
    for (auto i=afl.begin(); i!=afl.end(); i++) {
        command = {0x00, 0xB2, 0x00, static_cast<xpcsc::Byte>((*i & 0xF8) | 0x4), 0x00};

        i++;
        auto first_rec_num = *i;

        i++;
        auto last_rec_num = *i;

        for (size_t j=first_rec_num; j<=last_rec_num; j++) {
            command[2] = j;
            c.transmit(reader, command, &response);
            response_status = c.response_status(response);
            if ((response_status >> 8) == 0x6C) {
                command[4] = static_cast<xpcsc::Byte>(response_status & 0xFF);
                c.transmit(reader, command, &response);
            }

            response_status = c.response_status(response);
            if (response_status != 0x9000) {
                std::cerr << "failed to read record: " << c.response_status_str(response) << std::endl;
                return false;
            }

            // parse response as BER-TLV
            auto record_tlv_top(xpcsc::BerTlv::parse(response.substr(0, response.size()-2)));
            const auto & record_tlv = record_tlv_top->get_children().at(0);
            const auto & ch = record_tlv->get_children();

            for (auto x=ch.begin(); x!=ch.end(); x++) {
                auto t = tag_to_long((*x)->get_tag());
                values[t] = (*x)->get_data();
                std::cout << xpcsc::format((*x)->get_tag()) << " => " << tag_to_string(t) <<std::endl;
            }
        }

        i++;
    }

    // print known values
    std::cout << "Card number: " << xpcsc::format(values[0x5A]) << std::endl;
    std::cout << "Card holder name: " << values[0x5F20].c_str() << std::endl;
    std::cout << "Effective date: " << xpcsc::format(values[0x5F25]) << std::endl;
    std::cout << "Exp date: " << xpcsc::format(values[0x5F24]) << std::endl;

    // std::cout << "status: " <<  c.response_status_str(response) << std::endl;
    return true;
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
        // cannot fetch apps from PSE, fill list with known/support AIDs
        apps.push_back(AID_MASTERCARD);
        apps.push_back(AID_VISA_ELECTRON);
        apps.push_back(AID_PRO100);
        apps.push_back(AID_VISA_CLASSIC);
    }

    for (auto aid=apps.begin(); aid!=apps.end(); aid++) {
        if (read_app(c, reader, *aid)) {
            break;
        }
    }

    return 0;
}
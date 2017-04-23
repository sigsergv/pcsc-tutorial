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

#include <iostream>

#include "../include/xpcsc.hpp"
#include "debug.hpp"

#define CHECK_BIT(value, b) (((value) >> (b))&1)

namespace xpcsc {

BerTlv::BerTlv(const Bytes & tag, const Bytes & data)
{
    this->tag = tag;
    // PRINT_DEBUG("PARSING TAG: " << format(tag) << ", DATA: " << format(data));
    if (tag.empty()) {
        // assume it's topmost object, so parse data as BER-TLV
        raw = false;
    } else {
        Byte b = tag.at(0);
        if (CHECK_BIT(b, 5) == 0) {
            // "data" is primitive value, don't parse
            raw = true;
            this->data.assign(data);
            return;
        } else {
            // data is BER-TLV-encoded, parse 
            raw = false;
        }
    }

    // parse data as a list of BER-TLV encoded values
    uint16_t p = 0;

    while (true) {
        if (p >= data.size()) {
            break;
        }
        Byte tag_bytes[3];
        size_t tag_length = 1;

        Byte b = data.at(p);
        tag_bytes[0] = b;
        
        if ((b & 0x1F) == 0x1F) {
            while (true) {
                // catch all other tag bytes
                p++;
                tag_length++;
                if (tag_length > 3) {
                    throw std::out_of_range("tag length > 3, not supported in ISO 7816-4");
                }
                b = data.at(p);
                tag_bytes[tag_length-1] = b;

                if (CHECK_BIT(b, 7) == 0) {
                    // tag is complete
                    break;
                }
            }
        }

        // match length
        p++;
        b = data.at(p);
        uint32_t length;

        if (CHECK_BIT(b, 7) == 1) {
            size_t length_length = (b & 0x7F);
            length = 0;

            if (length_length > 4) {
                PRINT_DEBUG(length_length);
                throw std::out_of_range("length field length > 4, not supported in ISO 7816-4");
            }
            for (size_t i=0; i<length_length; i++) {
                p++;
                b = data.at(p);
                length = (length << 8) + b;
            }
        } else {
            length = b;
        }

        p++;
        BerTlvRef x(new BerTlv( Bytes(tag_bytes, tag_length), data.substr(p, length)));
        children.push_back(x);
        p += length;
    }
}

BerTlv::~BerTlv()
{
    PRINT_DEBUG("[D] BerTlv destroyed");
}

PBerTlv BerTlv::parse(const Bytes & data)
{
    Bytes empty_tag;
    auto x = new BerTlv(empty_tag, data);
    return x;
}

const BerTlvList & BerTlv::get_children() const
{
    return children;
}

const Bytes & BerTlv::get_tag() const
{
    return tag;
}

const Bytes & BerTlv::get_data() const
{
    return data;
}

bool BerTlv::is_raw() const
{
    return raw;
}

const BerTlvRef BerTlv::find_by_tag(const Bytes & tag) const
{
    BerTlvList::const_iterator i;
    BerTlvRef ref;

    for (i = children.begin(); i != children.end(); i++) {
        if ((*i)->get_tag().compare(tag) == 0) {
            ref = *i;
            break;
        }
    }

    return ref;
}

}
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

#include <sstream>

#include "../include/xpcsc.hpp"

namespace xpcsc {

const char *format_strings[] = {"%02X", "0x%02x", "\\x%02X"};
const char *sep_strings[] = {" ", ", ", ""};

std::string format(const Bytes & b, FormatOptions fo)
{
    char cbuf[32];
    const auto fmt = format_strings[fo];
    const auto sep = std::string(sep_strings[fo]);

    auto i = b.begin();
    auto end = b.end();
    std::stringstream ss;

    while (1) {
        if (i == end) {
            break;
        }
        snprintf(cbuf, 31, fmt, *i);
        ss << cbuf;
        i++;
        if (i != end) {
            ss << sep;
        }
    }

    return ss.str();
}

std::string format(const Byte & c, FormatOptions fo)
{
    char cbuf[32];
    const auto fmt = format_strings[fo];

    snprintf(cbuf, 31, fmt, c);
    return cbuf;
}

std::string format(const BerTlv & tlv, FormatOptions fo, uint8_t indent_level)
{
    std::stringstream ss;
    std::string indent(2*indent_level, ' ');
    const auto & tag = tlv.get_tag();
       
    if (tag.size() != 0) {
        ss << indent << "Tag: " << format(tlv.get_tag(), fo) << "\n";
    }
    if (tlv.is_raw()) {
        ss << indent << "Data: " << format(tlv.get_data(), fo) << "\n";
    }
    const auto & items = tlv.get_children();
    BerTlvList::const_iterator i;

    for (i=items.begin(); i!=items.end(); i++) {
        ss << format(*(*i), fo, indent_level+1);
    }
    return ss.str();
}

std::string format(const BerTlv & tlv, FormatOptions fo)
{
    return format(tlv, fo, 0);
}

}
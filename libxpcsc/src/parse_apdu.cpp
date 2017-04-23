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

#include <string>
#include <sstream>
#include <iostream>

#include <cctype>

#include "../include/xpcsc.hpp"

namespace xpcsc {

Byte char2byte(const char & c, bool & ok)
{
    ok = true;

    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }

    ok = false;
    return 0;
}

Bytes parse_apdu(const std::string & apdu)
{
    Byte bs[apdu.size()];
    uint16_t length = 0;


    bool ok;
    Byte b1;
    Byte b2;

    for (auto i=apdu.begin(); i!=apdu.end(); i++) {
        char c = tolower(*i);
        if (c == ' ') {
            continue;
        }
        b1 = char2byte(c, ok);
        if (!ok) {
            throw APDUParseError("Incorrect character");
        }
        i++;
        if (i==apdu.end()) {
            throw APDUParseError("Unexpected input end");
        }

        c = tolower(*i);
        b2 = char2byte(c, ok);
        if (!ok) {
            throw APDUParseError("Incorrect character");
        }

        bs[length] = (Byte)(b1*16 + b2);
        length++;
    }

    return Bytes(bs, length);
}

}
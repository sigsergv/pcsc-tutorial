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

#include <stdexcept>
#include <vector>
#include <string>

namespace xpcsc {

typedef std::basic_string<unsigned char> Bytes;
typedef std::vector<std::string> Strings;

/*
 * Incapsulates pcsc-lite error
 */
class PCSCError : public std::runtime_error {
public:
    PCSCError(long);
    long code() const throw ();
    virtual const char* what() const throw ();
private:
    long pcsc_code;
};

class ConnectionError : public std::runtime_error {
public:
    ConnectionError(const char * what);
};


class Connection {
    /*
     * Incapsulates pcsc-lite library
     */
public:
    Connection();

    ~Connection();

    void init();

    Strings readers();

    void wait_for_card(const std::string & reader);

    Bytes atr();

private:
    struct Private;
    Private * p;

    void handle_pcsc_response_code(long response);
    void release_context();
    void release_card_handle();
};


class ATRParseError : public std::runtime_error {
public:
    ATRParseError(const char * what);
};

class ATRParser
{
public:
    ATRParser();
    ATRParser(const Bytes & bytes);
    ~ATRParser();

    std::string str() const;

    void load(const Bytes & bytes);

private:
    struct Private;
    Private * p;
};


// formatting functions for bytes
typedef enum { 
    FormatHex = 0,  // HEX, like "01 ef 4d"
    FormatC,    // C, like "0x01 0xef 0x4d"
    FormatE     // string escaped, like "\x01\xef\x4d"
} FormatOptions;

std::string format(Bytes, FormatOptions fo = FormatHex);
std::string format(unsigned char, FormatOptions fo = FormatHex);

}
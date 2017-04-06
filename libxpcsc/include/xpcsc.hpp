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

#ifndef _H_7a75aca51ebac9ef34833761fe6b11e0
#define _H_7a75aca51ebac9ef34833761fe6b11e0

#include <stdexcept>
#include <vector>
#include <string>

namespace xpcsc {

typedef uint8_t Byte;
typedef std::basic_string<Byte> Bytes;
typedef std::vector<std::string> Strings;

// some useful constants
// CLA
const Byte CLA_PICC = 0xff;

// INS
const Byte INS_MIFARE_LOAD_KEYS = 0x82;
const Byte INS_MIFARE_GENERAL_AUTH = 0x86;
const Byte INS_MIFARE_READ_BINARY = 0xb0;
const Byte INS_MIFARE_UPDATE_BINARY = 0xd6;
const Byte INS_PICC_GET_DATA = 0xca;

// ATR features constants
typedef enum { 
    // smart card with contacts
    ATR_FEATURE_ICC,
    // proximity card
    ATR_FEATURE_PICC,
    // Mifare cards
    ATR_FEATURE_MIFARE_1K,
    ATR_FEATURE_MIFARE_4K,
    ATR_FEATURE_INFINEON_SLE_66R35 
} ATRFeature;

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

    void transmit(const Bytes & command, Bytes * response = 0);

    static int response_status(const Bytes & response);
    static std::string response_status_str(const Bytes & response);
    static Bytes response_data(const Bytes & response);

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

    bool checkFeature(ATRFeature);

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

typedef Byte BlocksAccessBits[4];

std::string format(Bytes, FormatOptions fo = FormatHex);
std::string format(Byte, FormatOptions fo = FormatHex);

bool parse_access_bits(Byte b7, Byte b8, BlocksAccessBits * bits);
}

#endif
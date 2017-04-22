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

#ifdef __APPLE__
#include <PCSC/pcsclite.h>
// #include <PCSC/winscard.h>
// #include <PCSC/wintypes.h>
#else
#include <pcsclite.h>
// #include <winscard.h>
// #include <wintypes.h>
#endif

#include "../include/xpcsc.hpp"


namespace xpcsc {

PCSCError::PCSCError(long code)
    : std::runtime_error(""), pcsc_code(code)
{
}

const char * PCSCError::what() const throw ()
{
    return pcsc_stringify_error(pcsc_code);
}

long PCSCError::code() const throw ()
{
    return pcsc_code;
}

ConnectionError::ConnectionError(const char * what)
    : std::runtime_error(what)
{
}

ATRParseError::ATRParseError(const char * what)
    : std::runtime_error(what)
{}

APDUParseError::APDUParseError(const char * what)
    : std::runtime_error(what)
{}

BERTLVParseError::BERTLVParseError(const char * what)
    : std::runtime_error(what)
{}

}

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

/**
 * @file pcsc_context.cpp
 * @author Sergey Stolyarov <sergei@regolit.com>
 * @brief Класс для работы со списком байтов.
 */

#include <stdexcept>
#include <iostream>
#include <sstream>
#include <string.h>
#include <stdio.h>

#include "bytes_list.h"
#include "debug.h"

std::string byte_format_str(bytes_list::format_t f);
std::string byte_sep_str(bytes_list::format_t f);

struct bytes_list::Private
{
    unsigned int size;
    unsigned char * data;

    Private() {
        size = 0;
        data = 0;
    };

    ~Private() {
        if (data != 0) {
            delete[] data;
        }
    }
};

bytes_list::bytes_list()
{
    p = new Private;
    PRINT_DEBUG("=> Created empty bytes_list instance");
}

bytes_list::bytes_list(const unsigned char * src, size_t size)
{
    p = new Private;
    PRINT_DEBUG("=> Created bytes_list instance from bytes");
    if (size == 0) {
        return;
    }

    p->size = size;
    p->data = new unsigned char[size];
    memcpy(p->data, src, size);
}

bytes_list::bytes_list(const bytes_list & src)
{
    p = new Private;
    p->size = (src.p)->size;

    if ((src.p)->size > 0) {
        p->data = new unsigned char[(src.p)->size];
        memcpy(p->data, (src.p)->data, (src.p)->size);
    }
    PRINT_DEBUG("=> Created bytes_list by copy.");
}

bytes_list::~bytes_list()
{
    PRINT_DEBUG("=> Destroyed bytes_list instance");
    delete p;
}

std::string bytes_list::str(bytes_list::format_t f) const
{
    char cbuf[32];
    std::string format_str = byte_format_str(f);
    std::string sep = byte_sep_str(f);
    std::stringstream ss;

    const char * fmt = format_str.c_str();

    for (size_t i=0; i < (p->size); i++) {
        snprintf(cbuf, 31, fmt, (p->data)[i]);
        ss << cbuf;
        if (i+1 != p->size) {
            ss << sep;
        }
    }
    return ss.str();
}

size_t bytes_list::size() const
{
    return p->size;
}

void bytes_list::replace(const bytes_list & src)
{
    if (p->size > 0) {
        delete[] p->data;
        p->size = 0;
    }

    p->size = (src.p)->size;

    if ((src.p)->size > 0) {
        p->data = new unsigned char[(src.p)->size];
        memcpy(p->data, (src.p)->data, (src.p)->size);
    }
}

unsigned char bytes_list::at(size_t index) const
{
    if (index >= p->size) {
        throw std::out_of_range("at(): out of range");
    }

    return p->data[index];
}

std::string bytes_list::byte_str(unsigned char byte, bytes_list::format_t f)
{
    char cbuf[32];
    std::string format_str = byte_format_str(f);

    const char * fmt = format_str.c_str();

    snprintf(cbuf, 31, fmt, byte);

    return cbuf;
}

std::string bytes_list::at_str(size_t index, bytes_list::format_t f) const
{
    return byte_str(at(index));
}

bytes_list bytes_list::slice(size_t pos, size_t length) const
{
    if (pos >= p->size && (pos+length) > p->size) {
        throw std::out_of_range("out of range");
    }

    return bytes_list(p->data + pos, length);
}

std::string byte_format_str(bytes_list::format_t f)
{
    std::string s;

    switch (f) {
    case bytes_list::format_c:
        s = "0x%02x";
        break;

    case bytes_list::format_o:
        s = "%02X";
        break;

    case bytes_list::format_s:
        s = "\\x%02X";
        break;
    }

    return s;
}

std::string byte_sep_str(bytes_list::format_t f)
{
    std::string s;

    switch (f) {
    case bytes_list::format_c:
        s = ", ";
        break;

    case bytes_list::format_o:
        s = " ";
        break;

    case bytes_list::format_s:
        s = "";
        break;
    }

    return s;
}
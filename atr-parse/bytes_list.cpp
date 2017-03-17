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
 * \file pcsc_context.cpp
 * \author Sergey Stolyarov <sergei@regolit.com>
 * \brief bytes list class
 */

#include <iostream>
#include <string.h>
#include <stdio.h>

#include "bytes_list.h"

struct bytes_list::Private
{
    unsigned int size = 0;
    unsigned char * data = 0;
};

bytes_list::bytes_list()
{
    std::cerr << "created bytes_list instance" << std::endl;
    p = new Private;
}

bytes_list::bytes_list(const unsigned char * src, size_t size)
{
    p = new Private;
    std::cerr << "created bytes_list instance from bytes" << std::endl;
    if (size == 0) {
        return;
    }

    p->size = size;
    p->data = new unsigned char[size];
    memcpy(p->data, src, size);
}

bytes_list::~bytes_list()
{
    std::cerr << "destroyed bytes_list instance" << std::endl;
    delete[] p->data;
    delete p;
}

std::string bytes_list::format()
{
    char cbuf[6];
    std::string s;

    for (size_t i=0; i < (p->size); i++) {
        snprintf(cbuf, 5, "%02X", (p->data)[i]);
        s.append(cbuf);
        s.append(" ");
    }
    return s;
}
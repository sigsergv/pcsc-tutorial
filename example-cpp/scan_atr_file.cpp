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
 * @file test_atr_parser.cpp
 * @author Sergey Stolyarov <sergei@regolit.com>
 * @brief Scan ATR from the file
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <list>

#include "bytes_list.h"
#include "atr_parser.h"

unsigned char char_to_byte(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return c+10-'a';
    }
    if (c >= 'A' && c <= 'F') {
        return c+10-'A';
    }

    return 255;
}
int main(int argc, char **argv)
{
    if (argc != 2) {
        std::cerr << "File name is required!" << '\n';
        return 1;
    }

    std::string filename(argv[1]);

    std::ifstream file;
    file.open(filename);

    std::string line;

    char digits[] = {'0'};
    char b1[5];

    unsigned char atr_buf[36];
    int atr_size;

    ATRParser parser;

    while (std::getline(file, line)) {
        atr_size = 0;

        int i;
        for (i=0; i<line.length(); i++) {
            char c = line[i];
            if (c == '\t') {
                break;
            }
            unsigned char x = char_to_byte(c);
            unsigned char y;

            if (x != 255) {
                // hex digit, next one must be hex digit too
                i++;
                y = char_to_byte(line[i]);
                if (y != 255) {
                    atr_buf[atr_size] = x*16+y;
                    atr_size++;
                } else {
                    std::cerr << "not supported line: " << line << std::endl;
                    atr_size = 0;
                    break;
                }
            } else if (c != ' ') { 
                std::cerr << "not supported line: " << line << std::endl;
                atr_size = 0;
                break;
            }
        }

        std::stringstream desc_ss;
        i++;
        for (; i<line.length(); i++) {
            desc_ss << line[i];
        }

        if (atr_size > 0) {
            bytes_list atr(atr_buf, atr_size);
            std::cout << desc_ss.str() << std::endl;
            std::cout << atr.str() << std::endl;
            try {
                parser.load(atr);
                std::cout << parser.str() << std::endl;
            } catch (std::length_error & e) {
                std::cerr << e.what() << std::endl;
            }
        }
    }
}
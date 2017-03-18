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
 */

#include <iostream>

#include "bytes_list.h"
#include "atr_parser.h"

int main(int argc, char **argv)
{
    // Mifare Classic 1K
    unsigned char A1[] = {0x3b, 0x8f, 0x80, 0x01, 0x80, 0x4f, 0x0c, 0xa0, 0x00, 0x00, 0x03, 0x06, 0x03, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x6a};

    // UEC (PICC)
    unsigned char A2[] = {0x3b, 0x8c, 0x80, 0x01, 0x80, 0x64, 0x11, 0x65, 0x01, 0x90, 0x73, 0x00, 0x00, 0x00, 0x81, 0x07, 0xf9};

    // TCS bank card (PICC)
    unsigned char A3[] = {0x3b, 0x8e, 0x80, 0x01, 0x80, 0x31, 0x80, 0x66, 0xb0, 0x84, 0x12, 0x01, 0x6e, 0x01, 0x83, 0x00, 0x90, 0x00, 0x03};

    // German "Versichertenkarte" Healthcare card, see https://www.eftlab.co.uk/index.php/site-map/knowledge-base/171-atr-list-full
    unsigned char A4[] = {0x3b, 0x0e, 0xbc, 0xe1, 0xca, 0xaf, 0xc2, 0xdf, 0xbc, 0xad, 0xbc, 0xd3, 0xc3, 0xdc, 0xbf, 0xa8};

    // Reliance SIM (Prepaid), Telecommunication, see https://www.eftlab.co.uk/index.php/site-map/knowledge-base/171-atr-list-full
    unsigned char A5[] = {0x3b, 0x3f, 0x11, 0x00, 0x80, 0x69, 0xaf, 0x03, 0x37, 0x00, 0x12, 0x00, 0x00, 0x00, 0x0e, 0x83, 0x18, 0x9f, 0x16};

    // bytes_list atr_1(A1, sizeof(A1));
    bytes_list atr_1(A1, sizeof(A1));
    bytes_list atr_2(A2, sizeof(A2));
    bytes_list atr_3(A3, sizeof(A3));
    bytes_list atr_4(A4, sizeof(A4));
    bytes_list atr_5(A5, sizeof(A5));

    ATRParser parser;

    std::cout << "\nRaw ATR1: " << atr_1.str(bytes_list::format_o) << '\n';
    std::cout << "Parsed ATR:" << '\n';
    parser.load(atr_1);
    std::cout << parser.str();

    std::cout << "\nRaw ATR2: " << atr_2.str(bytes_list::format_o) << '\n';
    std::cout << "Parsed ATR:" << '\n';
    parser.load(atr_2);
    std::cout << parser.str();

    std::cout << "\nRaw ATR3: " << atr_3.str(bytes_list::format_o) << '\n';
    std::cout << "Parsed ATR:" << '\n';
    parser.load(atr_3);
    std::cout << parser.str();

    // std::cout << "\nRaw ATR4: " << atr_4.str(bytes_list::format_o) << '\n';
    // std::cout << "Parsed ATR:" << '\n';
    // parser.load(atr_4);
    // std::cout << parser.str();

    // std::cout << "\nRaw ATR5: " << atr_5.str(bytes_list::format_o) << '\n';
    // std::cout << "Parsed ATR:" << '\n';
    // parser.load(atr_5);
    // std::cout << parser.str();

}
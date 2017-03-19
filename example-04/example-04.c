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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __APPLE__
#include <PCSC/pcsclite.h>
#include <PCSC/winscard.h>
#include <PCSC/wintypes.h>
#else
#include <pcsclite.h>
#include <winscard.h>
#include <wintypes.h>
#endif

#include "../util/util.h"

int main(int argc, char **argv)
{
    LONG result;
    SCARDCONTEXT sc_context;

    // initialize pcsc library
    result = SCardEstablishContext(SCARD_SCOPE_SYSTEM, NULL, NULL, &sc_context);
    if (result != SCARD_S_SUCCESS) {
        printf("%s\n", pcsc_stringify_error(result));
        return 1;
    }
    printf("Connection to PC/SC established\n");

    // calculate required memory size for a list of readers
    DWORD readers_size;

    result = SCardListReaders(sc_context, NULL, 0, &readers_size);
    if (result != SCARD_S_SUCCESS) {
        SCardReleaseContext(sc_context);
        printf("%s\n", pcsc_stringify_error(result));
        return 1;
    }

    // allocate memory and fetch readers list
    LPSTR readers;
    readers = calloc(1, readers_size);

    result = SCardListReaders(sc_context, NULL, readers, &readers_size);
    if (result != SCARD_S_SUCCESS) {
        SCardReleaseContext(sc_context);
        printf("%s\n", pcsc_stringify_error(result));
        return 1;
    }

    // take first reader, 256-bytes buffer should be enough
    char reader[256] = {0};

    if (readers_size > 1) {
        strncpy(reader, readers, 255);
    } else {
        printf("No readers found!\n");
        return 2;
    }

    free(readers);

    printf("Use reader '%s'\n", reader);

    // connect to reader and wait for card
    SCARD_READERSTATE sc_reader_states[1];
    sc_reader_states[0].szReader = reader;
    sc_reader_states[0].dwCurrentState = SCARD_STATE_EMPTY;

    result = SCardGetStatusChange(sc_context, INFINITE, sc_reader_states, 1);
    if (result != SCARD_S_SUCCESS) {
        SCardReleaseContext(sc_context);
        printf("%s\n", pcsc_stringify_error(result));
        return 1;
    }

    // connect to inserted card
    SCARDHANDLE card;
    DWORD active_protocol;

    result = SCardConnect(sc_context, reader, 
        SCARD_SHARE_SHARED, SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1,
        &card, &active_protocol);
    if (result != SCARD_S_SUCCESS) {
        SCardReleaseContext(sc_context);
        printf("%s\n", pcsc_stringify_error(result));
        return 1;
    }

    if (active_protocol == SCARD_PROTOCOL_T0) {
        printf("Card connected, protocol to use: T0.\n");
    }
    if (active_protocol == SCARD_PROTOCOL_T1) {
        printf("Card connected, protocol to use: T1.\n");
    }

    LONG send_buffer_size;
    BYTE send_buffer[100];
    const LONG recv_buffer_size = 100;
    BYTE recv_buffer[recv_buffer_size];
    BYTE SW1;
    BYTE SW2;
    int SW;

    // 1. load authentication key, see pcsc spec 3.2.2.1.4
    send_buffer_size = 11;
    // CLA = 0xff
    // INS = 0x82 "Load Keys Command"
    // P1 = 0x00 - Key structure, set of bits defined in pcsc spec 3.2.2.1.4
    // P2 = 0x00 - Key number, reader will store key in this "position", 0x00 or 0x01
    // Lc = 0x06 - Length of command bytes, key length (6 bytes for Mifare keys)
    // Command bytes = { key bytes }

    //                  CLA    INS    P1     P2     Lc
    memcpy(send_buffer, "\xff" "\x82" "\x00" "\x00" "\x06" "\xff\xff\xff\xff\xff\xff", send_buffer_size);

    // other possible keys
    // \xd3\xf7\xd3\xf7\xd3\xf7
    // \xa0\xa1\xa2\xa3\xa4\xa5
    // \x73\x06\x8f\x11\x8c\x13
    // \xff\xff\xff\xff\xff\xff
    DWORD recv_length = recv_buffer_size;

    result = SCardTransmit(card, SCARD_PCI_T1, 
        send_buffer, send_buffer_size, NULL,
        recv_buffer, &recv_length);
    if (result != SCARD_S_SUCCESS) {
        SCardDisconnect(card, SCARD_RESET_CARD);
        SCardReleaseContext(sc_context);
        printf("%s\n", pcsc_stringify_error(result));
        return 1;
    }

    SW1 = recv_buffer[recv_length-2];
    SW2 = recv_buffer[recv_length-1];
    SW = SW1 * 256 + SW2;

    if (SW != 0x9000) {
        printf("Failed to set keys\n");
        printf("Response APDU: ");
        print_bytes(recv_buffer, recv_length);
        SCardDisconnect(card, SCARD_RESET_CARD);
        SCardReleaseContext(sc_context);
        return 1;
    }

    // 2. authentication, see pcsc spec 3.2.2.1.6 
    recv_length = recv_buffer_size;
    send_buffer_size = 10;
    // CLA = 0xff
    // INS = 0x86 "General Authenticate Command"
    // P1 =  0x00 must be 0x00
    // P2 =  0x00 must be 0x00
    // Lc =  0x5  Length of command bytes
    // Command bytes = {
    //   0x01  Version
    //   0x00  Block number MSB
    //   0x00  Block number LSB, 0x0000 in this case
    //   0x60  Key type, 0x60 means Key A
    //   0x00  Key number 0x00
    // }
    //                  CLA    INS    P1     P2     Lc     Command bytes
    memcpy(send_buffer, "\xff" "\x86" "\x00" "\x00" "\x05" "\x01\x00\x00\x60\x00", send_buffer_size);

    result = SCardTransmit(card, SCARD_PCI_T1, 
        send_buffer, send_buffer_size, NULL,
        recv_buffer, &recv_length);
    if (result != SCARD_S_SUCCESS) {
        SCardDisconnect(card, SCARD_RESET_CARD);
        SCardReleaseContext(sc_context);
        printf("%s\n", pcsc_stringify_error(result));
        return 1;
    }

    SW1 = recv_buffer[recv_length-2];
    SW2 = recv_buffer[recv_length-1];
    SW = SW1 * 256 + SW2;

    if (SW != 0x9000) {
        printf("Failed to auth\n");
        printf("Response APDU: ");
        print_bytes(recv_buffer, recv_length);
        SCardDisconnect(card, SCARD_RESET_CARD);
        SCardReleaseContext(sc_context);
        return 1;
    }


    // 3. read binary block
    recv_length = recv_buffer_size;
    send_buffer_size = 5;
    // CLA = 0xff
    // INS = 0xb0 "Read Binary Command"
    // P1 =  Block number MSB
    // P2 =  Block number LSB, read block 0x0001
    // Le =  Expected response length 0x10 = 16 bytes
    //                  CLA    INS    P1     P2     Le
    memcpy(send_buffer, "\xff" "\xb0" "\x00" "\x01" "\x10", send_buffer_size);

    result = SCardTransmit(card, SCARD_PCI_T1, 
        send_buffer, send_buffer_size, NULL,
        recv_buffer, &recv_length);
    if (result != SCARD_S_SUCCESS) {
        SCardDisconnect(card, SCARD_RESET_CARD);
        SCardReleaseContext(sc_context);
        printf("%s\n", pcsc_stringify_error(result));
        return 1;
    }

    SW1 = recv_buffer[recv_length-2];
    SW2 = recv_buffer[recv_length-1];
    SW = SW1 * 256 + SW2;

    if (SW != 0x9000) {
        printf("Failed to fetch block\n");
        printf("Response APDU: ");
        print_bytes(recv_buffer, recv_length);
        SCardDisconnect(card, SCARD_RESET_CARD);
        SCardReleaseContext(sc_context);
        return 1;
    }

    printf("Block bytes: ");
    print_bytes(recv_buffer, recv_length-2);

    // disconnect from card
    result = SCardDisconnect(card, SCARD_RESET_CARD);
    if (result != SCARD_S_SUCCESS) {
        SCardReleaseContext(sc_context);
        printf("%s\n", pcsc_stringify_error(result));
        return 1;
    }

    // relase connection to library
    result = SCardReleaseContext(sc_context);
    if (result != SCARD_S_SUCCESS) {
        printf("%s\n", pcsc_stringify_error(result));
        return 1;
    }
    printf("Connection to PC/SC closed\n");


    return 0;
}

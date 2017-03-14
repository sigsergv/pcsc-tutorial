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

    // fetch reader status
    char reader_friendly_name[MAX_READERNAME];
    DWORD reader_friendly_name_size = MAX_READERNAME;
    DWORD state;
    DWORD protocol;
    DWORD atr_size = MAX_ATR_SIZE;
    BYTE atr[MAX_ATR_SIZE];

    result = SCardStatus(card, reader_friendly_name, &reader_friendly_name_size, 
        &state, &protocol, atr, &atr_size);
    if (result != SCARD_S_SUCCESS) {
        SCardDisconnect(card, SCARD_RESET_CARD);
        SCardReleaseContext(sc_context);
        printf("%s\n", pcsc_stringify_error(result));
        return 1;
    }

    printf("Card ATR: ");
    print_bytes(atr, atr_size);

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
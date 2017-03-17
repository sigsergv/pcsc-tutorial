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
 * \brief PC/SC Context wrapper object
 */

#include <stdexcept>
#include <string>
#include <iostream>

#ifdef __APPLE__
#include <PCSC/pcsclite.h>
#include <PCSC/winscard.h>
#include <PCSC/wintypes.h>
#else
#include <pcsclite.h>
#include <winscard.h>
#include <wintypes.h>
#endif

#include "pcsc_context.h"

struct PCSCContext::Private
{
    SCARDCONTEXT context = 0;
    SCARDHANDLE card = 0;

    void release_context() {
        if (context == 0) {
            return;
        }
        SCardReleaseContext(context);
        context = 0;
    };

    void release_card_handle() {
        if (card == 0) {
            return;
        }
        SCardDisconnect(card, SCARD_RESET_CARD);
        card = 0;
    };
};

#define CONTEXT_READY_CHECK if (p->context == 0) { throw std::runtime_error("Context not ready!"); }
#define CARD_READY_CHECK if (p->card == 0) { throw std::runtime_error("Card not ready!"); }

void make_runtime_error(LONG, const std::string &);

PCSCContext::PCSCContext()
{
    p = new Private;

    LONG result;

    result = SCardEstablishContext(SCARD_SCOPE_SYSTEM, NULL, NULL, &(p->context));
    if (result != SCARD_S_SUCCESS) {
        make_runtime_error(result, "SCardEstablishContext()");
    }
    std::cerr << "=> Created" << std::endl;
}

std::list<std::string> PCSCContext::readers()
{
    std::list<std::string> r;

    DWORD readers_buffer_size;
    LONG result;

    result = SCardListReaders(p->context, NULL, 0, &readers_buffer_size);
    if (result != SCARD_S_SUCCESS) {
        p->release_context();
        make_runtime_error(result, "SCardListReaders()");
    }

    // allocate memory and fetch readers list
    LPSTR readers_buffer = new char[readers_buffer_size];

    result = SCardListReaders(p->context, NULL, readers_buffer, &readers_buffer_size);
    if (result != SCARD_S_SUCCESS) {
        delete readers_buffer;
        p->release_context();
        make_runtime_error(result, "SCardListReaders()");
    }

    DWORD n = 0;
    for (DWORD i = 0; i+1 < readers_buffer_size; i++) {
        n++;
        std::string s(&readers_buffer[i]);
        r.push_back(s);
        while (readers_buffer[++i] != 0);
    }

    return r;
}

PCSCContext::~PCSCContext()
{
    p->release_card_handle();
    p->release_context();

    delete p;
    std::cerr << "=> Destroyed" << std::endl;
}

void PCSCContext::wait_for_card(std::string reader)
{
    CONTEXT_READY_CHECK

    LONG result;
    SCARD_READERSTATE sc_reader_states[1];
    sc_reader_states[0].szReader = reader.c_str();
    sc_reader_states[0].dwCurrentState = SCARD_STATE_EMPTY;

    result = SCardGetStatusChange(p->context, INFINITE, sc_reader_states, 1);
    if (result != SCARD_S_SUCCESS) {
        p->release_context();
        make_runtime_error(result, "SCardGetStatusChange()");
    }

    DWORD active_protocol;
    result = SCardConnect(p->context, reader.c_str(), 
        SCARD_SHARE_SHARED, SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1,
        &(p->card), &active_protocol);

    if (result != SCARD_S_SUCCESS) {
        p->release_context();
        make_runtime_error(result, "SCardGetStatusChange()");
    }
}

bytes_list PCSCContext::atr()
{
    CARD_READY_CHECK

    LONG result;
    DWORD state;
    char reader_friendly_name[MAX_READERNAME];
    DWORD reader_friendly_name_size = MAX_READERNAME;
    DWORD protocol;
    DWORD atr_size = MAX_ATR_SIZE;
    BYTE atr[MAX_ATR_SIZE];

    result = SCardStatus(p->card, reader_friendly_name, &reader_friendly_name_size, 
        &state, &protocol, atr, &atr_size);
    if (result != SCARD_S_SUCCESS) {
        p->release_context();
        p->release_card_handle();
        make_runtime_error(result, "SCardStatus()");   
    }

    bytes_list b(atr, atr_size);
    return b;
}


void make_runtime_error(LONG result, const std::string & name)
{
    char * pcsc_error = pcsc_stringify_error(result);
    std::string message = name + ": " + pcsc_error;
    throw std::runtime_error(message);
}
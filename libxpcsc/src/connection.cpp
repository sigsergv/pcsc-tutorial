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
#include <string>
#include <iostream>

#include "../include/xpcsc.hpp"
#include "debug.hpp"

namespace xpcsc {

// these are NOT asserts
#define CONTEXT_READY_CHECK() if (p->context == 0) { throw ConnectionError("Context not ready!"); }

#define PCSC_CALL(f) do {long _result = (f);  handle_pcsc_response_code(_result); } while (0)

// void make_runtime_error(LONG, const std::string &);

struct Connection::Private
{
    SCARDCONTEXT context;

    Private() {
        context = 0;
    }
};

Connection::Connection()
{
    p = new Private;
}

Connection::~Connection()
{
    release_context();
    delete p;
    PRINT_DEBUG("[D] Destroyed xpcsc::Connection object");
}

void Connection::init()
{
    PCSC_CALL( SCardEstablishContext(SCARD_SCOPE_SYSTEM, NULL, NULL, &(p->context)) );
    PRINT_DEBUG("[D] Created xpcsc::Connection object ");
}

Strings Connection::readers()
{
    CONTEXT_READY_CHECK();

    Strings r;

    DWORD readers_buffer_size;

    PCSC_CALL( SCardListReaders(p->context, NULL, 0, &readers_buffer_size) );

    // allocate memory and fetch readers list
    LPSTR readers_buffer = new char[readers_buffer_size];

    try {
        PCSC_CALL( SCardListReaders(p->context, NULL, readers_buffer, &readers_buffer_size) );
    } catch (PCSCError &e) {
        delete[] readers_buffer;
        throw e;
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

xpcsc::Reader Connection::wait_for_reader_card(const std::string & reader_name, DWORD preferred_protocols)
{
    xpcsc::Reader reader;

    CONTEXT_READY_CHECK();

    SCARD_READERSTATE sc_reader_states[1];
    DWORD state;

    sc_reader_states[0].szReader = reader_name.c_str();

    while (1) {
        // get current state
        sc_reader_states[0].dwCurrentState = SCARD_STATE_UNAWARE;
        PCSC_CALL(SCardGetStatusChange(p->context, INFINITE, sc_reader_states, 1));

        // card in the reader, stop
        if (sc_reader_states[0].dwEventState & SCARD_STATE_PRESENT) {
            break;
        }

        // clean events number (just in case)
        state = (sc_reader_states[0].dwEventState) & 0xffff;

        // and wait when state changes
        sc_reader_states[0].dwCurrentState = state;
        PCSC_CALL(SCardGetStatusChange(p->context, INFINITE, sc_reader_states, 1));
    }

    DWORD active_protocol;

    PCSC_CALL( SCardConnect(p->context, reader_name.c_str(), 
        SCARD_SHARE_SHARED, preferred_protocols,
        &(reader.handle), &active_protocol) );

    if (active_protocol == SCARD_PROTOCOL_T0) {
        reader.send_pci = SCARD_PCI_T0;
    } else if (active_protocol == SCARD_PROTOCOL_T1) {
        reader.send_pci = SCARD_PCI_T1;
    } else {
        throw ConnectionError("Not supported protocol!");
    }

    return reader;
}


void Connection::disconnect_card(const xpcsc::Reader & reader, DWORD disposition)
{
    // all possible values for disposition:
    //   SCARD_LEAVE_CARD  - do nothing
    //   SCARD_RESET_CARD - Reset the card (warm reset).
    //   SCARD_UNPOWER_CARD - Power down the card (cold reset).
    //   SCARD_EJECT_CARD - Eject the card.

    // don't need to check for card

    PCSC_CALL(SCardDisconnect(reader.handle, disposition));
}


Bytes Connection::atr(const xpcsc::Reader & reader)
{
    DWORD state;
    char reader_friendly_name[MAX_READERNAME];
    DWORD reader_friendly_name_size = MAX_READERNAME;
    DWORD protocol;
    DWORD atr_size = MAX_ATR_SIZE;
    BYTE atr[MAX_ATR_SIZE];

    PCSC_CALL(SCardStatus(reader.handle, reader_friendly_name, &reader_friendly_name_size, 
        &state, &protocol, atr, &atr_size));

    Bytes b(atr, atr_size);
    return b;
}


void Connection::transmit(const xpcsc::Reader & reader, const Bytes & command, Bytes * response)
{
    const LONG recv_buffer_size = 1024;
    LONG send_buffer_size = command.length();
    DWORD recv_length = recv_buffer_size;

    // PRINT_DEBUG("[D] Command length: " << send_buffer_size);

    Byte *recv_buffer = new Byte[recv_buffer_size];

    // TODO: select protocol correctly
    try {
        PCSC_CALL( SCardTransmit(reader.handle, reader.send_pci, 
            command.data(), send_buffer_size, NULL,
            recv_buffer, &recv_length) );
    } catch (PCSCError &e) {
        delete[] recv_buffer;
        throw e;
    }

    delete[] recv_buffer;

    if (response != 0) {
        response->assign(recv_buffer, recv_length);
    }
}


void Connection::wait_for_card_remove(const std::string & reader_name)
{
    CONTEXT_READY_CHECK();

    SCARD_READERSTATE sc_reader_states[1];

    sc_reader_states[0].szReader = reader_name.c_str();

    DWORD state;

    while (1) {
        // get current state
        sc_reader_states[0].dwCurrentState = SCARD_STATE_UNAWARE;
        PCSC_CALL(SCardGetStatusChange(p->context, INFINITE, sc_reader_states, 1));

        // no card in the reader, stop
        if (sc_reader_states[0].dwEventState & SCARD_STATE_EMPTY) {
            break;
        }

        // clean events number (just in case)
        state = (sc_reader_states[0].dwEventState) & 0xffff;

        // and wait when state changes
        sc_reader_states[0].dwCurrentState = state;
        PCSC_CALL(SCardGetStatusChange(p->context, INFINITE, sc_reader_states, 1));
    }

    // debugging code
    // 
    // DWORD d = sc_reader_states[0].dwEventState;
    // std::cout << "events: " << (d & 0xFFFF0000) << std::endl;
    // std::cout << "SCARD_STATE_UNAWARE: " << bool(d & SCARD_STATE_UNAWARE) << std::endl;
    // std::cout << "SCARD_STATE_IGNORE: " << bool(d & SCARD_STATE_IGNORE) << std::endl;
    // std::cout << "SCARD_STATE_CHANGED: " << bool(d & SCARD_STATE_CHANGED) << std::endl;
    // std::cout << "SCARD_STATE_UNKNOWN: " << bool(d & SCARD_STATE_UNKNOWN) << std::endl;
    // std::cout << "SCARD_STATE_UNAVAILABLE: " << bool(d & SCARD_STATE_UNAVAILABLE) << std::endl;
    // std::cout << "SCARD_STATE_EMPTY: " << bool(d & SCARD_STATE_EMPTY) << std::endl;
    // std::cout << "SCARD_STATE_PRESENT: " << bool(d & SCARD_STATE_PRESENT) << std::endl;
    // std::cout << "SCARD_STATE_EXCLUSIVE: " << bool(d & SCARD_STATE_EXCLUSIVE) << std::endl;
    // std::cout << "SCARD_STATE_INUSE: " << bool(d & SCARD_STATE_INUSE) << std::endl;
    // std::cout << "SCARD_STATE_MUTE: " << bool(d & SCARD_STATE_MUTE) << std::endl;
    //
    // std::cout << "SCARD_ABSENT: " << bool(state & SCARD_ABSENT) << std::endl;
    // std::cout << "SCARD_PRESENT: " << bool(state & SCARD_PRESENT) << std::endl;
    // std::cout << "SCARD_SWALLOWED: " << bool(state & SCARD_SWALLOWED) << std::endl;
    // std::cout << "SCARD_POWERED: " << bool(state & SCARD_POWERED) << std::endl;
    // std::cout << "SCARD_NEGOTIABLE: " << bool(state & SCARD_NEGOTIABLE) << std::endl;
    // std::cout << "SCARD_SPECIFIC: " << bool(state & SCARD_SPECIFIC) << std::endl;
}



int Connection::response_status(const Bytes & response)
{
    size_t size = response.size();
    if (size < 2) {
        return -1;
    }

    return response.at(size-2)*256 + response.at(size-1);
}

std::string Connection::response_status_str(const Bytes & response)
{
    size_t size = response.size();
    if (size < 2) {
        return "";
    }

    return format(response.substr(size-2, 2));
}


Bytes Connection::response_data(const Bytes & response)
{
    size_t size = response.size();
    return response.substr(0, size-2);
}

/*
 * You should not call this function, it's for PCSC_CALL() macro.
 */
void Connection::handle_pcsc_response_code(long response)
{
    if (response != SCARD_S_SUCCESS) {
        // perform cleanup in case of fatal errors
        switch (response) {
        case SCARD_E_INVALID_HANDLE:
        case SCARD_E_NO_SMARTCARD:
        case SCARD_W_UNPOWERED_CARD:
        case SCARD_W_UNRESPONSIVE_CARD:
        case SCARD_W_REMOVED_CARD:
        case SCARD_W_RESET_CARD:
            // release_card_handle();
            break;

        case SCARD_E_NO_SERVICE:
        case SCARD_E_READER_UNAVAILABLE:
        case SCARD_E_NO_READERS_AVAILABLE:
        case SCARD_E_NO_MEMORY:
        case SCARD_F_INTERNAL_ERROR:
            release_context();
            break;

        case SCARD_F_COMM_ERROR:
        case SCARD_E_PROTO_MISMATCH:
        case SCARD_E_UNSUPPORTED_FEATURE:
        case SCARD_E_UNKNOWN_READER:
        case SCARD_E_NOT_TRANSACTED:
        case SCARD_E_TIMEOUT:
        case SCARD_E_CANCELLED:
            // could be a temporary issue so no cleanup performed
            break;

        case SCARD_E_SHARING_VIOLATION:
        case SCARD_E_INSUFFICIENT_BUFFER:
        case SCARD_E_INVALID_PARAMETER:
        case SCARD_E_INVALID_VALUE:
            // perhaps we should throw a separate exceptions for this cases
            break;
        }
        throw PCSCError(response);
    }
}

void Connection::release_context() {
    if (p->context == 0) {
        return;
    }
    SCardReleaseContext(p->context);
    p->context = 0;
};

// void Connection::release_card_handle() {
//     if (p->card == 0) {
//         return;
//     }
//     SCardDisconnect(p->card, SCARD_RESET_CARD);
//     p->card = 0;
// };

}
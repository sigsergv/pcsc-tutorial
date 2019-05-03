/*
 * Copyright (c) 2019, Sergey Stolyarov <sergei@regolit.com>
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


import java.util.List;
import javax.smartcardio.*;
// import java.util.ArrayList;
import static java.util.Arrays.copyOfRange;
// import static java.lang.Math.max;

class WriteCard {
    public static void main(String[] args) {
        try {
            TerminalFactory factory = TerminalFactory.getDefault();
            List<CardTerminal> terminals = factory.terminals().list();

            if (terminals.size() == 0) {
                throw new Util.TerminalNotFoundException();
            }

            // get first terminal
            CardTerminal terminal = terminals.get(0);

            System.out.printf("Using terminal %s%n", terminal.toString());

            // wait for card, indefinitely until card appears
            terminal.waitForCardPresent(0);

            // establish a connection to the card using autoselected protocol
            Card card = terminal.connect("*");

            // obtain logical channel
            CardChannel channel = card.getBasicChannel();

            ResponseAPDU answer;

            // Instruction "9.3.6.1. SELECT_CARD_TYPE"
            // write 1 (in "Lc" field) byte, 06 (in DATA block) indicates card type
            // fields "P1" and "P2" are ignored
            //                                          INS P1  P2  Lc  DATA
            byte[] selectCommand = Util.toByteArray("FF A4  00  00  01  06");
            answer = channel.transmit(new CommandAPDU(selectCommand));
            if (answer.getSW() != 0x9000) {
                card.disconnect(false);
                throw new Util.CardCheckFailedException("Select failed");
            }

            // Instruction "9.3.6.7. PRESENT_CODE_MEMORY_CARD (SLE 4442 and SLE 5542)"
            // write 3 bytes of PSC
            // field "P1" is ignored
            //                                              INS P1  P2  Lc  Data
            byte[] presentPSCCommand = Util.toByteArray("FF 20  00  00  03  FF FF FF");
            answer = channel.transmit(new CommandAPDU(presentPSCCommand));
            if (answer.getSW() != 0x9007) {
                card.disconnect(false);
                throw new Util.CardCheckFailedException("PSC auth failed");
            }

            // Instruction "9.3.6.5. WRITE_MEMORY_CARD"
            // write 4 bytes "01 02 03 04" starting with address 0x40 (in "P2" field)
            // field "P1" is ignored
            //                                         INS P1 P2 Lc
            byte[] writeCommand = Util.toByteArray("FF D0  00 40 04 01 02 03 04");
            answer = channel.transmit(new CommandAPDU(writeCommand));
            if (answer.getSW() != 0x9000) {
                card.disconnect(false);
                throw new Util.CardCheckFailedException("Write command failed");
            }

            // disconnect card
            card.disconnect(false);

        } catch (Util.CardCheckFailedException e) {
            System.out.printf("Error: %s%n", e.getMessage());
        } catch (Util.TerminalNotFoundException e) {
            System.out.println("No connected terminals.");
        } catch (CardException e) {
            System.out.println("CardException: " + e.toString());
        }
    }
}
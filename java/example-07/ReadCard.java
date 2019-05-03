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

class ReadCard {
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

            // Instruction "9.3.6.2. READ_MEMORY_CARD"
            // Read all 256 bytes in two passes.
            // First read 32=0x20 (in "Le" field) bytes starting with address 0x00 (in "P2" field)
            // field "P1" is ignored
            //                                            INS P1  P2  Le
            byte[] readCardCommand = Util.toByteArray("FF B0  00  00  20");
            answer = channel.transmit(new CommandAPDU(readCardCommand));
            if (answer.getSW() != 0x9000) {
                card.disconnect(false);
                throw new Util.CardCheckFailedException("Cannot read card data");
            }
            byte[] EEPROMData1 = Util.responseDataOnly(answer.getBytes());

            // Then read remaining 224=0xE0 (in "Le" field) bytes starting with address 0x20 (in "P2" field)
            // field "P1" is ignored
            //                                     INS P1  P2  Le
            readCardCommand = Util.toByteArray("FF B0  00  20  E0");
            answer = channel.transmit(new CommandAPDU(readCardCommand));
            if (answer.getSW() != 0x9000) {
                card.disconnect(false);
                throw new Util.CardCheckFailedException("Cannot read card data");
            }
            byte[] EEPROMData2 = Util.responseDataOnly(answer.getBytes());

            byte[] EEPROMData = new byte[256];
            for (int i=0; i<32; i++) {
                EEPROMData[i] = EEPROMData1[i];
            }
            for (int i=0; i<224; i++) {
                EEPROMData[32+i] = EEPROMData2[i];
            }

            System.out.printf("EEPROM memory:%n");
            for (int i=0; i<8; i++) {
                for (int j=0; j<32; j++) {
                    int addr = (i*32) + j;
                    System.out.printf("%02X ", EEPROMData[addr]);
                }
                System.out.printf("%n");
            }

            // Instruction "9.3.6.4. READ_PROTECTION_BITS"
            // read 0x04 (in "Le" field) bytes of Protection memory
            // fields "P1" and "P2" are ignored
            //                                            INS P1  P2  Le
            byte[] readPROMCommand = Util.toByteArray("FF B2  00  00  04");
            answer = channel.transmit(new CommandAPDU(readPROMCommand));
            if (answer.getSW() != 0x9000) {
                card.disconnect(false);
                throw new Util.CardCheckFailedException("Cannot read protection memory data");
            }
            byte[] PRBData = Util.responseDataOnly(answer.getBytes());
            System.out.printf("Protection memory bits:%n");
            for (int i=0; i<32; i++) {
                System.out.printf("%02d ", i);
            }
            System.out.printf("%n");
            int[] protectionBits = new int[32];
            for (int k=0; k<4; k++) {
                byte b = PRBData[k];
                for (int i=0; i<8; i++) {
                    int addr = k*8 + i;
                    protectionBits[addr] = (b & 1);
                    b >>= 1;
                }
            }
            for (int i=0; i<32; i++) {
                System.out.printf(" %d ", protectionBits[i]);
            }
            System.out.printf("%n");

            // Instruction "9.3.6.3. READ_PRESENTATION_ERROR_COUNTER_MEMORY_CARD (SLE 4442 and SLE 5542)"
            // read 0x04 (in "Le" field) bytes of Security memory (only EC value is returned)
            // fields "P1" and "P2" are ignored
            //                                          INS P1  P2  Le
            byte[] readECCommand = Util.toByteArray("FF B1  00  00  04");
            answer = channel.transmit(new CommandAPDU(readECCommand));
            if (answer.getSW() != 0x9000) {
                card.disconnect(false);
                throw new Util.CardCheckFailedException("Cannot read security memory data");
            }
            byte[] ECData = Util.responseDataOnly(answer.getBytes());
            System.out.printf("EC: %02X%n", ECData[0]);

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
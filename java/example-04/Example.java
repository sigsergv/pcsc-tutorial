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

class Example {
    public static class TerminalNotFoundException extends Exception {}
    public static class CardNotFoundException extends Exception {}

    public static void main(String[] args) {
        try {
            TerminalFactory factory = TerminalFactory.getDefault();
            List<CardTerminal> terminals = factory.terminals().list();

            if (terminals.size() == 0) {
                throw new TerminalNotFoundException();
            }

            // get first reader
            CardTerminal reader = terminals.get(0);

            System.out.printf("Using reader %s%n", reader.toString());

            // connect with the card using any available protocol ("*")
            Card card;
            try {
                card = reader.connect("T=1");
            } catch (CardException e) {
                throw new CardNotFoundException();
            }

            // obtain logical channel
            CardChannel channel = card.getBasicChannel();
            ResponseAPDU answer;

            // 1. load key
            //                      CLA   INS   P1    P2    Lc    Command bytes
            int[] loadKeyCommand = {0xFF, 0x82, 0x00, 0x00, 0x06, 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,};
            answer = channel.transmit(new CommandAPDU(toByteArray(loadKeyCommand)));
            System.out.printf("Response: %s%n", hexify(answer.getBytes()));

            // 2. authentication
            //                           CLA   INS   P1    P2    Lc    Command bytes
            int[] authenticateCommand = {0xFF, 0x86, 0x00, 0x00, 0x05, 0x01,0x00,0x00,0x60,0x00};
            answer = channel.transmit(new CommandAPDU(toByteArray(authenticateCommand)));
            System.out.printf("Response: %s%n", hexify(answer.getBytes()));

            // 3. read data
            //                        CLA   INS   P1    P2    Le
            int[] readBlockCommand = {0xFF, 0xB0, 0x00, 0x00, 0x10};
            answer = channel.transmit(new CommandAPDU(toByteArray(readBlockCommand)));
            System.out.printf("Response: %s%n", hexify(answer.getBytes()));

            // disconnect card
            card.disconnect(false);

        } catch (TerminalNotFoundException e) {
            System.out.println("No connected readers.");
        } catch (CardNotFoundException e) {
            System.out.println("Card not connected.");
        } catch (CardException e) {
            System.out.println("CardException: " + e.toString());
        }
    }

    public static String hexify(byte[] bytes) {
        StringBuilder sb = new StringBuilder();
        for (byte b : bytes) {
            sb.append(String.format("%02X ", b));
        }
        return sb.toString();
    }

    public static byte[] toByteArray(int[] list) {
        int s = list.length;
        byte[] buf = new byte[s];
        int i;

        for (i=0; i<s; i++) {
            buf[i] = (byte)list[i];
        }
        return buf;
    }
}
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
import static java.util.Arrays.copyOfRange;
import static java.lang.Math.max;

class Example {
    public static class TerminalNotFoundException extends Exception {}
    public static class InstructionFailedException extends Exception {}
    public static class ByteCastException extends Exception {}

    public static void main(String[] args) {
        try {
            TerminalFactory factory = TerminalFactory.getDefault();
            List<CardTerminal> terminals = factory.terminals().list();

            if (terminals.size() == 0) {
                throw new TerminalNotFoundException();
            }

            // get first terminal
            CardTerminal terminal = terminals.get(0);

            System.out.printf("Using terminal %s%n", terminal.toString());

            // wait for card, indefinitely until card appears
            terminal.waitForCardPresent(0);

            // establish a connection to the card using protocol T=1
            Card card = terminal.connect("T=1");

            // obtain logical channel
            CardChannel channel = card.getBasicChannel();

            // execute command
            int[] command = {0xFF, 0xCA, 0x00, 0x00, 0x00};
            ResponseAPDU answer = channel.transmit(new CommandAPDU(toByteArray(command)));
            if (answer.getSW() != 0x9000) {
                throw new InstructionFailedException();
            }
            byte[] answerBytes = answer.getBytes();
            byte[] uidBytes = copyOfRange(answerBytes, 0, max(answerBytes.length-2, 0));
            System.out.printf("Card UID: %s%n", hexify(uidBytes));

            // disconnect card
            card.disconnect(false);

        } catch (TerminalNotFoundException e) {
            System.out.println("No connected terminals.");
        } catch (ByteCastException e) {
            System.out.println("Data casting error.");
        } catch (InstructionFailedException e) {
            System.out.println("Instruction execution failed.");
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

    public static byte[] toByteArray(int[] list)
        throws ByteCastException {
        int s = list.length;
        byte[] buf = new byte[s];
        int i;

        for (i=0; i<s; i++) {
            if (i < 0 || i > 255) {
                throw new ByteCastException();
            }
            buf[i] = (byte)list[i];
        }
        return buf;
    }
}
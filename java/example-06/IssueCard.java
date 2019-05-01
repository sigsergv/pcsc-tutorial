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

// This class Issues a new card:
//   * scan passed card
//   * if it match requirements then initialize with a new keys and data
//   * if it doesn't then print error message
//   * quit
class IssueCard {
    static class CardCheckFailedException extends Exception{
        public CardCheckFailedException(String message) {
            super(message);
        }
    };

    public static void main(String[] args) {
        Util.Config config = Util.loadConfig();

        try {
            TerminalFactory factory = TerminalFactory.getDefault();
            List<CardTerminal> terminals = factory.terminals().list();

            if (terminals.size() == 0) {
                throw new Util.TerminalNotFoundException();
            }

            CardTerminal terminal = terminals.get(0);

            System.out.printf("Issue a new Balance Card%n========================%n");

            if (!terminal.isCardPresent()) {
                System.out.println("Please place a new non-initialized card on the terminal.");
            }
            terminal.waitForCardPresent(0);

            System.out.printf("Checking card... ");
            Card card = terminal.connect("T=1");
            CardChannel channel = card.getBasicChannel();

            byte[] authenticateCommand = Util.toByteArray("FF 86 00 00 05 01 00 00 00 00");
            byte[] readBinaryCommand = Util.toByteArray("FF B0 00 00 10");
            byte firstBlock = (byte)(config.sector * 4);
            byte[] data;

            ResponseAPDU answer;

            // load Key A to cell 00
            byte[] loadKeysCommand = Util.toByteArray("FF 82 00 00 06 " + config.initial_key_a);
            answer = channel.transmit(new CommandAPDU(loadKeysCommand));
            if (answer.getSW() != 0x9000) {
                throw new CardCheckFailedException("Failed to load Key A into terminal.");
            }

            // load Key B to cell 01
            loadKeysCommand = Util.toByteArray("FF 82 00 01 06 " + config.initial_key_b);
            answer = channel.transmit(new CommandAPDU(loadKeysCommand));
            if (answer.getSW() != 0x9000) {
                throw new CardCheckFailedException("Failed to load Key B into terminal.");
            }

            // check Key A first: auth, read first block of the sector, read trailer, ...
            authenticateCommand[7] = firstBlock;
            authenticateCommand[8] = 0x60;
            answer = channel.transmit(new CommandAPDU(authenticateCommand));
            if (answer.getSW() != 0x9000) {
                card.disconnect(false);
                throw new CardCheckFailedException("Key A doesn't match.");
            }
            readBinaryCommand[3] = firstBlock;
            answer = channel.transmit(new CommandAPDU(readBinaryCommand));
            if (answer.getSW() != 0x9000) {
                throw new CardCheckFailedException("Failed to read block with Key A.");
            }
            // System.out.printf("DATA: %s%n", Util.hexify(Util.responseDataOnly(answer.getBytes())));
            // read trailer and check that we should be able to set both keys and change access 
            readBinaryCommand[3] = (byte)(firstBlock+3);
            answer = channel.transmit(new CommandAPDU(readBinaryCommand));
            if (answer.getSW() != 0x9000) {
                throw new CardCheckFailedException("Failed to read trailer with Key A.");
            }
            data = Util.responseDataOnly(answer.getBytes());
            byte b6 = data[6];
            byte b7 = data[7];
            byte b8 = data[8];
            String bits = Util.decodeAccessBits(b6,b7,b8, 0);
            System.out.printf("BITS 0: %s%n", bits);
            bits = Util.decodeAccessBits(b6,b7,b8, 3);
            System.out.printf("BITS 4: %s%n", bits);
            // System.out.printf("TRAILER: %s%n", Util.hexify(data));


            System.out.printf("success%n");

        } catch (Util.TerminalNotFoundException e) {
            System.out.println("No connected terminals.");
            System.exit(2);
        } catch (CardCheckFailedException e) {
            System.out.printf("failed%n");
            System.out.printf("Error: %s%n", e.getMessage());
        } catch (CardException e) {
            System.out.println("CardException: " + e.toString());
            System.exit(2);
        }
    }
}
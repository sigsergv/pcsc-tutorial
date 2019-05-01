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


// Add money to card.
class TopUpBalance {
    public static void main(String[] args) {
        // load project configuration data
        Util.Config config = Util.loadConfig();

        if (args.length != 1) {
            System.out.println("Amount of funds is required.");
            System.exit(1);
        }

        int funds = 0;
        try {
            funds = Integer.decode(args[0]);
        } catch (NumberFormatException e) {
            System.out.println("Incorrect amount specified.");
            System.exit(1);
        }

        if (funds <= 0) {
            System.out.println("Amount of funds must be a positive integer value.");
            System.exit(1);
        }
        
        try {
            TerminalFactory factory = TerminalFactory.getDefault();
            List<CardTerminal> terminals = factory.terminals().list();

            if (terminals.size() == 0) {
                throw new Util.TerminalNotFoundException();
            }

            CardTerminal terminal = terminals.get(0);

            System.out.printf("Top up card balance%n===================%n");

            if (!terminal.isCardPresent()) {
                System.out.println("Please place a card on the terminal.");
            }
            terminal.waitForCardPresent(0);

            Card card = terminal.connect("T=1");
            CardChannel channel = card.getBasicChannel();

            byte[] authenticateCommand = Util.toByteArray("FF 86 00 00 05 01 00 00 00 00");
            byte[] readBinaryCommand = Util.toByteArray("FF B0 00 00 10");
            byte[] updateBinaryCommand = Util.toByteArray("FF D6 00 00 10 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00");
            byte firstBlock = (byte)(config.sector * 4);
            ResponseAPDU answer;
            byte[] data;
            byte[] command;

            // load production Key B to cell 00
            byte[] loadKeysCommand = Util.toByteArray("FF 82 00 00 06 " + config.prod_key_b);
            answer = channel.transmit(new CommandAPDU(loadKeysCommand));
            if (answer.getSW() != 0x9000) {
                card.disconnect(false);
                throw new Util.CardCheckFailedException("Failed to load Key B into terminal.");
            }

            // authenticate using Key B
            authenticateCommand[7] = firstBlock;
            authenticateCommand[8] = 0x61; 
            answer = channel.transmit(new CommandAPDU(authenticateCommand));
            if (answer.getSW() != 0x9000) {
                card.disconnect(false);
                throw new Util.CardCheckFailedException("Key B doesn't match.");
            }

            // read balance block data
            readBinaryCommand[3] = firstBlock;
            answer = channel.transmit(new CommandAPDU(readBinaryCommand));
            if (answer.getSW() != 0x9000) {
                card.disconnect(false);
                throw new Util.CardCheckFailedException("Failed to read block with Key A.");
            }
            // take first 8 bytes
            data = Util.responseDataOnly(answer.getBytes());
            data = copyOfRange(data, 0, 8);
            long balance = Util.bytesToLong(data);

            long newBalance = balance + funds;

            // create APDU by cloning updateBinaryCommand template, specify target
            // block address and copy data block
            byte[] newBalanceBytes = Util.longToBytes(newBalance);
            data = new byte[16];
            for (int i=0; i<8; i++) {
                data[i] = newBalanceBytes[i];
            }
            command = updateBinaryCommand.clone();
            command[3] = firstBlock;
            for (int i=0; i<16; i++) {
                command[5+i] = data[i];
            }
            answer = channel.transmit(new CommandAPDU(command));
            if (answer.getSW() != 0x9000) {
                card.disconnect(false);
                throw new Util.CardUpdateFailedException("Failed to update data block.");
            }

            System.out.printf("New balance is: %d%n", newBalance);

            card.disconnect(false);
        } catch (Util.TerminalNotFoundException e) {
            System.out.println("No connected terminals.");
            System.exit(2);
        } catch (Util.CardCheckFailedException e) {
            System.out.printf("failed%n");
            System.out.printf("Error: %s%n", e.getMessage());
        } catch (Util.CardUpdateFailedException e) {
            System.out.printf("failed%n");
            System.out.printf("Error: %s%n", e.getMessage());
        } catch (CardException e) {
            System.out.println("CardException: " + e.toString());
            System.exit(2);
        }
    }
}
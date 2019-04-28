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
import java.util.ArrayList;
import javax.smartcardio.*;
import static java.util.Arrays.copyOfRange;
import static java.lang.Math.max;

class Example {
    public static class TerminalNotFoundException extends Exception {}
    // public static class InstructionFailedException extends Exception {}

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

            // define list of keys to test
            ArrayList<String> defaultKeys = new ArrayList<String>();
            defaultKeys.add("FF FF FF FF FF FF");  // default NXP key
            defaultKeys.add("A0 A1 A2 A3 A4 A5");  // default Infineon Key A
            defaultKeys.add("B0 B1 B2 B3 B4 B5");  // default Infineon Key B
            int keysNumber = defaultKeys.size();

            // store collected data in these variables
            ArrayList<byte[]> blocksData = new ArrayList<byte[]>(64);
            ArrayList<String> blocksKeys = new ArrayList<String>(64);
            ArrayList<char[]> blocksAccessBits = new ArrayList<char[]>(64);

            // print header for all sectors
            System.out.printf("Sectors: ");
            for (int sector=0; sector<16; sector++) {
                System.out.printf("0x%X  ", sector);
            }

            // print start portion string of block keys status, we will
            // print status for each sector while reading data
            System.out.printf("%nBlocks:  ");

            // walk through all sectors and read data
            for (int sector=0; sector<16; sector++) {
                // insert null object for all blocks in this sector, they
                // will be updated later
                for (int j=0; j<4; j++) {
                    blocksData.add(null);
                    blocksKeys.add(null);
                    blocksAccessBits.add(null);
                }
                boolean keyAFound = false;
                boolean keyBFound = false;

                // calculate first sector block address
                int firstBlock = sector * 4;

                // General Authenticate APDU template
                byte[] authenticateCommand = toByteArray("FF 86 00 00 05 01 00 00 00 00");

                // Read Binary APDU template
                byte[] readBinaryCommand = toByteArray("FF B0 00 00 10");

                for (String key : defaultKeys) {
                    // sonstruct Load Keys instruction APDU
                    byte[] loadKeysCommand = toByteArray("FF 82 00 00 06 " + key);
                    ResponseAPDU answer = channel.transmit(new CommandAPDU(loadKeysCommand));
                    if (answer.getSW() != 0x9000) {
                        System.out.println("Failed to load keys");
                        continue;
                    }

                    // try to auth using key as Key B for the first block
                    // prepare authenticateCommand
                    authenticateCommand[7] = (byte)firstBlock;
                    authenticateCommand[8] = 0x61;
                    answer = channel.transmit(new CommandAPDU(authenticateCommand));
                    if (answer.getSW() == 0x9000) {
                        keyBFound = true;
                        // success, try to read data from all blocks (for this sector only!):
                        for (int i=0; i<4; i++) {
                            int block = firstBlock + i;
                            readBinaryCommand[3] = (byte)block;
                            answer = channel.transmit(new CommandAPDU(readBinaryCommand));
                            if (answer.getSW() == 0x9000) {
                                blocksData.set(block, responseDataOnly(answer.getBytes()));
                                blocksKeys.set(block, "B: " + key);
                            }
                        }
                    }

                    // try to auth using key as Key A for the first block
                    // prepare authenticateCommand
                    authenticateCommand[7] = (byte)firstBlock;
                    authenticateCommand[8] = 0x60;
                    answer = channel.transmit(new CommandAPDU(authenticateCommand));
                    if (answer.getSW() == 0x9000) {
                        keyAFound = true;
                        // success, try to read data from all blocks (for this sector only!):
                        for (int i=0; i<4; i++) {
                            int block = firstBlock + i;
                            readBinaryCommand[3] = (byte)block;
                            answer = channel.transmit(new CommandAPDU(readBinaryCommand));
                            if (answer.getSW() == 0x9000) {
                                blocksData.set(block, responseDataOnly(answer.getBytes()));
                                blocksKeys.set(block, "A: " + key);
                            }
                        }
                    }
                }
                
                // print found key status for this sector
                if (keyAFound && keyBFound) {
                    System.out.printf("++++ ");
                } else if (keyAFound) {
                    System.out.printf("AAAA ");
                } else if (keyBFound) {
                    System.out.printf("BBBB ");
                } else {
                    System.out.printf("---- ");
                }

                // calculate and store access bits for this sector blocks
                int lastBlock = firstBlock + 3;
                byte[] trailerBytes = blocksData.get(lastBlock);
                if (trailerBytes != null) {
                    // get access control bytes
                    byte b7 = trailerBytes[7];
                    byte b8 = trailerBytes[8];
                    char[] bits;

                    bits = new char[3];
                    bits[0] = checkAccessBit(b7, 4);
                    bits[1] = checkAccessBit(b8, 0);
                    bits[2] = checkAccessBit(b8, 4);
                    blocksAccessBits.set(firstBlock, bits);

                    bits = new char[3];
                    bits[0] = checkAccessBit(b7, 5);
                    bits[1] = checkAccessBit(b8, 1);
                    bits[2] = checkAccessBit(b8, 5);
                    blocksAccessBits.set(firstBlock+1, bits);

                    bits = new char[3];
                    bits[0] = checkAccessBit(b7, 6);
                    bits[1] = checkAccessBit(b8, 2);
                    bits[2] = checkAccessBit(b8, 6);
                    blocksAccessBits.set(firstBlock+2, bits);

                    bits = new char[3];
                    bits[0] = checkAccessBit(b7, 7);
                    bits[1] = checkAccessBit(b8, 3);
                    bits[2] = checkAccessBit(b8, 7);
                    blocksAccessBits.set(firstBlock+3, bits);
                }
            }
            System.out.printf("%n%n");

            // now print found data
            System.out.printf("BLOCK | DATA                                            | KEY                  | ACCESS BITS%n");
            System.out.printf("                                                                                 C1 C2 C3%n");
            for (int i=0; i<64; i++) {
                // get stored block data
                String d = "?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ";
                byte[] bd = blocksData.get(i);
                if (bd != null) {
                    d = hexify(bd);
                }

                // get stored block key
                String key = blocksKeys.get(i);
                if (key == null) {
                    key = "   ?? ?? ?? ?? ?? ??";
                }

                // get stored block access control bits
                char c1 = '?';
                char c2 = '?';
                char c3 = '?';
                char[] accessBits = blocksAccessBits.get(i);
                if (accessBits != null) {
                    c1 = accessBits[0];
                    c2 = accessBits[1];
                    c3 = accessBits[2];
                }

                System.out.printf("0x%02X  | %s| %s | %c  %c  %c%n", i, d, key, c1, c2, c3);
            }


            // disconnect card
            card.disconnect(false);

        } catch (TerminalNotFoundException e) {
            System.out.println("No connected terminals.");
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

    public static byte[] responseDataOnly(byte[] data) {
        return copyOfRange(data, 0, max(data.length-2, 0));
    }

    public static byte[] toByteArray(String s) {
        int len = s.length();
        byte[] buf = new byte[len/2];
        int bufLen = 0;
        int i = 0;
        
        while (i < len) {
            char c1 = s.charAt(i);
            i++;
            if (c1 == ' ') {
                continue;
            }
            char c2 = s.charAt(i);
            i++;

            byte d = (byte)((Character.digit(c1, 16) << 4) + (Character.digit(c2, 16)));
            buf[bufLen] = d;
            ++bufLen;
        }

        return copyOfRange(buf, 0, bufLen);
    }

    public static char checkAccessBit(byte ab, int b) {
        return ((ab >> b) & 1)==1 ? '1' : '0';
    }
}
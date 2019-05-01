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

import java.io.FileInputStream;
import java.io.IOException;
import java.io.FileNotFoundException;
import java.util.Properties;
import static java.util.Arrays.copyOfRange;
import static java.lang.Math.max;

// local utility class
class Util {
    public static class TerminalNotFoundException extends Exception {}

    public static class Config {
        public int sector;
        public String initial_key_a;
        public String initial_key_b;
        public String prod_key_a;
        public String prod_key_b;
    }
    public static Config loadConfig() 
    {
        Properties props = new Properties();

        try {
            FileInputStream in = new FileInputStream("project.properties");
            props.load(in);
            in.close();
        } catch (FileNotFoundException e) {
            System.out.println("Configuration file `project.properties` not found");
            System.exit(1);
        } catch (IOException e) {
            System.out.println("Configuration file `project.properties` cannot be read");
            System.exit(1);
        }

        // read project configuration
        Config config = new Config();
        config.sector = new Integer(props.getProperty("sector"));
        config.initial_key_a = props.getProperty("initial_key_a");
        config.initial_key_b = props.getProperty("initial_key_b");
        config.prod_key_a = props.getProperty("prod_key_a");
        config.prod_key_b = props.getProperty("prod_key_b");
        return config;
    }

    public static String hexify(byte[] bytes) {
        StringBuilder sb = new StringBuilder();
        for (byte b : bytes) {
            sb.append(String.format("%02X ", b));
        }
        return sb.toString();
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

    public static byte[] responseDataOnly(byte[] data) {
        return copyOfRange(data, 0, max(data.length-2, 0));
    }

    public static String decodeAccessBits(byte b6, byte b7, byte b8, int block) {
        String bits = "";

        switch (block) {
        case 0:
            bits += checkAccessBit(b7, 4) + checkAccessBit(b8, 0) + checkAccessBit(b8, 4);
            break;
        case 3:
            bits += checkAccessBit(b7, 7) + checkAccessBit(b8, 3) + checkAccessBit(b8, 7);
            break;
        }

        return bits;
    }

    public static String checkAccessBit(byte ab, int b) {
        return ((ab >> b) & 1)==1 ? "1" : "0";
    }
}
import java.util.List;
import javax.smartcardio.*;

class Example {
    public static class TerminalNotFoundException extends Exception {}

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
            Card card = reader.connect("*");

            // read protocol and card ATR
            System.out.printf("  Card protocol: %s%n", card.getProtocol());
            System.out.printf("  Card ATR: %s%n", printBytes(card.getATR().getBytes()));

        } catch (TerminalNotFoundException e) {
            System.out.println("No connected readers.");
        } catch (CardException e) {
            System.out.println("CardException: " + e.toString());
        }
    }

    public static String printBytes(byte[] bytes) {
        StringBuilder sb = new StringBuilder();
        for (byte b : bytes) {
            sb.append(String.format("%02X ", b));
        }
        return sb.toString();
    }
}
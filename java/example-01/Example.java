import java.util.List;
import javax.smartcardio.*;

class Example {
    public static void main(String[] args) {
        try {
            TerminalFactory factory = TerminalFactory.getDefault();
            List<CardTerminal> terminals = factory.terminals().list();
            System.out.println("Found readers:");

            int i = 0;
            for (CardTerminal t : terminals) {
                ++i;
                System.out.printf("  Reader #%d: %s %n", i, t.toString());
            }
        } catch (CardException e) {
            System.out.println("CardException: " + e.toString());
        }
    } 
}
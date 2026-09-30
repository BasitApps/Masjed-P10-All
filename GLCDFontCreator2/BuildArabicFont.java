import java.awt.image.BufferedImage;
import java.awt.image.IndexColorModel;
import java.io.*;
import java.nio.charset.Charset;
import java.nio.file.*;
import java.util.*;
import java.util.regex.*;
import util.*;

/** Converts a GLCD header into an editable GLCDFontCreator2 .ftsd project. */
public class BuildArabicFont {
    private static int[] bytes(String file) throws Exception {
        String s = new String(Files.readAllBytes(Paths.get(file)), Charset.forName("UTF-8"));
        int body = s.indexOf("const uint8_t");
        s = s.substring(s.indexOf('{', body));
        // Generated headers annotate every slot with a 0xNN label. Remove
        // comments before reading bytes so labels cannot corrupt the stream.
        s = s.replaceAll("(?s)/\\*.*?\\*/", "")
             // Match both LF and Windows CRLF source files.
             .replaceAll("(?m)//[^\\r\\n]*", "");
        Matcher m = Pattern.compile("0x([0-9A-Fa-f]{2})").matcher(s);
        ArrayList<Integer> v = new ArrayList<Integer>();
        while (m.find()) v.add(Integer.parseInt(m.group(1), 16));
        int[] a = new int[v.size()];
        for (int i = 0; i < a.length; i++) a[i] = v.get(i);
        return a;
    }

    private static String label(int cp) {
        if (cp == 0x20) return "SPACE";
        if (cp >= 0x21 && cp <= 0x7E) return "ASCII '" + (char) cp + "'";
        if (cp == 0x7F) return "DEL";
        String[] x = {"ء","آ","أ","إ","ا","د","ذ","ر","ز","و","ى","ؤ",
            "ب","ة","ت","ث","ج","ح","خ","س","ش","ص","ض","ط","ظ","ع","غ",
            "ف","ق","ك","ل","م","ن","ه","ي","ئ"};
        String[] f = {"منفصل", "أول", "وسط", "نهائي"};
        int slot = 0x80;
        for (int i = 0; i < x.length; i++) {
            int formCount = i == 0 ? 1 : (i < 12 ? 2 : 4);
            for (int j = 0; j < formCount; j++) {
            if (slot++ == cp) return x[i] + " " +
                (formCount == 2 ? (j == 0 ? "منفصل" : "نهائي") : f[j]);
            }
        }
        String[] lig = {"لا منفصل", "لا نهائي", "لأ منفصل", "لأ نهائي",
                        "لإ منفصل", "لإ نهائي", "لآ منفصل", "لآ نهائي"};
        return cp >= 0xF7 && cp <= 0xFE ? lig[cp - 0xF7] : "";
    }

    public static void main(String[] args) throws Exception {
        if (args.length != 2) throw new IllegalArgumentException("source.h output.ftsd");
        int[] b = bytes(args[0]);
        int height = b[3], sourceFirst = b[4], sourceCount = b[5], nominal = b[2];
        // Keep the complete source range so the editor shows ASCII and Arabic
        // in one project instead of producing an Arabic-only partial font.
        int first = sourceFirst, count = sourceCount;
        String fontName = Paths.get(args[1]).getFileName().toString().replace(".ftsd", "");
        Font font = new Font(nominal, nominal, height, first, count, fontName, 1, null);
        int hb = (height + 7) / 8, off = 6 + sourceCount;
        IndexColorModel cm = font.getEditorColorModel();
        for (int i = 0; i < count; i++) {
            int cp = first + i, w = b[6 + (cp - sourceFirst)];
            FontCharacter fc = font.getChar(i);
            if (w > 0) {
                BufferedImage img = new BufferedImage(w, height, BufferedImage.TYPE_BYTE_INDEXED, cm);
                for (int page = 0; page < hb; page++) for (int x = 0; x < w; x++) {
                    int data = b[off + page * w + x];
                    for (int y = 0; y < 8 && page * 8 + y < height; y++)
                        if ((data & (1 << y)) != 0) img.getRaster().setSample(x, page * 8 + y, 0, 1);
                }
                fc.setImage(img);
            }
            String c = label(cp);
            if (!c.isEmpty()) fc.setComment(c);
            off += w * hb;
        }
        ObjectOutputStream out = new ObjectOutputStream(new FileOutputStream(args[1]));
        out.writeObject(new SerializableFont(font));
        out.close();
        System.out.println("wrote " + args[1] + " (" + count + " slots, " + height + "px)");
    }
}

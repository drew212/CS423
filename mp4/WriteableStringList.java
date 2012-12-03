import java.lang.StringBuilder;
import java.io.IOException;
import java.util.*;
import org.apache.hadoop.io.Writable;
import java.io.DataOutput;
import java.io.DataInput;

public class WriteableStringList implements Writable {
    ArrayList<StringIntPair> pairs;

    public WriteableStringList()
    {
        pairs = new ArrayList<StringIntPair>();
    }

    public void add(String string, int num) {
        pairs.add(new StringIntPair(string, num));
    }

    public StringIntPair get(int i)
    {
        return pairs.get(i);
    }

    public int size()
    {
        return pairs.size();
    }
    @Override
    public void write(DataOutput out) throws IOException {
        int i;
        StringBuilder sb = new StringBuilder();
        //TOOD: sort(pairs, comparator);
        Collections.sort(pairs);
        for(i = 0; i < pairs.size()-1; i++)
        {
            sb.append(pairs.get(i).string + ":-:" + pairs.get(i).value + ":~:");
        }
        sb.append(pairs.get(i).string + ":-:" + pairs.get(i).value);

        out.writeUTF(sb.toString());
    }

    @Override
    public void readFields(DataInput in) throws IOException {
        String str = in.readUTF();
        String[] words = str.split(":~:");
        for(int i = 0; i < words.length; i++)
        {
            String[] wordCount = words[i].split(":-:");
            pairs.add(new StringIntPair(wordCount[0], Integer.parseInt(wordCount[1])));
        }
    }

    public String toString() {
        int i;
        StringBuilder sb = new StringBuilder();
        Collections.sort(pairs);
        for(i = 0; i < pairs.size()-1; i++) {
            sb.append(pairs.get(i).string + ":-:" + pairs.get(i).value + ":~:");
        }

        sb.append(pairs.get(i).string + ":-:" + pairs.get(i).value);

        return sb.toString();
    }

}

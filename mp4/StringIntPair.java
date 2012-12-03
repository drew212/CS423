public class StringIntPair implements Comparable
{
    public String string;
    public int value;

    public StringIntPair(String _string, int _value) {
        string = _string;
        value = _value;
    }

    @Override
    public int compareTo(Object othr)
    {
        StringIntPair other = (StringIntPair) othr;
        return other.value - this.value;
    }
}

import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.InputStream;
import java.util.Iterator;
import java.io.IOException;
import java.util.*;
import java.io.RandomAccessFile;

public class Search {
    public static int displayMax = 10;

    public static void main(String[] args)
    {
        if(args.length < 1){
            System.out.println("No index specified");
            return;
        }
        if(args.length < 2){
            System.out.println("No search term specified");
            return;
        }
        if(args.length == 3){
            displayMax = Integer.parseInt(args[2]);
        }



        String index = args[0];
        String needle = args[1].toLowerCase() + "\t";
        try{
            RandomAccessFile wordFile = new RandomAccessFile(index, "r");
            long start = 0;
            long mid = 0;
            long end = wordFile.length();

            while(true) {
                if(start > end) {
                    System.out.println("Search string not found");
                    return;
                }

                mid = (start + end) / 2;

                wordFile.seek(mid);
                seekToBeginingOfLine(wordFile);
                long midStart = wordFile.getFilePointer();
                String line = wordFile.readLine();
                long midEnd = wordFile.getFilePointer();

                if(line.startsWith(needle)){
                    //Line found
                    displayResults(line);
                    return;
                }
                if(line.compareTo(needle) > 0){
                    start = start;
                    end = midStart - 1;
                } else {
                    start = midEnd;
                    end = end;
                }
            }
        } catch (Exception e){
            System.out.println(e.toString());
        }

    }

    public static void seekToBeginingOfLine(RandomAccessFile file){
        try {
            long seekPosition = file.getFilePointer();
            if(seekPosition == 0) return;

            char readChar = ' ';

            while ( seekPosition > 0 && readChar != '\n') {
                //Thread.sleep(1);
                seekPosition -= 1;
                file.seek(seekPosition);
                if(seekPosition == 0)
                    break;
                readChar = (char)file.readByte();
            }
        } catch (Exception e) {
            System.out.println(e.toString());
        }
    }

    public static void displayResults(String line){
        String[] entry = line.split("\t");
        String[] siteList = entry[1].split(":~:");
        for(int i = 0; i < siteList.length && i < displayMax; i++){
            String[] siteParts = siteList[i].split(":-:");
            System.out.println(siteParts[1] + " " + siteParts[0]);
        }
    }
}

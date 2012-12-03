import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.InputStream;
import java.util.Iterator;
import java.io.IOException;
import java.util.*;
import java.io.RandomAccessFile;

public class Search {

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

        String index = args[0];
        String needle = args[1].toLowerCase();
        try{
            RandomAccessFile wordFile = new RandomAccessFile(index, "r");
            String firstLine = wordFile.readLine();
            //System.out.println("Line to test: " + firstLine);
            if(firstLine.startsWith(needle)){
                displayResults(firstLine);
                return;
            }

            //Begin Binary search
            String matchingLine = search(wordFile, needle, 0, wordFile.length());
            if(matchingLine != null){
                displayResults(matchingLine);
            } else {
                System.out.println("Search string not found");
            }


        } catch (Exception e){

        }
    }

    public static String search(RandomAccessFile wordFile, String needle, long start, long end){
        try {
            if(start > end)
                return null;

            long mid = (start + end)/2;

            //System.out.println("New start, mid, end: " + start + ", " + mid + ", " + end);
            wordFile.seek(mid);

            long tempMid = mid;
            String candidate = wordFile.readLine();
            while (tempMid >= 0 && !candidate.trim().equals("")){
                wordFile.seek(--tempMid);
                candidate = wordFile.readLine();
                //System.out.println("Candidate: " + candidate);
            }
            wordFile.seek(++tempMid);

            String lineToTest = wordFile.readLine();
            //System.out.println("Line to test: " + lineToTest);
            if(lineToTest.startsWith(needle)){
                return lineToTest;
            } else if (lineToTest.compareTo(needle) > 0){
                return search(wordFile, needle, start, mid-1);
            } else {
                return search(wordFile, needle, mid+1, end);
            }
        } catch (Exception e){

        }
        return null;
    }

    public static void displayResults(String line){
        String[] entry = line.split("\t");
        String[] siteList = entry[1].split(":~:");
        for(int i = 0; i < siteList.length; i++){
            String[] siteParts = siteList[i].split(":-:");
            System.out.println(siteParts[1] + " " + siteParts[0]);
        }
        //System.out.println(line);
    }
}

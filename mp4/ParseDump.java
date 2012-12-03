import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.InputStream;
import java.util.Iterator;
import java.io.IOException;
import java.util.*;
import javax.xml.stream.XMLEventReader;
import javax.xml.stream.XMLInputFactory;
import javax.xml.stream.XMLStreamException;
import javax.xml.stream.events.Attribute;
import javax.xml.stream.events.EndElement;
import javax.xml.stream.events.StartElement;
import javax.xml.stream.events.XMLEvent;

public class ParseDump {

//    public static List<String> parseText(String fileName)
    public static void main(String[] args)
    {
        if(args.length < 1){
            System.out.println("No xml file specified");
            return;
        }

        String xml = args[0];
        try{
            XMLInputFactory inputFactory = XMLInputFactory.newInstance();
            InputStream in = new FileInputStream(xml);
            XMLEventReader eventReader = inputFactory.createXMLEventReader(in);

            while(eventReader.hasNext()){
                XMLEvent event = eventReader.nextEvent();

                if(event.isStartElement()) {
                    StartElement startElement = event.asStartElement();
                    if( startElement.getName().getLocalPart().equals("title")){
                        event = eventReader.nextEvent();
                        String title = event.asCharacters().getData();
                        String[] titleWords = (event.asCharacters().getData()).toLowerCase().split("[^a-z]");
                        for(int i = 0; i < titleWords.length; i++){
                            if(!titleWords[i].trim().equals("")){
                                System.out.println(title + ":~:" + titleWords[i].trim());
                            }
                        }

                        while(!event.isStartElement() || !event.asStartElement().getName().getLocalPart().equals("text")){
                            event = eventReader.nextEvent();
                        }
                        while(true){
                            event = eventReader.nextEvent();
                            if(event.isEndElement()){
                                break;
                            }
                            String[] words = (event.asCharacters().getData()).toLowerCase().split("[^a-z]");
                            for(int i = 0; i < words.length; i++){
                                if(!words[i].trim().equals("")){
                                    System.out.println(title + ":~:" + words[i].trim());
                                }
                            }
                        }
                    }
                }
            }
        } catch (Exception e){

        }
    }
}

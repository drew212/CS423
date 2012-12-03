HADOOP_HOME=../../hadoop-1.0.4

FILENAME=BuildIndex

include makesettings

make:
	javac -classpath $(HADOOP_HOME)/hadoop-core-1.0.4.jar $(FILENAME).java WriteableStringList.java StringIntPair.java
	jar cvf $(FILENAME).jar *.class
	cp $(FILENAME).jar $(HADOOP_HOME)

clean:
	rm *.class $(FILENAME).jar

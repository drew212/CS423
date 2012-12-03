HADOOP_HOME=../../hadoop-1.0.4

FILENAME=BuildIndex

make:
	javac -classpath $(HADOOP_HOME)/hadoop-core-1.0.4.jar $(FILENAME).java
	jar cvf $(FILENAME).jar *.class

clean:
	rm *.class $(FILENAME).jar




HADOOP_HOME=../../hadoop-1.0.4

FILENAME=BuildIndex

include make.settings

make:
	javac -classpath $(HADOOP_HOME)/hadoop-core-1.0.4.jar $(FILENAME).java
	jar cvf $(FILENAME).jar *.class
	cp $(FILENAME).jar $(HADOOP_HOME)

clean:
	rm *.class $(FILENAME).jar




import java.io.IOException;
import java.util.*;

import org.apache.hadoop.fs.Path;
import org.apache.hadoop.conf.*;
import org.apache.hadoop.io.*;
import org.apache.hadoop.mapred.*;
import org.apache.hadoop.util.*;

public class BuildIndex {

    public static class WeightMap extends MapReduceBase implements Mapper<LongWritable, Text, Text, IntWritable> {
        private final static IntWritable one = new IntWritable(1);
        private Text word = new Text();

        public void map(LongWritable key, Text value, OutputCollector<Text, IntWritable> output, Reporter reporter) throws IOException {
            word.set(value.toString());
            output.collect(word, one);
        }
    }

    public static class WeightReduce extends MapReduceBase implements Reducer<Text, IntWritable, Text, IntWritable> {
        public void reduce(Text key, Iterator<IntWritable> values, OutputCollector<Text, IntWritable> output, Reporter reporter) throws IOException {
            int sum = 0;
            while (values.hasNext()) {
                sum += values.next().get();
            }
            output.collect(key, new IntWritable(sum));
        }
    }

    public static class RankingMap extends MapReduceBase implements Mapper<LongWritable, Text, Text, WriteableStringList> {
        private Text word = new Text();
        private Text webCount = new Text();

        public void map(LongWritable key, Text value, OutputCollector<Text, WriteableStringList> output, Reporter reporter) throws IOException {
            String line = value.toString();
            String[] tokens = line.split(":~:");
            String[] wordCount = tokens[1].split("\t");
            if(wordCount.length != 2 || tokens.length != 2)
                System.out.println(line);
            word.set(wordCount[0]);
            webCount.set(tokens[0] + ":" + wordCount[1]);
            WriteableStringList collection = new WriteableStringList();
            collection.add(tokens[0], Integer.parseInt(wordCount[1]));
            output.collect(word, collection);
        }
    }

    public static class RankingReduce extends MapReduceBase implements Reducer<Text, WriteableStringList, Text, WriteableStringList> {
        public void reduce(Text key, Iterator<WriteableStringList> values, OutputCollector<Text, WriteableStringList> output, Reporter reporter) throws IOException {
            //WriteableStringList collection = new WriteableStringList();
            //while (values.hasNext()) {
            //    WriteableStringList value = values.next();
            //    for(int i = 0; i < value.size(); i++)
            //    {
            //        collection.add(value.get(i).string, value.get(i).value);
            //    }
            //}
            //output.collect(key, collection);
            while (values.hasNext()) {
                output.collect(key, values.next());
            }
        }
    }

        public static void main(String[] args) throws Exception {

            // WEIGHTS
            JobConf weightConf = new JobConf(BuildIndex.class);
            weightConf.setJobName("computeWeight");

            weightConf.setOutputKeyClass(Text.class);
            weightConf.setOutputValueClass(IntWritable.class);

            weightConf.setMapperClass(WeightMap.class);
            weightConf.setCombinerClass(WeightReduce.class);
            weightConf.setReducerClass(WeightReduce.class);

            weightConf.setInputFormat(TextInputFormat.class);
            weightConf.setOutputFormat(TextOutputFormat.class);

            FileInputFormat.setInputPaths(weightConf, new Path("input"));
            FileOutputFormat.setOutputPath(weightConf, new Path("output/websites"));

            JobClient.runJob(weightConf);

            // Rankings
            JobConf rankingsConf = new JobConf(BuildIndex.class);
            rankingsConf.setJobName("computeRankings");

            rankingsConf.setOutputKeyClass(Text.class);
            rankingsConf.setOutputValueClass(WriteableStringList.class);

            rankingsConf.setMapperClass(RankingMap.class);
            //rankingsConf.setCombinerClass(RankingReduce.class);
            rankingsConf.setReducerClass(RankingReduce.class);

            rankingsConf.setInputFormat(TextInputFormat.class);
            rankingsConf.setOutputFormat(TextOutputFormat.class);

            FileInputFormat.setInputPaths(rankingsConf, new Path("output/websites"));
            FileOutputFormat.setOutputPath(rankingsConf, new Path("output/words"));

            JobClient.runJob(rankingsConf);
        }
    }

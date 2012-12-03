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
            String line = value.toString();
            StringTokenizer tokenizer = new StringTokenizer(line);
            while (tokenizer.hasMoreTokens()) {
                word.set(tokenizer.nextToken());
                output.collect(word, one);
            }
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

    public static class RankingMap extends MapReduceBase implements Mapper<LongWritable, Text, Text, IntWritable> {
        private final static IntWritable one = new IntWritable(1);
        private Text word = new Text();

        public void map(LongWritable key, Text value, OutputCollector<Text, IntWritable> output, Reporter reporter) throws IOException {
            String line = value.toString();
            StringTokenizer tokenizer = new StringTokenizer(line);
            while (tokenizer.hasMoreTokens()) {
                word.set(tokenizer.nextToken());
                output.collect(word, one);
            }
        }
    }

    public static class RankingReduce extends MapReduceBase implements Reducer<Text, IntWritable, Text, IntWritable> {
        public void reduce(Text key, Iterator<IntWritable> values, OutputCollector<Text, IntWritable> output, Reporter reporter) throws IOException {
            int sum = 0;
            while (values.hasNext()) {
                sum += values.next().get();
            }
            output.collect(key, new IntWritable(sum));
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

        FileInputFormat.setInputPaths(weightConf, new Path("sources"));
        FileOutputFormat.setOutputPath(weightConf, new Path("output/websites"));

        JobClient.runJob(weightConf);

        // Rankings
        JobConf rankingsConf = new JobConf(BuildIndex.class);
        rankingsConf.setJobName("computeRankings");

        rankingsConf.setOutputKeyClass(Text.class);
        rankingsConf.setOutputValueClass(IntWritable.class);

        rankingsConf.setMapperClass(RankingMap.class);
        rankingsConf.setCombinerClass(RankingReduce.class);
        rankingsConf.setReducerClass(RankingReduce.class);

        rankingsConf.setInputFormat(TextInputFormat.class);
        rankingsConf.setOutputFormat(TextOutputFormat.class);

        FileInputFormat.setInputPaths(rankingsConf, new Path("output/websites"));
        FileOutputFormat.setOutputPath(rankingsConf, new Path("output/words"));

        JobClient.runJob(rankingsConf);
    }
}

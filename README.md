Tweeps
======

This is my project for Distributed Computing Systems course (ECE677) at the University of Arizona, Fall 2013

It is a very rudimentary statistics collecter that could have been done better, but had to cook this up in <4days and settle with it after procrastinating for the good part of the semester.

The idea is very simple. The Stanford Network Analysis Project maintains a very rich collection of social networking data. I chose their Twitter dataset and tried to gather some basic statistics from it.

The file contained ~45million lines, each containing 2 IDs. The IDs are anonymized user names, presented simply as 2 numbers. The first ID in a line indicates a User and the second ID indicates who he is following.

This program is written using MPI for C and runs in a pseudo scatter-gather mechanism. For now, it is hardcoded to run on 1 master and 9 worker processes. Here is how it works:
* The master process reads data via stdin and dispatches them to the worker processes. 
* Each line is read and dispatched to process whose ID is 1st non-zero digit of the user ID that is read.
* The worker process receives the whole line, breaks the 2 user IDs and inserts them both into a Trie data structure and increases their Follwing/Followed-by count in the process.
* An empty string to all the workers denotes end of input.
* Each worker then traverses the Trie and sends each of the node data as a string back to the Master
* The master receives them all and maintains its own Trie, summing all the counts in the process.
* Each of the worker writes the node that it processed to its own stdout file.
* Finally, all the connections are gathered by master and are written to 0.stdout file.

This is just a demonstration of MPI, mainly for educational purpose. The scatter-gather mechanism illustrated here isn't' really great. It performs worse than writing it in a single thread. 
I want to revisit this and make some design changes and make it a P2P mechanism b/w the workers and not involve the master at all. Finally, each worker will be able to get the overall count of all the IDs he is responsible for.

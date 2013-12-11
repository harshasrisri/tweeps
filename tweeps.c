#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mpi.h"

#define Children (node->children)
#define WS (node->ws)
#define Word (node->ws->word)
#define IC (node->ws->incoming)
#define OG (node->ws->outgoing)

#ifndef DEBUG
#define DEBUG 0
#endif

#define DBG(str) if (DEBUG) fprintf (stderr, "Debug | %s : %s\n", __func__, (str))

struct word_struct {
	char *word;
	int incoming;
	int outgoing;
};

struct node {
	struct node *children[10];
	struct word_struct *ws;
};

struct node *node_head;
int key;
char *name;
int numtasks, taskid;
FILE *fp[10];


void print (struct node *node) {
	int i;

	if (WS) {
		fprintf (fp[taskid], "%s %d %d\n", Word, IC, OG);
		free (Word);
		free (WS);
	}

	for (i = 0; i < 10; i++)
		if (Children[i])
			print (Children[i]);

	free (node);
}

struct node *init_node () {
	int i;
	struct node *node;

	node = (struct node *) malloc (sizeof (struct node));
	WS = NULL;

	for (i = 0; i < 10; i++)
		Children[i] = NULL;

	return node;
}

void insert (struct node *node, char *word, int location) {

	if (!*word) {
		if (!WS) {
			WS = (struct word_struct *) malloc (sizeof (struct word_struct));
			Word = strdup (name);
			IC = OG = 0;
		}
		if (location)
			IC++;
		else
			OG++;
		fprintf (fp[taskid], "Inserted %s as %s\n", Word, location ? "incoming" : "outgoing");
		return;
	}

	key = word[0] - '0';

	if (!Children[key])
		Children[key] = init_node ();

	insert (Children[key], &word[1], location);
}

int main (int argc, char **argv) {
	char src_node[20], dst_node[20], line[50];
	MPI_Status status;
	int rc, dest, i = 0, j, tag;

	/***** Initializations *****/
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
	MPI_Comm_rank(MPI_COMM_WORLD,&taskid);


	/* MASTER */
	if (!taskid) {
		while (fgets (line, 50, stdin)) {
			* (char *) (strchr (line, '\n')) = '\0';

			/* i++; */
			/* if (i >= numtasks) i = 1; */

			/* destination task ID is the first digit of the source in each line. */
			dest = line[0] - '0';
			MPI_Send (line, 50, MPI_UNSIGNED_CHAR, dest, 0, MPI_COMM_WORLD);
		}

		for (i = 1; i < numtasks; i++)
			MPI_Send ("", 50, MPI_UNSIGNED_CHAR, i, 0, MPI_COMM_WORLD);
	}
	/* SLAVES */
	else {
		node_head = init_node ();
		char filename[10];
		sprintf (filename, "task_%d", taskid);
		fp[taskid] = fopen (filename, "w");

		while (1) {
			MPI_Recv (line, 50, MPI_UNSIGNED_CHAR, 0, 0, MPI_COMM_WORLD, &status);

			if (!line[0]) {
				fflush (fp[taskid]);
				fclose (fp[taskid]);
				break;
			}

			sscanf (line, "%s %s", src_node, dst_node);
			/* fprintf (fp[taskid], "%10s -> %10s\n", src_node, dst_node); */

			insert (node_head, name = src_node, 0);
			insert (node_head, name = dst_node, 1);
		}

		/* print (node_head); */
	}

	MPI_Finalize();
}

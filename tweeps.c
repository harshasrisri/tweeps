#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mpi.h"

#define Children (node->children)
#define UInfo (node->uinfo)

#define UID (node->uinfo->userid)
#define Followers (node->uinfo->followers)
#define Following (node->uinfo->following)

#define ID (task->id)
#define Head (task->node_head)
#define FOUT (task->fp_out)
#define FERR (task->fp_err)

#define Free(x) if(x) free(x)
#define F_ERR(fmt, ...) do {fprintf (FERR, fmt, __VA_ARGS__);} while (0)
#define F_OUT(fmt, ...) do {fprintf (FOUT, fmt, __VA_ARGS__);} while (0)

/* Data structure to hold a user's information */
struct user_info {
	char *userid;
	int followers;
	int following;
};

/* Data structure for a TRIE */
struct node {
	struct node *children[10];
	struct user_info *uinfo;
};

/* Data structure to hold all entities related to a task */
struct task {
	int id;
	struct node *node_head;
	FILE *fp_out;
	FILE *fp_err;
};

struct task *task;

int key; 		// The index of the array of children in each trie node
char *name; 	// Global reference to the node being entered into the trie

/* MPI variables */
int numtasks, taskid;
MPI_Status status;

/* Identifying if a node is following or followed by */
enum { FOLLOWER, FOLLOWING };

/* Recursive Function to free all the nodes in a TRIE */
void free_trie (struct node *node) {
	int i;
	
	if (UInfo) {
		Free (UID);
		Free (UInfo);
	}

	for (i = 0; i < 10; i++)
		if (Children[i])
			free_trie (Children[i]);

	Free (node);
}

/* Recursive Function to print the nodes of a TRIE in in-order fashion */
void print_trie (struct node *node) {
	int i;
	
	if (UInfo)
		F_OUT ("%s %d %d\n", UID, Followers, Following);

	for (i = 0; i < 10; i++)
		if (Children[i])
			print_trie (Children[i]);
}

/* Function to initialize the elements of a node of a TRIE sanely */
struct node *init_node () {
	int i;
	struct node *node;

	node = (struct node *) malloc (sizeof (struct node));
	UInfo = NULL;

	for (i = 0; i < 10; i++)
		Children[i] = NULL;

	return node;
}

/* Function to insert an element into a TRIE */
struct node *insert (struct node *node, char *userid, int connection) {

	if (!*userid) { 		// Reached end of string. let's add it to the TRIE
		if (!UInfo) { 		// If the user info was not there already, create it
			UInfo = (struct user_info *) malloc (sizeof (struct user_info));
			UID = strdup (name);
			Followers = Following = 0;
		}
		/* If it is a master process, then we gather all the info */
		if (!ID) {
			/* Followers += gBufNode.followers; */
			/* Following += gBufNode.following; */
		}
		/* If it is a slave process, we keep count of individual's statistics */
		else {
			if (connection)
				Followers++;
			else
				Following++;
		}
#if 0
		F_ERR ("Inserted %s as %s in task %d\n", UID, connection ? "destination" : "source", taskid); */
#endif
		return node;
	}

	key = userid[0] - '0';

	if (!Children[key])
		Children[key] = init_node ();

#if 0
	F_ERR ("%s -> ", userid);
#endif

	Children[key] = insert (Children[key], &userid[1], connection);
	return node;
}

/* Function to initialize all data used in this particular task */
void task_init () {
	char filename[10];

	task = (struct task *) malloc (sizeof (struct task));

	ID = taskid;
	Head = init_node ();

	sprintf (filename, "%d.stdout", taskid);
	FOUT = fopen (filename, "w");

	sprintf (filename, "%d.stderr", taskid);
	FERR = fopen (filename, "w");
}

/* Function to close and clean up a task */
void task_close () {
	print_trie (Head);
	fflush (FOUT); fflush (FOUT);
	fclose (FERR); fclose (FERR);
	free_trie (Head);
	Free (task);
}

/* Read input from the STDIN and dispatch it to all the other tasks in MPI_COMM_WORLD */
void dispatch_input () {
	int i, dest;
	char line[50];

	while (fgets (line, 50, stdin)) {
		* (char *) (strchr (line, '\n')) = '\0';

		/* destination task ID is the first digit of the source in each line. */
		for (i = 0; line[i] == '0'; i++) {}	// Find the first non-zero digit
		dest = line[i] - '0'; 				// dest taskid is the 1st digit
		dest = dest % (numtasks - 1) + 1; 	// Adjust dest taskid for fewer threads

		MPI_Send (line, 50, MPI_UNSIGNED_CHAR, dest, 0, MPI_COMM_WORLD);
	}

	for (i = 1; i < numtasks; i++) 			// Mark end of transmission with a blank string
		MPI_Send ("", 50, MPI_UNSIGNED_CHAR, i, 0, MPI_COMM_WORLD);
}

/* Function used by slave task to receive input from Master */
void receive_input () {
	char src_node[20], dst_node[20], line[50];

	while (1) { 		// loop till inputs are exhausted and break afterwards
		MPI_Recv (line, 50, MPI_UNSIGNED_CHAR, 0, 0, MPI_COMM_WORLD, &status);

		if (!line[0]) break; // break when you get a blank line

		sscanf (line, "%s %s", src_node, dst_node);

		Head = insert (Head, name = src_node, FOLLOWER);
		Head = insert (Head, name = dst_node, FOLLOWING);
	}
}

/* And so it begins... */
int main (int argc, char **argv) {

	/***** Initializations *****/
	MPI_Init (&argc, &argv);
	MPI_Comm_size (MPI_COMM_WORLD, &numtasks);
	MPI_Comm_rank (MPI_COMM_WORLD, &taskid);

	task_init ();

	/* MASTER */
	if (!taskid) {
		dispatch_input ();
	}

	/* SLAVES */
	else {
		receive_input ();
	}

	task_close ();
	MPI_Finalize ();
	return 0;
}

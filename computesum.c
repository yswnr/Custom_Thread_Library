#include <stdio.h>
#include <stdlib.h>
#include "foothread.h"

#define MAX_NODES 100
#define MAX_INPUT_LEN 100

// Global variables
int n;                                // Number of nodes
int root;                             // Root node
int parent[MAX_NODES];                // Parent array
int sums[MAX_NODES];                  // Partial sums
int numchilds[MAX_NODES];             // Number of children
foothread_mutex_t mutexes[MAX_NODES]; // Mutexes for each node
foothread_mutex_t common;             // Common mutex
foothread_barrier_t barrier[MAX_NODES];
foothread_barrier_t barrier1;

// Function prototypes
void *node_function(void *arg);
void read_tree(const char *filename);
void update_sum(int node, int value);
void print_total_sum();

int main()
{
    int i;

    // Initialize numchilds array
    for (i = 0; i < MAX_NODES; i++)
    {
        numchilds[i] = 0;
    }

    // Read tree information from file
    read_tree("tree.txt");

    // Initialize synchronization resources
    for (i = 0; i < n; i++)
    {
        foothread_mutex_init(&mutexes[i]);
    }

    foothread_mutex_init(&common);

    // Create barrier
    for (i = 0; i < n; i++)
    {
        foothread_barrier_init(&barrier[i], numchilds[i]);
    }

    foothread_barrier_init(&barrier1, n);

    // Create threads for each node in the tree
    foothread_t threads[MAX_NODES];
    for (i = 0; i < n; i++)
    {
        foothread_create(&threads[i], NULL, node_function, (void *)i);
    }

    // Wait for all threads to complete
    foothread_barrier_wait(&barrier1);

    // Print total sum at root node
        print_total_sum();

    // Clean up resources
    for (i = 0; i < n; i++)
    {
        foothread_mutex_destroy(&mutexes[i]);
        foothread_barrier_destroy(&barrier[i]);
    }
    foothread_barrier_destroy(&barrier1);

    // Synchronize threads before exiting
    foothread_exit();

    return 0;
}

// Thread function for each node in the tree
void *node_function(void *arg)
{
    int node = (int)arg;
    int value;

    // Read user input for leaf nodes
    if (numchilds[node] == 0)
    {
        foothread_mutex_lock(&common);
        printf("Leaf node %d :: Enter a positive integer: ", node);
        scanf("%d", &value);
        update_sum(node, value);
        foothread_mutex_unlock(&common);
        foothread_barrier_wait(&barrier[parent[node]]);
    }

    // Calculate partial sum for internal nodes
    if (numchilds[node] != 0)
    {
        // int parent_node = parent[node];
        // foothread_mutex_lock(&mutexes[parent_node]);
        // sums[parent_node] += sums[node];
        // foothread_mutex_unlock(&mutexes[parent_node]);
        foothread_barrier_wait(&barrier[node]);
        foothread_mutex_lock(&common);
        foothread_mutex_lock(&mutexes[node]);
        for (int i = 0; i < n; i++)
        {
            if (parent[i] == node)
            {
                sums[node] += sums[i];
            }
        }
        foothread_mutex_unlock(&mutexes[node]);
        printf("Internal node %d gets the partial sum %d from its children\n", node, sums[node]);
        foothread_mutex_unlock(&common);
        if (root != node)
        {
            foothread_barrier_wait(&barrier[parent[node]]);
        }
    }

    // Synchronize threads
    foothread_barrier_wait(&barrier1);

    return NULL;
}

// Read tree information from file
void read_tree(const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (file == NULL)
    {
        perror("fopen");
        exit(EXIT_FAILURE);
    }
    fscanf(file, "%d", &n);
    for (int i = 0; i <= n - 1; i++)
    {
        int child_node, parent_node;
        fscanf(file, "%d %d", &child_node, &parent_node);
        if (child_node == parent_node)
        {
            root = child_node;
            continue;
        }
        parent[child_node] = parent_node;
        numchilds[parent_node]++;
        sums[child_node] = 0;
    }
    fclose(file);
}

// Update sum at parent node
void update_sum(int node, int value)
{
    foothread_mutex_lock(&mutexes[node]);
    sums[node] += value;
    foothread_mutex_unlock(&mutexes[node]);
}

// Print total sum at root node
void print_total_sum()
{
    printf("Sum at root(node %d): %d\n",root, sums[root]);
}

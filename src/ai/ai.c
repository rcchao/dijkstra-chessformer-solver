#include <time.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include <assert.h>
#include <stdbool.h>


#include "ai.h"
#include "utils.h"
#include "hashtable.h"
#include "chessformer.h"
#include "node.h"
#include "queue.h"

node_t *create_init_node(chessformer_t* chessformer) {
	node_t *n = (node_t *) malloc(sizeof(node_t));
	assert(n);

	n->depth = 0;
	n->num_childs = 0;
	n->move_delta_x = 0;
	n->move_delta_y = 0;
	n->state.map = (char **) malloc(sizeof(char *) * chessformer->lines);
	assert(n->state.map);
	int mapLength = chessformer->num_chars_map / chessformer->lines;
	for(int i = 0; i < chessformer->lines; i++){
		n->state.map[i] = (char *) malloc(sizeof(char) * (mapLength + 1));
		assert(n->state.map[i]);
		for(int j = 0; j < mapLength; j++){
			n->state.map[i][j] = chessformer->map[i][j];
		}
		n->state.map[i][mapLength] = '\0';
	}

	n->state.player_x = chessformer->player_x;
	n->state.player_y = chessformer->player_y;

	n->parent = NULL;

	return n;
}

/**
 * Copy a src into a dst state
*/
void copy_state(chessformer_t* init_data, state_t* dst, state_t* src){
	dst->map = malloc(sizeof(char *) * init_data->lines);
	assert(dst->map);
	for (int i = 0; i < init_data->lines; i++){
		int width = strlen(src->map[i]) + 1;
		dst->map[i] = malloc(width);
		assert(dst->map[i]);
		memcpy(dst->map[i], src->map[i], width);
	}
	dst->player_x = src->player_x;
	dst->player_y = src->player_y;
}

node_t* create_node(chessformer_t* init_data, node_t* parent){
	node_t *new_n = (node_t *) malloc(sizeof(node_t));
	new_n->parent = parent;
	new_n->depth = parent->depth + 1;
	copy_state(init_data, &(new_n->state), &(parent->state));
	return new_n;
}

/**
 * Apply an action to node n, create a new node resulting from 
 * executing the action, and return if the player moved
*/
bool applyAction(chessformer_t *init_data, node_t *n, node_t **new_node, int move_delta_x, int move_delta_y){
	bool player_moved = false;

    *new_node = create_node(init_data, n);
    (*new_node)->move_delta_x = move_delta_x;
	(*new_node)->move_delta_y = move_delta_y;

	// execute_move in utils.c both returns true if the move is legal and was conducted
    player_moved = execute_move(init_data, &((*new_node)->state), move_delta_x, move_delta_y);

	return player_moved;
}

void free_node(chessformer_t *init_data, node_t* n){
	if (n == NULL) {
		return;
	}

   // Free the dynamically allocated map in the node's state
    if (n->state.map != NULL) {
        for (int i = 0; i < init_data->lines; i++) {
            if (n->state.map[i] != NULL) {
                // Free each row of the map in the node
				free(n->state.map[i]); 
            }
        }
		// Free the array of pointers itself
        free(n->state.map); 
    }
	// Free the node itself
	free(n);
}

/**
 * Given a 2D map, returns a 1D map
*/
void flatten_map(chessformer_t* init_data, char **dst_map, char **src_map){

	int current_i = 0;
	for (int i = 0; i < init_data->lines; ++i)
	{
		int width = strlen(src_map[i]);
		for (int j = 0; j < width; j++){
			(*dst_map)[current_i] = src_map[i][j];
			current_i++;
		}
	}
}

bool winning_state(chessformer_t chessformer, struct state_s state) {

    for (int i = 0; i< chessformer.lines; i++) { 
		for (int j = 0; j < chessformer.num_chars_map/chessformer.lines; j++) { 
			if (state.map[i][j] == '$') {
				return false;  
            }
        } 
    }

    return true; 
}

char *saveSolution(node_t *finalNode) {
	int hierarchyCount = 0;
	node_t *current = finalNode;
	while(current){
		hierarchyCount++;
		current = current->parent;
	}
	// Each move is 2 characters, so 2 * (hierarchyCount - 1)
	// +1 for the null terminator and -1 as the root node has no parent
	char *soln = (char *) malloc(sizeof(char) * (hierarchyCount - 1)* 2 + 1);
	assert(soln);
	
	current = finalNode;
	int left = hierarchyCount - 1;
	while(current){
		if(! current->parent){
			current = current->parent;
			continue;
		}
		left--;
		char x = current->parent->state.player_x + current->move_delta_x + '`';
		char y = current->parent->state.player_y + current->move_delta_y + '0';
		soln[2 * left] = x;
		soln[2 * left + 1] = y;
		current = current->parent;
	}

	// Null-terminate the string
	soln[(hierarchyCount - 1) * 2] = '\0';

	return soln;
}



#define DEBUG 0

#include <assert.h>
#define DYNAMIC 2
#define MAX_CAPACITY 26*9 
#define BORDERS 2
#define PLACEHOLDER 1

/**
 * Find a solution by exploring all possible paths
 */
void find_solution(chessformer_t* init_data, bool show_solution){
	// Statistics' variables
	// Keep track of solving time
	clock_t start = clock();
	unsigned int exploredNodes = 0;
	unsigned int generatedNodes = 0;
	unsigned int duplicatedNodes = 0;
	unsigned solution_size = 0;

	// Solution string which contains the sequence of moves
	char* solution = NULL;

	// Buffer for dequeue
	chessformer_t current_state = *init_data; 
	current_state.soln = "";
	
	// Determine the rows and columns of the map, excluding the border
	int map_rows = (init_data->lines) - BORDERS;
	int map_columns = (init_data->num_chars_map / init_data->lines) - BORDERS;

	// Implementation of Psuedocode for algorithm 2 starts here
	node_t *n = create_init_node(init_data);
	queue *queue = create_queue();
	enqueue(queue, n);

	// Initialise an empty array for explored nodes
	node_t **explored_table = (node_t **)malloc(sizeof(node_t *) * DYNAMIC);
	int current_table_size = DYNAMIC;
	int table_index = 0; //TODO: remove

	// Initialise a hashtable for duplicate detection
	HashTable hash_table;
	size_t key_size = init_data->num_chars_map; // Size of map
	size_t value_size = sizeof(int); // Size of value
	size_t capacity = MAX_CAPACITY; // Given in skeleton code to cover all
	ht_setup(&hash_table, key_size, value_size, capacity);

	// Initialise a temporary variable to store key (1D representation of the map) for hash table
	char *flat_map = calloc(init_data->num_chars_map, sizeof(char));
	assert(flat_map);

	while (!is_queue_empty(queue)) {
		node_t *curr = dequeue(queue);
		explored_table[table_index++] = curr;
		// If the table has met maximum size, dynamically allocate more space
		if (table_index == current_table_size){
			current_table_size *= DYNAMIC;
			explored_table = realloc(explored_table, current_table_size * sizeof(node_t *));
		}
		exploredNodes++;

		// Check if current state is a winning one
		if (winning_state(current_state, curr->state)) {
			solution = saveSolution(curr);
			solution_size = curr->depth;
			break;
		}

		// Iterate through all pieces within the borders of the board
		for (int y = 1; y <= map_rows; y++) {
			for (int x = 1; x <= map_columns; x++) {
				// Find the change in x and y values from player's current position
				int move_delta_x = x - curr->state.player_x;
				int move_delta_y = y - curr->state.player_y;
				
				// Create Child new node to store the node with the applied action
				node_t *new_node = NULL;
				bool player_moved = applyAction(init_data, curr, &new_node, move_delta_x, move_delta_y);
				generatedNodes++;
				
				// If the move was not conducted, the new node is not useful for further exploration
				if (!player_moved) {
					free_node(&current_state, new_node);
					continue;
				} else {
					// Flatten the map of the new node into a 1D representation
					flatten_map(init_data, &flat_map, new_node->state.map);

					// Check if the flattened map is a duplicate map which has been seen before
					if (ht_contains(&hash_table, flat_map) == HT_FOUND) {
						duplicatedNodes++;
						free_node(&current_state, new_node);
					} else {
						// Insert the new flat map into hash table
						// Key is the flat map, value doesn't matter so we use PLACEHOLDER
						int random_val = PLACEHOLDER;
						ht_insert(&hash_table, flat_map, &random_val);

						// Enqueue the new node to explore 
						enqueue(queue,new_node);
					}
					
				}
			}
		}
	}


	// Implementation of Psuedocode for algorithm 2 has ended

	// Stop clock
	clock_t end = clock();
	double cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;

	// Free memory for generated nodes still in queue
	while (!is_queue_empty(queue)) {
		node_t *remaining_node = dequeue(queue);
		free_node(&current_state, remaining_node);
	}
	free(queue);

	// Free memory for nodes stored in explored table
	for (int i = 0; i < table_index; i++) {
		free_node(&current_state, explored_table[i]);
	}
	free(explored_table);

	// Free temporary variable for storing the key for the hash table
	free(flat_map);
		
	// Free the hash table and its contents
	ht_clear(&hash_table);
	ht_destroy(&hash_table);


	// Report statistics.

	// Show Solution	 
	if(show_solution && solution != NULL) {
		play_solution(*init_data, solution);
	}

	// endwin();

	if(solution != NULL){
		printf("\nSOLUTION:                               \n");
		printf( "%s\n\n", solution);
		FILE *fptr = fopen("solution.txt", "w");
		if (fptr == NULL) {
			printf("Could not open file");
			return ;
		}
		fprintf(fptr,"%s\n", solution);
		fclose(fptr);
		
		free(solution);
	}

	printf("STATS: \n");
	printf("\tExpanded nodes: %'d\n\tGenerated nodes: %'d\n\tDuplicated nodes: %'d\n", exploredNodes, generatedNodes, duplicatedNodes);
	printf("\tSolution Length: %d\n", solution_size);
	printf("\tExpanded/seconds: %d\n", (int)(exploredNodes/cpu_time_used));
	printf("\tTime (seconds): %f\n", cpu_time_used);	
}



void solve(char const *path, bool show_solution)
{
	/**
	 * Load Map
	*/
	chessformer_t chessformer = make_map(path, chessformer);
	
	/**
	 * Count number of boxes and Storage locations
	*/
	map_check(chessformer);

	/**
	 * Locate player x,y position
	*/
	chessformer = find_player(chessformer);
	
	chessformer.base_path = path;

	find_solution(&chessformer, show_solution);

	for(int i = 0; i < chessformer.lines; i++){
		free(chessformer.map[i]);
		free(chessformer.map_save[i]);
	}
	free(chessformer.map);
	free(chessformer.map_save);
	free(chessformer.buffer);

}

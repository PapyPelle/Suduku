#define  _CRT_SECURE_NO_WARNINGS // remove scanf warnings on Windows

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <omp.h>

typedef struct _cell
{
	char* values;
	int values_size;
	int count;
	int unique_value;
	omp_lock_t lock;
} Cell;

typedef struct _action {
	char val;
	Cell* cell;
	struct _action* next;
} Action;

typedef struct _stack {
	Action* first;
	int choices;
	omp_lock_t lock;
} Stack;

typedef struct _sudoku
{
	Cell* data;
	Stack* stack;
	int dim;
	int data_size;
	int value_range;
	char blocked;
	char dirty;
	char dirty_tmp;
	char* dirty_rows;
	char* dirty_rows_tmp;
	char* dirty_columns;
	char* dirty_columns_tmp;
	char* dirty_blocks;
	char* dirty_blocks_tmp;
} Sudoku;

int		cell_init(Sudoku* s, Cell* c, int value);
void	cell_free(Cell* c);
void	cell_set_empty(Cell* c);
void	cell_set_filled(Cell* c, int val);
int		cell_has_possible_values(const Cell* c);
int		cell_has_unique_value(const Cell* c);
int		cell_can_have_value(const Cell* c, int v);
int		cell_get_xth_value(const Cell* c, int x);
int		cell_get_first_value(const Cell* c);
int		cell_get_unique_value(const Cell* c);
int 	cell_set_value(Cell* c, int v);
int 	cell_unset_value(Cell* c, int v);

Sudoku*	sudoku_create(FILE* input);
void	sudoku_free(Sudoku* s);
Cell*	sudoku_get_cell(Sudoku* s, int row, int col);
Cell*	sudoku_get_row_cell(Sudoku* s, int row, int index);
Cell*	sudoku_get_column_cell(Sudoku* s, int col, int index);
Cell*	sudoku_get_block_cell(Sudoku* s, int block, int index);
int		sudoku_get_cell_block(Sudoku* s, int row, int col);
void	sudoku_clear_tmp_dirty_bits(Sudoku* s);
int		sudoku_force_set_value(Sudoku* s, Cell* c, int v);
void	sudoku_update_row(Sudoku* s, int index);
void	sudoku_update_column(Sudoku* s, int index);
void	sudoku_update_block(Sudoku* s, int index);
int		sudoku_make_choice(Sudoku* s, int choice_number);
void 	sudoku_print(Sudoku* s);

void	stack_push(Stack* st, Cell* c, int val);
void	stack_push_breakpoint(Stack* st, int val);
Action	stack_pop(Stack* st);
Stack*	stack_init();
void	stack_free(Stack* st);
void	stack_print(Stack* st);

// ================================================================================= //

// Crée la pile (malloc du pointeur)
Stack* stack_init()
{
	Stack* st = malloc(sizeof(Stack));
	if (st == NULL) return NULL;
	st->first = NULL;
	st->choices = 0;
	omp_init_lock(&(st->lock));
	return st;
}

// Libère la pile (et ce qu'il reste dedans)
void stack_free(Stack* st)
{
	omp_destroy_lock(&(st->lock));
	Action* to_free = NULL;
	while (st->first != NULL)
	{
		to_free = st->first;
		st->first = to_free->next;
		free(to_free);
	}
	free(st);
}

// Rajoute un élément dans la pile
void stack_push(Stack* st, Cell* c, int val)
{
	Action* new_action = malloc(sizeof(Action));
	if (new_action == NULL) { printf("Stack push failed"); abort(); return; }
	new_action->val = val;
	new_action->cell = c;
	omp_set_lock(&(st->lock));
	new_action->next = st->first;
	st->first = new_action;
	omp_unset_lock(&(st->lock));
}

// Rajoute un point critique
void stack_push_breakpoint(Stack* st, int val)
{
	Action* new_action = malloc(sizeof(Action));
	if (new_action == NULL) { printf("Stack push failed"); abort(); return; }
	new_action->val = val;
	new_action->cell = NULL;
	omp_set_lock(&(st->lock));
	new_action->next = st->first;
	st->first = new_action;
	st->choices += 1;
	omp_unset_lock(&(st->lock));
}

// Renvoie une copie du premier élément et le détruit de la pile
Action stack_pop(Stack* st)
{
	Action ret_action;
	Action* first_action;

	omp_set_lock(&(st->lock));
	first_action = st->first;
	st->first = first_action->next;
	if (first_action->cell == NULL)
		st->choices -= 1;
	omp_unset_lock(&(st->lock));

	ret_action.cell = first_action->cell;
	ret_action.val = first_action->val;
	ret_action.next = NULL;
	free(first_action);

	return ret_action;
}

void stack_print(Stack* st)
{
	int i = 0;
	Action* it = st->first;
	while (it != NULL)
	{
		printf("%d : (%p, %d) \n", i++, it->cell, it->val);
		it = it->next;
	}
}


// =================================================================================


// Initialise une case avec une valeur donnée (0 si vide). Retourne 1 si réussi, 0 sinon.
int cell_init(Sudoku* s, Cell* c, int value)
{
	c->values = (char*)malloc(s->value_range * sizeof(char));
	if (c->values == NULL) return 0;
	c->values_size = s->value_range;
	omp_init_lock(&(c->lock));

	if (value == 0)
		cell_set_empty(c);
	else
		cell_set_filled(c, value);

	return 1;
}

// Libère les ressources allouées pour une case. NE LIBERE PAS LA CASE EN ELLE MEME.
void cell_free(Cell* c)
{
	omp_destroy_lock(&(c->lock));
	free(c->values);
	c->values = NULL;
	c->values_size = 0;
	c->count = 0;
	c->unique_value = 0;
}

// Indique qu'une case peut contenir toutes les valeurs
__inline void cell_set_empty(Cell* c)
{
	memset(c->values, 1, c->values_size * sizeof(char));
	c->count = c->values_size;
	c->unique_value = 0;
}

// Indique qu'une case est occupée par une seule valeur
__inline void cell_set_filled(Cell* c, int val)
{
	memset(c->values, 0, c->values_size * sizeof(char));
	c->values[val - 1] = 1;
	c->count = 1;
	c->unique_value = val;
}

// Renvoie 0 si la case n'a aucune valeur possible : le sudoku est cassé
__inline int cell_has_possible_values(const Cell* c)
{
	return (c->count > 0);
}

// Renvoie 1 si la case possède un chiffre unique, 0 sinon
__inline int cell_has_unique_value(const Cell* c)
{
	return (c->count == 1);
}

__inline int cell_can_have_value(const Cell* c, int v)
{
	return (int)(c->values[v - 1]);
}

// Renvoie la 1ere valeur trouvée
__inline int cell_get_first_value(const Cell* c)
{
	return cell_get_xth_value(c, 1);
}

// Renvoie la xeme valeur trouvée
__inline int cell_get_xth_value(const Cell* c, int x)
{
	int count = 0;
	int i;
	for (i = 0; i < c->values_size; i++) {
		if (c->values[i]) {
			count++;
			if (count >= x) {
				return i + 1;
			}
		}
	}
	return 0;
}

// Renvoie la valeure unique d'une case, 0 sinon
__inline int cell_get_unique_value(const Cell* c) {
	if (!cell_has_unique_value(c))
		return 0;
	else
		return c->unique_value;
}

// Indique que la case peut contenir la valeur n
__inline int cell_set_value(Cell* c, int v)
{
	if (c->values[v - 1] == 1)
		return 0;
	c->values[v - 1] = 1;
	c->count++;
	if (c->count != 1)
		c->unique_value = 0;
	return 1;
}

// Indique que la case ne peut pas contenir la valeur n
__inline int cell_unset_value(Cell* c, int v)
{
	if (c->values[v - 1] == 0)
		return 0;
	if (c->count == 1) // S'il ne reste qu'une seule valeur possible, et qu'on veut l'enlever, c'est qu'on est bloqué
		return -1;
	c->values[v - 1] = 0;
	c->count--;
	if (c->count == 1)
		c->unique_value = cell_get_first_value(c);
	return 1;
}

// ================================================================================= //

// Crée un sudoku à partir d'un fichier de données d'entrée
Sudoku* sudoku_create(FILE* input)
{
	Sudoku* s;
	int i;
	int v;
	int dim, value_range, data_size;

	if (fscanf(input, "%d", &dim) <= 0) return NULL;

	value_range = dim * dim;
	data_size = value_range * value_range;

	s = (Sudoku*)malloc(sizeof(Sudoku));
	if (s == NULL) return NULL;
	s->data_size = 0;

	s->stack = stack_init();
	if (s->stack == NULL) { sudoku_free(s); return NULL; }

	s->data = (Cell*)malloc(data_size * sizeof(Cell));
	if (s->data == NULL) { sudoku_free(s); return NULL; }

	// Row dirty bits
	s->dirty_rows = (char*)malloc(value_range * sizeof(char));
	if (s->dirty_rows == NULL) { sudoku_free(s); return NULL; }
	memset(s->dirty_rows, 1, value_range * sizeof(char));

	s->dirty_rows_tmp = (char*)malloc(value_range * sizeof(char));
	if (s->dirty_rows_tmp == NULL) { sudoku_free(s); return NULL; }
	memset(s->dirty_rows_tmp, 0, value_range * sizeof(char));

	// Column dirty bits
	s->dirty_columns= (char*)malloc(value_range * sizeof(char));
	if (s->dirty_columns == NULL) { sudoku_free(s); return NULL; }
	memset(s->dirty_columns, 1, value_range * sizeof(char));

	s->dirty_columns_tmp = (char*)malloc(value_range * sizeof(char));
	if (s->dirty_columns_tmp == NULL) { sudoku_free(s); return NULL; }
	memset(s->dirty_columns_tmp, 0, value_range * sizeof(char));

	// Block dirty bits
	s->dirty_blocks = (char*)malloc(value_range * sizeof(char));
	if (s->dirty_blocks == NULL) { sudoku_free(s); return NULL; }
	memset(s->dirty_blocks, 1, value_range * sizeof(char));

	s->dirty_blocks_tmp = (char*)malloc(value_range * sizeof(char));
	if (s->dirty_blocks_tmp == NULL) { sudoku_free(s); return NULL; }
	memset(s->dirty_blocks_tmp, 0, value_range * sizeof(char));

	s->dim = dim;
	s->value_range = value_range;
	s->data_size = data_size;
	s->blocked = 0;
	s->dirty = 1;
	s->dirty_tmp = 0;

	for (i = 0; i < s->data_size; i++)
	{
		if (fscanf(input, "%d", &v) <= 0 || !cell_init(s, &(s->data[i]), v)) { sudoku_free(s); return NULL; }
	}

	return s;
}

// Libère les ressources allouées par un sudoku
void sudoku_free(Sudoku* s)
{
	int i;

	for (i = 0; i < s->data_size; i++)
		cell_free(&(s->data[i]));
	free(s->data);
	s->data = NULL;
	s->data_size = 0;
	
	stack_free(s->stack);
	s->stack = NULL;

	free(s->dirty_rows);
	free(s->dirty_columns);
	free(s->dirty_blocks);
	s->dirty_rows = s->dirty_columns = s->dirty_blocks = NULL;

	free(s->dirty_rows_tmp);
	free(s->dirty_columns_tmp);
	free(s->dirty_blocks_tmp);
	s->dirty_rows_tmp = s->dirty_columns_tmp = s->dirty_blocks_tmp = NULL;
	
	free(s);
}

// Retourne la case du sudoku à la ligne/colonne indiquée
__inline Cell* sudoku_get_cell(Sudoku* s, int row, int col)
{
	return &(s->data[row * s->value_range + col]);
}

// Retourne une cellule d'une ligne
__inline Cell* sudoku_get_row_cell(Sudoku* s, int row, int index)
{
	return sudoku_get_cell(s, row, index);
}

// Retourne une cellule d'une colonne
__inline Cell* sudoku_get_column_cell(Sudoku* s, int col, int index)
{
	return sudoku_get_cell(s, index, col);
}

// Retourne l'identifiant du bloc contenant la case à la ligne/colonne indiqué
__inline int sudoku_get_cell_block(Sudoku* s, int row, int col)
{
	return (row / s->dim) * s->dim + (col / s->dim);
}

// Retourne une cellule d'un bloc
__inline Cell* sudoku_get_block_cell(Sudoku* s, int group, int index)
{
	return sudoku_get_cell(s, (group / s->dim) * s->dim + (index / s->dim), (group % s->dim) * s->dim + (index % s->dim));
}

__inline void sudoku_set_row_dirty_INTERNAL(Sudoku* s, int row)
{
	s->dirty_tmp = 1;
	s->dirty_rows_tmp[row] = 1;
}

__inline void sudoku_set_column_dirty_INTERNAL(Sudoku* s, int col)
{
	s->dirty_tmp = 1;
	s->dirty_columns_tmp[col] = 1;
}

__inline void sudoku_set_block_dirty_INTERNAL(Sudoku* s, int block)
{
	s->dirty_tmp = 1;
	s->dirty_blocks_tmp[block] = 1;
}

__inline void sudoku_set_dirty_INTERNAL(Sudoku* s, int row, int col)
{
	sudoku_set_row_dirty_INTERNAL(s, row);
	sudoku_set_column_dirty_INTERNAL(s, col);
	sudoku_set_block_dirty_INTERNAL(s, sudoku_get_cell_block(s, row, col));
}

__inline void sudoku_dirty_setter_row(Sudoku* s, int row, int index)
{
	sudoku_set_dirty_INTERNAL(s, row, index);
}

__inline void sudoku_dirty_setter_column(Sudoku* s, int col, int index)
{
	sudoku_set_dirty_INTERNAL(s, index, col);
}

__inline void sudoku_dirty_setter_block(Sudoku* s, int block, int index)
{
	sudoku_set_dirty_INTERNAL(s, (block / s->dim) * s->dim + (index / s->dim), (block % s->dim) * s->dim + (index % s->dim));
}


__inline void sudoku_force_set_dirty(Sudoku* s, int row, int col)
{
	s->dirty = 1;
	s->dirty_rows[row] = 1;
	s->dirty_columns[col] = 1;
	s->dirty_blocks[sudoku_get_cell_block(s, row, col)] = 1;
}

__inline void sudoku_clear_tmp_dirty_bits(Sudoku* s)
{
	s->dirty_tmp = 0;
	memset(s->dirty_rows_tmp, 0, s->value_range * sizeof(char));
	memset(s->dirty_columns_tmp, 0, s->value_range * sizeof(char));
	memset(s->dirty_blocks_tmp, 0, s->value_range * sizeof(char));
}

__inline void sudoku_apply_dirty_bits(Sudoku* s)
{
	s->dirty = s->dirty_tmp;
	memcpy(s->dirty_rows, s->dirty_rows_tmp, s->value_range * sizeof(char));
	memcpy(s->dirty_columns, s->dirty_columns_tmp, s->value_range * sizeof(char));
	memcpy(s->dirty_blocks, s->dirty_blocks_tmp, s->value_range * sizeof(char));
	sudoku_clear_tmp_dirty_bits(s);
}

// Force la valeur d'une case à v
__inline int sudoku_force_set_value(Sudoku* s, Cell* c, int v)
{
	if (c->values[v - 1] == 1 && c->count == 1)
		return 0;
	int i, m;
	for (i = 0; i < c->values_size; i++)
	{
		if (v != i + 1)
		{
			m = cell_unset_value(c, i + 1);
			if (m == -1)
				return -1;
			if (m)
				stack_push(s->stack, c, i + 1);
		}
	}
	return 1;
}

// Affiche la grille de sudoku
void sudoku_print(Sudoku* s)
{
	int i, j;
	int v;

	printf("+++++++++++++++++++++++++++++\n");
	for (i = 0; i < s->value_range; i++)
	{
		for (j = 0; j < s->value_range; j++)
		{
			v = cell_get_unique_value(sudoku_get_cell(s, i, j));
			if (v != 0)
				printf("%d ", v);
			else
				printf(". ");

			/*
			// Affichage des valeurs possibles pour chaque case
			int k;
			printf("(");
			for (k = 0; k < s->value_range; k++)
				if (cell_can_have_value(sudoku_get_cell(s, i, j), k + 1))
					printf("%d", k + 1);
			printf(")  ");
			*/
		}
		printf("\n");
	}
	printf("+++++++++++++++++++++++++++++\n");
	
	/*
	printf("DIRTY : %d\n", s->dirty);
	printf("rows : ");
	for (i = 0; i < s->value_range; i++)
		printf("%d ", s->dirty_rows[i]);
	printf("\ncols : ");
	for (i = 0; i < s->value_range; i++)
		printf("%d ", s->dirty_columns[i]);
	printf("\nblocks : ");
	for (i = 0; i < s->value_range; i++)
		printf("%d ", s->dirty_blocks[i]);
	printf("\n");
	printf("+++++++++++++++++++++++++++++\n");
	*/
}

// Fonction interne mettant à jour les cases selon un *getter* donné.
// Cf sudoku_update_{row|column|block}
void sudoku_update_INTERNAL
(
	Sudoku* s, 
	Cell* (*getter)(Sudoku* _s, int _group_index, int _cell_index), 
	void (*dirty_setter)(Sudoku* _s, int _group_index, int _cell_index),
	int group
)
{
	int i, j, m;

	//printf("[%d/%d]\n", omp_get_thread_num(), omp_get_num_threads());

	// Si une valeur est "écrite" dans une case, alors on peut retirer cette valeur des valeurs possibles dans les autres cases du groupe
	for (i = 0; i < s->value_range; i++)
	{
		Cell* c = getter(s, group, i);

		omp_set_lock(&(c->lock));
		int v = cell_get_unique_value(c);
		omp_unset_lock(&(c->lock));

		if (v != 0)
		{
			for (j = 0; j < s->value_range; j++)
			{
				if (i != j)
				{
					Cell* c2 = getter(s, group, j);

					omp_set_lock(&(c2->lock));
					m = cell_unset_value(c2, v);
					omp_unset_lock(&(c2->lock));

					if (m == -1)
					{
						s->blocked = 1;
						return;
					}
					if (m)
					{
						dirty_setter(s, group, j);
						stack_push(s->stack, c2, v);
					}
				}

			}
		}
	}

	// Si une case d'un groupe est la seule à pouvoir accueillir une valeur, alors on peut "écrire" cette valeur dans la case.
	for (i = 0; i < s->value_range; i++)
	{
		int v = i + 1;
		Cell* unique = NULL;
		int unique_index = -1;

		for (j = 0; j < s->value_range; j++)
		{
			Cell* c = getter(s, group, j);

			omp_set_lock(&(c->lock));
			int can_have_value = cell_can_have_value(c, v);
			omp_unset_lock(&(c->lock));

			if (can_have_value)
			{
				if (unique == NULL)
				{
					unique = c;
					unique_index = j;
				}
				else
				{
					unique = NULL;
					break;
				}
			}
		}

		if (unique != NULL)
		{
			omp_set_lock(&(unique->lock));
			m = sudoku_force_set_value(s, unique, v);
			omp_unset_lock(&(unique->lock));
			
			if (m == -1)
			{
				s->blocked = 1;
				return;
			}
			if (m)
				dirty_setter(s, group, unique_index);
		}
	}
}

// Met à jour les valeurs possibles pour une ligne donnée
__inline void sudoku_update_row(Sudoku* s, int index)
{
	sudoku_update_INTERNAL(s, &sudoku_get_row_cell, &sudoku_dirty_setter_row, index);
}

// Met à jour les valeurs possibles pour une colonne donnée
__inline void sudoku_update_column(Sudoku* s, int index)
{
	sudoku_update_INTERNAL(s, &sudoku_get_column_cell, &sudoku_dirty_setter_column, index);
}

// Met à jour les valeurs possibles pour un bloc donné
__inline void sudoku_update_block(Sudoku* s, int index)
{
	sudoku_update_INTERNAL(s, &sudoku_get_block_cell, &sudoku_dirty_setter_block, index);
}

// Choisit une valeur arbitraire parmis une des cases à plusieurs possibilités
// ATTENTION : à n'utiliser qu'en cas de blocage
// retourne 1 si un choix est fait, 0 sinon (= fin du sudoku puisque 100% value unique)
int sudoku_make_choice(Sudoku* s, int choice_number)
{
	//printf("[%d] BLOCKED : MAKING CHOICE (%d) -> ", omp_get_thread_num(), s->stack->choices);
	int i, j, k;
	for (i = 0; i < s->value_range; i++)
	{
		for (j = 0; j < s->value_range; j++)
		{
			Cell* c = sudoku_get_cell(s, i, j);
			if (!cell_has_unique_value(c))
			{
				int count_v = 0;
				for (k = 0; k < s->value_range; k++)
				{
					if (cell_can_have_value(c, k + 1))
					{
						count_v++;
						if (count_v > choice_number)
						{
							stack_push_breakpoint(s->stack, count_v);
							sudoku_force_set_value(s, c, k + 1);
							sudoku_force_set_dirty(s, i, j);
							//printf("choosing %d for cell %d:%d (test %d)\n", k + 1, i, j, count_v);
							return 1;
						}
					}
				}
				//printf("no more choice, need to backtrack\n");
				return 0;
			}
		}
	}
	printf("You shouldn't be here\n");
	abort();
	return 0;
}

// Renvoie 0 si les sudoku est faux, 1+ sinon (nbr de cases encore vides)
int sudoku_is_dead(Sudoku* s)
{
	int i, j;
	int ret = 1;
	for (i = 0; i < s->value_range; i++)
	{
		for (j = 0; j < s->value_range; j++)
		{
			Cell* c = sudoku_get_cell(s, i, j);
			if (!cell_has_unique_value(c))
			{
				if (!cell_has_possible_values(c))
				{
					return 0;
				}
				ret++;
			}
		}
	}
	return ret;
}


void sudoku_bactrack_full(Sudoku* s)
{
	Action a;
	while (s->stack->first != NULL)
	{
		a = stack_pop(s->stack);
		if (a.cell == NULL)
			continue;
		cell_set_value(a.cell, a.val);
	}
}

int sudoku_backtrack(Sudoku* s)
{
	Action a;
	while (s->stack->first != NULL)
	{
		a = stack_pop(s->stack);
		if (a.cell == NULL)
			return a.val;
		cell_set_value(a.cell, a.val);
	}
	return -1;
}

void sudoku_resolution(Sudoku* s)
{
	int end, it, i, check;
	it = 0;
	end = 1;


	while (end) {
		s->blocked = 0;

		do {
			for (i = 0; i < s->value_range; i++)
			{
				if (s->dirty_rows[i])
				{
					#pragma omp task
					sudoku_update_row(s, i);
				}

				if (s->dirty_columns[i])
				{
					#pragma omp task
					sudoku_update_column(s, i);
				}

				if (s->dirty_blocks[i])
				{
					#pragma omp task
					sudoku_update_block(s, i);
				}
			}

			#pragma omp taskwait

			sudoku_apply_dirty_bits(s);

			//printf("----- %d\n", it);
			it++;
			if (it % 100 == 0)
			{
				printf(".");
				fflush(stdout);
			}

			if (s->blocked)
				break;

		} while (s->dirty);
		
		if (s->blocked)
			check = 0;
		else
			check = sudoku_is_dead(s);

		if (check == 1) // toutes les cases ont une valeur unique
		{
			end = 0;
		}
		else if (check == 0) // une case ne peut plus être remplie
		{
			int r;
			do {
				int choice_nbr = sudoku_backtrack(s); // retour précédent
				if (choice_nbr == -1)
				{
					printf("This sudoku has no solution\n");
					return;
				}
				r = sudoku_make_choice(s, choice_nbr);
				//printf("Backtraking to choice %d\n", s->stack->choices - 1);
			} while (r != 1); // r = 1 -> un nouveau choix peut être fait
		}
		else // il reste des cases vides
		{
			sudoku_make_choice(s, 0);
		}
	}
}

// ================================================================================= //

// MMMMAAAAAAAAIIIINNNN
int main(int argc, char* argv[])
{
	Sudoku* s;

	s = sudoku_create(stdin);
	if (s == NULL) { fprintf(stderr, "Sudoku initialization failed\n"); return EXIT_FAILURE; }

	sudoku_print(s);

	#pragma omp parallel
	#pragma omp single nowait
	sudoku_resolution(s);

	sudoku_print(s);

	sudoku_free(s);

	return EXIT_SUCCESS;
}
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
} Cell;

typedef struct _action {
	char val;
	Cell* cell;
	struct _action* next;
} Action;

typedef struct _stack {
	Action* first;
	int choices;
} Stack;

typedef struct _sudoku
{
	Cell* data;
	Stack* stack;
	int dim;
	int data_size;
	int value_range;
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
int		sudoku_force_set_value(Sudoku* s, Cell* c, int v);
int 	sudoku_update_row(Sudoku* s, int index);
int		sudoku_update_column(Sudoku* s, int index);
int 	sudoku_update_block(Sudoku* s, int index);
int		sudoku_make_choice(Sudoku* s, int choice_number);
int 	sudoku_print(Sudoku* s);

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
	return st;
}

// Libère la pile (et ce qu'il reste dedans)
void stack_free(Stack* st)
{
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
	if (new_action == NULL) { printf("Stack push failed"); return; }
	new_action->val = val;
	new_action->cell = c;
	new_action->next = st->first;
	st->first = new_action;
}

// Rajoute un point critique
void stack_push_breakpoint(Stack* st, int val)
{
	Action* new_action = malloc(sizeof(Action));
	if (new_action == NULL) { printf("Stack push failed"); return; }
	new_action->val = val;
	new_action->cell = NULL;
	new_action->next = st->first;
	st->first = new_action;
	st->choices += 1;
}

// Renvoie une copie du premier élément et le détruit de la pile
Action stack_pop(Stack* st)
{
	Action ret_action;
	Action* first_action = st->first;
	ret_action.cell = first_action->cell;
	ret_action.val = first_action->val;
	ret_action.next = NULL;
	st->first = first_action->next;
	free(first_action);
	if (ret_action.cell == NULL)
		st->choices -= 1;
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


// ================================================================================= //


// Initialise une case avec une valeur donnée (0 si vide). Retourne 1 si réussi, 0 sinon.
int cell_init(Sudoku* s, Cell* c, int value)
{
	c->values = (char*)malloc(s->value_range * sizeof(char));
	if (c->values == NULL) return 0;
	c->values_size = s->value_range;

	if (value == 0)
		cell_set_empty(c);
	else
		cell_set_filled(c, value);

	return 1;
}

// Libère les ressources allouées pour une case. NE LIBERE PAS LA CASE EN ELLE MEME.
void cell_free(Cell* c)
{
	free(c->values);
	c->values = NULL;
	c->values_size = 0;
	c->count = 0;
	c->unique_value = 0;
}

// Indique qu'une case peut contenir toutes les valeurs
inline void cell_set_empty(Cell* c)
{
	memset(c->values, 1, c->values_size * sizeof(char));
	c->count = c->values_size;
	c->unique_value = 0;
}

// Indique qu'une case est occupée par une seule valeur
inline void cell_set_filled(Cell* c, int val)
{
	memset(c->values, 0, c->values_size * sizeof(char));
	c->values[val - 1] = 1;
	c->count = 1;
	c->unique_value = val;
}

// Renvoie 0 si la case n'a aucune valeur possible : le sudoku est cassé
inline int cell_has_possible_values(const Cell* c)
{
	return (c->count > 0);
}

// Renvoie 1 si la case possède un chiffre unique, 0 sinon
inline int cell_has_unique_value(const Cell* c)
{
	return (c->count == 1);
}

inline int cell_can_have_value(const Cell* c, int v)
{
	return (int)(c->values[v - 1]);
}

// Renvoie la 1ere valeur trouvée
inline int cell_get_first_value(const Cell* c)
{
	return cell_get_xth_value(c, 1);
}

// Renvoie la xeme valeur trouvée
inline int cell_get_xth_value(const Cell* c, int x)
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
inline int cell_get_unique_value(const Cell* c) {
	if (!cell_has_unique_value(c))
		return 0;
	else
		return c->unique_value;
}

// Indique que la case peut contenir la valeur n
inline int cell_set_value(Cell* c, int v)
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
inline int cell_unset_value(Cell* c, int v)
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

	s->stack = stack_init();
	if (s->stack == NULL) { free(s); return NULL; }

	s->data = (Cell*)malloc(data_size * sizeof(Cell));
	if (s->data == NULL) { free(s->stack); free(s); return NULL; }

	s->dim = dim;
	s->value_range = value_range;
	s->data_size = data_size;

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
	free(s);
}

// Retourne la case du sudoku à la ligne/colonne indiquée
inline Cell* sudoku_get_cell(Sudoku* s, int row, int col)
{
	return &(s->data[row * s->value_range + col]);
}

// Retourne une cellule d'une ligne
inline Cell* sudoku_get_row_cell(Sudoku* s, int row, int index)
{
	return sudoku_get_cell(s, row, index);
}

// Retourne une cellule d'une colonne
inline Cell* sudoku_get_column_cell(Sudoku* s, int col, int index)
{
	return sudoku_get_cell(s, index, col);
}

// Retourne l'identifiant du bloc contenant la case à la ligne/colonne indiqué
inline int sudoku_get_cell_block(Sudoku* s, int row, int col)
{
	return (row / s->dim) * s->dim + (col / s->dim);
}

// Retourne une cellule d'un bloc
inline Cell* sudoku_get_block_cell(Sudoku* s, int group, int index)
{
	return sudoku_get_cell(s, (group / s->dim) * s->dim + (index / s->dim), (group % s->dim) * s->dim + (index % s->dim));
}

// Force la valeur d'une case à v
inline int sudoku_force_set_value(Sudoku* s, Cell* c, int v)
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
int sudoku_print(Sudoku* s)
{
	static int it = 0;
	printf("---------------- %d\n", it++);
	int i, j;
	int v;
	for (i = 0; i < s->value_range; i++)
	{
		for (j = 0; j < s->value_range; j++)
		{
			v = cell_get_unique_value(sudoku_get_cell(s, i, j));
			if (v != 0)
				printf("%d ", v);
			else
				printf(". ");

			
			// Affichage des valeurs possibles pour chaque case
			int k;
			printf("(");
			for (k = 0; k < s->value_range; k++)
				if (cell_can_have_value(sudoku_get_cell(s, i, j), k + 1))
					printf("%d", k + 1);
			printf(")  ");
			
		}
		printf("\n");
	}
	return it;
}

// Fonction interne mettant à jour les cases selon un *getter* donné.
// Cf sudoku_update_{row|column|block}
int sudoku_update_INTERNAL(Sudoku* s, Cell* (*getter)(Sudoku* _s, int _group_index, int _cell_index), int index)
{
	int i, j, m;
	int modif = 0;

	// Si une valeur est "écrite" dans une case, alors on peut retirer cette valeur des valeurs possibles dans les autres cases du groupe
	for (i = 0; i < s->value_range; i++)
	{
		int v = cell_get_unique_value(getter(s, index, i));

		if (v != 0)
		{
			for (j = 0; j < s->value_range; j++)
			{
				if (i != j)
				{
					Cell* c = getter(s, index, j);
					m = cell_unset_value(c, v);
					if (m == -1)
						return -1;
					modif |= m;
					if (m)
						stack_push(s->stack, c, v);
				}

			}
		}
	}

	// Si une case d'un groupe est la seule à pouvoir accueillir une valeur, alors on peut "écrire" cette valeur dans la case.
	for (i = 0; i < s->value_range; i++)
	{
		int v = i + 1;
		Cell* unique = NULL;

		for (j = 0; j < s->value_range; j++)
		{
			Cell* c = getter(s, index, j);
			if (cell_can_have_value(c, v))
			{
				if (unique == NULL)
					unique = c;
				else
				{
					unique = NULL;
					break;
				}
			}
		}

		if (unique != NULL)
		{
			m = sudoku_force_set_value(s, unique, v);
			if (m == -1)
				return -1;
			modif |= m;
		}
	}

	return modif;
}

// Met à jour les valeurs possibles pour une ligne donnée
inline int sudoku_update_row(Sudoku* s, int index)
{
	return sudoku_update_INTERNAL(s, &sudoku_get_row_cell, index);
}

// Met à jour les valeurs possibles pour une colonne donnée
inline int sudoku_update_column(Sudoku* s, int index)
{
	return sudoku_update_INTERNAL(s, &sudoku_get_column_cell, index);
}

// Met à jour les valeurs possibles pour un bloc donné
inline int sudoku_update_block(Sudoku* s, int index)
{
	return sudoku_update_INTERNAL(s, &sudoku_get_block_cell, index);
}

// Choisit une valeur arbitraire parmis une des cases à plusieurs possibilités
// ATTENTION : à n'utiliser qu'en cas de blocage
// retourne 1 si un choix est fait, 0 sinon (= fin du sudoku puisque 100% value unique)
int sudoku_make_choice(Sudoku* s, int choice_number)
{
	printf("BLOCKED : MAKING CHOICE (%d) -> ", s->stack->choices);
	int i, j, k, count;
	count = 0;
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
						if (count + count_v > choice_number)
						{
							stack_push_breakpoint(s->stack, count + count_v);
							sudoku_force_set_value(s, c, k + 1);
							printf("choosing %d for cell %d:%d (test %d)\n", k + 1, i, j, count + count_v);
							return 1;
						}
					}
				}
				count += count_v;
			}
		}
	}
	printf("no more choices\n");
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
	return 0;
}

void sudoku_resolution(Sudoku* s)
{
	int max_it = 10000;
	int end, it, modif, ret, i, check;
	it = 0;
	end = 1;
	while (end) {
		modif = 1;
		while (modif) // Tant que la déduction fonctionne
		{
			modif = 0;
			ret = -1;
			for (i = 0; i < s->value_range; i++)
			{
				ret = sudoku_update_row(s, i);
				if (ret == -1)
					break;
				modif |= ret;
				ret = sudoku_update_column(s, i);
				if (ret == -1)
					break;
				modif |= ret;
				ret = sudoku_update_block(s, i);
				if (ret == -1)
					break;
				modif |= ret;
			}
			printf("----- %d\n", it);
			it++;
			if (ret == -1)
				break;
		}

		if (it > max_it)
			return;
		
		if (ret == -1)
			check = 0;
		else
			check = sudoku_is_dead(s);

		if (check == 1) // toutes les cases ont une valeur unique
		{
			end = 0;
		}
		else if (check == 0) // une case ne peut plus être remplie
		{
			do {
				int choice_nbr = sudoku_backtrack(s); // retour précédent
				end = sudoku_make_choice(s, choice_nbr);
				printf("Backtraking to choice %d\n", s->stack->choices);
			} while (end != 1); // end = 1 -> un nouveau choix peut être fait
		}
		else // il reste des cases vides
		{
			sudoku_print(s);
			end = sudoku_make_choice(s, 0);
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

	sudoku_resolution(s);

	sudoku_print(s);

	sudoku_free(s);

	return EXIT_SUCCESS;
}
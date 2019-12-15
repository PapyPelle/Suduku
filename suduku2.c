#define  _CRT_SECURE_NO_WARNINGS // remove scanf warnings on Windows

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct _cell
{
	char* values;
	int values_size;
	int count;
	int unique_value;
} Cell;

typedef struct _sudoku
{
	Cell* data;
	int dim;
	int data_size;
	int value_range;
} Sudoku;

typedef enum _group_type
{
	GROUP_ROW = 0,
	GROUP_COLUMN,
	GROUP_BLOCK,
	LAST_GROUP_TYPE
} GroupType;

int		cell_init(Sudoku* s, Cell* c, int value);
void	cell_free(Cell* c);
void	cell_set_empty(Cell* c);
void	cell_set_filled(Cell* c, int val);
int		cell_has_unique_value(const Cell* c);
int		cell_can_have_value(const Cell* c, int v);
int		cell_get_first_value(const Cell* c);
int		cell_get_unique_value(const Cell* c);
void	cell_set_value(Cell* c, int v);
void	cell_unset_value(Cell* c, int v);

Sudoku*	sudoku_create(FILE* input);
void	sudoku_free(Sudoku* s);
Cell*	sudoku_get_cell(Sudoku* s, int row, int col);
Cell*	sudoku_get_row_cell(Sudoku* s, int row, int index);
Cell*	sudoku_get_column_cell(Sudoku* s, int col, int index);
Cell*	sudoku_get_block_cell(Sudoku* s, int block, int index);
int		sudoku_get_cell_block(Sudoku* s, int row, int col);
void	sudoku_update_row(Sudoku* s, int index);
void	sudoku_update_column(Sudoku* s, int index);
void	sudoku_update_block(Sudoku* s, int index);
void	sudoku_print(Sudoku* s);


// =============================================================== //


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

// Renvoie 1 si la case possède un chiffre unique, 0 sinon
inline int cell_has_unique_value(const Cell* c)
{
	return (c->count == 1);
}

inline int cell_can_have_value(const Cell* c, int v)
{
	return (int)(c->values[v - 1]);
}

// Renvoie la première valeur trouvée
inline int cell_get_first_value(const Cell* c)
{
	int i;
	for (i = 0; i < c->values_size; i++) {
		if (c->values[i]) {
			return i + 1;
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
inline void cell_set_value(Cell* c, int v)
{
	if (c->values[v - 1] == 0)
	{
		c->values[v - 1] = 1;
		if (c->count == 1)
			c->unique_value = 0;
		c->count++;
	}
}

// Indique que la case ne peut pas contenir la valeur n
inline void cell_unset_value(Cell* c, int v)
{
	if (c->values[v - 1] == 1)
	{
		c->values[v - 1] = 0;
		c->count--;
		if (c->count == 1)
			c->unique_value = cell_get_first_value(c);
	}
}

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

	s->data = (Cell*)malloc(data_size * sizeof(Cell));
	if (s->data == NULL) { free(s); return NULL; }

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

// Affiche la grille de sudoku
void sudoku_print(Sudoku* s)
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

			/*
			// Affichage des valeurs possibles pour chaque case
			int k;
			printf("(");
			for (k = 0; k < s->value_range; k++)
				if (cell_can_have_value(sudoku_get_cell(s, i, j), k + 1))
					printf("%d ", k + 1);
				else
					printf(". ", k + 1);
			printf(")   ");
			*/
		}
		printf("\n");
	}
}

// Fonction interne mettant à jour les cases selon un *getter* donné.
// Cf sudoku_update_{row|column|block}
void sudoku_update_INTERNAL(Sudoku* s, Cell* (*getter)(Sudoku* _s, int _group_index, int _cell_index), int index)
{
	int i, j;

	// Si une valeur est "écrite" dans une case, alors on peut retirer cette valeur des valeurs possibles dans les autres cases du groupe
	for (i = 0; i < s->value_range; i++)
	{
		int v = cell_get_unique_value(getter(s, index, i));

		if (v != 0)
		{
			for (j = 0; j < s->value_range; j++)
			{
				if (i != j)
					cell_unset_value(getter(s, index, j), v);
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
			cell_set_value(unique, v);
	}
}

// Met à jour les valeurs possibles pour une ligne donnée
inline void sudoku_update_row(Sudoku* s, int index)
{
	sudoku_update_INTERNAL(s, &sudoku_get_row_cell, index);
}

// Met à jour les valeurs possibles pour une colonne donnée
inline void sudoku_update_column(Sudoku* s, int index)
{
	sudoku_update_INTERNAL(s, &sudoku_get_column_cell, index);
}

// Met à jour les valeurs possibles pour un bloc donné
inline void sudoku_update_block(Sudoku* s, int index)
{
	sudoku_update_INTERNAL(s, &sudoku_get_block_cell, index);
}

// MMMMAAAAAAAAIIIINNNN
int main(int argc, char* argv[])
{
	int i;
	Sudoku* s;

	s = sudoku_create(stdin);
	if (s == NULL) { fprintf(stderr, "Sudoku initialization failed\n"); return EXIT_FAILURE; }

	sudoku_print(s);

	while (1)
	{
		for (i = 0; i < s->value_range; i++)
		{
			sudoku_update_row(s, i);
			sudoku_update_column(s, i);
			sudoku_update_block(s, i);
		}
		sudoku_print(s);
	}

	sudoku_free(s);

	return EXIT_SUCCESS;
}
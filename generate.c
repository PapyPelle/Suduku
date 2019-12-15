// **************************
// programme de F. Lemieux
// usage : ./exe 3 > grille.txt
// **************************

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void permute(int*, int*, int, int, int); //Permute les lignes et colonnes d'une matrice
void injecte(int*, int*, int, int, int); //Insère une matrice  nXn dans une matrice n^2 X n^2
void print(int*, int); //Affiche une matrice n X n

// Constrution d'un Sudoku
// Le Sudoku M est vu comme une grille nXn constituée de blocs S de dimention nXn
// Chaque bloc contient des entiers entre 0 et n^2 sans répétition des entiers >0
// L'entier 0 représente une case vide
// Chaque bloc S est construit en permutant adéquatement les lignes et colonnes 
// d'un bloc T fixe généré aléatoirement.
int main(int argc, char* argv[]) {
	int i, j;
	int n; // Le sudoku sera de dimension n^2 X n^2
	int* T; //matrice nXn contenant les valeurs 1 à n^2
	int* S; //matrice nXn contenant une permutation des lignes et colnnes de T
	int* M; //matrice n^2 X n^2 : Sudoku
	time_t secondes = time(NULL);
	srand(secondes);

	if (argc < 2) return EXIT_FAILURE;
	n = atoi(argv[1]);
	printf("%d\n", n);

	T = (int*)malloc(n * n * sizeof(int));
	S = (int*)malloc(n * n * sizeof(int));
	M = (int*)malloc(n * n * n * n * sizeof(int));

	// Initialisation aléatoire de la matrice T
	// Cette matrice n'est jamais modifiée par a suite
	for (i = 0; i < n * n; i++) T[i] = i + 1;
	for (i = 0; i < n * n; i++) {
		j = rand() % (n * n);
		int x = T[j];
		T[j] = T[i];
		T[i] = x;
	}

	// On construit d'abord un Sudoku sans case libre
	for (i = 0; i < n; i++) {
		for (j = 0; j < n; j++) {
			permute(T, S, n, i, j);
			injecte(S, M, n, i, j);
		}
	}

	// On remplace aléatoirement certains des éléments du Sudoku 
	// par 0 dans une proportion de 1/2
	for (i = 0; i < n * n; i++)
		for (j = 0; j < n * n; j++)
			if (rand() % 2) M[i * n * n + j] = 0;
	print(M, n * n);

	return 0; // fin du main
}

// Cette fonction permute les lignes et colonnes de T et place le résultat dans S.
// T demeure inchangé
void permute(int* T, int* S, int n, int a, int b) {
	int i, j;
	for (i = 0; i < n; i++)
		for (j = 0; j < n; j++)
			S[i * n + j] = T[((i + b) % n) * n + (j + a) % n];
}

// Affiche la matrice S
void print(int* S, int n) {
	int i, j;
	for (i = 0; i < n; i++) {
		for (j = 0; j < n; j++)
			printf("%d\t", S[i * n + j]);
		printf("\n");
	}
}

// Le Sudoku M est vu comme une grille nXn dont chaque élément 
// est un bloc S de dimension nXn 
// La fonction suivante insère la matrice S à la position (a,b) du Sudoku M
void injecte(int* S, int* M, int n, int a, int b) {
	int i, j;
	for (i = 0; i < n; i++)
		for (j = 0; j < n; j++) {
			M[(n * n * n * a + n * n * i) + n * b + j] = S[i * n + j];
		}
}
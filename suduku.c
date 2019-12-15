
static const little_size = 3;

struct Case {
    char value[little_size + 1];
};

// Renvoie 1 si la case possède un chiffre unique, 0 sinon
int uniqueValue(const struct Case *c) {
    if (c->value[0] == 1) {
        return 1;
    }
    int count = 0;
    int i;
    for(i=1; i < little_size * little_size + 1; i++) {
        if (c->value[i]) {
            count ++;
            if (count > 1) {
                return 0;
            }
        }
    }
    return 1;
}

// Renvoie la première valeur trouvée
int returnFirstUnique(const struct Case *c) {
    int i;
    for(i=1; i < little_size * little_size + 1; i++) {
        if (c->value[i]) {
            return i;
        }
    }
}

// Renvoie la valeure unique d'une case, 0 sinon
int returnValue(const struct Case *c) {
    int count = 0;
    int ret_val = 0;
    int i;
    for(i=1; i < little_size * little_size + 1; i++) {
        if (c->value[i]) {
            ret_val = i;
            count ++;
            if (count > 1) {
                return 0;
            }
        }
    }
    if (count) {
        return ret_val;
    }
    return 0; 
}

// Indique que la case peut contenir la valeur n
void setValue(struct Case *c, int n) {
    c->value[n] = 1;
}

// Crée une case qui peut contenir toutes les valeurs
void setEmptyCase(int c, int l, struct Case **s) {
    s[c][l].value[0] = 0;
    int i;
    for(i=1; i < little_size * little_size + 1; i++) {
        s[c][l].value[i] = 1;
    }
}

// Crée une case occupée par une valeur
void setFilledCase(int c, int l, int val, struct Case **s) {
    int i;
    for(i=0; i < little_size * little_size + 1; i++) {
        s[c][l].value[i] = 0;
    }
    s[c][l].value[val] = 1;
}

// Parcours une ligne pour éliminer les possibilités existentes
void checkLine(struct Case **s, int l) {
    // Création d'une liste des valeurs unique
    struct Case known;
    int i;
    for(i=1; i < little_size * little_size + 1; i++) {
        known.value[i] = 0;
    }
    int unique[little_size*little_size];
    // Récupération des valeurs uniques
    int c;
    for(c=0; c < little_size*little_size; c++) {
        int v = returnValue(&s[c][l]);
        if (v) {
            unique[c] = 1;
            known.value[v] = 1;
        }
        else {
            unique[c] = 0;
        }
    }
    // Retire les valeurs connues des autres cases
    for(c=0; c < little_size*little_size; c++) {
        if (!unique[c])
        for(i=1; i < little_size*little_size + 1; i++) {
            if (known.value[i]) {
                s[c][l].value[i] = 0;
            }
        }
    }
}





// MMMMAAAAAAAAIIIINNNN
int main(int argc, char *argv[]) {
    struct Case s[little_size*little_size][little_size*little_size];

    return 0;
}
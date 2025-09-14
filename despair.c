#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>


#define MAX_WORDS 16000
#define MAX_WORD_LEN 100

typedef struct {
    char *text;
    int *next;
    int next_len;
    int next_cap;
} Node;

Node nodes[MAX_WORDS];
int node_count = 0;

int find_or_add(const char *w) {
    for (int i = 0; i < node_count; ++i) if (strcmp(nodes[i].text, w) == 0) return i;
    if (node_count >= MAX_WORDS) return -1;
    nodes[node_count].text = strdup(w);
    nodes[node_count].next = NULL;
    nodes[node_count].next_len = 0;
    nodes[node_count].next_cap = 0;
    return node_count++;
}

void add_transition(int a, int b) {
    if (a < 0 || b < 0) return;
    Node *n = &nodes[a];
    if (n->next_len + 1 > n->next_cap) {
        int newcap = n->next_cap ? n->next_cap * 2 : 4;
        n->next = realloc(n->next, newcap * sizeof(int));
        n->next_cap = newcap;
    }
    n->next[n->next_len++] = b;
}

void lower_and_tokenize(const char *s, char tokens[][MAX_WORD_LEN], int *tcount) {
    int i = 0, j = 0;
    char cur[MAX_WORD_LEN] = {0};
    *tcount = 0;
    while (s[i]) {
        if (isalpha((unsigned char)s[i]) || s[i]=='\'' || s[i]=='-') {
            if (j < MAX_WORD_LEN-1) cur[j++] = tolower((unsigned char)s[i]);
        } else {
            if (j>0) {
                cur[j] = 0;
                strncpy(tokens[*tcount], cur, MAX_WORD_LEN);
                (*tcount)++;
                j = 0;
                cur[0]=0;
            }
        }
        i++;
    }
    if (j>0) {
        cur[j]=0;
        strncpy(tokens[*tcount], cur, MAX_WORD_LEN);
        (*tcount)++;
    }
}

void train(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) { fprintf(stderr, "Cannot open %s , probably because it does not yet exist\n", filename); exit(1); }
    char line[1024];
    char tokens[512][MAX_WORD_LEN];
    while (fgets(line, sizeof(line), f)) {
        int tc;
        lower_and_tokenize(line, tokens, &tc);
        if (tc == 0) continue;
        int prev = find_or_add(tokens[0]);
        for (int i = 1; i < tc; ++i) {
            int cur = find_or_add(tokens[i]);
            add_transition(prev, cur);
            prev = cur;
        }
    }
    fclose(f);
}

int pick_random_from_node(int idx) {
    Node *n = &nodes[idx];
    if (n->next_len == 0) return rand() % node_count;
    int r = rand() % n->next_len;
    return n->next[r];
}

void gen_line_words(int words) {
    if (node_count == 0) { printf("No data. Add text to corpus.txt\n"); return; }
    int cur = rand() % node_count;
    for (int i = 0; i < words; ++i) {
        printf("%s", nodes[cur].text);
        if (i+1 < words) printf(" ");
        cur = pick_random_from_node(cur);
    }
    printf("\n");
}

int main(int argc, char **argv) {
    srand((unsigned)time(NULL) ^ (unsigned)getpid());
    int words = 20;
    int lines = 1;
    if (argc >= 2) words = atoi(argv[1]) > 0 ? atoi(argv[1]) : words;
    if (argc >= 3) lines = atoi(argv[2]) > 0 ? atoi(argv[2]) : lines;
    train("corpus.txt");
    for (int i = 0; i < lines; ++i) gen_line_words(words);
    return 0;
}

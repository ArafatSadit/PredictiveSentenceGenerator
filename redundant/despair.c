#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>

#define MAX_WORDS 20000
#define MAX_WORD_LEN 100
#define MAX_SENTENCE_LENGTH 200
#define MAX_LINE_LENGTH 1024
#define CONTEXT_WINDOW 3

typedef struct {
    char *text;
    int *next;
    int *next_counts;
    int next_len;
    int next_cap;
    bool can_end_sentence;
    bool can_start_sentence;
    int total_occurrences;
} Node;

Node nodes[MAX_WORDS];
int node_count = 0;

// Function prototypes
int find_or_add(const char *w);
void add_transition(int a, int b);
void lower_and_tokenize(const char *s, char tokens[][MAX_WORD_LEN], int *tcount, bool *ends_with_period);
void train(const char *filename);
int pick_next_word(int current, int recent_words[], int recent_count);
void generate_sentence();
void free_memory();
bool is_sentence_ender(const char *word);
bool is_sentence_starter(const char *word);
void capitalize_word(char *word);

int main(int argc, char **argv) {
    srand((unsigned)time(NULL) ^ (unsigned)getpid());
    int sentences = 5;
    
    if (argc >= 2) {
        sentences = atoi(argv[1]);
        if (sentences <= 0) sentences = 5;
    }
    
    printf("Training model on corpus.txt...\n");
    train("corpus.txt");
    
    printf("\nGenerated text:\n");
    printf("====================\n");
    
    for (int i = 0; i < sentences; ++i) {
        generate_sentence();
    }
    
    free_memory();
    return 0;
}

int find_or_add(const char *w) {
    for (int i = 0; i < node_count; ++i) {
        if (strcmp(nodes[i].text, w) == 0) {
            nodes[i].total_occurrences++;
            return i;
        }
    }
    
    if (node_count >= MAX_WORDS) return -1;
    
    nodes[node_count].text = strdup(w);
    nodes[node_count].next = NULL;
    nodes[node_count].next_counts = NULL;
    nodes[node_count].next_len = 0;
    nodes[node_count].next_cap = 0;
    nodes[node_count].can_end_sentence = is_sentence_ender(w);
    nodes[node_count].can_start_sentence = is_sentence_starter(w);
    nodes[node_count].total_occurrences = 1;
    
    return node_count++;
}

void add_transition(int a, int b) {
    if (a < 0 || b < 0) return;
    
    Node *n = &nodes[a];
    
    // Check if transition already exists
    for (int i = 0; i < n->next_len; i++) {
        if (n->next[i] == b) {
            n->next_counts[i]++;
            return;
        }
    }
    
    // Add new transition
    if (n->next_len + 1 > n->next_cap) {
        int newcap = n->next_cap ? n->next_cap * 2 : 4;
        int *new_next = realloc(n->next, newcap * sizeof(int));
        int *new_counts = realloc(n->next_counts, newcap * sizeof(int));
        
        if (!new_next || !new_counts) {
            fprintf(stderr, "Memory allocation failed\n");
            return;
        }
        
        n->next = new_next;
        n->next_counts = new_counts;
        n->next_cap = newcap;
    }
    
    n->next[n->next_len] = b;
    n->next_counts[n->next_len] = 1;
    n->next_len++;
}

void lower_and_tokenize(const char *s, char tokens[][MAX_WORD_LEN], int *tcount, bool *ends_with_period) {
    int i = 0, j = 0;
    char cur[MAX_WORD_LEN] = {0};
    *tcount = 0;
    *ends_with_period = false;
    bool in_word = false;
    
    while (s[i]) {
        if (isalpha((unsigned char)s[i]) || s[i] == '\'' || s[i] == '-') {
            if (j < MAX_WORD_LEN - 1) {
                cur[j++] = tolower((unsigned char)s[i]);
            }
            in_word = true;
        } else {
            if (in_word) {
                cur[j] = 0;
                strncpy(tokens[*tcount], cur, MAX_WORD_LEN);
                (*tcount)++;
                j = 0;
                cur[0] = 0;
                in_word = false;
            }
            
            // Check if this non-alpha character is a sentence terminator
            if (s[i] == '.' || s[i] == '!' || s[i] == '?') {
                *ends_with_period = true;
                
                // Add the punctuation as a token
                if (*tcount < 512) {
                    tokens[*tcount][0] = s[i];
                    tokens[*tcount][1] = 0;
                    (*tcount)++;
                }
            }
        }
        i++;
    }
    
    if (in_word) {
        cur[j] = 0;
        strncpy(tokens[*tcount], cur, MAX_WORD_LEN);
        (*tcount)++;
    }
}

bool is_sentence_ender(const char *word) {
    if (strlen(word) == 1) {
        return word[0] == '.' || word[0] == '!' || word[0] == '?';
    }
    return false;
}

bool is_sentence_starter(const char *word) {
    // Words that often start sentences based on your training data
    const char *common_starters[] = {
        "i", "you", "we", "the", "it", "this", "that", "what", "why", "how",
        "if", "when", "where", "who", "my", "your", "our", "their", "one",
        "sometimes", "every", "each", "no", "any", "some", "many", "most",
        "first", "last", "next", "then", "now", "here", "there", "always",
        "never", "often", "sometimes", "usually", "maybe", "perhaps", "probably"
    };
    
    int num_starters = sizeof(common_starters) / sizeof(common_starters[0]);
    for (int i = 0; i < num_starters; i++) {
        if (strcmp(word, common_starters[i]) == 0) {
            return true;
        }
    }
    
    return false;
}

void train(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) { 
        fprintf(stderr, "Cannot open %s\n", filename); 
        exit(1); 
    }
    
    char line[MAX_LINE_LENGTH];
    char tokens[512][MAX_WORD_LEN];
    int line_num = 0;
    
    while (fgets(line, sizeof(line), f)) {
        line_num++;
        int tc;
        bool ends_with_period;
        
        lower_and_tokenize(line, tokens, &tc, &ends_with_period);
        if (tc == 0) continue;
        
        // Mark the first word as a potential sentence starter
        int first_word_idx = find_or_add(tokens[0]);
        if (first_word_idx >= 0) {
            nodes[first_word_idx].can_start_sentence = true;
        }
        
        // Mark the last word as a potential sentence ender if the line ended with punctuation
        if (ends_with_period && tc > 0) {
            int last_word_idx = find_or_add(tokens[tc-1]);
            if (last_word_idx >= 0) {
                nodes[last_word_idx].can_end_sentence = true;
            }
        }
        
        // Create transitions between words
        for (int i = 0; i < tc - 1; ++i) {
            int current = find_or_add(tokens[i]);
            int next = find_or_add(tokens[i+1]);
            add_transition(current, next);
        }
    }
    
    fclose(f);
    printf("Processed %d lines, learned %d words\n", line_num, node_count);
}

int pick_next_word(int current, int recent_words[], int recent_count) {
    Node *n = &nodes[current];
    
    if (n->next_len == 0) {
        // If no transitions, pick a random word that can start sentences
        int attempts = 0;
        while (attempts < 20) {
            int random_idx = rand() % node_count;
            if (nodes[random_idx].can_start_sentence) {
                return random_idx;
            }
            attempts++;
        }
        return rand() % node_count; // Fallback
    }
    
    // Calculate total weight for all possible next words
    int total_weight = 0;
    for (int i = 0; i < n->next_len; i++) {
        total_weight += n->next_counts[i];
    }
    
    // Pick a random weighted choice
    int r = rand() % total_weight;
    int cumulative = 0;
    
    for (int i = 0; i < n->next_len; i++) {
        cumulative += n->next_counts[i];
        if (r < cumulative) {
            return n->next[i];
        }
    }
    
    return n->next[0]; // Fallback
}

void capitalize_word(char *word) {
    if (word[0] >= 'a' && word[0] <= 'z') {
        word[0] = toupper(word[0]);
    }
}

void generate_sentence() {
    if (node_count == 0) { 
        printf("No data. Add text to corpus.txt\n"); 
        return; 
    }
    
    // Start with a word that can begin sentences
    int start_index = -1;
    int attempts = 0;
    while (start_index < 0 && attempts < 50) {
        int candidate = rand() % node_count;
        if (nodes[candidate].can_start_sentence) {
            start_index = candidate;
            break;
        }
        attempts++;
    }
    
    // Fallback if no suitable start word found
    if (start_index < 0) start_index = rand() % node_count;
    
    int cur = start_index;
    int word_count = 0;
    char sentence[MAX_SENTENCE_LENGTH * MAX_WORD_LEN] = {0};
    char temp[MAX_WORD_LEN];
    int recent_words[CONTEXT_WINDOW] = {-1, -1, -1};
    int recent_index = 0;
    
    // Capitalize first word
    strcpy(temp, nodes[cur].text);
    capitalize_word(temp);
    strcat(sentence, temp);
    word_count++;
    recent_words[recent_index] = cur;
    recent_index = (recent_index + 1) % CONTEXT_WINDOW;
    
    // Continue the sentence
    while (word_count < 25) { // Limit sentence length
        int next_word = pick_next_word(cur, recent_words, CONTEXT_WINDOW);
        
        // Check if we should end the sentence
        bool should_end = false;
        if (word_count > 5) {
            if (nodes[next_word].can_end_sentence && (rand() % 100 < 30)) {
                should_end = true;
            } else if (word_count > 15 && (rand() % 100 < 20)) {
                should_end = true;
            }
        }
        
        strcat(sentence, " ");
        strcat(sentence, nodes[next_word].text);
        word_count++;
        
        // Update recent words context
        recent_words[recent_index] = next_word;
        recent_index = (recent_index + 1) % CONTEXT_WINDOW;
        cur = next_word;
        
        if (should_end) {
            // Add a period if the last character isn't punctuation
            int len = strlen(sentence);
            if (len > 0 && sentence[len-1] != '.' && 
                sentence[len-1] != '!' && sentence[len-1] != '?') {
                strcat(sentence, ".");
            }
            break;
        }
        
        // Force end if we've reached max words
        if (word_count >= 25) {
            int len = strlen(sentence);
            if (len > 0 && sentence[len-1] != '.' && 
                sentence[len-1] != '!' && sentence[len-1] != '?') {
                strcat(sentence, ".");
            }
        }
    }
    
    printf("%s\n", sentence);
}

void free_memory() {
    for (int i = 0; i < node_count; i++) {
        free(nodes[i].text);
        free(nodes[i].next);
        free(nodes[i].next_counts);
    }
}
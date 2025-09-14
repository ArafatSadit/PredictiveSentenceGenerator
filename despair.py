import random
import re
from collections import defaultdict, Counter
import os

class MarkovChain:
    def __init__(self, n=2):
        self.n = n
        self.model = defaultdict(Counter)
        self.starters = set()
        self.enders = set()

    def train(self, filename):
        if not os.path.exists(filename):
            raise FileNotFoundError(f"Error: File '{filename}' not found.")

        with open(filename, 'r', encoding='utf-8') as f:
            text = f.read()

        sentences = re.split(r'[.!?]+', text)
        for sentence in sentences:
            tokens = self._tokenize(sentence)
            if len(tokens) < self.n:
                continue
            if tokens[0]:
                self.starters.add(tokens[0])
            if tokens[-1]:
                self.enders.add(tokens[-1])
            for i in range(len(tokens) - self.n):
                context = tuple(tokens[i:i + self.n])
                next_word = tokens[i + self.n]
                self.model[context][next_word] += 1

    def _tokenize(self, text):
        words = re.findall(r"\b[\w'-]+\b", text.lower())
        return [word for word in words if word]

    def generate_sentence(self, max_length=15):
        if not self.model:
            return "No data available. Train the model first."

        valid_starters = [c for c in self.model.keys() if c[0] in self.starters]
        if not valid_starters:
            context = random.choice(list(self.model.keys()))
        else:
            context = random.choice(valid_starters)

        sentence = list(context)
        while len(sentence) < max_length:
            next_words = self.model.get(context)
            if not next_words:
                break
            words, weights = zip(*next_words.items())
            next_word = random.choices(words, weights=weights, k=1)[0]
            sentence.append(next_word)
            context = tuple(sentence[-self.n:])
            if next_word in self.enders and len(sentence) >= 5:
                if random.random() < 0.7:
                    break

        if sentence:
            sentence[0] = sentence[0].capitalize()
            if sentence[-1][-1] not in '.!?':
                sentence[-1] += '.'
            return ' '.join(sentence)
        return "Could not generate sentence."

    def generate_text(self, num_sentences=5):
        return ' '.join(self.generate_sentence() for _ in range(num_sentences))

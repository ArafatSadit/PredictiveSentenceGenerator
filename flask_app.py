from flask import Flask, render_template_string
from despair import MarkovChain

app = Flask(__name__)

# Initialize and train your Markov model once
mc = MarkovChain(n=2)
mc.train('/home/arafatsadit/app0/corpus.txt')  # make sure the file exists

@app.route('/')
def home():
    text = mc.generate_text(5)
    # Simple HTML template to display
    html = f"""
    <html>
        <head><title>Despair Generator</title></head>
        <body>
            <h1>Generated Text:</h1>
            <p>{text}</p>
        </body>
    </html>
    """
    return render_template_string(html)

if __name__ == '__main__':
    app.run(debug=True)

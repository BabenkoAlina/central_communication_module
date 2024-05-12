from flask import Flask, jsonify, render_template
import json

app = Flask(__name__)

def load_data():
    with open('../data/data.json', 'r') as file:
        data = json.load(file)
    return data

@app.route('/')
def index():
    data = load_data()
    return render_template('index.html', data=data)

if __name__ == '__main__':
    app.run(debug=True, port=3001)

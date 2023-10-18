from flask import Flask, render_template, request
import chess
#import chess.svg

app = Flask(__name__)
board = chess.Board()

@app.route("/")
def index():
    return render_template("index.html", board=board)

@app.route("/move", methods=["POST"])
def move():
    move_str = request.form["move"]
    move = chess.Move.from_uci(move_str)
    if move in board.legal_moves:
        board.push(move)
    return render_template("index.html", board=board)

if __name__ == "__main__":
    app.run()

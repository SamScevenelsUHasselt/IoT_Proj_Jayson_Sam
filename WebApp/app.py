import eventlet
from flask import Flask, render_template
from flask_mqtt import Mqtt
from flask_socketio import SocketIO
import chess
import chess.svg

eventlet.monkey_patch()

app = Flask(__name__)
app.config['MQTT_BROKER_URL'] = 'broker.hivemq.com'  # use the free broker from HIVEMQ
app.config['MQTT_BROKER_PORT'] = 1883  # default port for non-tls connection
app.config['MQTT_USERNAME'] = ''  # set the username here if you need authentication for the broker
app.config['MQTT_PASSWORD'] = ''  # set the password here if the broker demands authentication
app.config['MQTT_KEEPALIVE'] = 5  # set the time interval for sending a ping to the broker to 5 seconds
app.config['MQTT_TLS_ENABLED'] = False  # set TLS to disabled for testing purposes

mqtt = Mqtt(app)
socketio = SocketIO(app)


@mqtt.on_connect()
def handle_connect(client, userdata, flags, rc):
    mqtt.subscribe('chess-sj')


@mqtt.on_message()
def handle_mqtt_message(client, userdata, message):
    data = dict(
        topic=message.topic,
        payload=message.payload.decode()
    )
    try:
        board = chess.Board(data['payload'])
        socketio.emit('mqtt_message', data=chess.svg.board(board))
    except ValueError:
        print("not valid FEN code")


@socketio.on('connect')
def test_connect(auth):
    board = chess.Board()
    socketio.emit('mqtt_message', data=chess.svg.board(board))
    print("SOCKETIO Connected")


@app.route('/')
def hello_world():  # put application's code here
    return render_template('index.html')


socketio.run(app, host='localhost', port=5000, use_reloader=True, debug=True, allow_unsafe_werkzeug=True)
import pgzrun
import random
import serial

# --- GAME CONSTANTS ---
TILE_SIZE = 28
DIRS = {'UP': (0, -1), 'DOWN': (0, 1), 'LEFT': (-1, 0), 'RIGHT': (1, 0), 'NONE': (0, 0)}

# We keep this as a pure blueprint so we can reload it on restart
LEVEL_BLUEPRINT = [
    [1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1],
    [1,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,1],
    [1,0,1,1,1,0,1,1,1,0,1,0,1,1,1,0,1,1,1,0,1],
    [1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1],
    [1,0,1,1,1,0,1,0,1,1,1,1,1,0,1,0,1,1,1,0,1],
    [1,0,0,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,0,0,1],
    [1,1,1,1,1,0,1,1,1,2,1,2,1,1,1,0,1,1,1,1,1],
    [2,2,2,2,1,0,1,2,2,2,2,2,2,2,1,0,1,2,2,2,2],
    [1,1,1,1,1,0,1,2,1,1,3,1,1,2,1,0,1,1,1,1,1],
    [2,2,2,2,2,0,2,2,1,3,3,3,1,2,2,0,2,2,2,2,2],
    [1,1,1,1,1,0,1,2,1,1,1,1,1,2,1,0,1,1,1,1,1],
    [2,2,2,2,1,0,1,2,2,2,2,2,2,2,1,0,1,2,2,2,2],
    [1,1,1,1,1,0,1,2,1,1,1,1,1,2,1,0,1,1,1,1,1],
    [1,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,1],
    [1,0,1,1,1,0,1,1,1,0,1,0,1,1,1,0,1,1,1,0,1],
    [1,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,1],
    [1,1,1,0,1,0,1,0,1,1,1,1,1,0,1,0,1,0,1,1,1],
    [1,0,0,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,0,0,1],
    [1,0,1,1,1,1,1,1,1,0,1,0,1,1,1,1,1,1,1,0,1],
    [1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1],
    [1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1]
]

WIDTH = len(LEVEL_BLUEPRINT[0]) * TILE_SIZE
HEIGHT = len(LEVEL_BLUEPRINT) * TILE_SIZE

# --- SERIAL SETUP ---
PORT = 'COM3' # <-- Make sure this is your Pico's actual COM port!
BAUD_RATE = 115200

try:
    ser = serial.Serial(PORT, BAUD_RATE, timeout=0.01)
    print(f"Connected to Pico on {PORT}")
except Exception as e:
    print(f"Could not open port {PORT}. Game will run without hardware.")
    ser = None

# --- GAME STATE VARIABLES ---
# We declare these global so all functions can see them
level_data = []
player = {}
ghosts = []
score = 0
frames = 0
game_over = False

def reset_game():
    """Restores the game to its initial state."""
    global level_data, player, ghosts, score, frames, game_over
    
    # Create a fresh copy of the map so all dots return
    level_data = [row[:] for row in LEVEL_BLUEPRINT]
    
    score = 0
    frames = 0
    game_over = False
    
    player = {'c': 10, 'r': 15, 'dir': 'NONE', 'next_dir': 'NONE'}
    
    ghosts = [
        {'color': 'red',    'c': 10, 'r': 7, 'dir': 'LEFT', 'mode': 'ACTIVE', 'delay': 0,   'chase': 0.8},
        {'color': 'pink',   'c': 10, 'r': 9, 'dir': 'UP',   'mode': 'HOUSE',  'delay': 180, 'chase': 0.6},
        {'color': 'cyan',   'c': 9,  'r': 9, 'dir': 'UP',   'mode': 'HOUSE',  'delay': 360, 'chase': 0.4},
        {'color': 'orange', 'c': 11, 'r': 9, 'dir': 'UP',   'mode': 'HOUSE',  'delay': 540, 'chase': 0.2}
    ]

def draw():
    screen.clear()
    
    for r in range(len(level_data)):
        for c in range(len(level_data[r])):
            x, y = c * TILE_SIZE, r * TILE_SIZE
            if level_data[r][c] == 1:
                screen.draw.filled_rect(Rect((x+1, y+1), (TILE_SIZE-2, TILE_SIZE-2)), "blue")
            elif level_data[r][c] == 0:
                screen.draw.filled_circle((x + TILE_SIZE//2, y + TILE_SIZE//2), 3, "yellow")
            elif level_data[r][c] == 3:
                screen.draw.filled_rect(Rect((x+1, y+1), (TILE_SIZE-2, TILE_SIZE-2)), (20, 20, 50))
                
    px = player['c'] * TILE_SIZE + TILE_SIZE // 2
    py = player['r'] * TILE_SIZE + TILE_SIZE // 2
    screen.draw.filled_circle((px, py), TILE_SIZE // 2.2, "yellow")
    
    for g in ghosts:
        gx = g['c'] * TILE_SIZE + TILE_SIZE // 2
        gy = g['r'] * TILE_SIZE + TILE_SIZE // 2
        screen.draw.filled_circle((gx, gy), TILE_SIZE // 2.2, g['color'])
        
    screen.draw.text(f"Score: {score}", (10, 10), fontsize=30, color="white")
    
    if game_over:
        screen.draw.filled_rect(Rect((0, HEIGHT//2 - 60), (WIDTH, 120)), "black")
        screen.draw.text("GAME OVER", center=(WIDTH//2, HEIGHT//2 - 15), fontsize=70, color="red")
        screen.draw.text("Press SPACE to Restart", center=(WIDTH//2, HEIGHT//2 + 35), fontsize=30, color="white")

def on_key_down(key):
    # Handle the Restart mechanic
    if game_over:
        if key == keys.SPACE:
            reset_game()
        return # Block movement keys if dead

    if key == keys.UP: player['next_dir'] = 'UP'
    if key == keys.DOWN: player['next_dir'] = 'DOWN'
    if key == keys.LEFT: player['next_dir'] = 'LEFT'
    if key == keys.RIGHT: player['next_dir'] = 'RIGHT'

def update():
    global frames, score, game_over
    if game_over: return
    
    frames += 1

    # === HARDWARE CONTROLLER LOGIC ===
    if ser and ser.in_waiting > 0:
        try:
            # Read the incoming line, clean it up, and split by commas
            # Your Pico sends: UP, DOWN, LEFT, RIGHT
            line = ser.readline().decode('utf-8').strip()
            if line:
                u, d, l, r = map(int, line.split(','))
                
                # Update the player's next direction based on the hardware
                if u == 1: player['next_dir'] = 'UP'
                elif d == 1: player['next_dir'] = 'DOWN'
                elif l == 1: player['next_dir'] = 'LEFT'
                elif r == 1: player['next_dir'] = 'RIGHT'
        except Exception:
            pass # Ignore any garbled data from the stream
    
    # === PLAYER MOVEMENT: Fast Update (Every 4 frames) ===
    if frames % 4 == 0:
        nr = player['r'] + DIRS[player['next_dir']][1]
        nc = player['c'] + DIRS[player['next_dir']][0]
        
        if nc < 0: nc = len(level_data[0]) - 1
        elif nc >= len(level_data[0]): nc = 0
            
        if level_data[nr][nc] not in [1, 3]:
            player['dir'] = player['next_dir'] 
            
        nr = player['r'] + DIRS[player['dir']][1]
        nc = player['c'] + DIRS[player['dir']][0]
        
        if nc < 0: nc = len(level_data[0]) - 1
        elif nc >= len(level_data[0]): nc = 0
            
        if level_data[nr][nc] not in [1, 3]:
            player['r'], player['c'] = nr, nc
            
            if level_data[nr][nc] == 0:
                level_data[nr][nc] = 2
                score += 10

    # === GHOST MOVEMENT: Slow Update (Every 16 frames) ===
    if frames % 16 == 0:
        for g in ghosts:
            if g['mode'] == 'HOUSE':
                if frames > g['delay']:
                    g['c'], g['r'] = 10, 7
                    g['mode'] = 'ACTIVE'
                continue
                
            valid_moves = []
            for d_name, d_vec in DIRS.items():
                if d_name == 'NONE': continue
                gr, gc = g['r'] + d_vec[1], g['c'] + d_vec[0]
                
                if gc >= 0 and gc < len(level_data[0]) and level_data[gr][gc] != 1:
                    opp = {'UP':'DOWN', 'DOWN':'UP', 'LEFT':'RIGHT', 'RIGHT':'LEFT'}
                    if d_name != opp.get(g['dir']) or len(valid_moves) == 0:
                        valid_moves.append((d_name, gr, gc))
            
            if valid_moves:
                if random.random() < g['chase']:
                    best_move = None
                    min_dist = 9999
                    for m in valid_moves:
                        dist = abs(m[1] - player['r']) + abs(m[2] - player['c'])
                        if dist < min_dist:
                            min_dist = dist
                            best_move = m
                    g['dir'], g['r'], g['c'] = best_move
                else:
                    g['dir'], g['r'], g['c'] = random.choice(valid_moves)

    # === COLLISION DETECTION ===
    for g in ghosts:
        if g['mode'] == 'ACTIVE' and g['r'] == player['r'] and g['c'] == player['c']:
            game_over = True

# Initialize the game variables for the very first run
reset_game()

if __name__ == "__main__":
    pgzrun.go()
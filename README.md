The Asymmetry of Memory: Why i++ Always Wins
(ここに、赤が画面を侵食しているスクリーンショットを貼ってください / Place your screenshot here)
<img width="912" height="944" alt="correct" src="https://github.com/user-attachments/assets/f7aa5e9d-c9a9-4b6b-b2e7-4a5378527654" />

🔍 Overview
"In a theoretically symmetrical universe, does the order of memory processing determine the fate of civilizations?"

This project is a C-based simulation designed to test the physical bias of the for loop. We simulated a war between two factions:
🔴 Red Team: Moves Right (Positive direction)
🔵 Blue Team: Moves Left (Negative direction)
Logically, their strength is 50/50. However, in a sequential processing environment (Single Thread / Single Buffer), Red always exterminates Blue.

This is not a bug. It is the visual proof of "First-Come-First-Served" bias inherent in computer memory architecture.

🧪 The Hypothesis
If we have a 1D array representing a universe, and we iterate through it from index 0 to N:

C
for (int i = 0; i < N; i++) {
    update_particle(i);
}
The particles at the beginning of the array (lower indices) get to act before the particles at the end. If a Red particle (at i) and a Blue particle (at i+2) both try to move into the empty spot at i+1:
The CPU processes i first. Red claims the spot.
The CPU processes i+2 later. The spot is taken. Blue dies (collision).
Result: The physics of the for loop grants "Red" an absolute strategic advantage.

🚀 Experiments & Versions
V7.6: The Mirror Universe (Control Group)
Method: Double Buffering (Synchronous Update).
Result: Perfect Stalemate.
Since all moves are calculated before the state is updated, memory order does not matter. The population remains exactly 50/50. This proved the algorithm itself is fair.
V8.0: The Microscope
Method: Nano-step visualization.
Result: Visualized the CPU scanline. We observed the exact moment Blue particles vanished immediately after the "White Scanline" passed them.
V9.0: The Final Verdict (Current)
Method: Single Buffering (Sequential Update) + Safe Rendering.
Result: Red Domination.
By removing the double-buffer safety net, we exposed the raw physical bias of the hardware. The Red team inevitably consumes the entire universe.
🛠️ How to Run
Requirements
C Compiler (GCC or Clang)
SDL2 Library
Mac (macOS)
Bash
# 1. Install SDL2 via Homebrew
brew install sdl2

# 2. Compile
clang -framework SDL2 -O3 -o asymmetry main.c

# 3. Run
./asymmetry
Linux (Ubuntu/Debian)
Bash
sudo apt-get install libsdl2-dev
gcc -o asymmetry main.c -lSDL2
./asymmetry
🎮 Controls
SPACE: Pause / Resume
R: Reset Simulation
ESC: Quit
📝 Author
Matsui
Concept & Logic Implementation
Discovery of the "Red Domination" Phenomenon

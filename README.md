# 📝 NoteBox — Shared Memory-Based Collaborative Notepad

**NoteBox** is a multi-user, terminal-based collaborative notepad built in C. It allows multiple processes (users) to concurrently add, view, edit, and delete shared notes in real time, using POSIX shared memory and inter-process synchronization. The interface is built with `ncurses`, providing a responsive, colored, and user-friendly UI.

---

## 🚀 Features

- 🧠 **Shared Memory Communication** — All user processes share notes through a single shared memory segment.
- 🔐 **Mutex Synchronization** — Access is safely synchronized using `pthread_mutex_t` with `PTHREAD_PROCESS_SHARED`.
- 🖥️ **UI with `ncurses`** — Full-screen UI with menus, colors, prompts, and input validation.
- ✏️ **Edit, Add, Delete Notes** — Users can only modify their own notes.
- 🧽 **Auto Cleanup** — Shared memory is automatically removed when the last user exits.
- 🧪 **Robust Input Handling** — Graceful error handling for invalid inputs and permission checks.

---

## 🛠️ Technologies Used

- **C (C11)**  
- **`ncurses`** for UI  
- **POSIX Shared Memory** (`shmget`, `shmat`, `shmctl`)  
- **POSIX Mutexes** for inter-process locking  

---

### 📦 Requirements
- Linux/Unix system
- GCC compiler
- `ncurses` development headers
- CMake (version 3.20 or higher recommended)

---

## ⚙️ Installation

On Debian/Ubuntu-based systems, install dependencies with:

```bash
sudo apt update
sudo apt install build-essential libncurses5-dev cmake
```

---

## 🏗️ Build and Run
```bash
git clone https://github.com/Levon90210/notebox.git
cd NoteBox
mkdir build
cd build
cmake ..
cmake --build .
./NoteBox
```

---

## 🗂️ Project Structure
```
NoteBox/
├── include/
│ └── notebox.h         # Shared data structures and function declarations for NoteBox
├── src/
│ └── notebox.c         # Core implementation of NoteBox functionality
├── main.c              # Entry point, initializes and runs the app
├── CMakeLists.txt      # Build configuration for CMake
├── README.md           # Project documentation
└── .gitignore          # Git ignore rules
```
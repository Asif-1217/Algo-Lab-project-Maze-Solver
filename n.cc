#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <stack>
#include <queue>
#include <set>
#include <algorithm>
#include <iomanip>
#include <random>
#include <limits>
#include <cstdlib>
#include <chrono>
using namespace std;

#ifdef _WIN32
    #define CLEAR_CMD "cls"
#else
    #define CLEAR_CMD "clear"
#endif


static void clearScreen() { if (system(CLEAR_CMD) != 0) {  } }

static void waitForEnter() {
    cout << "\nPress ENTER to continue...";
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    cin.get();
}


class Maze {
public:
    vector<string> grid;
    int rows = 0, cols = 0;
    pair<int,int> start{-1,-1};
    pair<int,int> end{-1,-1};

    void clearMaze() {
        grid.clear();
        rows = cols = 0;
        start = {-1,-1};
        end   = {-1,-1};
    }

    
    string validate() const {
        if (rows == 0 || cols == 0 || grid.empty())
            return "Error: Maze is empty. Load, type, or generate a maze first.";

        for (int i = 0; i < rows; i++) {
            if ((int)grid[i].size() != cols)
                return "Error: Row " + to_string(i) + " has a different length than the "
                       "rest (ragged maze). Every row must have the same width.";
        }

        int startCount = 0, endCount = 0;
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                char c = grid[i][j];
                if (c != '#' && c != '.' && c != 'S' && c != 'E')
                    return string("Error: Invalid character '") + c + "' at (" +
                           to_string(i) + "," + to_string(j) +
                           "). Only '#', '.', 'S', 'E' are allowed.";
                if (c == 'S') startCount++;
                if (c == 'E') endCount++;
            }
        }
        if (startCount == 0) return "Error: No start cell 'S' found in the maze.";
        if (startCount > 1)  return "Error: Duplicate 'S' found - only one start cell allowed.";
        if (endCount == 0)   return "Error: No end/target cell 'E' found in the maze.";
        if (endCount > 1)    return "Error: Duplicate 'E' found - only one end cell allowed.";
        return "";
    }

    void locateEndpoints() {
        for (int i = 0; i < rows; i++)
            for (int j = 0; j < cols; j++) {
                if (grid[i][j] == 'S') start = {i, j};
                if (grid[i][j] == 'E') end   = {i, j};
            }
    }

    
    bool loadFromFile(const string& path) {
        ifstream fin(path);
        if (!fin.is_open()) return false;
        clearMaze();
        string line;
        while (getline(fin, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (line.empty()) continue;
            grid.push_back(line);
        }
        rows = (int)grid.size();
        cols = rows ? (int)grid[0].size() : 0;
        return true;
    }

  
    bool saveToFile(const string& path) const {
        ofstream fout(path);
        if (!fout.is_open()) return false;
        for (auto& row : grid) fout << row << "\n";
        return true;
    }

   
    void loadFromManualInput() {
        clearMaze();
        cout << "Rows: ";
        bool okRows = (bool)(cin >> rows);
        cout << "Cols: ";
        bool okCols = (bool)(cin >> cols);
        if (!okRows || !okCols) {
            cin.clear(); cin.ignore(10000, '\n');
            rows = cols = 0;
            cout << "Error: invalid numbers entered.\n";
            return;
        }
        cin.ignore();
        if (rows <= 0 || cols <= 0) {
            cout << "Error: rows/cols must be positive.\n";
            rows = cols = 0;
            return;
        }
        cout << "Type the maze row by row (# wall, . open, S start, E end).\n"
             << "You can type cells with or without spaces between them,\n"
             << "e.g. 'S..#' or 'S . . #' both work:\n";
        for (int i = 0; i < rows; i++) {
            string line;
            getline(cin, line);
            
            string cleaned;
            for (char ch : line)
                if (!isspace((unsigned char)ch)) cleaned += ch;
            grid.push_back(cleaned);
        }
    }

    // ---- Quick BFS reachability check, used by the random generator
    bool isSolvableQuickCheck() const {
        if (rows == 0 || cols == 0) return false;
        vector<vector<bool>> vis(rows, vector<bool>(cols, false));
        queue<pair<int,int>> q;
        q.push(start);
        vis[start.first][start.second] = true;
        int dr[] = {-1,1,0,0}, dc[] = {0,0,-1,1};
        while (!q.empty()) {
            auto cur = q.front(); q.pop();
            if (cur == end) return true;
            for (int d = 0; d < 4; d++) {
                int nx = cur.first + dr[d], ny = cur.second + dc[d];
                if (nx >= 0 && nx < rows && ny >= 0 && ny < cols &&
                    !vis[nx][ny] && grid[nx][ny] != '#') {
                    vis[nx][ny] = true;
                    q.push({nx, ny});
                }
            }
        }
        return false;
    }

    
    void generateRandomMaze(int r, int c, double wallDensity, bool ensureSolvable) {
        mt19937 rng(random_device{}());
        uniform_real_distribution<double> dist(0.0, 1.0);
        uniform_int_distribution<int> rd(0, r - 1), cd(0, c - 1);
        int attempts = 0;
        do {
            clearMaze();
            rows = r; cols = c;
            grid.assign(rows, string(cols, '.'));
            for (int i = 0; i < rows; i++)
                for (int j = 0; j < cols; j++)
                    if (dist(rng) < wallDensity) grid[i][j] = '#';

            do { start = {rd(rng), cd(rng)}; } while (grid[start.first][start.second] == '#');
            do { end   = {rd(rng), cd(rng)}; } while (grid[end.first][end.second] == '#' || end == start);

            grid[start.first][start.second] = 'S';
            grid[end.first][end.second]     = 'E';
            attempts++;
        } while (ensureSolvable && !isSolvableQuickCheck() && attempts < 300);
    }

    
    void generatePerfectMaze(int r, int c) {
        if (r % 2 == 0) r++;
        if (c % 2 == 0) c++;
        clearMaze();
        rows = r; cols = c;
        grid.assign(rows, string(cols, '#'));
        mt19937 rng(random_device{}());
        vector<vector<bool>> visited(rows, vector<bool>(cols, false));
        stack<pair<int,int>> st;
        grid[1][1] = '.';
        visited[1][1] = true;
        st.push({1, 1});
        int dr[] = {-2,2,0,0}, dc[] = {0,0,-2,2};
        while (!st.empty()) {
            int x = st.top().first, y = st.top().second;
            vector<int> dirs = {0,1,2,3};
            shuffle(dirs.begin(), dirs.end(), rng);
            bool moved = false;
            for (int d : dirs) {
                int nx = x + dr[d], ny = y + dc[d];
                if (nx > 0 && nx < rows - 1 && ny > 0 && ny < cols - 1 && !visited[nx][ny]) {
                    grid[nx][ny] = '.';
                    grid[x + dr[d]/2][y + dc[d]/2] = '.';
                    visited[nx][ny] = true;
                    st.push({nx, ny});
                    moved = true;
                    break;
                }
            }
            if (!moved) st.pop();
        }
        start = {1, 1};
        end   = {rows - 2, cols - 2};
        if (grid[end.first][end.second] == '#') grid[end.first][end.second] = '.';
        grid[start.first][start.second] = 'S';
        grid[end.first][end.second]     = 'E';
    }

    void display(const vector<pair<int,int>>& path = {},
                 const vector<vector<bool>>& visited = {},
                 bool useColor = true) const {
        set<pair<int,int>> pathSet(path.begin(), path.end());
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                char c = grid[i][j];
                bool onPath     = pathSet.count({i, j}) && c != 'S' && c != 'E';
                bool wasVisited = !visited.empty() && (int)visited.size() > i &&
                                  visited[i][j] && c == '.';
                if (useColor) {
                    if (c == '#')            cout << "\033[90m# \033[0m";
                    else if (c == 'S')       cout << "\033[1;34mS \033[0m";
                    else if (c == 'E')       cout << "\033[1;31mE \033[0m";
                    else if (onPath)         cout << "\033[1;32m* \033[0m";
                    else if (wasVisited)     cout << "\033[36m. \033[0m";
                    else                     cout << ". ";
                } else {
                    if (onPath) cout << "* ";
                    else        cout << c << ' ';
                }
            }
            cout << "\n";
        }
    }
};

// ---------------------------------------------------------------------
//  SOLVER  --  DFS and BFS, both returning path + stats for reporting
// ---------------------------------------------------------------------
struct SolveResult {
    bool found = false;
    vector<pair<int,int>> path;            // final S -> E path (empty if none)
    vector<pair<int,int>> exploredOrder;    // order cells were processed
    vector<vector<bool>> visited;
    int nodesExplored = 0;
    double timeMs = 0.0;
    string algorithm;
};


SolveResult solveDFS(const Maze& maze) {
    SolveResult res; res.algorithm = "DFS";
    auto t0 = chrono::high_resolution_clock::now();

    int R = maze.rows, C = maze.cols;
    vector<vector<bool>> visited(R, vector<bool>(C, false));
    vector<vector<pair<int,int>>> parent(R, vector<pair<int,int>>(C, {-1,-1}));
    stack<pair<int,int>> st;
    st.push(maze.start);
    visited[maze.start.first][maze.start.second] = true;
    int dr[] = {-1,1,0,0}, dc[] = {0,0,-1,1};
    bool found = false;

    while (!st.empty()) {
        auto cur = st.top(); st.pop();
        res.nodesExplored++;
        res.exploredOrder.push_back(cur);
        if (cur == maze.end) { found = true; break; }
        for (int d = 0; d < 4; d++) {
            int nx = cur.first + dr[d], ny = cur.second + dc[d];
            if (nx >= 0 && nx < R && ny >= 0 && ny < C &&
                !visited[nx][ny] && maze.grid[nx][ny] != '#') {
                visited[nx][ny] = true;
                parent[nx][ny] = cur;
                st.push({nx, ny});
            }
        }
    }

    res.found = found;
    res.visited = visited;
    if (found) {
        pair<int,int> cur = maze.end;
        while (cur != maze.start) {
            res.path.push_back(cur);
            cur = parent[cur.first][cur.second];
        }
        res.path.push_back(maze.start);
        reverse(res.path.begin(), res.path.end());
    }

    auto t1 = chrono::high_resolution_clock::now();
    res.timeMs = chrono::duration<double, milli>(t1 - t0).count();
    return res;
}


SolveResult solveBFS(const Maze& maze) {
    SolveResult res; res.algorithm = "BFS";
    auto t0 = chrono::high_resolution_clock::now();

    int R = maze.rows, C = maze.cols;
    vector<vector<bool>> visited(R, vector<bool>(C, false));
    vector<vector<pair<int,int>>> parent(R, vector<pair<int,int>>(C, {-1,-1}));
    queue<pair<int,int>> q;
    q.push(maze.start);
    visited[maze.start.first][maze.start.second] = true;
    int dr[] = {-1,1,0,0}, dc[] = {0,0,-1,1};
    bool found = false;

    while (!q.empty()) {
        auto cur = q.front(); q.pop();
        res.nodesExplored++;
        res.exploredOrder.push_back(cur);
        if (cur == maze.end) { found = true; break; }
        for (int d = 0; d < 4; d++) {
            int nx = cur.first + dr[d], ny = cur.second + dc[d];
            if (nx >= 0 && nx < R && ny >= 0 && ny < C &&
                !visited[nx][ny] && maze.grid[nx][ny] != '#') {
                visited[nx][ny] = true;
                parent[nx][ny] = cur;
                q.push({nx, ny});
            }
        }
    }

    res.found = found;
    res.visited = visited;
    if (found) {
        pair<int,int> cur = maze.end;
        while (cur != maze.start) {
            res.path.push_back(cur);
            cur = parent[cur.first][cur.second];
        }
        res.path.push_back(maze.start);
        reverse(res.path.begin(), res.path.end());
    }

    auto t1 = chrono::high_resolution_clock::now();
    res.timeMs = chrono::duration<double, milli>(t1 - t0).count();
    return res;
}


void printLegend() {
    cout << "\033[90m# \033[0mwall   . open   \033[1;34mS \033[0mstart   \033[1;31mE \033[0mend   "
         << "\033[1;32m* \033[0mpath   \033[36m. \033[0mexplored\n";
}

int countOpenCells(const Maze& m) {
    int cnt = 0;
    for (auto& row : m.grid) for (char c : row) if (c != '#') cnt++;
    return cnt;
}

// Prints a short human explanation of the numbers, not just raw stats.
void explainResult(const SolveResult& r, int totalOpenCells) {
    cout << "\n--- " << r.algorithm << " report ---\n";
    if (!r.found) {
        cout << "Result      : NO PATH FOUND - the end cell is unreachable from the start.\n";
        cout << "Cells tried : " << r.nodesExplored << " / " << totalOpenCells << " open cells\n";
        cout << "Time        : " << fixed << setprecision(4) << r.timeMs << " ms\n";
        return;
    }
    cout << "Result      : Path found!\n";
    cout << "Path length : " << (r.path.size() - 1) << " steps (" << r.path.size() << " cells)\n";
    cout << "Cells tried : " << r.nodesExplored << " (nodes popped from the "
         << (r.algorithm == "DFS" ? "stack" : "queue") << ")\n";
    cout << "Time        : " << fixed << setprecision(4) << r.timeMs << " ms\n";
    if (r.algorithm == "DFS")
        cout << "Note        : DFS dives down one branch at a time and backtracks on dead ends.\n"
             << "              It finds A path fast, but usually NOT the shortest one.\n";
    else
        cout << "Note        : BFS expands the search one ring/level at a time, so the first\n"
             << "              time it reaches 'E' the path is guaranteed to be the shortest.\n";
}


void runSingle(Maze& maze, const string& which) {
    string err = maze.validate();
    if (!err.empty()) { cout << "\n" << err << "\n"; return; }
    maze.locateEndpoints();

    SolveResult r = (which == "DFS") ? solveDFS(maze) : solveBFS(maze);

    cout << "\nMaze after " << which << " search:\n";
    printLegend();
    maze.display(r.path, r.visited);
    explainResult(r, countOpenCells(maze));
}

void runCompare(Maze& maze) {
    string err = maze.validate();
    if (!err.empty()) { cout << "\n" << err << "\n"; return; }
    maze.locateEndpoints();

    SolveResult dfs = solveDFS(maze);
    SolveResult bfs = solveBFS(maze);
    int openCells = countOpenCells(maze);

    cout << "\n=== DFS path ===\n";
    printLegend();
    maze.display(dfs.path, dfs.visited);

    cout << "\n=== BFS path ===\n";
    maze.display(bfs.path, bfs.visited);

    cout << "\nTotal open (walkable) cells in maze: " << openCells << "\n";
    cout << left
         << setw(12) << "Algorithm" << setw(14) << "Path Found"
         << setw(16) << "Path Length" << setw(16) << "Nodes Explored"
         << setw(12) << "Time (ms)" << "\n";
    cout << string(70, '-') << "\n";

    auto printRow = [&](const SolveResult& r) {
        cout << left << setw(12) << r.algorithm
             << setw(14) << (r.found ? "Yes" : "No")
             << setw(16) << (r.found ? to_string(r.path.size() - 1) : string("-"))
             << setw(16) << r.nodesExplored
             << setw(12) << fixed << setprecision(4) << r.timeMs << "\n";
    };
    printRow(dfs);
    printRow(bfs);

    cout << "\nVerdict: ";
    if (!dfs.found && !bfs.found) {
        cout << "Neither algorithm could reach 'E' - the target is unreachable in this maze.\n";
    } else if (dfs.found && bfs.found) {
        long dfsLen = (long)dfs.path.size() - 1, bfsLen = (long)bfs.path.size() - 1;
        if (dfsLen > bfsLen)
            cout << "BFS found a SHORTER path (" << bfsLen << " vs " << dfsLen
                 << " steps). BFS guarantees shortest path in unweighted mazes; DFS does not.\n";
        else
            cout << "Both happened to find equal-length paths here, but BFS is the one that\n"
                 << "always GUARANTEES the shortest path, regardless of maze shape.\n";
    } else {
        cout << (dfs.found ? "DFS" : "BFS") << " found a path while the other did not.\n";
    }
}


void showMazeStats(const Maze& maze) {
    string err = maze.validate();
    if (!err.empty()) { cout << "\n" << err << "\n"; return; }

    int walls = 0, open = 0;
    for (auto& row : maze.grid)
        for (char c : row) {
            if (c == '#') walls++;
            else open++;
        }
    int total = maze.rows * maze.cols;
    int manhattan = abs(maze.start.first - maze.end.first) +
                     abs(maze.start.second - maze.end.second);

    cout << "\n----- Maze statistics -----\n";
    cout << "Dimensions       : " << maze.rows << " rows x " << maze.cols << " cols "
         << "(" << total << " cells)\n";
    cout << "Wall cells       : " << walls << " (" << fixed << setprecision(1)
         << (100.0 * walls / total) << "%)\n";
    cout << "Open cells       : " << open << " (" << fixed << setprecision(1)
         << (100.0 * open / total) << "%)\n";
    cout << "Start position   : (" << maze.start.first << ", " << maze.start.second << ")\n";
    cout << "End position     : (" << maze.end.first << ", " << maze.end.second << ")\n";
    cout << "Manhattan dist.  : " << manhattan << " cells (straight-line lower bound,\n"
         << "                   ignoring walls - the real path can only be >= this)\n";
}

void showHelp() {
    cout << "\n============================ HELP / ABOUT ============================\n"
         << "This program builds or loads a grid maze and solves it two ways:\n\n"
         << "  DFS (Depth-First Search)\n"
         << "    - Uses a STACK (LIFO): last cell pushed is the first one explored.\n"
         << "    - Commits to one corridor, backtracks only when it hits a dead end.\n"
         << "    - Finds *a* valid path quickly, but it is often longer than needed.\n"
         << "    - Time complexity: O(V + E) over the grid graph, Space: O(V).\n\n"
         << "  BFS (Breadth-First Search)\n"
         << "    - Uses a QUEUE (FIFO): explores every cell at distance d before any\n"
         << "      cell at distance d+1 - it grows outward in rings from the start.\n"
         << "    - Because every move costs exactly 1 step, the first time BFS\n"
         << "      reaches 'E' that path is PROVABLY the shortest one possible.\n"
         << "    - Time complexity: O(V + E), Space: O(V) - same order as DFS, but\n"
         << "      BFS is the correct tool whenever you specifically need the\n"
         << "      shortest path in an unweighted graph/grid.\n\n"
         << "Maze file format:\n"
         << "    # = wall      . = open floor      S = start      E = end\n"
         << "    Exactly one S and one E are required; all rows must be equal width.\n\n"
         << "Tips:\n"
         << "  - Use option 3 if you want a maze that is ALWAYS solvable.\n"
         << "  - Use option 8 (Compare) to see DFS and BFS side by side with a\n"
         << "    results table and a plain-English verdict.\n"
         << "========================================================================\n";
}

// ---------------------------------------------------------------------
//  Menu
// ---------------------------------------------------------------------
void printMenu() {
    cout << "\n==================== MAZE SOLVER =====================\n"
         << " 1. Load maze from file\n"
         << " 2. Type a maze manually\n"
         << " 3. Generate random obstacle maze\n"
         << " 4. Generate guaranteed-solvable perfect maze (DFS carve)\n"
         << " 5. Show current maze\n"
         << " 6. Solve with DFS\n"
         << " 7. Solve with BFS\n"
         << " 8. Compare DFS vs BFS\n"
         << " 9. Save current maze to file\n"
         << "10. Show maze statistics\n"
         << "11. Help / About (algorithm explanations)\n"
         << " 0. Exit\n"
         << "=======================================================\n"
         << "Choice: ";
}

int main() {
    Maze maze;
    cout << "Welcome to the Maze Solver project (DFS vs BFS)\n";

    while (true) {
        printMenu();
        int choice;
        if (!(cin >> choice)) { cin.clear(); cin.ignore(10000, '\n'); continue; }

        if (choice == 0) { cout << "Bye!\n"; break; }

        switch (choice) {
            case 1: {
                cout << "File path: ";
                string path; cin >> path;
                if (!maze.loadFromFile(path)) { cout << "Error: could not open file '" << path << "'.\n"; break; }
                cout << "Loaded. ";
                { string err = maze.validate();
                  if (!err.empty()) cout << err << "\n"; else { maze.locateEndpoints(); cout << "Maze looks valid.\n"; } }
                break;
            }
            case 2: {
                maze.loadFromManualInput();
                string err = maze.validate();
                if (!err.empty()) cout << err << "\n"; else { maze.locateEndpoints(); cout << "Maze looks valid.\n"; }
                break;
            }
            case 3: {
                int r, c; double density;
                cout << "Rows: ";
                cin >> r;
                cout << "Cols: ";
                cin >> c;
                cout << "Wall density (0.0-0.5 suggested): ";
                cin >> density;
                if (!cin) { cin.clear(); cin.ignore(10000, '\n'); cout << "Error: invalid input.\n"; break; }
                if (r <= 1 || c <= 1 || density < 0 || density >= 1) {
                    cout << "Error: invalid dimensions or density.\n"; break;
                }
                maze.generateRandomMaze(r, c, density, true);
                cout << "Random solvable maze generated.\n";
                break;
            }
            case 4: {
                int r, c;
                cout << "Rows (odd numbers work best): ";
                cin >> r;
                cout << "Cols (odd numbers work best): ";
                cin >> c;
                if (!cin) { cin.clear(); cin.ignore(10000, '\n'); cout << "Error: invalid input.\n"; break; }
                if (r <= 1 || c <= 1) { cout << "Error: invalid dimensions.\n"; break; }
                maze.generatePerfectMaze(r, c);
                cout << "Perfect maze generated (guaranteed solvable).\n";
                break;
            }
            case 5: {
                string err = maze.validate();
                if (!err.empty()) { cout << err << "\n"; break; }
                printLegend();
                maze.display();
                break;
            }
            case 6: runSingle(maze, "DFS"); break;
            case 7: runSingle(maze, "BFS"); break;
            case 8: runCompare(maze); break;
            case 9: {
                string err = maze.validate();
                if (!err.empty()) { cout << err << "\n"; break; }
                cout << "Save as: ";
                string path; cin >> path;
                if (maze.saveToFile(path)) cout << "Saved to " << path << "\n";
                else cout << "Error: could not write to '" << path << "'.\n";
                break;
            }
            case 10: showMazeStats(maze); break;
            case 11: showHelp(); break;
            default:
                cout << "Unknown option.\n";
        }
    }
    return 0;
}

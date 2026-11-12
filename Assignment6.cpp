/*
Name: EECS 348 Assignment 5
Description: Sudoku solver program
Inputs: 5 text files puzzle(1-5).txt
Output: Fully solved sudoku puzzle(s) in terminal result and solutions.txt
All Collaborators: None other than GenAI
Sources: Original by Joshua Liou, revisions by ChatGPT and Gemini
Author: Joshua Liou
Original Code author: Joshua Louis
Creation date: 10/15/26
Revised by: Joshua Liou
Revised date: 10/29/26

Revision #2 by: Joshua Louis
Revision #2 Date: 11/12/26
*/

//for basic input/output, reading files, and strings
#include <iostream>
#include <fstream>  
#include <string>   
#include <vector>    //import use of vectors
#include <cctype>    //allows checking character types like isspace

class SudokuSolver {  //create the class used for the program. establish puzzle board and method
public:  //private, only methods in Sudoku class can access the below:
    SudokuSolver(const std::string& filename) : filename_(filename) {  //constructor that stores the puzzle filename
        grid_.resize(9, std::vector<char>(9, '_'));  //create empty 9x9 board
        valid_ = readPuzzle();  //load puzzle and if valid
    }  //end of constructor

    bool isValidPuzzle() const {  //allow other code to check the result of loading the puzzle
        return valid_;  //true means the input passed the checks
    }  //end of isValidPuzzle

    //REVISED: write this puzzle's result to the shared output stream instead of a per-puzzle file.
    bool solveAndPrint(std::ostream& outfile) {  //handle this puzzle and return whether its input succeeded
        std::cout << "Puzzle file: " << filename_ << "\n";  //print which file is being solved

        //ADDED: each puzzle begins with a single line naming the puzzle file.
        outfile << "Puzzle: " << filename_ << "\n";  //write the required puzzle header line

        if (!valid_) {  //skip solving if the file was missing or its contents were invalid
            std::cout << "No solution found\n";  //treat bad puzzles as unsolvable
            outfile << "No solution found.\n\n";  //ADDED: required no-solution file text, blank line separates puzzles
            outfile.flush();  //ADDED: force the write now so a failure is caught for this puzzle
            if (!outfile) reportWriteFailure();  //ADDED: report if this puzzle's output failed to write
            std::cout << "----------------------------------------\n";  //separate puzzle from next
            return false;  //report that input validation failed
        }  //end of invalid puzzle check

        std::cout << "Puzzle:\n";  //print a label for the puzzle
        printGrid(grid_);  //print the unsolved puzzle
        std::cout << "\n";  //print a blank line

        solutions_.clear();  //remove any old solutions
        solve(0, 0);  //start solving from the top-left cell

        if (solutions_.empty()) {  //check if no solutions were found
            std::cout << "No solution found\n";  //tell the user there is no solution
            outfile << "No solution found.\n";  //ADDED: only one line when unsolved
        } else {  //otherwise solutions exist
            for (size_t i = 0; i < solutions_.size(); ++i) {  //loop through each solution
                std::cout << "Solution " << (i + 1) << ":\n";  //print the solution number
                printGrid(solutions_[i]);  //print this solved grid
                std::cout << "\n";  //print a blank line
                //ADDED: write the solved grid without terminal labels.
                printGrid(solutions_[i], outfile);  //use the input's space-separated grid format
                if (i + 1 < solutions_.size()) outfile << '\n';  //separate multiple solutions
            }  //end of solution loop
            std::cout << "Solutions found: " << solutions_.size() << "\n";  //print how many solutions were found
            outfile << "Solutions found: " << solutions_.size() << "\n";  //ADDED: final count
        }  //end of solution check
        outfile << '\n';  //ADDED: blank line separates this puzzle from the next
        //ADDED: check right after this puzzle's output so the failing puzzle is identified immediately.
        outfile.flush();  //ADDED: force buffered text to disk now so a failed write shows up for this puzzle
        if (!outfile) {  //the stream failed while writing this puzzle
            reportWriteFailure();  //name the puzzle whose output could not be written
            std::cout << "----------------------------------------\n";  //print a divider line
            return false;  //report the failed write to main
        }
        std::cout << "----------------------------------------\n";  //print a divider line
        return true;  //a valid puzzle, including an unsolvable one, was processed
    }  //end of solveAndPrint

private:  //everything below can only be used inside the class
    //ADDED: one place to report which puzzle's output could not be written to solutions.txt.
    void reportWriteFailure() const {  //print an error naming the puzzle being written
        std::cerr << "Error: could not write the output for '" << filename_  //name the puzzle file
                  << "' to 'solutions.txt'.\n";  //name the output file
    }  //end of reportWriteFailure

    static const int SIZE = 9;  //the board has nine rows and nine columns
    static const int BOX_SIZE = 3;  //each Sudoku box is three rows by three columns

    std::string filename_;  //stores the puzzle filename
    std::vector<std::vector<char>> grid_;  //stores the 9x9 puzzle grid
    std::vector<std::vector<std::vector<char>>> solutions_;  //stores every solution found
    bool valid_ = false;  //remembers if the puzzle passed all checks

    bool isLegalCell(char c) const {  //checks if a character can go in the grid
        if (c == '_') return true;  //blank cells are allowed
        if (c >= '1' && c <= '9') return true;  //digits 1 to 9 are allowed
        return false;  //anything else is not allowed
    }  //end of isLegalCell

    bool isDigitChar(char c) const {  //checks if a character is any digit, used for error messages
        return c >= '0' && c <= '9';  //true if the character is between 0 and 9
    }  //end of isDigitChar

    bool hasConflicts(const std::vector<std::vector<char>>& g,  //takes the grid to check
                      int& conflictRow, int& conflictCol) const {  //outputs where the conflict is
        for (int r = 0; r < SIZE; ++r) {  //loop through each row
            for (int c = 0; c < SIZE; ++c) {  //loop through each column
                char val = g[r][c];  //grab the value in this cell
                if (val == '_') continue;  //skip blank cells

                //isValid skips the current cell, so g stays read-only.
                bool ok = isValid(g, r, c, val);  //check peers without blanking or restoring the cell

                if (!ok) {  //check if a rule was broken
                    conflictRow = r;  //save the row of the conflict
                    conflictCol = c;  //save the column of the conflict
                    return true;  //a conflict was found
                }  //end of conflict check
            }  //end of column loop
        }  //end of row loop
        return false;  //no conflicts found anywhere
    }  //end of hasConflicts

    bool readPuzzle() {  //load nine puzzle rows and reject unsupported or conflicting input
        std::ifstream infile(filename_);  //open the puzzle file
        if (!infile) {  //check if the file failed to open
            std::cerr << "Error: could not open file '" << filename_ << "'.\n";  //print an error message
            return false;  //report the failure
        }  //end of file check

        std::vector<std::string> lines;  //hold the nonblank rows until their sizes are checked
        std::string line;  //reuse this string for each line read from the input
        while (std::getline(infile, line)) {  //read the file one line at a time
            //ignore whitespace-only lines, including CRLF blank lines.
            if (line.find_first_not_of(" \t\r\n\f\v") != std::string::npos) {
                if (lines.size() == SIZE) {  //reject a tenth row without storing an unbounded file
                    std::cerr << "Error in '" << filename_ << "': expected 9 non-empty lines.\n";
                    return false;  //the puzzle contains extra non-empty rows
                }
                lines.push_back(line);  //save the line
            }  //end of empty line check
        }  //end of line reading loop
        //distinguish a read failure from ordinary end-of-file.
        if (infile.bad()) {  //check for a read error before accepting the collected rows
            std::cerr << "Error: could not finish reading '" << filename_ << "'.\n";
            return false;  //reject a file that could not be read completely
        }
        infile.close();  //close the file

        if (lines.size() != SIZE) {  //check that the puzzle has exactly 9 rows
            std::cerr << "Error in '" << filename_  //identify the file with the wrong number of rows
                      << "': expected " << SIZE  //say how many rows were expected
                      << " non-empty lines, found " << lines.size() << ".\n";  //say how many rows were found
            return false;  //report the failure
        }  //end of row count check

        for (int r = 0; r < SIZE; ++r) {  //loop through each row
            std::vector<char> tokens;  //stores the cells found in this row
            for (char ch : lines[r]) {  //look at each character in the row
                if (!std::isspace(static_cast<unsigned char>(ch))) {  //skip spaces and tabs
                    tokens.push_back(ch);  //save the character as a cell
                }  //end of whitespace check
            }  //end of character loop

            if (tokens.size() != SIZE) {  //check that the row has exactly 9 cells
                std::cerr << "Error in '" << filename_ << "' at row "  //start printing an error about the row
                          << (r + 1) << ": expected " << SIZE  //say how many cells were expected
                          << " cells, found " << tokens.size() << ".\n";  //say how many cells were found
                return false;  //report the failure
            }  //end of cell count check

            for (int c = 0; c < SIZE; ++c) {  //loop through each cell in the row
                char cell = tokens[c];  //grab the character for this cell
                if (!isLegalCell(cell)) {  //check if the character is not allowed
                    if (isDigitChar(cell)) {  //check if it is a digit like 0
                        std::cerr << "Error in '" << filename_ << "' at row "  //start printing an out-of-range digit error
                                  << (r + 1) << ", column " << (c + 1)  //say where the bad digit is
                                  << ": digit '" << cell  //show the bad digit
                                  << "' is out of range (must be 1-9).\n";  //explain the allowed range
                    } else {  //otherwise it is a random invalid character
                        std::cerr << "Error in '" << filename_ << "' at row "  //start printing an invalid character error
                                  << (r + 1) << ", column " << (c + 1)  //say where the bad character is
                                  << ": invalid character '" << cell  //show the bad character
                                  << "' (only '_' or '1'-'9' allowed).\n";  //explain what is allowed
                    }  //end of error type check
                    return false;  //report the failure
                }  //end of legal cell check
                grid_[r][c] = cell;  //store the valid character in the grid
            }  //end of cell loop
        }  //end of row loop

        int conflictRow = -1, conflictCol = -1;  //will hold the location of any duplicate
        if (hasConflicts(grid_, conflictRow, conflictCol)) {  //check for duplicate digits breaking the rules
            std::cerr << "Error in '" << filename_ << "': duplicate value '"  //start printing a duplicate error
                      << grid_[conflictRow][conflictCol] << "' at row "  //show the duplicated value
                      << (conflictRow + 1) << ", column "  //say which row it is in
                      << (conflictCol + 1)  //say which column it is in
                      << " conflicts with another cell in the same row, "  //explain the conflict
                         "column, or 3x3 box.\n";  //finish the explanation
            return false;  //report the failure
        }  //end of conflict check

        return true;  //the puzzle passed every check
    }  //end of readPuzzle

    //one formatter supports both the console and an output file.
    void printGrid(const std::vector<std::vector<char>>& g,
                   std::ostream& output = std::cout) const {  //console is the default destination
        for (int r = 0; r < SIZE; ++r) {  //loop through each row
            for (int c = 0; c < SIZE; ++c) {  //loop through each column
                output << g[r][c];  //write the character to the selected destination
                if (c < SIZE - 1) output << ' ';  //add a space between cells
            }  //end of column loop
            output << '\n';  //move to the next line after each row
        }  //end of row loop
    }  //end of printGrid

    bool isValid(const std::vector<std::vector<char>>& g,  //the board whose cells will be checked
                 int row, int col, char num) const {  //test this value at the supplied row and column
        for (int c = 0; c < SIZE; ++c) {  //check and store each of the nine cells in this row
            if (c != col && g[row][c] == num) return false;  //test other row cells
        }
        for (int r = 0; r < SIZE; ++r) {  //loop through each cell in the column
            if (r != row && g[r][col] == num) return false;  //test other column cells
        } 
        int boxRow = (row / BOX_SIZE) * BOX_SIZE; //calculate where this cell's three-row box begins
        int boxCol = (col / BOX_SIZE) * BOX_SIZE;  //calculate where this cell's three-column box begins
        for (int r = boxRow; r < boxRow + BOX_SIZE; ++r) {  //visit each row inside the cell's box
            for (int c = boxCol; c < boxCol + BOX_SIZE; ++c) {  //visit each column inside that box
                //ignore the target cell when validating an existing clue.
                if ((r != row || c != col) && g[r][c] == num) return false;
            }  //end of box column loop
        }  //end of box row loop
        return true;  //no conflicts found, so the placement is valid
    }  //end of isValid

    void solve(int row, int col) {  //try legal values in empty cells and back up when a choice fails
        //move past clues with a loop instead of extra recursive calls.
        while (row < SIZE && grid_[row][col] != '_') {   //keep moving until an empty cell is found or the board is finished
            ++col;  //check the next column
            if (col == SIZE) {  //move to the next row when this row ends
                ++row;  //advance the row
                col = 0;  //restart at its first column
            }
        }
        if (row == SIZE) {  //reaching row 9 means every cell has been filled
            solutions_.push_back(grid_);  //save a copy of the solved grid
            return;  //return to the previous call so it can try any remaining choices
        }  //end of solution found check

        int nextRow = row;  //start calculating position after current empty cell
        int nextCol = col + 1;  //the next cell is one column to the right
        if (nextCol == SIZE) {  //check whether that next position goes beyond this row
            nextRow = row + 1;  //use the following row as the next position
            nextCol = 0;  //go back to the first column
        }  //end of column wrap check

        for (char num = '1'; num <= '9'; ++num) {  //try every number from 1 to 9
            if (isValid(grid_, row, col, num)) {  //check if this number can go here
                grid_[row][col] = num;  //temp fill the current blank with candidate
                solve(nextRow, nextCol);  //search the remaining cells using the current placement
                grid_[row][col] = '_';  //undo the placement and try the next number
            }  //end of valid placement check
        }  //end of number loop
    }  //end of solve
};  //end of SudokuSolver class

int main() {  //where the program starts
    int status = 0;  //track failed input or output while processing all five files
    std::vector<std::string> puzzleFiles = {  //make a list of puzzle files to solve
        "puzzle1.txt",  
        "puzzle2.txt",  
        "puzzle3.txt", 
        "puzzle4.txt", 
        "puzzle5.txt"   
    };  

    //ADDED: open the single required output file before processing any puzzle.
    std::ofstream outfile("solutions.txt");  //create or overwrite the shared solution file
    if (!outfile) {  //stop if the required output cannot be created
        std::cerr << "Error: could not create file 'solutions.txt'.\n";
        return 1;  //nothing can be saved without the output file
    }  //end of output file check

    for (const auto& file : puzzleFiles) {  //handle every filename in the list
        SudokuSolver solver(file);  //create a SudokuSolver for this file
        if (!solver.solveAndPrint(outfile)) status = 1;  //retain failure status and continue
    }  //end of file loop

    //ADDED: closing explicitly allows a failed write or flush to be reported.
    outfile.close();  //flush and close the completed solution file
    if (!outfile) {  //propagate write failures to the return status
        std::cerr << "Error: could not finish file 'solutions.txt'.\n";
        status = 1;  //the required file was not saved successfully
    }  //end of write failure check

    return status;  //return nonzero if any input or output failed
}  //end of main
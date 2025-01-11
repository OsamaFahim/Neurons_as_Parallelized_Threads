#include <iostream>
#include <vector>
#include <cstdlib>
#include <vector>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fstream>
using namespace std;

// This function reads data from a named pipe and returns a vector of double values.

vector<double> read_pipe()
{
    const char *pipe_name = "my_pipe";        // Name of the named pipe
    vector<double> values_prv;                // Vector to hold the double values read from the pipe
    char *buffer2 = new char[100]{};          // Allocate memory for buffer to store data read from pipe
    int fd2 = open(pipe_name, O_RDONLY);      // Open the named pipe in read-only mode
    int bytes_read = read(fd2, buffer2, 100); // Read data from the named pipe into the buffer

    // Check if there was an error reading from the pipe
    if (bytes_read < 0)
    {
        std::cerr << "Error reading from pipe!\n";
        return values_prv; // Return empty vector if there was an error
    }

    cout.flush(); // Flush cout buffer

    int size_of_buffer = 0, values = 0; // Initialize variables to keep track of buffer size and number of values read
    string message = {};                // Initialize empty string to hold the string representation of double values

    // Loop through the buffer and extract the double values separated by commas
    for (int i = 0; buffer2[i] != '\0' && i < 100; i++)
    {
        size_of_buffer++;

        // If a comma is encountered, convert the message string to a double and add it to the vector
        if (buffer2[i] == ',')
        {
            values_prv.push_back(stod(message));
            values++;
            message = {};
        }
        else
            message += buffer2[i]; // If a comma is not encountered, add the character to the message string
    }

    unlink(pipe_name); // Remove the named pipe from the file system

    return values_prv; // Return the vector of double values read from the named pipe
}

// This function takes a file path, size and a delimiter as input parameters
// It reads data from the file and returns an array of doubles
double *getData(string path, int size, char delimiter)
{
    // Open the file at the given path
    ifstream file(path);
    // Check if the file was opened successfully
    if (!file)
    {
        // If the file could not be opened, print an error message and return an empty array
        cout << "File could not be opened properly\n";
        return {};
    }
    else
    {
        // If the file was opened successfully, initialize some variables
        string reader = {};
        double *data = new double[size]{};
        int data_counter = 0;

        // Read each line from the file
        while (getline(file, reader))
        {
            int itr = 0, data_Length = reader.length();
            string temp = {};

            // Parse the line and extract the individual data values
            for (int itr = 0; itr <= data_Length; itr++)
            {
                // Skip any spaces in the line
                if (reader[itr] == ' ')
                    continue;

                // Extract each value from the line using the delimiter character
                while (itr < data_Length && reader[itr] != delimiter)
                    temp += reader[itr++];

                // Convert the extracted value to a double and store it in the data array
                if (data_counter < size)
                    data[data_counter++] = stod(temp);
                else
                    return data;

                temp = {};
            }
        }

        // Return the data array
        return data;
    }
}

// This function writes a vector of doubles to a named pipe
void write_pipe(vector<double> values_prv)
{
    const char *pipe_name = "my_pipe";
    int res = mkfifo(pipe_name, 0666); // create the named pipe with permissions
    if (res < 0)
        cerr << "Error creating pipe!\n";
    // Open the pipe for reading and writing
    int fd = open(pipe_name, O_RDWR);
    if (fd < 0)
        cerr << "Error opening pipe for read/write!\n";
    string message = {};
    // Loop over the values in the vector, convert each to a string, and add a comma
    for (int i = 0; i < values_prv.size(); i++)
    {
        message = to_string(values_prv[i]);
        message += ',';
        // Write the message to the pipe
        if (write(fd, message.c_str(), message.length()) <= 0)
            cout << "Writing failed" << endl;
        message = {};
    }
    const char ch = '\0';
    // Write a null character to the pipe to signal the end of data
    write(fd, &ch, 1);
    // cout << message << endl;
}

// This is a templated function to allocate a 2D array of a given datatype with a given number of rows and columns
// The function takes two arguments: the number of rows and the number of columns
// It returns a pointer to a pointer to the allocated array

template <class datatype>
datatype **allocate_2d(int row_size, int col_size)
{
    datatype **arr_2d = new datatype *[row_size] {};
    // Allocate an array of the given datatype for each row
    // Set each element of the array to 0
    for (int i = 0; i < row_size; i++)
        arr_2d[i] = new datatype[col_size]{};

    // Return a pointer to the allocated 2D array
    return arr_2d;
}

// This is a templated function that returns a 2D array of datatype.
template <class datatype>
datatype **Populate_Input_2dArray(string path, int row_size, int col_size)
{
    // Open the file in the given path
    ifstream file(path);
    // If the file could not be opened
    if (!file)
        cout
            << "File Could not be opened\n";
    else
    {
        // Allocate memory for the 2D array using a helper function allocate_2d.
        datatype **data = allocate_2d<datatype>(row_size, col_size);
        int curr_row = 0, curr_col = 0;

        string reader = {};

        // Read the file line by line
        while (getline(file, reader))
        {
            int reader_length = reader.length();
            string temp = {};

            // Iterate through each character of the line
            for (int itr = 0; itr < reader_length; itr++)
            {
                // If the current row is filled, move to the next row and reset the current column.
                if (curr_col == col_size)
                    curr_row++, curr_col = 0;

                // If all rows are filled, return the populated 2D array.
                if (curr_row == row_size)
                    return data;

                // Extract the data from the line, separated by ','.
                while (reader[itr] && reader[itr] != ',')
                    temp += reader[itr++];

                // Populate the current element of the 2D array with the extracted data.
                if (curr_col < col_size)
                    data[curr_row][curr_col++] = stod(temp);

                temp = {};
            }
        }
        // Return the populated 2D array.
        return data;
    }

    // If the file could not be opened, return NULL.
    return NULL;
}

double generate_input1(double x) // Function to generate 1st next input
{
    return ((x * x) + x + 1) / 2.0;
}

double generate_input2(double x) // Function to generate 2nd next input
{
    return ((x * x) - x) / 2.0;
}
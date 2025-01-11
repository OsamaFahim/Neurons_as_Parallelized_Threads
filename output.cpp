#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fstream>
#include <vector>
#include "General.cpp"
using namespace std;

// Define a global pointer to store the values of the second layer of neurons
double *layer2;

// Define a global variable that is used to lock threads
int thread_lock = {};

// Define a struct to store thread-specific data
struct thread_data
{
    double **weights;
    vector<double> layer1;
    int i;
    int size;
};

// Define a function to be run by each thread that will calculate the value of a single neuron in the second layer
void *layer_func(void *arg)
{
    // Cast the void pointer to a pointer to a thread_data struct
    thread_data *data = (thread_data *)arg;
    // Calculate the value of the neuron using the weights and values from the previous layer
    double val = 0.0;
    for (int j = 0; j < data->size; j++)
    {
        val += (data->layer1[j] * data->weights[j][data->i]);
        if (std::abs(val) < 1e-9)
            val = 0.0;
    }

    // Store the calculated value in the global layer2 array
    layer2[data->i] = val;

    // Increment the thread_lock variable to signal that the thread has finished executing
    thread_lock++;

    // Exit the thread
    pthread_exit(0);
}

// Define a function to write the values of the second layer of neurons to a named pipe
void write_pipe(int &layer_size)
{
    // Define the name of the named pipe
    const char *pipe_name = "my_pipe";

    // Create the named pipe with permissions
    int res = mkfifo(pipe_name, 0666);
    if (res < 0)
        cerr << "Error creating pipe!\n";

    // Open the named pipe for reading and writing
    int fd = open(pipe_name, O_RDWR);
    if (fd < 0)
        cerr << "Error opening pipe for read/write!\n";

    // Iterate over the values of the second layer of neurons and write them to the named pipe
    string message = {};
    for (int i = 0; i < layer_size; i++)
    {
        message = to_string(layer2[i]);
        message += ',';
        if (write(fd, message.c_str(), message.length()) <= 0)
            cout << "Writing failed" << endl;
        message = {};
    }

    // Write a null character to signal the end of the message
    const char ch = '\0';
    write(fd, &ch, 1);
}

int main(int argc, char *argv[])
{
    if (argc != 5)
    {
        fprintf(stderr, "Usage: program2 int_var1 int_var2 bool_var\n"); // print usage message if incorrect number of arguments are provided
        exit(EXIT_FAILURE);
    }
    int size = atoi(argv[1]);
    int count = atoi(argv[2]);
    bool dir = atoi(argv[3]) != 0;
    bool rep = atoi(argv[4]) != 0;
    char var1_str[10]{}, var2_str[10]{}, var3_str[6]{}, var4_str[6]{};

    if (dir)
    {
        // Print to console that the output file has been reached.
        cout << "Output File Reached" << endl;
        // Initialize the layer size to 1 (the output size)
        int layer_size = 1;

        // Initialize thread lock to 0
        thread_lock = 0;

        // Define the file path to the output weights.
        string weights = "Output_Weights.txt";

        // Read values from the pipe.
        vector<double> values_prv = read_pipe();

        // Allocate memory for 2D array of weights.
        double **weight = Populate_Input_2dArray<double>(weights, values_prv.size(), layer_size);

        // Create an array of threads.
        pthread_t *neurons = new pthread_t[1];

        // Initialize the layer2 array.
        layer2 = new double[1];

        // Loop over the layer_size.
        for (int i = 0; i < layer_size; i++)
        {
            // Create a new integer pointer 'val' and set its value to i.
            int *val = new int;
            *val = i;

            // Create a new thread_data struct 'data'.
            thread_data *data = new thread_data;

            // Set the 'i', 'layer1', 'size', and 'weights' variables of 'data'.
            data->i = *val;
            data->layer1 = values_prv;
            data->size = values_prv.size();
            data->weights = weight;

            // Create a new thread and pass in 'data'.
            pthread_create(&neurons[i], NULL, layer_func, (void *)data);
        }

        // Print an empty line.
        cout << endl;

        // Wait until all threads are finished.
        while (thread_lock < layer_size)
            ;

        // If 'rep' variable is true.
        if (rep)
        {
            // Define the name of the named pipe.
            const char *pipe_name = "my_pipe";

            // Create the named pipe with permissions.
            int res = mkfifo(pipe_name, 0666);

            // If an error occurs while creating the pipe, print an error message.
            if (res < 0)
                cerr << "Error creating pipe!\n";

            // Open the pipe for reading and writing.
            int fd = open(pipe_name, O_RDWR);

            // If an error occurs while opening the pipe, print an error message.
            if (fd < 0)
                cerr << "Error opening pipe for read/write!\n";

            // Define an empty string 'message'.
            string message = {};

            // Loop over the layer_size.
            for (int i = 0; i < layer_size; i++)
            {
                // Convert the value in layer2[i] to a string and append ',' to 'message'.
                message = to_string(layer2[i]);
                message += ',';

                // Write 'message' to the pipe.
                if (write(fd, message.c_str(), message.length()) <= 0)
                    cout << "Writing failed" << endl;

                // Reset 'message' to an empty string.
                message = {};
            }

            // Define a variable to store the new input.
            double new_input = {};

            // Print the output at the first traverse to the console.
            cout << " The Output at 1st Traverse is : " << layer2[0] << endl;

            // Print the next two inputs generated to the console.
            cout << " The Next 2 Inuts Generated are : " << endl;
            cout << " The Next 2 Inuts Generated are : " << endl;
            new_input = generate_input1(layer2[0]);
            cout << "1. " << new_input << endl;
            message = to_string(new_input);
            message += ',';
            new_input = generate_input2(layer2[0]);
            cout << "1. " << new_input << endl;
            message += to_string(new_input);
            message += ',';
            if (write(fd, message.c_str(), message.length()) <= 0)
                cout << "Writing failed" << endl;
            const char ch = '\0';
            write(fd, &ch, 1);
            // cout << message << endl;
        }
        // This section is using snprintf to convert integer values to strings and store them in respective variables
        snprintf(var1_str, sizeof(var1_str), "%d", size);
        count = size - 1;
        snprintf(var2_str, sizeof(var2_str), "%d", count);
        dir = false;
        snprintf(var3_str, sizeof(var3_str), "%d", dir);
        snprintf(var4_str, sizeof(var4_str), "%d", rep);

        // This section is flushing the output stream and executing the 'hidden' program with arguments based on the value of rep variable
        cout.flush();
        if (rep)
        {
            rep = false;
            snprintf(var4_str, sizeof(var4_str), "%d", rep);
            const char *const argv[] = {"hidden", var1_str, var2_str, var3_str, var4_str, NULL};
            execv("hidden", const_cast<char *const *>(argv));
        }
        else // This section is executed if the rep variable is false
        {
            cout.flush();
            cout << " The Final Output is : " << layer2[0] << endl;
            cout.flush();
            unlink("my_pipe");
            pthread_exit(0);
        }

        // This section is printing an error message if the execv() function call fails
        perror("execv");
    }
}

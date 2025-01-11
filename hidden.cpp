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

using namespace std;

double *layer2;       // Global pointer to the output of the neural network layer
int thread_lock = 0;  // Counter for the number of threads that finished processing
pthread_mutex_t lock; // A mutex lock to synchronize access to shared resources between threads

// A struct to pass data to the thread function
struct thread_data
{
    double **weights;       // The weight matrix for the neural network layer
    vector<double> *layer1; // The input vector to the neural network layer
    int i;                  // The index of the output neuron that the thread is responsible for computing
    int size;               // The size of the input vector to the neural network layer
};

// The function that each thread will execute
void *layer_func(void *arg)
{
    // Acquire the lock before accessing shared resources
    pthread_mutex_lock(&lock);

    // Cast the argument back to the thread data struct
    thread_data *data = (thread_data *)arg;

    // Compute the output of the neuron assigned to this thread
    double val = 0.0;
    for (int j = 0; j < data->size; j++)
    {
        val += (data->layer1->at(j) * data->weights[j][data->i]);
        if (std::abs(val) < 1e-9)
            val = 0.0;
    }

    // Write the output of this neuron to the global output layer
    layer2[data->i] = val;

    // Increment the thread lock counter to signal that this thread has finished
    thread_lock++;

    // Release the lock after accessing shared resources
    pthread_mutex_unlock(&lock);

    // Exit the thread
    pthread_exit(0);
}

// A function to write the output of the neural network layer to a named pipe
void write_pipe(int &layer_size)
{
    const char *pipe_name = "my_pipe";

    // Create the named pipe with the specified permissions
    int res = mkfifo(pipe_name, 0666);
    if (res < 0)
        cerr << "Error creating pipe!\n";

    // Open the named pipe for reading and writing
    int fd = open(pipe_name, O_RDWR);
    if (fd < 0)
        cerr << "Error opening pipe for read/write!\n";

    string message = {};

    // Write the output of each neuron in the layer to the named pipe
    for (int i = 0; i < layer_size; i++)
    {
        message = to_string(layer2[i]);
        message += ',';
        if (write(fd, message.c_str(), message.length()) <= 0)
            cout << "Writing failed" << endl;
        message = {};
    }

    // Write a null character to signal the end of the output
    const char ch = '\0';
    write(fd, &ch, 1);
}

int main(int argc, char *argv[])
{
    if (argc != 5)
    {
        fprintf(stderr, "Usage: program2 int_var1 int_var2 bool_var\n");
        exit(EXIT_FAILURE);
    }
    // Converting the first command line argument to an integer and storing it in size variable
    int size = atoi(argv[1]);

    // Converting the second command line argument to an integer and storing it in count variable
    int count = atoi(argv[2]);

    // Converting the third command line argument to a boolean value and storing it in dir variable
    bool dir = atoi(argv[3]) != 0;

    // Converting the fourth command line argument to a boolean value and storing it in rep variable
    bool rep = atoi(argv[4]) != 0;

    // Creating four char arrays to store integer and boolean values in string format
    char var1_str[10]{}, var2_str[10]{}, var3_str[6]{}, var4_str[6]{};

    // If dir is true, then the program is executing for a forward pass in neural network
    if (dir)
    {
        // Initializing a mutex lock
        pthread_mutex_init(&lock, NULL);

        // Setting the size of the layer
        int layer_size = 8;
        cout << " Enter the Number of neurons for Layer " << count << " : ";
        cin >> layer_size;

        // Setting the weights file name based on the count variable
        string weights = "Weights";
        weights += to_string(count - 1);
        weights += ".txt";
        if (count == 1)
            weights = "Input.txt";

        // Creating a named pipe and reading data from it
        const char *pipe_name = "my_pipe";
        vector<double> values_prv = read_pipe();

        // Populating a 2D array with weight values from the file
        double **weight = Populate_Input_2dArray<double>(weights, values_prv.size(), layer_size);

        // Creating threads for each neuron in the layer
        pthread_t *neurons = new pthread_t[layer_size];
        layer2 = new double[layer_size];
        for (int i = 0; i < layer_size; i++)
        {
            int *val = new int;
            *val = i;
            thread_data *data = new thread_data;
            data->i = *val;
            data->layer1 = &values_prv;
            data->size = values_prv.size();
            data->weights = weight;
            pthread_create(&neurons[i], NULL, layer_func, (void *)data);
        }

        // Waiting for all threads to complete execution
        while (thread_lock < layer_size)
            ;

        // Writing data to the named pipe
        write_pipe(layer_size);

        // Converting integer and boolean values to string format
        snprintf(var1_str, sizeof(var1_str), "%d", size);
        ++count;
        snprintf(var2_str, sizeof(var2_str), "%d", count);
        snprintf(var3_str, sizeof(var3_str), "%d", dir);
        snprintf(var4_str, sizeof(var4_str), "%d", rep);

        // If count is less than or equal to size, then execute the hidden program with new arguments
        if (count <= size)
        {
            const char *const argv[] = {"hidden", var1_str, var2_str, var3_str, var4_str, NULL};
            execv("hidden", const_cast<char *const *>(argv));
        }
        else
        {
            const char *const argv[] = {"output", var1_str, var2_str, var3_str, var4_str, NULL};
            execv("output", const_cast<char *const *>(argv));
        }
        perror("execv");
        exit(EXIT_FAILURE);
    }
    else
    {
        // Else go back to the previous hidden layer using execv
        string weights = "Hidden";
        weights += to_string(count + 1);
        cout << " BAck to hidden layer : " << weights;

        // Reading the pipe to get the previous hidden layer inputs
        vector<double> values_prv = read_pipe();

        // Printing the newly generated inputs
        cout << endl
             << " The newly generated inputs are : " << endl;
        for (int i = 1; i < values_prv.size(); i++)
            cout << values_prv[i] << ' ';
        cout << endl;

        // Writing the previous hidden layer inputs to the pipe
        write_pipe(values_prv);

        // Converting the integer values to char arrays
        snprintf(var1_str, sizeof(var1_str), "%d", size);
        count--;

        // Converting the updated count value to char array
        snprintf(var2_str, sizeof(var2_str), "%d", count);

        // Setting the direction and repetition flags to false
        bool dir = false;
        bool rep = false;

        // Converting the boolean flag values to char arrays
        snprintf(var3_str, sizeof(var3_str), "%d", dir);
        snprintf(var4_str, sizeof(var4_str), "%d", rep);

        // Calling the next hidden layer using execv
        if (count >= 0)
        {
            const char *const argv[] = {"hidden", var1_str, var2_str, var3_str, var4_str, NULL};
            execv("hidden", const_cast<char *const *>(argv));
        }
        else // Else call the main function using execv
        {
            const char *const argv[] = {"main", var1_str, var2_str, var3_str, var4_str, NULL};
            execv("main", const_cast<char *const *>(argv));
        }
    }

    // Exit the pthread
    pthread_exit(0);
}

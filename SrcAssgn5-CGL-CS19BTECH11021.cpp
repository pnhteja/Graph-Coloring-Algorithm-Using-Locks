/* Header Files */
#include<iostream>
#include<fstream>
#include<sstream>
#include<thread>
#include<pthread.h>
#include<vector>
#include<set>
#include<chrono>
#include<ctime>
#include<cstdlib>


using namespace std;


/* Defining type aliases */
typedef chrono::system_clock SystemClock;
typedef chrono::duration <double> Duration;
typedef chrono::microseconds Microseconds;


/* Defining Node class */
class Node
{
    public:
    
    int vertexNumber;
    int color;
    int partitionNumber;
    
    /* Constructor */
    Node ( int v )
    {
        vertexNumber = v;
        color = 0;
        partitionNumber = -1;
    }
};


/* Global Variables */

int k;                              /* Number of threads / partitions */
int n;                              /* Number of vertices */

vector<Node> vertices;              /* Vector of nodes */
vector<vector<int>> partitions;

pthread_mutex_t mutex;              /* Mutex Lock */

void colorPartition ( int index );  /* Thread executes this function to color a partition */

bool adjMatrix [10000][10000];          /* Adjacency matrix */

int main (void)
{
    /* Stream class object to read from files */
    ifstream inputFile;
    
    /* Opening the "inp_params.txt" file */
    inputFile.open("input_params.txt");
    
    /* If "input_params.txt" cannot be opened, exit with return status 1 */
    if ( !inputFile ) {
        cout << "input_params.txt cannot be opened !!" << endl;
        exit(1);
    }
    
    /* Stream class object to write to files */
    ofstream outputFile;
    
    /* If "output.txt" cannot be opened, exit with return status 1 */
    outputFile.open("output.txt", ios::out | ios:: trunc );
    
    /* If "output.txt" cannot be opened, exit with return status 1 */
    if ( !outputFile ) {
        cout << "output.txt cannot be opened" << endl;
        exit(1);
    }
    
    /* Reading k, n */
    inputFile >> k >> n;
    
    /* Variable to read the vertex numbers in input */
    int vertex_number;
    
    /* Reading vertex numbers */
    for ( int i = 0 ; i < n ; ++i ){
        inputFile >> vertex_number;
    }
    
    /* Reading adjacency matrix entries */
    for ( int i = 0 ; i <= n-1 ; ++i ){
        
        inputFile >> vertex_number;
        
        for ( int j = 0 ; j <= n-1 ; ++j ){
            inputFile >> adjMatrix[i][j];
        }
        
        vertices.push_back(Node(i));
        
        /* Initializing */
        if ( i < k )
            partitions.push_back(vector<int>{});
    }
    
    /* Variables used for generating k partitions */
    int remVertices = n;
    int remPartitions = k;
    int partitionLength = 0;
    
    vertex_number = 0;
    
    /* Seeding the rand functrion */
    srand(time(NULL));
    
    /* Generating k-1 partitions */
    while ( remPartitions != 1 ){
        
        /* Selecting partition length */
        do {
            partitionLength = rand() % n + 1;
        } while ( remVertices - partitionLength < remPartitions - 1 );
        
        
        int count = partitionLength;
        
        /* Selecting partition elements randomly */
        while ( count != 0 ){
            
            vertex_number = rand() % n;
            
            /* If already not assigned to a partition */
            if ( vertices[vertex_number].partitionNumber == -1 ){
                partitions[k - remPartitions].push_back(vertex_number);
                vertices[vertex_number].partitionNumber = k - remPartitions;
            }
            
            --count;
        }
        
        /* Updating variables */
        remVertices -= partitionLength;
        remPartitions -= 1;
    }
    
    /* Assinging remaining elements to last partition */
    for ( int i = 0 ; i <= n-1 ; ++i ){
        if ( vertices[i].partitionNumber == -1 ){
            partitions[k-1].push_back(i);
            vertices[i].partitionNumber = k - remPartitions;
        }
    }
    
    
    /*
    for ( int i = 0 ; i <= k-1 ;++i ){
        for ( int j = 0 ; j < partitions[i].size() ; ++j ){
            cout << partitions[i][j] << " ";
        }
        cout << endl;
    }
    
    for ( int i = 0 ; i < n ; ++i ){
        cout << "V" << i << " - " << vertices[i].partitionNumber << endl;
    }
    */
    
    /* Creating a vector for threads */
    vector<thread> Threads;
    
    /* Initializing the mutex lock */
    pthread_mutex_init (&mutex, NULL);
    
    /* Storing the starting time in Microseconds*/
    auto startTime = chrono::duration_cast<Microseconds>(SystemClock::now().time_since_epoch()).count();
    
    /* Creating threads to color each partition */
    for ( int i = 0 ; i <= k-1 ; ++i ){
        Threads.push_back(thread( colorPartition, i ));
    }
    
    /* Waiting for threads to complete */
    for ( int i = 0 ; i <= k-1 ; ++i ){
        Threads[i].join();
    }
    
    /* Stroing the end time */
    auto endTime = chrono::duration_cast<Microseconds>(SystemClock::now().time_since_epoch()).count();
    
    outputFile << "Coarse-Grained Lock" << endl;
    
    /* Variable for storing total number of colors */
    int totalColors = 0;
    
    stringstream ss;
    
    ss << "Colours: " << endl;
    
    /* Calculating total number of colors and printing color of each vertex in string stream */
    for ( int i = 0 ; i < n ; ++i ){
        
        if ( i != n-1 )
            ss << "v" << i+1 << " - " << vertices[i].color << ", ";
        
        else
            ss << "v" << i+1 << " - " << vertices[i].color << endl;
        
        if ( totalColors < vertices[i].color )
            totalColors = vertices[i].color;
    }
    
    outputFile << "No of colours used: " << totalColors << endl;
    outputFile << "Time taken by the algorithm using: " << (double)(endTime - startTime)/1000 << " Milliseconds" << endl;
    outputFile << ss.str() << endl;

    /* Closing all files */
    inputFile.close();
    outputFile.close();
    
    return 0;
}


/* A thread starts execution from here and starts coloring it's assigned partition */
void colorPartition ( int index )
{
    /* To store colors of adjacent vertices */
    set <int> colorsUsed;
    
    /* To store external vertices of a partition */
    vector <int> externalVertices;
    
    /* Represents wether a vertex is internal or external */
    bool isInternal = false;
    
    /* Traversing through the partiton */
    for ( int i = 0 ; i < partitions[index].size() ; ++i ) {
        
        /* Traversing through it's row in adjacency matrix */
        for ( int j = 0 ; j < n ; ++j ) {
            
            if ( partitions[index][i] != j ) {
                
                /* If there's an edge */
                if ( adjMatrix[partitions[index][i]][j] ) {

                    /* If present in same partition */
                    if ( vertices[j].partitionNumber == index ) {

                        /* If colored */
                        if ( vertices[j].color != 0 )
                            colorsUsed.insert(vertices[j].color);
                    }
                    
                    else {
                        isInternal = false;
                        break;
                    }
                }
            }
            
            /* If all neighbours present in same partition */
            if ( j == n-1 )
                isInternal = true;
        }
        
        
        /* If Internal Vertex then it is colored */
        if ( isInternal ){
            
            int Colour = 1;
            bool colored = false;
            
            /* Assigning color 1 to vertex */
            if ( colorsUsed.empty() )
                vertices[partitions[index][i]].color = Colour;
            
            else  {
                
                /* Traversing thorugh the set */
                for ( auto it = colorsUsed.begin() ; it != colorsUsed.end() ; ++it ){
                    
                    /* If Colour already used */
                    if ( Colour == *it )
                        ++Colour;
                        
                    else {
                        
                        /* Assigning colour */
                        vertices[partitions[index][i]].color = Colour;
                        colored = true;
                        break;
                    }
                    
                    /* If all colours used then new colour is used */
                    if ( !colored )
                        vertices[partitions[index][i]].color = Colour;
                }
            }
        }
        
        /* If external vertex then added to list of external vertices */
        else 
            externalVertices.push_back(partitions[index][i]);
        
        /* Claring the set */
        colorsUsed.clear();
    }
    
    /* Using global lock to color external vertices */
    pthread_mutex_lock(&mutex);
    
    /* Traversing through the external vertices vector */
    for ( int i = 0 ; i < externalVertices.size() ; ++i ){
        
        /* Finding colors used to color it's neighbours */
        for ( int j = 0 ; j < n ; ++j ){
            
            /* If edge is present and is colored */
            if ( adjMatrix[externalVertices[i]][j] && vertices[j].color != 0 )
                    colorsUsed.insert(vertices[j].color);
        }
        
        
        int Colour = 1;
        bool colored = false;
        
        /* Assigning color 1 to vertex */
        if ( colorsUsed.empty() ){
            vertices[externalVertices[i]].color = Colour;
            colored = true;
        }
        
        else {
            
            /* Traversing thorugh the set */
            for ( auto it = colorsUsed.begin() ; it != colorsUsed.end() ; ++it ){
                
                /* If Colour already used */
                if ( Colour == *it )
                    ++Colour;
                    
                else {
                    
                    /* Assigning colour */
                    vertices[externalVertices[i]].color = Colour;
                    colored = true;
                    break;
                }
            }
            
            /* If all colours used then new colour is used */
            if ( !colored )
                vertices[externalVertices[i]].color = Colour;
        }
        
        /* Clearing the set */
        colorsUsed.clear();
    }
    
    /* Releasing the global lock */
    pthread_mutex_unlock(&mutex);
}

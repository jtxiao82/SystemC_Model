#define SC_INCLUDE_FX
#include "systemc"
#include <iomanip>
#include <vector>

#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include <sstream>
#include <errno.h>
#include <error.h>

#define SERVER_FIFO "/tmp/addition_fifo_server"
#define CLIENT_FIFO "/tmp/add_client_fifo"
#define MAX_NUMBERS 500

using namespace sc_dt;
using namespace std;
using namespace sc_core;

// patch namespace is used to convert input to string
namespace patch
{
    template < typename T > std::string to_string( const T& n )
    {
        std::ostringstream stm ;
        stm << n ;
        return stm.str() ;
    }
}

SC_MODULE(Input) {
  SC_CTOR(Input):count(0) {
    SC_THREAD(input);
      sensitive << i_clk.pos();
  }

    vector<double> IPC_Server() {
      int fd, fd_client, bytes_read, i;
    
      // Use buf to store parameters and receive from another proccess
      char buf [4096];
    
      char *return_fifo;
      char *numbers [MAX_NUMBERS];
    
      // Create fifo (a new file at local machine)
      if ((mkfifo (SERVER_FIFO, 0664) == -1) && (errno != EEXIST)) {
        perror ("mkfifo");
        exit (1);
      }
    
      // Open Server fifo file
      if ((fd = open (SERVER_FIFO, O_RDONLY)) == -1)
        perror ("open");
    
      while (1) {
        // get a message
        memset (buf, '\0', sizeof (buf));
    
        // Use read command to receive datas from another proccess and store in buf
        bytes_read = read (fd, buf, sizeof (buf));
    
        if (bytes_read > 0) {
          return_fifo = strtok (buf, ", \n");
          i = 0;
          numbers [i] = strtok (NULL, ", \n");
    
          std::vector<double> param;
    
          while (numbers [i] != NULL && i < MAX_NUMBERS) {
            //printf ("Number%d: %s\n", i, numbers[i]);
    
            // Push recevie datas at a vector
            param.push_back(atof(numbers[i]));
            numbers [++i] = strtok (NULL, ", \n");
          }
     
          // Debug for observe receive data size
          std::cout << param.size() <<std::endl;
    
          // Open Server fifo file
          if ((fd_client = open (return_fifo, O_WRONLY)) == -1) {
              perror ("open: client fifo");
              continue;
          }
    
          // Use write command to notify another proccess
          // Represent the it has receive successfully
          if (write (fd_client, buf, strlen (buf)) != strlen (buf))
              perror ("write");
       
          // Close server fifo file
          if (close (fd_client) == -1)
              perror ("close");   
    
          return param;
    
        }
      }
    }

    void input() {
      // First cycle 
      //   Receives param0
      // Second cycle 
      //   Receives param1 
      //   Decide end cycles
      //   Send param0 and param1 to fifo to other module
      //   Send end cycle to other module
      //   Wait for next cycle
      // Each cycle send a pair of parameters to other module

      while(1) {
        o_input_valid.write(false);
        o_done.write(false);

        // First cycle
        if(count == 0) {

          // Receive parameters at first cycles
          param_0 = IPC_Server();
        } else if(count == 1) {

          // Receive parameters at second cycles
          param_1 = IPC_Server();

          // Decide end cycle
          // End cycles are the maximum between param_0 and param_1       
          if(param_0.size() >= param_1.size())
            end_count = param_0.size();
          else 
            end_count = param_1.size();
        }
 
        // Second cycle
        if(count >= 1) {
          // Decide which params dimension is bigger
          // Need to decide which parameter is constant
          // and which parameter is 1D array
          if(param_0.size() < param_1.size()) {
            // Param0 is a constant
            // Param1 is a 1D array
            o_param0_channel.write(param_0[0]);
            o_param1_channel.write(param_1[0+count-1]);
            o_end_count_channel.write(end_count);
          } else {
            // Param0 is 1D array
            // Param1 is a constant
            o_param0_channel.write(param_0[0+count-1]);
            o_param1_channel.write(param_1[0]);
            o_end_count_channel.write(end_count);
          }
        }
        o_input_valid.write(true);
        do {
          wait();
        } while(i_input_ready.read() == false);

        if(count == end_count && count >= 1) { // End count
          //unlink(SERVER_FIFO);
          //unlink(CLIENT_FIFO);
          sc_stop();
        }
        o_done.write(true);
        count++;
      }
    }

  public:
    int count;
    double test;
    double param1;
    int end_count;
 
    vector<double> param_0;
    vector<double> param_1;
    
    sc_in_clk i_clk;
    
    sc_out<float> o_param0_channel;
    sc_out<int> o_end_count_channel;
    sc_out<float> o_param1_channel;
    
    sc_out< bool > o_input_valid;
    sc_in< bool > i_input_ready;
    sc_out <bool> o_done;
    sc_in <bool> i_done;
};

SC_MODULE(Cal) {
  SC_CTOR(Cal): count(0) {
    SC_THREAD(calculate);
      sensitive << i_clk.pos();
    SC_THREAD(mul);
      sensitive << i_clk.pos();
  }
    void IPC_Client(std::vector<float> param) {
      // IPC Client here
      // Need to transfer every parameters to Server
      // parameter array to buffer test
    
      // Feed char to my_fifo_name
      char my_fifo_name [128];
      
      // Use buf2 to store parameters and send to another proccess
      // The char array size will demonate the communication throughput
      char buf2 [4096];
    
      // Suppose allocate fifo at location "/tmp/add_client_fifo6813"
      int id = 6813;
      sprintf (my_fifo_name, "/tmp/add_client_fifo%d", id);
    
      // Create fifo (a new file at local machine)
      if (mkfifo (my_fifo_name, 0664) == -1)
        perror ("mkfifo");
    
      int fd_server, fd, bytes_read;
    
      // Use v[] to store parameters and string cat to buf2
      // The char array size will demonate the communication throughput
      char v[32];
    
      // Send parameters to char buffers
      double integer = param[0];
      sprintf(v, "%2.3f", integer);
    
      char const *pchar;
      for(int i=1; i<param.size(); i++) {
        strcat (v," ");
        std::string s = patch::to_string(param[i]);
        pchar = s.c_str();
        strcat (v,pchar);
      }
      strcpy (buf2, my_fifo_name);
      strcat (buf2, " ");
      strcat (buf2, v);
    
      // Open Server fifo file
      if ((fd_server = open (SERVER_FIFO, O_WRONLY)) == -1) {
          perror ("open: server fifo");
      }
    
      // Use write command to send buf2 to another proccess
      if (write (fd_server, buf2, strlen (buf2)) != strlen (buf2)) {
          perror ("write");
      }
    
      // Close the Serveri fifo file
      if (close (fd_server) == -1) {
          perror ("close");
      }    
    
      // Open Server fifo file
      if ((fd = open (SERVER_FIFO, O_RDONLY)) == -1)
         perror ("open");
    
      // Should it be removed ?
      memset (buf2, '\0', sizeof (buf2));
    
      // Use read command to read the datas from another proccess
      // This function call "read()" will blocking this proccess until the datas are be read !
      if ((bytes_read = read (fd, buf2, sizeof (buf2))) == -1)
          perror ("read");
    
      // Open Server fifo file
      if (close (fd) == -1) {
          perror ("close");
      }
     
      // Remove the server fifo file at local 
      unlink(my_fifo_name);
    }

    void calculate() {
      // Each cycle
      //   Receive param0, param1 and end cycle
      while(1) {

        o_cal_ready.write(true);
        o_done.write(false);
        do {
          wait();
        } while(i_cal_valid.read() == false);
        o_cal_ready.write(false);

        // Read parameters and end cycles from fifo
        param0[0] = i_param0_channel.read();
        param1[0] = i_param1_channel.read();
        end_count = i_end_count_channel.read();

        // Debug information
        if(count >= 1) {
          cout << "Param0: " << param0[0] << endl;
          cout << "Param1: " << param1[0] << endl;
        }
        o_done.write(true);
        event.notify();
      }
    }

    void mul() {
      while(1) {
        wait(event);

        // Counter
        count++;

        // Do mulplixer
        result = param0[0] * param1[0];

        // Counter should be bigger than 2
        // output will store all result which is param0 * param1
        if(count >= 2) { 
	  cout << "Result: " << result << endl;
          output.push_back(result);
        }

        // Reach to "End cycle + 1" means end of calculation
        if (count >= 2 && count == end_count+1) { 
          // Use IPC to send output to another proccess
          IPC_Client(output);
        }
        cout << endl;
      }
    }


  public:
    int count;
    int end_count;
    float param0[5];
    float param1[5];
    float result;
    vector<float> output;

    sc_in_clk i_clk;

    sc_in<float> i_param0_channel;
    sc_in<float> i_param1_channel;
    sc_in<int> i_end_count_channel;

    sc_event event;
  
    sc_in< bool > i_cal_valid;
    sc_out< bool > o_cal_ready;
    sc_out< bool > o_done;
    sc_in< bool > i_done;
};


SC_MODULE(Top)
{
  Input *input;
  Cal *cal;
  SC_CTOR(Top)
  {
    for(int i=0;i<1;i++)
    {
      input= new Input ("Input");
      cal= new Cal ("Cal");
    }
    input->i_clk(clk);
    input->o_param0_channel(param0_channel);
    input->o_param1_channel(param1_channel);
    input->o_end_count_channel(end_count_channel);
    input->o_input_valid( input_valid );
    input->i_input_ready( input_ready );
    input->o_done( input_done );
    input->i_done( cal_done );

    cal->i_clk(clk);
    cal->i_done(input_done);
    cal->i_param0_channel(param0_channel);
    cal->i_param1_channel(param1_channel);
    cal->i_end_count_channel(end_count_channel);
    cal->i_cal_valid(input_valid);
    cal->o_cal_ready(input_ready);
    cal->o_done(cal_done);
  }
  
  public:
    sc_signal<float> param0_channel;
    sc_signal<float> param1_channel;
    sc_signal<int> end_count_channel;

    sc_clock clk;
  
    sc_signal< bool > input_valid;
    sc_signal< bool > input_ready;
    sc_signal< bool > input_done;

    sc_signal< bool > cal_valid;
    sc_signal< bool > cal_ready;
    sc_signal< bool > cal_done;
    
};

int sc_main(int argc,char *argv[])
{
  Top top("top");
  sc_start();
  return 0;
}

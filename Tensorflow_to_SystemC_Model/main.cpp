#define SC_INCLUDE_FX
#include "systemc"
#include <iomanip>
#include <vector>

#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>


#include <errno.h>
#include <error.h>

#define SERVER_FIFO "/tmp/addition_fifo_server"
#define CLIENT_FIFO "/tmp/add_client_fifo"
#define MAX_NUMBERS 500

using namespace sc_dt;
using namespace std;
using namespace sc_core;

SC_MODULE(Input) {
  SC_CTOR(Input):count(0) {
    SC_THREAD(input);
      sensitive << i_clk.pos();
  }

    vector<double> IPC_Server() {
      int fd, fd_client, bytes_read, i;
      char buf [4096];
      char *return_fifo;
      char *numbers [MAX_NUMBERS];

      if ((mkfifo (SERVER_FIFO, 0664) == -1) && (errno != EEXIST)) {
        perror ("mkfifo");
        exit (1);
      }
      if ((fd = open (SERVER_FIFO, O_RDONLY)) == -1)
        perror ("open");

      while (1) {
        // get a message
        memset (buf, '\0', sizeof (buf));
        if ((bytes_read = read (fd, buf, sizeof (buf))) == -1)
          perror ("read");
        if (bytes_read == 0)
          continue;

        if (bytes_read > 0) {
          return_fifo = strtok (buf, ", \n");

          i = 0;
          numbers [i] = strtok (NULL, ", \n");

          vector<double> param;
          while (numbers [i] != NULL && i < MAX_NUMBERS) {
            printf ("Number%d: %s\n", i, numbers[i]);
            param.push_back(atof(numbers[i]));
            numbers [++i] = strtok (NULL, ", \n");
          }
          
          return param;
        }
      }
    }

    void input() {
      while(1) {
        o_input_valid.write(false);
        o_done.write(false);
        if(count == 0) {
          param_0 = IPC_Server();

        } else if(count == 1) {
          param_1 = IPC_Server();
          if(param_0.size() >= param_1.size())
            end_count = param_0.size();
          else 
            end_count = param_1.size();
        }
        if(count >= 1) {
          if(param_0.size() < param_1.size()) {
            o_param0_channel.write(param_0[0]);
            o_param1_channel.write(param_1[0+count-1]);
          } else {
            o_param0_channel.write(param_0[0+count-1]);
            o_param1_channel.write(param_1[0]);
          }
        }
        o_input_valid.write(true);
        do {
          wait();
        } while(i_input_ready.read() == false);

        if(count == end_count && count >= 1) { // End count
          unlink(SERVER_FIFO);
          unlink(CLIENT_FIFO);
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

    void calculate() {
      while(1) {

        o_cal_ready.write(true);
        o_done.write(false);
        do {
          wait();
        } while(i_cal_valid.read() == false);
        o_cal_ready.write(false);

        param0[0] = i_param0_channel.read();
        param1[0] = i_param1_channel.read();
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
        count++;
        result = param0[0] * param1[0];
        if(count >= 2) // Counter should be bigger than 2
	  cout << "Result: " << result << endl;
          cout << endl;
      }
    }


  public:
    int count;
    float param0[5];
    float param1[5];
    float result;

    sc_in_clk i_clk;

    sc_in<float> i_param0_channel;
    sc_in<float> i_param1_channel;

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
    input->o_input_valid( input_valid );
    input->i_input_ready( input_ready );
    input->o_done( input_done );
    input->i_done( cal_done );

    cal->i_clk(clk);
    cal->i_done(input_done);
    cal->i_param0_channel(param0_channel);
    cal->i_param1_channel(param1_channel);
    cal->i_cal_valid(input_valid);
    cal->o_cal_ready(input_ready);
    cal->o_done(cal_done);
  }
  
  public:
    sc_signal<float> param0_channel;
    sc_signal<float> param1_channel;

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

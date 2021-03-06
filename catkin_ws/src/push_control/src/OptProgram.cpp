#include <iostream>
#include <dlfcn.h>
#include <vector>
#include <stdio.h>
#include <unistd.h>
#include <ctime>
#include <math.h>
#include <pthread.h>
#include <fstream>
#include <string>
#include <Eigen/Dense>
#include "gurobi_c++.h"
#include <fstream>
#include <memory>
#include <cstdlib>
#include "pushing.h"
#include "ABBRobot.h"
#include "OptProgram.h"
#include "helper.h"
#include <time.h>
#include <iomanip>
#include <sys/time.h>
#include <sys/resource.h>

#include "PracticalSocket/PracticalSocket.h" // For UDPSocket and SocketException
#include "egm.pb.h" // generated by Google protoc.exe
#include "tf2_msgs/TFMessage.h"
#include "tf/LinearMath/Transform.h"

#include <ros/ros.h>
#include "tf/tf.h"
#include <tf/transform_datatypes.h>
#include <tf/transform_listener.h>
#include "geometry_msgs/Twist.h"
#include "geometry_msgs/WrenchStamped.h"
#include "std_msgs/String.h"

using namespace abb::egm;
using namespace tf;
using namespace std;
using Eigen::MatrixXd;



//********************************************************************
// Optimization Thread
//********************************************************************
void *rriMain(void *thread_arg)
{
    struct thread_data *my_data;
    my_data = (struct thread_data *) thread_arg;

    //Define variables from argument pointers
    pthread_mutex_lock(&nonBlockMutex);
    MatrixXd *pq_slider = my_data->_q_slider;
    MatrixXd *pdq_slider= my_data->_dq_slider;
    MatrixXd *pq_pusher = my_data->_q_pusher;
    MatrixXd *pTarget = my_data->_Target;
    MatrixXd *pu_control = my_data->_u_control;
    MatrixXd *pap = my_data->_ap;
    double *ptang_vel = my_data->_tang_vel ;

    MatrixXd &q_slider = *pq_slider;
    MatrixXd &dq_slider = *pdq_slider;
    MatrixXd &q_pusher = *pq_pusher;
    MatrixXd &Target = *pTarget;
    MatrixXd &u_control = *pu_control;
    MatrixXd &ap = *pap;
    double &tang_vel  = *ptang_vel ;
    pthread_mutex_unlock(&nonBlockMutex);

    //Define local variables
    double fval1;
    double fval2;
    double fval3;
    double t_init;
    double T_des, delta_t, t_ini;
    double step_size = 1.f/100;
    int minIndex, maxCol;
    float min;

    //Define local matrix variables
    MatrixXd fval(3,1);
    MatrixXd _q_slider_(3,1);
    MatrixXd _dq_slider_(3,1);
    MatrixXd _q_pusher_(2,1);
    MatrixXd _Target_(2,1);

    //Define text file string variables
    char Q_str[] = "/home/mcube/cpush/catkin_ws/src/push_control/data/H.txt";
    char Abar_str[] = "/home/mcube/cpush/catkin_ws/src/push_control/data/A_bar.txt";
    char Ain_stick_str[] = "/home/mcube/cpush/catkin_ws/src/push_control/data/Ain_stick.txt";
    char bin_stick_str[] = "/home/mcube/cpush/catkin_ws/src/push_control/data/bin_stick.txt";
    char Aeq_stick_str[] = "/home/mcube/cpush/catkin_ws/src/push_control/data/Aeq_stick.txt";
    char beq_stick_str[] = "/home/mcube/cpush/catkin_ws/src/push_control/data/beq_stick.txt";
    char Ain_up_str[]    = "/home/mcube/cpush/catkin_ws/src/push_control/data/Ain_up.txt";
    char bin_up_str[]    = "/home/mcube/cpush/catkin_ws/src/push_control/data/bin_up.txt";
    char Aeq_up_str[]    = "/home/mcube/cpush/catkin_ws/src/push_control/data/Aeq_up.txt";
    char beq_up_str[]   = "/home/mcube/cpush/catkin_ws/src/push_control/data/beq_up.txt";
    char Ain_down_str[] = "/home/mcube/cpush/catkin_ws/src/push_control/data/Ain_down.txt";
    char bin_down_str[] = "/home/mcube/cpush/catkin_ws/src/push_control/data/bin_down.txt";
    char Aeq_down_str[] = "/home/mcube/cpush/catkin_ws/src/push_control/data/Aeq_down.txt";
    char beq_down_str[] = "/home/mcube/cpush/catkin_ws/src/push_control/data/beq_down.txt";


    //Define object for 3 family of modes
    Push * pStick;
    Push * pUp;
    Push * pDown;

    pStick = new Push (Q_str, Abar_str, Ain_stick_str, bin_stick_str, Aeq_stick_str, beq_stick_str);
    pUp    = new Push (Q_str, Abar_str, Ain_up_str, bin_up_str, Aeq_up_str, beq_up_str);
    pDown  = new Push (Q_str, Abar_str, Ain_down_str, bin_down_str, Aeq_down_str, beq_down_str);

    Push &Stick= *pStick;
    Push &Up   = *pUp;
    Push &Down = *pDown;

    //Build optimization models
    Stick.build_model();
    Up.build_model();
    Down.build_model();

    //**********************************************************************
    //************************ Begin Loop **********************************
    //**********************************************************************
    double time = 0;
    double counter = 0;

    // cout<<  "Thread Loop Start"<< endl;

    while(time<50000 && ros::ok())
        {
        if (time==0){t_ini = gettime();}
        time = gettime()- t_ini;      

        //Read state of robot and object from shared variables
        pthread_mutex_lock(&nonBlockMutex);       
        _q_slider_ = q_slider;
        _dq_slider_= dq_slider;
        _q_pusher_= q_pusher;
        _Target_= Target;
        pthread_mutex_unlock(&nonBlockMutex);

        //Update and solve optimization models
        Stick.update_model(_q_slider_, _dq_slider_, _q_pusher_,_Target_);
        Up.update_model(_q_slider_, _dq_slider_, _q_pusher_,_Target_);
        Down.update_model(_q_slider_, _dq_slider_, _q_pusher_,_Target_);

        fval1 = Stick.optimize();
        fval2 = Up.optimize();
        fval3 = Down.optimize();

        //Find optimial control input
        fval << fval1, fval2, fval3;
        min = fval.minCoeff(&minIndex, &maxCol); //Find

        //Assign new control input to shared variables
        pthread_mutex_lock(&nonBlockMutex);
        if (minIndex==0){ u_control = Stick.delta_u; }
        else if (minIndex==1){  u_control = Up.delta_u;}
        else{   u_control = Down.delta_u;}
        // Determine desired velocity of pusher
        // MatrixXd Output(4,1);
        // Output = inverse_dynamics2(_q_pusher_, _q_slider_, _dq_slider_, u_control, tang_vel );
        // ap(0) = Output(0);
        // ap(1) = Output(1);
        // tang_vel   = Output(3);
        
        pthread_mutex_unlock(&nonBlockMutex);
        counter++;

        }
    //*********** End Loop **************************************************
    pthread_exit((void*) 0);
}


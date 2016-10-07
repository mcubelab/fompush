// Include header files
#include "json/json.h"
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
#include "helper.h"
#include "ABBRobot.h"
#include "OptProgram.h"
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

//Name shortcuts
using namespace abb::egm;
using namespace tf;
using namespace std;
using Eigen::MatrixXd;

//Define Global Variables
const int num_ineq_constraints = NUM_INEQ_CONSTRAINTS;
const int num_eq_constraints = NUM_EQ_CONSTRAINTS;
const int num_variables = NUM_VARIABLES;
const int num_constraints = num_ineq_constraints+num_eq_constraints;
std::vector<geometry_msgs::WrenchStamped> ft_wrenches;
pthread_mutex_t nonBlockMutex;
// //
Json::Value cnOut;
Json::Value beta1Out;
Json::Value beta2Out;
Json::Value dpsiOut;
Json::Value psiOut;
Json::Value aoxOut;
Json::Value aoyOut;
Json::Value aozOut;
Json::Value aipixOut;
Json::Value aipiyOut;
Json::Value aipizOut;
Json::Value abpbxOut;
Json::Value abpbyOut;
Json::Value rbpbxOut;
Json::Value rbpbyOut;
Json::Value vbpbxOut;
Json::Value vbpbyOut;
Json::Value fFrictionxOut;
Json::Value fFrictionyOut;
Json::Value fFrictionzOut;
//
Json::Value JsonOutput;
Json::Value timeOut;
Json::Value qSliderxOut;
Json::Value qSlideryOut;
Json::Value qSliderzOut;
Json::Value dqSliderxOut;
Json::Value dqSlideryOut;
Json::Value dqSliderzOut;
Json::Value _x_tcpOut;
Json::Value _y_tcpOut;
Json::Value x_tcpOut;
Json::Value y_tcpOut;
Json::Value vpxOut;
Json::Value vpyOut;
Json::Value apxOut;
Json::Value apyOut;
Json::Value fxOut;
Json::Value fyOut;
Json::Value fzOut;

Json::Value pos_command;
Json::Value pos_sensor;
// Json::Value pVel1;

//
Json::StyledWriter styledWriter;
//~ 
struct thread_data thread_data_array[1];

//********************************************************************
// Main Program
//********************************************************************
int
main(int argc,  char *argv[])
{
    ros::init(argc, argv, "push_control");
    ros::NodeHandle n;
    tf::TransformListener listener;
    ros::Subscriber sub = n.subscribe("/netft_data", 1, chatterCallback);

    // Declare Matrix variables
    MatrixXd q_pusher(2,1);
    MatrixXd q_slider(3,1);
    MatrixXd vp(2,1);
    MatrixXd _q_pusher_sensor(2,1);

    //Define double variables
    double t_init;
    double time;
    double T_des, delta_t, t_ini;
    double  z_tcp, x_tcp, y_tcp;
    double _x_tcp, _y_tcp, _z_tcp;
    double tcp_width = 0.00475;
    bool has_robot = false;
    bool has_vicon_pos = false;
    bool has_vicon_vel = false;
    int lv2;
    FILE *myFile = NULL;

    //Define Mutex
    // pthread_mutex_t nonBlockMutex;
    // pthread_mutex_init(&nonBlockMutex, NULL);

    //Define Thread
    // pthread_t rriThread;
    // pthread_attr_t attrR;
    // pthread_attr_init(&attrR);
    // pthread_attr_setdetachstate(&attrR, PTHREAD_CREATE_JOINABLE);
        
    // Create socket and wait for robot's connection
    UDPSocket* EGMsock;
    const int portNumber = 6510;
    string sourceAddress;             // Address of datagram source
    unsigned short sourcePort;        // Port of datagram source

    EGMsock = new UDPSocket(portNumber);
    EgmSensor *pSensorMessage = new EgmSensor();
    EgmRobot *pRobotMessage = new EgmRobot();
    string messageBuffer;
   
    
    //Initialize Vicon and robot data collection
    int tmp = 0;
    int lv1 = 0;
    // geometry_msgs::WrenchStamped contact_wrench_ini;
    // geometry_msgs::WrenchStamped contact_wrench_bias;
    // MatrixXd contact_wrench(3,1);

    //First Loop
    //|| !has_vicon_pos
    while(!has_robot  || (tmp < 1000) && ros::ok())
    {
        tmp++;
        //Read robot position
        cout << " In first loop" << endl;
        cout << " tmp " << tmp << endl;
        cout << " has_vicon_pos "<< has_vicon_pos << endl;
        cout << " has_robot "    << has_robot << endl;
        cout << " ros::ok() " << ros::ok() << endl;
        if(getRobotPose(EGMsock, sourceAddress, sourcePort, pRobotMessage, x_tcp, y_tcp, z_tcp)){
                        has_robot = true;
            //CreateSensorMessageEmpty(pSensorMessage);
            CreateSensorMessage(pSensorMessage,0.23,-0.04);
            pSensorMessage->SerializeToString(&messageBuffer);
            EGMsock->sendTo(messageBuffer.c_str(), messageBuffer.length(), sourceAddress, sourcePort);
        }
        if(getViconPose(q_slider, listener)){
            has_vicon_pos = true;}
        ros::spinOnce();
        lv1+=1;
        usleep(4e3);
    }

    //Initialize position of pusher
    q_pusher(0) = x_tcp;
    q_pusher(1) = y_tcp;

    //Create Thread
    // pthread_create(&rriThread, &attrR, rriMain, (void*) &thread_data_array[0]) ;

    //Second Loop
    // for(int i=0;i<1000;i++){
        // cout << " In Second loop " << i << endl;
        // if(getRobotPose(EGMsock, sourceAddress, sourcePort, pRobotMessage, x_tcp, y_tcp, z_tcp)){
            // has_robot = true;
            // CreateSensorMessageEmpty(pSensorMessage);
            // pSensorMessage->SerializeToString(&messageBuffer);
            // EGMsock->sendTo(messageBuffer.c_str(), messageBuffer.length(), sourceAddress, sourcePort);
        // }
        // ros::spinOnce();
        // usleep(4e3);
    // }
    
    // cout << " Second loop terminated" << endl;
        // if(getRobotPose(EGMsock, sourceAddress, sourcePort, pRobotMessage, _x_tcp, _y_tcp, _z_tcp)){
            // _q_pusher_sensor<<_x_tcp,_y_tcp;
        // }
    
    //****************************************************************
    //************** Main Control Loop ****************************
    //****************************************************************
    vp << 0,0;
    double x_tcp_ini, y_tcp_ini;
    x_tcp_ini = x_tcp;
    y_tcp_ini = y_tcp;
    ros::Rate r(1000); // 10 hz


    for (int i=0;i<5000  && ros::ok();i++){

        if (i==0){t_ini = gettime();}
        time = gettime()- t_ini;

        //**********************  Get State of robot and object from ROS *********************************
       if(getRobotPose(EGMsock, sourceAddress, sourcePort, pRobotMessage, _x_tcp, _y_tcp, _z_tcp)){
           _q_pusher_sensor<<_x_tcp,_y_tcp;
        }         
        //**********************************************************************************
        double h = 1.0f/1000;
        ros::spinOnce();
        cout << " time "  <<time<< endl;
        //wait for 1 sec before starting position control 
        if (time<=1)
          {x_tcp = x_tcp;
          // contact_wrench_ini = ft_wrenches.back();
          // cout << " In first condition "  << endl;
          // cout << " time "  <<time<< endl;
          }

        // else if (time>=1 and time <=1.3)
        // {    vp(0) = 0.05;
             // vp(1) = 0;
        // x_tcp = x_tcp + h*vp(0);
        // y_tcp = y_tcp + h*vp(1);
        // }
        else
        {

        double vel_x;
        
        timeOut.append(time);
        pos_command[0].append(x_tcp);
        pos_command[1].append(y_tcp);
        pos_sensor[0].append(_x_tcp);
        pos_sensor[1].append(_y_tcp);
        
        //Position Control

        double freq=0.5;
        // double freq=1;
        // double freq=3;
        vp(0) = 0.05;
        vp(1) = 0;
        // ap(0) = 5;
        // ap(1) = 0;
        // 
        // x_tcp = x_tcp + h*vp(0);
        // y_tcp = y_tcp + h*vp(1);
        // x_tcp = x_tcp + h*vp(0) + .5*h*h*ap(0);
        // y_tcp = y_tcp + h*vp(1) + .5*h*h*ap(1);
        x_tcp = x_tcp_ini + 0.05*(time-1);
        y_tcp = y_tcp_ini + 0.1*sin(2*3.14157*freq*(time-1));

          }
 
        // Send robot commands
        CreateSensorMessage(pSensorMessage, x_tcp, y_tcp);
        pSensorMessage->SerializeToString(&messageBuffer);
        EGMsock->sendTo(messageBuffer.c_str(), messageBuffer.length(), sourceAddress, sourcePort);
        
        //Sleep for 1000Hz loop
        r.sleep();
    }

    // (void) pthread_join(rriThread, NULL);
    // outputJSON_file();
    
    JsonOutput["time"] = timeOut;
    JsonOutput["pos_command"] = pos_command;
    JsonOutput["pos_sensor"] = pos_sensor;
    
    ofstream myOutput;
    myOutput.open ("/home/mcube/cpush/catkin_ws/src/push_control/data/Raw_0_10_3_Run10_4_1000_Sine_0_5.json");
    myOutput << styledWriter.write(JsonOutput);
    myOutput.close();

    cout<<" End of Program "<<endl;

    return 0;
}

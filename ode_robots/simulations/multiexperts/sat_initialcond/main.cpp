/***************************************************************************
 *   Copyright (C) 2005 by Robot Group Leipzig                             *
 *    martius@informatik.uni-leipzig.de                                    *
 *    fhesse@informatik.uni-leipzig.de                                     *
 *    der@informatik.uni-leipzig.de                                        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 *                                                                         *
 *   $Log$
 *   Revision 1.1  2007-06-14 07:59:30  martius
 *   nice scanning of initial parameters
 *
 *
 ***************************************************************************/

#include "simulation.h"

#include "odeagent.h"
#include "octaplayground.h"
#include "playground.h"
#include "passivesphere.h"

#include <selforg/ffnncontroller.h>
#include <selforg/noisegenerator.h>
#include <selforg/one2onewiring.h>
#include <selforg/selectiveone2onewiring.h>
#include <selforg/matrix.h>

#include "sphererobot3masses.h"
#include "barrel2masses.h"
#include "axisorientationsensor.h"
#include "speedsensor.h"

// fetch all the stuff of lpzrobots into scope
using namespace lpzrobots;
using namespace matrix;
using namespace std;

int accelerationTime=40;

class BarrelNetControl : public FFNNController {
public:
  BarrelNetControl(const std::string& networkfilename):
    FFNNController(networkfilename, 1, false) {
  }
  
  virtual matrix::Matrix assembleNetworkInputXY(matrix::Matrix* xbuffer, matrix::Matrix* ybuffer) const {
    int tp = t+buffersize;
    Matrix m(xbuffer[tp%buffersize]);
    for(int i=1; i<=history; i++){
      m=m.above(xbuffer[(tp-i)%buffersize].above(ybuffer[(tp-i)%buffersize]));
    }
    return m;
  }

  virtual matrix::Matrix assembleNetworkOutput(const matrix::Matrix& output) const {    
    return output.rows(number_sensors, number_sensors + number_motors);
  }
};

class SatNetControl : public FFNNController {
public:
  SatNetControl(const std::string& networkfilename):
    //    FFNNController(networkfilename, 1, false, accelerationTime+1) {
    FFNNController(networkfilename, 1, false, 10000) {
  }
  
  virtual matrix::Matrix assembleNetworkInputXY(matrix::Matrix* xbuffer, matrix::Matrix* ybuffer) const {
    int tp = t+buffersize;
    Matrix m(xbuffer[tp%buffersize]);
    for(int i=1; i<=history; i++){
      m=m.above(xbuffer[(tp-i)%buffersize].above(ybuffer[(tp-i)%buffersize]));
    }
    return m;
  }

  virtual matrix::Matrix assembleNetworkOutput(const matrix::Matrix& output) const {    
    return output.rows(number_sensors, number_sensors + number_motors);
  }
};



class ThisSim : public Simulation {
public:
  AbstractController *controller;
  AbstractWiring* wiring;
  OdeAgent* agent;
  OdeRobot* sphere;
  const char* networkfilename;
  SpeedSensor* speedsensor;
  vector<Matrix> startAngles;
  unsigned int runcounter;
  int counter;
  int maxCounter;
  Sphererobot3MassesConf conf;
  ofstream logfile;

  ThisSim(const char* netname) : networkfilename(netname) {}

  // starting function (executed once at the beginning of the simulation loop)
  void start(const OdeHandle& odeHandle, const OsgHandle& osgHandle, GlobalData& global) 
  {
    setCameraHomePos(Pos(-0.497163, 11.6358, 3.67419),  Pos(-179.213, -11.6718, 0));
    // initialization
    global.odeConfig.setParam("noise",0.0); 
    //  global.odeConfig.setParam("gravity",-10);
    global.odeConfig.setParam("controlinterval",2);
    global.odeConfig.setParam("realtimefactor",1);
    
    runcounter=0;
    maxCounter=10000;
    //    maxCounter=500;
    counter = maxCounter;
    sphere=0;

    // we want to start the sphere in different initial conditions and 
    //  see how the sat networks will end up driving the robot
    string fn=string(networkfilename) + string("scan.log");
    logfile.open(fn.c_str(),ofstream::out);
    logfile << "# speed scan for sat network " << networkfilename << endl; 
    logfile << "#C sx sy sz ex ey ez" << endl; 

    // generate start speeds
    double stepsize=M_PI/12;
    double powers[3] = {5,10,15};    
    startAngles.push_back(Matrix(3,1)); // add zero initial condition
    //{ double theta =0;
    for(int p=0; p < 3; p++){
      stepsize=M_PI/(6.0*(p+1.0));
      for(double theta=-M_PI/2; theta < M_PI/2; theta +=stepsize){      
	//       { double omega=0;
	for(double omega=-M_PI; omega < M_PI; omega += (stepsize/cos(theta)) ){
	  Matrix m(3,1);
	  m.val(0,0)=theta;
	  m.val(1,0)=omega;
	  m.val(2,0)=powers[p];
	  startAngles.push_back(m);
	}
      } 
    }     
//     // generate start speeds
//     double stepsize=M_PI/5;
//     double powers[5] = {0,10,15,20,25};    
//     for(double theta=-M_PI; theta < M_PI; theta +=stepsize){
//       { double omega=0;
//       // for(double omega=-M_PI; omega < M_PI; omega +=stepsize){
// 	for(int p=1; p < 2; p++){
// 	  Matrix m(3,1);
// 	  m.val(0,0)=theta;
// 	  m.val(1,0)=omega;
// 	  m.val(2,0)=powers[p];
// 	  startAngles.push_back(m);
// 	}
//       } 
//     }     

      
    speedsensor = new SpeedSensor(5, SpeedSensor::RotationalRel);
  
    //****************
    conf = Sphererobot3Masses::getDefaultConf();  
    conf.pendularrange  = 0.15; 
    conf.motorpowerfactor  = 150;    
    conf.motorsensor=false;
    conf.addSensor(new AxisOrientationSensor(AxisOrientationSensor::ZProjection));
    
    showParams(global.configs);
  }

  virtual void end(GlobalData& globalData){
    logfile.close();
  }

  virtual void addCallback(GlobalData& global, bool draw, bool pause, bool control) {
    if(!control || pause) return;
    if(counter == maxCounter){
      counter = 0;
      if(sphere) {
	// measure final speed
	double dat[3];
	speedsensor->sense(global);
	speedsensor->get(dat,3);
	Matrix s(1,3,dat);
	logfile << s << endl;      		
	cout << "End:   " << s << endl;
	if(runcounter>=startAngles.size()){
	  this->pause=true;
	  simulation_time_reached=true;
	  return;
	}

	// destroy sphere
	global.agents.pop_back();
	delete sphere;	
	delete controller;
	delete wiring;
	delete agent;
      }
      // create new sphere
      sphere = new Sphererobot3Masses ( odeHandle, osgHandle.changeColor(Color(0,0.0,2.0)), 
					conf, "Sphere_test_sat", 0.3); 
      
      // we will accelerate the sphere in positive x direction
      //  we place it the way that all (straight) rolling modes are possible
      double theta = startAngles[runcounter].val(0,0);
      double omega = startAngles[runcounter].val(1,0);
      sphere->place ( osg::Matrix::rotate(omega, 0,0,1)*osg::Matrix::rotate(theta, 1,0,0));

      //      sphere->place ( osg::Matrix::rotate(M_PI/2, 0,1,0));
      //      sphere->place ( osg::Matrix::rotate(M_PI/2, 0,0,1));
      
      speedsensor->init(sphere->getMainPrimitive());

      controller = new SatNetControl(networkfilename);    

      wiring = new One2OneWiring ( new ColorUniformNoise(0.20) );
      agent = new OdeAgent ( plotoptions );
      agent->init ( controller , sphere , wiring );
      //      agent->setTrackOptions(TrackRobot(true, true, false, true, "ZSens", 50));
      global.agents.push_back ( agent );

      runcounter++;
    } else if(counter > 1 && counter < accelerationTime){ // measure start speed      
      // speed up sphere
      double power = startAngles[runcounter-1].val(2,0);
      dBodyAddTorque ( sphere->getMainPrimitive()->getBody() , 0, power, 0 );
    }else if(counter == accelerationTime){
      double dat[3];
      speedsensor->sense(global);
      speedsensor->get(dat,3);
      Matrix s(1,3,dat);
      logfile << s;
      cout << "Run: " << runcounter << "/" << startAngles.size() << endl;
      cout << "Start: " << s <<  endl;
      
    }
    
    counter++;
  }
  
  static void primitiveAddRelTorque(Primitive* p, double x, double y, double z){
    Matrix local = osgMatrix2Matrixlib(p->getPose());    
    Matrix f(4,1); // we have a vector and not a point (homogeneous coordinates)
    f.val(0,0)=x;    f.val(1,0)=y;     f.val(2,0)=z;
    f=(local^T)*f;  // this is m^T= f^T*local, ode matrix multiplications are the other way around (left sided) 
    dBodyAddTorque ( p->getBody() , f.val(0,0) , f.val(1,0) , f.val(2,0) );
  }

  // add own key handling stuff here, just insert some case values
  virtual bool command(const OdeHandle&, const OsgHandle&, GlobalData& globalData, int key, bool down)
  {
    if (down) { // only when key is pressed, not when released
      switch ( (char) key )
	{
	case 'y' : dBodyAddForce ( sphere->getMainPrimitive()->getBody() , 30 ,0 , 0 ); break;
	case 'Y' : dBodyAddForce ( sphere->getMainPrimitive()->getBody() , -30 , 0 , 0 ); break;
	case 'x' : dBodyAddTorque ( sphere->getMainPrimitive()->getBody() , 0 , 10 , 0 ); break;
	case 'X' : dBodyAddTorque ( sphere->getMainPrimitive()->getBody() , 0 , -10 , 0 ); break;
	case 'S' : controller->setParam("sinerate", controller->getParam("sinerate")*1.2); 
	  printf("sinerate : %g\n", controller->getParam("sinerate"));
	  break;
	case 's' : controller->setParam("sinerate", controller->getParam("sinerate")/1.2); 
	  printf("sinerate : %g\n", controller->getParam("sinerate"));
	  break;
	default:
	  return false;
	  break;
	}
    }
    return false;
  }

};

int main (int argc, char **argv)
{ 
  if(argc<2){
    printf("Provide network name!\n");
    return 1;
  }else{
    ThisSim sim(argv[1]);
    // run simulation
    return sim.run(argc, argv) ? 0 : 1;
  }
}
 
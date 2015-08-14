#!/usr/bin/env python

# this is to find out the transform between the vicon frame and robot frame

import tf.transformations as tfm
import numpy as np
from ik.roshelper import lookupTransform
from ik.roshelper import ROS_Wait_For_Msg
import tf
import rospy
from tf.broadcaster import TransformBroadcaster
import roslib; roslib.load_manifest("robot_comm")
from robot_comm.srv import *

from visualization_msgs.msg import Marker
from marker_helper import createMeshMarker
from vicon_bridge.msg import Markers

from mpl_toolkits.mplot3d import Axes3D
import matplotlib.pyplot as plt

limits = [0.2, 0.4, -0.3, +0.3, 0.3, 0.6]  #[xmin, xmax, ymin, ymax, zmin, zmax]
ori = [0, 0.7071, 0.7071, 0]

rospy.init_node('vicon_vs_robot')
setCartRos = rospy.ServiceProxy('/robot2_SetCartesian', robot_SetCartesian)
setZone = rospy.ServiceProxy('/robot2_SetZone', robot_SetZone)
listener = tf.TransformListener()

def xyztolist(pos):
    return [pos.x, pos.y, pos.z]

def setCart(pos, ori):
    param = (np.array(pos) * 1000).tolist() + ori
    #print 'setCart', param
    #pause()
    setCartRos(*param)

setZone(0)

xs = []
ys = []
zs = []
xt = []
yt = []
zt = []


for x in np.linspace(limits[0],limits[1], 5):
    for y in np.linspace(limits[2],limits[3], 5):
        for z in np.linspace(limits[4],limits[5], 1):
            setCart([x,y,z], ori)
            
            # get the marker pos from vicon
            vmarkers = ROS_Wait_For_Msg('/vicon/markers', Markers).getmsg()
            rospy.sleep(0.2)
            vmpos = (np.array(xyztolist(vmarkers.markers[-1].translation)) / 1000.0).tolist()
            
            # get the marker pos from robot
            (vicontrans,rot) = lookupTransform('/viconworld','/vicon_tip', listener)
            vmpos_robot = list(vicontrans)
            print 'vicon pos %.4f %.4f %.4f' % tuple(vmpos)
            print 'robot pos %.4f %.4f %.4f' % tuple(vmpos_robot)
            xs = xs + [vmpos[0]]
            ys = ys + [vmpos[1]]
            zs = zs + [vmpos[2]]
            xt = xt + [vmpos_robot[0]]
            yt = yt + [vmpos_robot[1]]
            zt = zt + [vmpos_robot[2]]
            
plt.scatter(xs, ys, c='r', marker='o')
plt.scatter(xt, yt, c='b', marker='o')

plt.show()


